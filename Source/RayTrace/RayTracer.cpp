#include <NanoCore/Jobs.h>
#include <NanoCore/Threads.h>
#include "RayTracer.h"
#include <stdlib.h>
#include <NanoCore/File.h>
#include <NanoCore/String.h>




//#define JobsLog NanoCore::DebugOutput
#define JobsLog

static float randf() {
	return rand() / 32767.f;
}

static float randf( float a, float b ) {
	return a + randf()*(b - a);
}

static float3 randUnitSphere() {
	float3 v( randf(-1.f, 1.f), randf(-1.f,1.f), randf(-1.f,1.f));
	return normalize( v );
}

static float3 GetTexel( const NanoCore::Image::Ptr & pImage, float2 uv ) {
	if( !pImage )
		return float3(1,1,1);
	int pix[4] = {0};
	pImage->GetPixel( uv.x, uv.y, pix );
	return float3( pix[0]/255.0f, pix[1]/255.0f, pix[2]/255.0f );
}

static float3 ComputeNormal( RayInfo & ri, const Raytracer::Material & M, float2 uv ) {
	if( !M.pBumpMap )
		return ri.n;

	float2 uv0 = ri.tri->uv[1] - ri.tri->uv[0];
	float2 uv1 = ri.tri->uv[2] - ri.tri->uv[0];

	if( uv1.y == 0.0f ) return ri.n;

	float V = 0.5f;
	float W = -uv0.y * V / uv1.y;

	float U = 1.0f - V - W;

	float3 N = ri.n;
	float3 tangent = normalize( ri.tri->pos[0]*U + ri.tri->pos[1]*V + ri.tri->pos[2]*W - ri.tri->pos[0] );
	float3 bitangent = normalize( cross( N, tangent ) );
	tangent = cross( bitangent, N );

	float3 tex = GetTexel( M.pBumpMap, uv );
	tex = tex * 2.0f - 1.0f;

	float3 N2;
	N2.x = tex.x*tangent.x + tex.y*bitangent.x + tex.z*N.x;
	N2.y = tex.x*tangent.y + tex.y*bitangent.y + tex.z*N.y;
	N2.z = tex.x*tangent.z + tex.y*bitangent.z + tex.z*N.z;

	return N2;

	// (u0,v0)*A + (u1,v1)*B = (U,0)
	// v0*0.5 + v1*B = 0   B = -v0*0.5 / v1
}

static void ComputeProgressiveDistribution( int size, std::vector<int> & order ) {
	for( int i=0; i<size*size; order.push_back( i++ ));
	for( int i=0; i<size*size; ++i ) {
		int a = rand() % (size*size);
		int b = rand() % (size*size);
		int temp = order[a]; order[a] = order[b]; order[b] = temp;
	}
}

void Raytracer::RaytracePreviewPixel( int x, int y, int * pixel ) {
	RayInfo ri;
	ri.Init( x, y, *m_pCamera, m_pImage->GetWidth(), m_pImage->GetHeight() );

	for( ;; ) {
		m_pKDTree->Intersect( ri );
		if( !ri.tri || m_Materials.empty())
			break;

		const NanoCore::Image::Ptr & img = m_Materials[ri.tri->mtl].pAlphaMap;
		if( img ) {
			float2 uv = ri.tri->GetUV( ri.barycentric );

			int pix[4];
			img->GetPixel( uv.x, uv.y, pix );
			int alpha = (img->GetBpp() == 32) ? pix[3] : pix[0];

			if( !alpha ) {
				ri.pos = ri.hit + ri.dir;
				ri.hitlen = 10000000.0f;
			} else
				break;
		} else
			break;
	}

	pixel[0] = pixel[1] = pixel[2] = 0;

	if( ri.tri ) {
		float shade = 255.0f;
		RayInfo rSun;

		switch( m_Context.Shading ) {
			case eShading_ColoredCubeShadowed: {
					rSun.pos = ri.GetHit();

					if( dot( ri.dir, ri.n ) < 0 ) rSun.pos += ri.n*0.01f; else rSun.pos += -ri.n*0.01f;

					rSun.dir = m_SunDir;
					m_pKDTree->Intersect( rSun );
					float shadow = rSun.tri ? 1.0f : 0.0f;
					shade = 255 - shadow*200.0f;
			}
			case eShading_ColoredCube: {
				pixel[0] = int((ri.n.x*0.5f+0.5f) * shade);
				pixel[1] = int((ri.n.y*0.5f+0.5f) * shade);
				pixel[2] = int((ri.n.z*0.5f+0.5f) * shade);

				/*pixel[0] = int(ri.barycentric.x*255.0f);
				pixel[1] = int(ri.barycentric.y*255.0f);
				pixel[2] = int((1.0f-ri.barycentric.x-ri.barycentric.y)*255.0f);

				float2 uv = ri.tri->GetUV( ri.barycentric );

				pixel[0] = int(uv.x*255.0f);
				pixel[1] = int(uv.y*255.0f);
				pixel[2] = 0;*/

				break;
		}
		case eShading_TriangleID: {
#ifdef KEEP_TRIANGLE_ID
			uint32 c = ri.tri->triangleID ^ (ri.tri->triangleID << 18) ^ (ri.tri->triangleID << 5);
#else
			uint32 c = (uint32)ri.tri;
#endif
			pixel[0] = c&0xFF;
			pixel[1] = (c>>8)&0xFF;
			pixel[2] = (c>>16)&0xFF;
#ifdef KEEP_TRIANGLE_ID
			if( ri.tri->triangleID == m_SelectedTriangle )
				pixel[0] = pixel[1] = pixel[2] = 0xFF;
#endif
			break;
		}
		case eShading_Checker: {
			float3 hit = ri.GetHit();
			float shade = dot( ri.n, normalize( float3(1,8,1)));
			if( shade < 0.0f ) shade = 0.0f;
			shade = shade*0.5f + 0.5f;
			int c = int(hit.x) + int(hit.y) + int(hit.z);
			pixel[0] = pixel[1] = pixel[2] = int( ((c&1) ? 255 : 50)*shade );
			break;
		}
		case eShading_Diffuse: {
			float2 uv = ri.tri->GetUV( ri.barycentric );
			if( ri.tri->mtl>=0 && !m_Materials.empty()) {
				const Material & mtl = m_Materials[ri.tri->mtl];
				const NanoCore::Image::Ptr & img = mtl.pDiffuseMap;
				if( img ) {
					float3 tex = GetTexel( img, uv ) * mtl.Kd;
					pixel[0] = int(tex.x*255.0f);
					pixel[1] = int(tex.y*255.0f);
					pixel[2] = int(tex.z*255.0f);
				}
			}
			break;
		}
		case eShading_Specular: {
			float2 uv = ri.tri->GetUV( ri.barycentric );
			if( ri.tri->mtl >= 0 && !m_Materials.empty()) {
				const Material & mtl = m_Materials[ri.tri->mtl];
				const NanoCore::Image::Ptr & img = mtl.pSpecularMap;
				const NanoCore::Image::Ptr & img2 = mtl.pRoughnessMap;
				if( img || img2 ) {
					float3 tex = GetTexel( img ? img : img2, uv ); // * mtl.Ks;
					pixel[0] = int(tex.x*255.0f);
					pixel[1] = int(tex.y*255.0f);
					pixel[2] = int(tex.z*255.0f);
				}
			}
			break;
		}
		case eShading_Bump: {
			float2 uv = ri.tri->GetUV( ri.barycentric );
			if( ri.tri->mtl >= 0 && !m_Materials.empty()) {
				const Material & mtl = m_Materials[ri.tri->mtl];
				float3 tex = ComputeNormal( ri, mtl, uv );
				tex = tex*0.5f + 0.5f;
				pixel[0] = int(tex.x*255.0f);
				pixel[1] = int(tex.y*255.0f);
				pixel[2] = int(tex.z*255.0f);
			}
			break;
		}
		}
	}
}

void Raytracer::TraceRay( RayInfo & ri ) {
	for( ;; ) {
		m_pKDTree->Intersect( ri );
		if( !ri.tri || m_Materials.empty())
			break;

		const NanoCore::Image::Ptr & img = m_Materials[ri.tri->mtl].pAlphaMap;
		if( img ) {
			float2 uv = ri.tri->GetUV( ri.barycentric );

			int pix[4];
			img->GetPixel( uv.x, uv.y, pix );
			int alpha = (img->GetBpp() == 32) ? pix[3] : pix[0];

			if( !alpha ) {
				ri.pos = ri.hit + ri.dir * 0.001f;
				ri.hitlen = 10000000.0f;
			} else
				break;
		} else
			break;
	}
}

static float3 BRDF( float3 V, float3 L, float3 N, float3 LightColor, const Raytracer::Material & M, float2 uv ) {
	float3 Albedo = GetTexel( M.pDiffuseMap, uv ) * M.Kd;
	float3 Diffuse = LightColor * Albedo * Max( dot( N, L ), 0.0f );

	float3 Contrib(0,0,0);

	//Contrib += Diffuse;

	if( M.Ns > 0.0f || M.pRoughnessMap ) {
		float3 H = normalize( V + L );
		float spec = Max( dot( N, H ), 0.0f );

		if( M.pRoughnessMap ) {
			float r = GetTexel( M.pRoughnessMap, uv ).x;
			float power = (1.0f - r);
			power = ncPow( power, 2.0f );
			power *= 1000.0f;

			spec = ncPow( spec, power ) * (power + 8.0f) / (M_PI*8.0f);
		} else {
			spec = ncPow( spec, M.Ns );
		}

		float3 SpecMap(0,0,0); // = M.Ks;
		if( M.pSpecularMap )
			SpecMap = GetTexel( M.pSpecularMap, uv );
		else
			SpecMap = Albedo;
		Contrib += SpecMap * LightColor * spec;
	}
	return Contrib;
}

static void Tonemap( const float3 & hdrColor, int * ldrColor ) {
	float lum = Max( hdrColor.x, Max( hdrColor.y, hdrColor.z ));
	float3 color = hdrColor * ( 1.0f / (1.0f + lum) );
	ldrColor[0] = int(color.x*255.0f);
	ldrColor[1] = int(color.y*255.0f);
	ldrColor[2] = int(color.z*255.0f);
}

float3 Raytracer::RaytraceRay( RayInfo & ri, int bounces ) {
	float3 Sky = m_Context.SkyColor * m_Context.SkyStrength;

	TraceRay( ri );
	if( !ri.tri )
		return Sky;

	float3 V = -ri.dir;

	float sunDiskTan = tan( DEG2RAD(m_Context.SunDiskAngle) * 0.5f );

	Material mtlWhite;
	mtlWhite.Kd = float3(0.5,0.5,0.5);

	float3 Sun = m_Context.SunColor * m_Context.SunStrength;
	float3 Contrib(0,0,0);

	float2 UV = ri.tri->GetUV( ri.barycentric );
	const Material & M = m_Materials.empty() ? mtlWhite : m_Materials[ri.tri->mtl];
	float3 hit = ri.hit + ri.n * 0.001f;

	float3 N = ComputeNormal( ri, M, UV );

	for( int i=0; i<m_Context.SunSamples; ++i ) {
		RayInfo rs;
		rs.pos = hit;
		rs.dir = m_SunDir;
		if( i ) {
			rs.dir += randUnitSphere() * sunDiskTan;
			rs.dir = normalize( rs.dir );
		}
		TraceRay( rs );
		if( !rs.tri )
			Contrib += BRDF( V, m_SunDir, N, Sun, M, UV );
	}
	for( int i=0; i<m_Context.GISamples; ++i ) {
		RayInfo rs;
		rs.pos = hit;
		rs.dir = randUnitSphere();
		if( dot( rs.dir, ri.n ) < 0.0f )
			rs.dir = reflect( rs.dir, ri.n );
		TraceRay( rs );
		if( !rs.tri )
			Contrib += BRDF( V, rs.dir, N, Sky, M, UV );
		else if( bounces ) {
			// trace further here....
		}
	}
	Contrib *= 1.0f / float( m_Context.SunSamples + m_Context.GISamples );
	return Contrib;
}

void Raytracer::RaytracePixel( int x, int y, int * pixel )
{
	if( x == m_DebugX && y == m_DebugY ) {
		NanoCore::DebugOutput( "%d", 1 );
	}
	if( m_Context.Shading < eShading_Previews ) {
		RaytracePreviewPixel( x, y, pixel );
		return;
	}

	RayInfo ri;
	ri.Init( x, y, *m_pCamera, m_pImage->GetWidth(), m_pImage->GetHeight() );

	float3 color = RaytraceRay( ri, m_Context.GIBounces );

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

void Raytracer::Render( Camera & camera, NanoCore::Image & image, KDTree & kdTree, const Context & context, IStatusCallback * pCallback )
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
	m_Context = context;

	matrix m1, m2;
	m1.setRotationAxis( float3(1,0,0), DEG2RAD(context.SunAngle1) );
	m2.setRotationAxis( float3(0,0,1), DEG2RAD(context.SunAngle2) );
	m_SunDir = m2 * m1 * float3(0,0,-1);

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

NanoCore::Image::Ptr Raytracer::LoadImage( std::wstring path, std::string file ) {
	if( file.empty())
		return NULL;

	path += NanoCore::StrMbsToWcs( file.c_str() );

	auto it = m_TextureMaps.find( path );
	if( it != m_TextureMaps.end())
		return it->second;

	NanoCore::Image::Ptr pImage( new NanoCore::Image() );
	if( pImage->Load( path.c_str()) ) {
		m_TextureMaps[path] = pImage;
		m_ImageCountLoaded++;
		m_ImageSizeLoaded += pImage->GetSize();
		return pImage;
	}
	NanoCore::DebugOutput( "Warning: FAILED to load image '%ls'\n", path.c_str());
	return NULL;
}

void Raytracer::LoadMaterials( IObjectFileLoader & loader, IStatusCallback * pCallback ) {
	std::wstring path = NanoCore::StrGetPath( loader.GetFilename() );
	int num = loader.GetNumMaterials();
	m_Materials.resize( num );

	m_ImageCountLoaded = 0;
	m_ImageSizeLoaded = 0;
	m_TextureMaps.clear();

	for( int i=0; i<num; ++i ) {
		auto src = loader.GetMaterial(i);
		auto & dst = m_Materials[i];
		dst.name = src->name;
		dst.Kd = src->Kd;
		dst.Ks = src->Ks;
		dst.Ke = src->Ke;
		dst.Tr = src->Transparency;
		dst.Ns = src->Ns;

		if( pCallback )
			pCallback->SetStatus( "Loading images (%d %%)", i*100 / num );

		dst.pDiffuseMap = LoadImage( path, src->mapKd );
		dst.pSpecularMap = LoadImage( path, src->mapKs );
		dst.pBumpMap = LoadImage( path, src->mapBump );
		dst.pAlphaMap = LoadImage( path, src->mapAlpha );
		dst.pRoughnessMap = LoadImage( path, src->mapNs );
		if( !dst.pAlphaMap && dst.pDiffuseMap && dst.pDiffuseMap->GetBpp() == 32 )
			dst.pAlphaMap = dst.pDiffuseMap;
	}
	NanoCore::DebugOutput( "%d materials loaded\n", num );
	NanoCore::DebugOutput( "%d images loaded (%d Mb)\n", m_ImageCountLoaded, m_ImageSizeLoaded / (1024*1024) );

	if( pCallback )
		pCallback->SetStatus( NULL );
}