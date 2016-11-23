#include <NanoCore/Jobs.h>
#include <NanoCore/Threads.h>
#include "RayTracer.h"
#include <stdlib.h>
#include <NanoCore/File.h>
#include <NanoCore/String.h>




//#define JobsLog NanoCore::DebugOutput
#define JobsLog


static void ComputeProgressiveDistribution( int size, std::vector<int> & order ) {
	for( int i=0; i<size*size; order.push_back( i++ ));
	for( int i=0; i<size*size; ++i ) {
		int a = rand() % (size*size);
		int b = rand() % (size*size);
		int temp = order[a]; order[a] = order[b]; order[b] = temp;
	}
}

bool Raytracer::TraceRay( Ray & V, IntersectResult & result ) {
	for( ;; ) {
		m_pScene->IntersectRay( V, result );
		if( !result.triangle || m_Materials.empty())
			return false;

		//return true;

		const Texture::Ptr & tex = m_Materials[result.materialId].pAlphaMap;
		if( !tex )
			break;

		m_pScene->InterpolateTriangleAttributes( result, IntersectResult::eUV );

		int pix[4];
		tex->GetTexel( result.GetUV(), pix );

		int alpha = (tex->mips[0]->GetBpp() == 32) ? pix[3] : pix[0];
		if( alpha )
			break;
		/*if( V.hitlen < INFINITE_HITLEN )
			V.hitlen -= len( V.origin - result.hit );
		else*/
			V.hitlen = INFINITE_HITLEN;

		V.origin = result.hit + V.dir * 0.001f;
		result.triangle = NULL;
		result.materialId = 0;
	}
	if( dot( result.n, V.dir ) > 0.0f )
		result.hit -= result.n * 0.001f;
	else
		result.hit += result.n * 0.001f;
	result.material = &m_Materials[result.materialId];
	return true;
}

static void Tonemap( const float3 & hdrColor, int * ldrColor ) {
	float lum = Max( hdrColor.x, Max( hdrColor.y, hdrColor.z ));
	float3 color = hdrColor * ( 1.0f / (1.0f + lum) );
	ldrColor[0] = int(color.x*255.0f);
	ldrColor[1] = int(color.y*255.0f);
	ldrColor[2] = int(color.z*255.0f);
}

float3 Raytracer::RenderRay( Ray & V, IShader * pShader, void * context ) {
	IntersectResult hit;
	TraceRay( V, hit );
	return pShader->Shade( V, hit , *m_pEnv, this, context );
}

void Raytracer::RaytracePixel( int x, int y, int * pixel )
{
	if( x == m_DebugX && y == m_DebugY ) {
		NanoCore::DebugOutput( "%d", 1 );
	}

	float3 dir = m_pCamera->ConstructRay( x, y, m_pImage->GetWidth(), m_pImage->GetHeight() );
	Ray ri( m_pCamera->pos, dir );

	float3 color = RenderRay( ri, m_pShader, m_pShader->CreateContext() );
	Tonemap( color, pixel );
}

static std::vector<int> progressive_order;

class ProgressiveRaytraceJob : public NanoCore::IJob {
public:
	int tile_x, tile_y, index, id;
	Raytracer * pRaytracer;

	ProgressiveRaytraceJob():IJob(0) {}
	ProgressiveRaytraceJob( int tile_x, int tile_y, Raytracer * ptr, int id ) : IJob(0), tile_x(tile_x), tile_y(tile_y), index(-1), pRaytracer(ptr), id(id) {}

	virtual const wchar_t * GetName() { return L"ProgressiveRaytraceJob"; }

	virtual void Execute() {
		int tile_size = 1 << pRaytracer->m_ScreenTileSizePow2;
		int order = progressive_order[index];
		int x = tile_x * tile_size + order % tile_size;
		int y = tile_y * tile_size + order / tile_size;

		JobsLog( "  prog[%d]: %d, %d, %d\n", id, tile_x, tile_y, index );

		if( x < pRaytracer->m_pImage->GetWidth() && y < pRaytracer->m_pImage->GetHeight()) {
			int rgb[3];
			pRaytracer->RaytracePixel( x, y, rgb );
			int t = rgb[0]; rgb[0] = rgb[2]; rgb[2] = t;
			pRaytracer->m_pImage->SetPixel( x, y, rgb );
			pRaytracer->m_PixelCompleteCount++;
		}
	}
};

static std::vector<ProgressiveRaytraceJob> ProgJobs;

class SpawnProgressiveJobsJob : public NanoCore::IJob {
public:
	SpawnProgressiveJobsJob() : IJob(1) {}
	Raytracer * pRaytracer;
	IStatusCallback * pCallback;
	virtual void Execute();
	virtual const wchar_t * GetName() { return L"SpawnProgressiveJobs"; }
};

void SpawnProgressiveJobsJob::Execute() {
	int max_index = 1 << pRaytracer->m_ScreenTileSizePow2;
	max_index *= max_index;
	int curr_order;

	JobsLog( "SpawnProgressiveJobsJob: index = %d\n", ProgJobs[0].index );
	for( size_t i=0; i<ProgJobs.size(); ++i ) {
		curr_order = ++ProgJobs[i].index;
		NanoCore::JobManager::AddJob( &ProgJobs[i] );
	}

	static uint64 t0;
	if( curr_order == 0 )
		t0 = NanoCore::GetTicks();

	if( pCallback )
		pCallback->SetStatus( "Rendering: %d %%, %0.2f s", curr_order * 100 / max_index, float( NanoCore::TickToMicroseconds( NanoCore::GetTicks() - t0 ) / 1000 ) *0.001f );

	if( curr_order < max_index-1 ) {
		JobsLog( "SpawnProgressiveJobsJob: adding self\n" );
		NanoCore::JobManager::AddJob( this, 0 );
	} else {
		NanoCore::DebugOutput( "Rendering finished for %0.3f ms\n", float(NanoCore::TickToMicroseconds( NanoCore::GetTicks() - t0 )) / 1000.0f );
	}
}

static SpawnProgressiveJobsJob SpawnProgJobsJob;





Raytracer::Raytracer() {
	NanoCore::JobManager::Init( 0, 2 );
	m_ScreenTileSizePow2 = 6;
	m_NumThreads = 3;
	m_SelectedTriangle = -1;
	ComputeProgressiveDistribution( 1 << m_ScreenTileSizePow2, progressive_order );
}

Raytracer::~Raytracer() {
	NanoCore::JobManager::Done();
}

void Raytracer::Stop() {
	NanoCore::JobManager::Wait( NanoCore::JobManager::efClearPendingJobs | NanoCore::JobManager::efDisableJobAddition );
}

void Raytracer::Render( Camera & camera, NanoCore::Image & image, IScene * pScene, const Environment & env, IShader * pShader, IStatusCallback * pCallback )
{
	if( pScene->IsEmpty())
		return;

	Stop();

	if( NanoCore::JobManager::GetNumThreads() != m_NumThreads ) {
		NanoCore::JobManager::Done();
		NanoCore::JobManager::Init( m_NumThreads, 2 );
	}

	m_pScene = pScene;
	m_pImage = &image;
	m_pCamera = &camera;
	m_pEnv = &env;
	m_pShader = pShader;

	pShader->BeginShading( env );

	int tileSize = 1 << m_ScreenTileSizePow2;

	m_pImage->Fill( 0 );

	m_PixelCompleteCount = 0;
	m_TotalPixelCount = m_pImage->GetWidth() * m_pImage->GetHeight();

	int tw = (m_pImage->GetWidth() + tileSize-1 ) / tileSize, th = (m_pImage->GetHeight() + tileSize - 1) / tileSize;

	ProgJobs.clear();
	ProgJobs.reserve( tw*th );
	for( int y=0; y<th; ++y )
		for( int x=0; x<tw; ++x ) {
			ProgJobs.push_back( ProgressiveRaytraceJob( x, y, this, x + y*tw ));
		}
	SpawnProgJobsJob.pRaytracer = this;
	SpawnProgJobsJob.pCallback = pCallback;
	NanoCore::JobManager::AddJob( &SpawnProgJobsJob );
}

bool Raytracer::IsRendering() {
	return NanoCore::JobManager::IsRunning();
}

Texture::Ptr Raytracer::LoadTexture( std::wstring path, std::string file ) {
	if( file.empty())
		return NULL;

	path += NanoCore::StrMbsToWcs( file.c_str() );

	auto it = m_TextureMaps.find( path );
	if( it != m_TextureMaps.end())
		return it->second;

	NanoCore::Image::Ptr pImage( new NanoCore::Image() );
	if( pImage->Load( path.c_str()) ) {
		Texture::Ptr pTexture( new Texture() );
		pTexture->Init( pImage );
		m_TextureMaps[path] = pTexture;
		m_ImageCountLoaded++;
		m_ImageSizeLoaded += pImage->GetSize();
		return pTexture;
	}
	NanoCore::DebugOutput( "Warning: FAILED to load image '%ls'\n", path.c_str());
	return NULL;
}

void Raytracer::LoadMaterials( ISceneLoader * pLoader, IStatusCallback * pCallback ) {
	std::wstring path = NanoCore::StrGetPath( pLoader->GetFilename() );
	int num = pLoader->GetNumMaterials();
	m_Materials.resize( num );

	m_ImageCountLoaded = 0;
	m_ImageSizeLoaded = 0;
	m_TextureMaps.clear();

	for( int i=0; i<num; ++i ) {
		auto src = pLoader->GetMaterial(i);
		auto & dst = m_Materials[i];
		dst.name = src->name;
		dst.Kd = src->Kd;
		dst.Ks = src->Ks;
		dst.Ke = src->Ke;
		dst.opacity = 1.0f - src->Transparency;
		dst.Ns = src->Ns;

		if( pCallback )
			pCallback->SetStatus( "Loading images (%d %%)", i*100 / num );

		dst.pDiffuseMap = LoadTexture( path, src->mapKd );
		dst.pSpecularMap = LoadTexture( path, src->mapKs );
		dst.pBumpMap = LoadTexture( path, src->mapBump );
		dst.pAlphaMap = LoadTexture( path, src->mapAlpha );
		dst.pRoughnessMap = LoadTexture( path, src->mapNs );
		if( !dst.pAlphaMap && dst.pDiffuseMap && dst.pDiffuseMap->mips[0]->GetBpp() == 32 )
			dst.pAlphaMap = dst.pDiffuseMap;
	}
	NanoCore::DebugOutput( "%d materials loaded\n", num );
	NanoCore::DebugOutput( "%d images loaded (%d Mb)\n", m_ImageCountLoaded, m_ImageSizeLoaded / (1024*1024) );

	if( pCallback )
		pCallback->SetStatus( NULL );
}