#include <NanoCore/Jobs.h>
#include <NanoCore/Threads.h>
#include "RayTracer.h"
#include <stdlib.h>
#include <NanoCore/File.h>
#include <NanoCore/String.h>




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
				if( img && img->GetWidth()) {
					img->GetPixel( uv.x, uv.y, pixel );
				} else {
					pixel[0] = int(mtl.Kd.x * 255.0f);
					pixel[1] = int(mtl.Kd.y * 255.0f);
					pixel[2] = int(mtl.Kd.z * 255.0f);
				}
			} else {
				pixel[0] = pixel[1] = pixel[2] = 0;
			}
			break;
		}
		}
	} else {
		pixel[0] = pixel[1] = pixel[2] = 0;
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

static float3 GetTexel( const NanoCore::Image::Ptr & pImage, float2 uv ) {
	if( !pImage )
		return float3(1,1,1);
	int pix[4];
	pImage->GetPixel( uv.x, uv.y, pix );
	return float3( pix[0]/255.0f, pix[1]/255.0f, pix[2]/255.0f );
}

static float3 BRDF( float3 V, float3 L, float3 N, float3 LightColor, const Raytracer::Material & M, float2 uv ) {
	float3 Albedo = GetTexel( M.pDiffuseMap, uv ) * M.Kd;
	float3 Diffuse = LightColor * Albedo * Max( dot( N, L ), 0.0f );

	float3 H = normalize( V + L );
	float spec = Max( dot( N, H ), 0.0f );
	spec = ncPow( spec, M.Ns );

	return Diffuse + LightColor*spec;
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

	for( int i=0; i<m_Context.SunSamples; ++i ) {
		RayInfo rs;
		rs.pos = hit;
		rs.dir = m_SunDir + randUnitSphere() * sunDiskTan;
		TraceRay( rs );
		if( !rs.tri )
			Contrib += BRDF( V, m_SunDir, ri.n, Sun, M, UV );
	}
	for( int i=0; i<m_Context.GISamples; ++i ) {
		RayInfo rs;
		rs.pos = hit;
		rs.dir = randUnitSphere();
		if( dot( rs.dir, ri.n ) < 0.0f )
			rs.dir = reflect( rs.dir, ri.n );
		TraceRay( rs );
		if( !rs.tri )
			Contrib += BRDF( V, rs.dir, ri.n, Sky, M, UV );
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

void Raytracer::Render( Camera & camera, NanoCore::Image & image, KDTree & kdTree, const Context & context )
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

void Raytracer::LoadMaterials( IObjectFileLoader & loader ) {
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

		dst.pDiffuseMap = LoadImage( path, src->mapKd );
		dst.pSpecularMap = LoadImage( path, src->mapKs );
		dst.pBumpMap = LoadImage( path, src->mapBump );
		dst.pAlphaMap = LoadImage( path, src->mapAlpha );
		if( !dst.pAlphaMap && dst.pDiffuseMap && dst.pDiffuseMap->GetBpp() == 32 )
			dst.pAlphaMap = dst.pDiffuseMap;
	}
	NanoCore::DebugOutput( "%d materials loaded\n", num );
	NanoCore::DebugOutput( "%d images loaded (%d Mb)\n", m_ImageCountLoaded, m_ImageSizeLoaded / (1024*1024) );
}