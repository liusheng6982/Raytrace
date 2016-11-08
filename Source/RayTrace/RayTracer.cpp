#include <NanoCore/Jobs.h>
#include <NanoCore/Threads.h>
#include "RayTracer.h"
#include <stdlib.h>
#include <NanoCore/File.h>

//#define JobsLog NanoCore::DebugOutput
#define JobsLog



static float randf()
{
	return rand() / 32767.f;
}

static float randf( float a, float b )
{
	return a + randf()*(b - a);
}

static float3 randUnitSphere()
{
	float3 v( randf(-1.f, 1.f), randf(-1.f,1.f), randf(-1.f,1.f));
	return normalize( v );
}

/*

static Vec3 randhemi( Vec3 n )
{
	Vec3 v( randf(-1,1), randf(-1,1), randf(-1,1) );
	if( dot( v, n ) < 0 ) v = reflect( v, n );
	v = normalize( v + n * 0.1f );
	return v;
}

void RenderTask::TracePreviewRay( int x, int y, int threadId )
{
	RayInfo ray;
	camera.InitRay( x, y, ray );

	pKDTree->IntersectRay( ray );

	uint8 I = 0;
	if( ray.tri )
		I = uint8( (ray.n.y+1)*127.0f );

	uint8 * p = pRenderBuffers[threadId] + (x + y*render_width)*3;
	p[0] = p[1] = p[2] = I;
}

void RenderTask::TraceRay( int x, int y, int threadId )
{
	RayInfo ray;
	camera.InitRay( x, y, ray );

	Vec3 at, up, right;
	camera.GetAxes( at, up, right );
	right *= 1.0f / render_width;
	up *= 1.0f / render_height;

	Vec3 cSun( 1, 1, 0.8f );
	Vec3 cSky( 0.5f, 0.8f, 1.0f );
	Vec3 rgb( 0, 0, 0 );
	Vec3 vSun = normalize( Vec3( 1,2,1));
	cSun *= 500.0f;
	cSky *= 40.0f;

	{
		float fDiffuse = 1.f / 3.1415f * 0.6f;
		Vec3 hit1 = ray.GetHit();

		for( int a=0; a<numSamples; ++a ) {

			RayInfo ray1 = ray;
			ray1.dir += right * randf(-1,1) + up * randf(-1,1);
			pKDTree->IntersectRay( ray1 );
			if( !ray1.tri ) {
				rgb += ray1.dir.y > 0 ? cSky : Vec3(0,0,0);
				continue;
			}
			hit1 = ray1.GetHit();


			RayInfo rsun;
			rsun.dir = normalize( vSun + Vec3( randf(-0.025f,0.025f), randf(-0.025f,0.025f), randf(-0.025f,0.025f) ));

			Vec3 hit = hit1;

			float contrib = 1;
			RayInfo ri = ray1;
			for( int i=0; i<maxBounces; ++i ) {

				// test for the sun
				if( 0 && rand() < 20000 ) {
					rsun.pos = hit;
					rsun.hitlen = 1000000.0f;
					rsun.tri = 0;
					if( ! pKDTree->IntersectRay( rsun )) {
						contrib *= fDiffuse * dot( rsun.dir, ri.n );
						rgb += cSun * contrib;
						break;
					}
				}

				// prepare ray for bouncing in random hemispheric direction
				ri.pos = hit;
				ri.dir = randhemi( ri.n );
				ri.hitlen = 100000.0f;
				contrib *= fDiffuse * dot( ri.dir, ri.n );
				if( contrib < 0.001f ) break;
				ri.tri = 0;

				pKDTree->IntersectRay( ri );

				// nothing hit -> sky
				if( !ri.tri ) {
					rgb += cSky * contrib;
					break;
				}

				hit = ri.GetHit();
			}
		}
		rgb = rgb * (1.0f / numSamples);
	}
	rgb.x /= (rgb.x+1);
	rgb.y /= (rgb.y+1);
	rgb.z /= (rgb.z+1);

	uint8 * p = pRenderBuffers[threadId] + (x + y*render_width)*3;
	p[0] = uint8( 255 * min( 1, rgb.z ));
	p[1] = uint8( 255 * min( 1, rgb.y ));
	p[2] = uint8( 255 * min( 1, rgb.x ));
}

void RenderTask::InitFromHalfResImage( uint8 * dest, uint8 * src, int src_width, int src_height )
{
	const int w = src_width*6;
	for( int y=0; y<src_height; ++y ) {
		for( int x=0; x<src_width; ++x, dest += 6, src += 3 ) {
			dest[0] = dest[3] = dest[w+0] = dest[w+3] = src[0];
			dest[1] = dest[4] = dest[w+1] = dest[w+4] = src[1];
			dest[2] = dest[5] = dest[w+2] = dest[w+5] = src[2];
		}
		dest += w;
	}
}

void RenderTask::FinishCurrentMip()
{
	render_width *= 2;
	render_height *= 2;
	camera.width *= 2;
	camera.height *= 2;

	numSamples *= 2;

	pHalfResBuffer = pRenderBuffer;
	pRenderBuffer = new uint8[ render_width * render_height * 3];
	memset( pRenderBuffer, 0, render_width*render_height*3 );
	//InitFromHalfResImage( pRenderBuffer, pHalfResBuffer, render_width/2, render_height/2 );
	for( int i=0; i<numThreads; ++i ) {
		delete[] pRenderBuffers[i];
		pRenderBuffers[i] = new uint8[ render_width * render_height * 3];
		memcpy( pRenderBuffers[i], pRenderBuffer, render_width*render_height*3 );
	}
	pixelsComplete = 0;

	WriteImage( pHalfResBuffer, render_width/2, render_height/2, "HalfRes" );
	WriteImage( pRenderBuffer, render_width, render_height, "FullRes" );
}

float sqr( float x ) { return x*x; }

static float ColorDistance( uint8 * clr1, uint8 * clr2 )
{
	return sqrtf( sqr(float(clr1[0])-float(clr2[0])) + sqr(float(clr1[1])-float(clr2[1])) + sqr(float(clr1[2])-float(clr2[2])) );
}

static float ComputeImageError( int x, int y, uint8 * img, int w, int h )
{
	float err = 0;
	img += (x + y*w)*3;
	int w3 = w*3;
	if( x>0 ) {
		if( y>0 ) err = max( err, ColorDistance( img, img+w3+3 ));
		err = max( err, ColorDistance( img+w3, img+w3+3 ));
		if( y+1<h ) err = max( err, ColorDistance( img+w3*2, img+w3+3 ));
	}
	if( x+1<w ) {
		if( y>0 ) err = max( err, ColorDistance( img+6, img+w3+3 ));
		err = max( err, ColorDistance( img+w3+6, img+w3+3 ));
		if( y+1<h ) err = max( err, ColorDistance( img+w3*2+6, img+w3+3 ));
	}
	if( y>0 ) err = max( err, ColorDistance( img+3, img+w3+3 ));
	if( y+1<h ) err = max( err, ColorDistance( img+w3*2+3, img+w3+3 ));

	return err;
}

float RenderTask::ComputeError( int x, int y )
{
	return ComputeImageError( x/2, y/2, pHalfResBuffer, render_width/2, render_height/2 );
}

void RenderTask::SetResolution( int w, int h )
{
	if( w == render_width && h == render_height ) return;
	render_width = w;
	render_height = h;
	camera.width = w;
	camera.height = h;
}
*/

static void ComputeProgressiveDistribution( int size, std::vector<int> & order )
{
	for( int i=0; i<size*size; order.push_back( i++ ));
	for( int i=0; i<size*size; ++i ) {
		int a = rand() % (size*size);
		int b = rand() % (size*size);
		int temp = order[a]; order[a] = order[b]; order[b] = temp;
	}
}

void Raytracer::RaytracePixel( int x, int y, int * pixel )
{
	RayInfo ri;
	ri.Init( x, y, *m_pCamera, m_pImage->GetWidth(), m_pImage->GetHeight() );
	m_pKDTree->Intersect( ri );

	if( ri.tri ) {
		float shade = 255.0f;
		RayInfo rSun;

		switch( m_Shading ) {
			case ePreviewShading_ColoredCubeShadowed:
				if( m_SunSamples ) {
					int intersect = 0;

					float len = ncTan( m_SunDiskAngle * 0.5f * 3.1415f / 180.0f );

					for( int i=0; i<m_SunSamples; ++i )
					{
						rSun.pos = ri.GetHit() + ri.n*0.01f;
						rSun.dir = normalize( float3(1,8,1));

						if( m_SunSamples > 1 ) {
							rSun.dir += randUnitSphere()*len;
							rSun.dir = normalize( rSun.dir );
						}

						rSun.tri = NULL;
						rSun.hitlen = 10000000.0f;
						m_pKDTree->Intersect( rSun );
						if( rSun.tri ) intersect++;
					}
					float shadow = float(intersect) / m_SunSamples;
					shade = 255 - shadow*200.0f;
				}
			case ePreviewShading_ColoredCube:
				pixel[0] = int((ri.n.x*0.5f+0.5f) * shade);
				pixel[1] = int((ri.n.y*0.5f+0.5f) * shade);
				pixel[2] = int((ri.n.z*0.5f+0.5f) * shade);
				break;
			case ePreviewShading_TriangleID: {
				uint32 c = ri.tri->triangleID ^ (ri.tri->triangleID << 18) ^ (ri.tri->triangleID << 5);
				pixel[0] = c&0xFF;
				pixel[1] = (c>>8)&0xFF;
				pixel[2] = (c>>16)&0xFF;

				/*if( pixel[0] > pixel[1] && pixel[0] > pixel[2] )
					pixel[1] /= 2, pixel[2] /= 2;
				else if( pixel[1] > pixel[0] && pixel[1] > pixel[2] )
					pixel[0] /= 2, pixel[2] /= 2;
				else
					pixel[0] /= 2, pixel[1] /= 2;*/

				if( ri.tri->triangleID == m_SelectedTriangle )
					pixel[0] = pixel[1] = pixel[2] = 0xFF;
				break;
			}
			case ePreviewShading_Checker: {
				float3 hit = ri.GetHit();
				float shade = dot( ri.n, normalize( float3(1,8,1)));
				if( shade < 0.0f ) shade = 0.0f;
				shade = shade*0.5f + 0.5f;
				int c = int(hit.x) + int(hit.y) + int(hit.z);
				pixel[0] = pixel[1] = pixel[2] = int( ((c&1) ? 255 : 50)*shade );
				break;
			}
		}
	} else {
		pixel[0] = pixel[1] = pixel[2] = 0;
	}
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
	if( curr_order < max_index-1 ) {
		JobsLog( "SpawnProgressiveJobsJob: adding self\n" );
		NanoCore::JobManager::AddJob( this, 0 );
	}
}

static SpawnProgressiveJobsJob SpawnProgJobsJob;





Raytracer::Raytracer()
{
	NanoCore::JobManager::Init( 0, 2 );
	m_ScreenTileSizePow2 = 6;
	m_Shading = ePreviewShading_ColoredCube;
	m_SunSamples = 1;
	m_SunDiskAngle = 0.52f;
	m_GIBounces = 3;
	m_NumThreads = 3;
	m_SelectedTriangle = -1;

	ComputeProgressiveDistribution( 1 << m_ScreenTileSizePow2, progressive_order );
}

Raytracer::~Raytracer()
{
	NanoCore::JobManager::Done();
}

void Raytracer::Stop()
{
	NanoCore::JobManager::Wait( NanoCore::JobManager::efClearPendingJobs | NanoCore::JobManager::efDisableJobAddition );
}

void Raytracer::Render( Camera & camera, Image & image, KDTree & kdTree )
{
	if( kdTree.Empty())
		return;

	Stop();

	if( NanoCore::JobManager::GetNumThreads() != m_NumThreads ) {
		NanoCore::JobManager::Done();
		NanoCore::JobManager::Init( m_NumThreads, 2 );
	}

	m_pKDTree = &kdTree;
	m_pImage = &image;
	m_pCamera = &camera;

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
	NanoCore::JobManager::AddJob( &SpawnProgJobsJob );
}

bool Raytracer::IsRendering() {
	return NanoCore::JobManager::IsRunning();
}

void Raytracer::GetStatus( std::wstring & status ) {
	if( m_PixelCompleteCount < m_TotalPixelCount ) {
		status += L" | ";
		wchar_t buf[128];
		swprintf( buf, 128, L"%d %%", m_PixelCompleteCount * 100 / m_TotalPixelCount );
		status += buf;
	}
}

static void LoadAsBmp( std::wstring path, std::string file, Image & img ) {
	if( file.empty()) {
		img.Clear();
		return;
	}
	std::wstring wfile( file.begin(), file.end());
	path += wfile;
	NanoCore::StrReplaceExtension( path, L"bmp" );
	img.Load( path.c_str());
}

void Raytracer::LoadMaterials( IObjectFileLoader & loader ) {
	std::wstring path = NanoCore::StrGetPath( loader.GetFilename() );
	int num = loader.GetNumMaterials();
	m_Materials.resize( num );
	for( int i=0; i<num; ++i ) {
		auto src = loader.GetMaterial(i);
		auto & dst = m_Materials[i];
		dst.Kd = src->Kd;
		dst.Ks = src->Ks;
		dst.Ke = src->Ke;
		dst.Tr = src->Transparency;

		LoadAsBmp( path, src->mapKd, dst.Md );
		LoadAsBmp( path, src->mapKs, dst.Ms );
		LoadAsBmp( path, src->mapBump, dst.Mb );
	}
}