#ifndef __INC_RAYTRACE_COMMON
#define __INC_RAYTRACE_COMMON

#include <NanoCore/Common.h>
#include <NanoCore/Mathematics.h>
#include <NanoCore/File.h>
#include <NanoCore/Image.h>

#define INFINITE_HITLEN 10000000.0f



struct Texture {
	std::vector<NanoCore::Image::Ptr> mips;
	int width, height;

	Texture();

	void Init( NanoCore::Image::Ptr pImage );

	bool   IsEmpty() const { return width > 0; }
	void   GetTexel( float2 uv, float4 & pix ) const;
	void   GetTexel( float2 uv, int pix[4] ) const;
	float3 GetTexel( float2 uv ) const;
};

struct Material {
	std::string name;
	Texture ambient, diffuse, specular, roughness, bump, alpha;
	float3 Kd, Ks, Ke;
	float Ns, opacity;

	Material();
};



struct IntersectResult {
	const void * triangle;
	const Material * material;
	int    materialId;
	float3 hit, n;
	float3 barycentric;
	float3 iNormal;
	float3 tangent, bitangent;
	float2 uv;

	IntersectResult();
};



struct Ray {
	float3 origin, dir;
	float hitlen;

	Ray() {}
	Ray( float3 origin, float3 dir ) : origin(origin), dir(dir), hitlen(INFINITE_HITLEN) {}
};



class IStatusCallback {
public:
	virtual ~IStatusCallback() {}
	virtual void SetStatus( const char * pcFormat, ... ) = 0;

	void ShowLoadingProgress( const char * pcFormat, NanoCore::IFile::Ptr & pFile ) {
		SetStatus( pcFormat, int(pFile->Tell()*100 / pFile->GetSize()));
	}
};



class ISceneLoader {
public:
	struct Triangle {
		int pos[3], uv[3], normal[3];
		int material;
	};

	struct Material {
		std::string name;
		float Ns, Transparency;
		float3 Ka, Kd, Ks, Ke;
		std::string mapKa, mapKd, mapKs, mapBump, mapAlpha, mapNs;
		Material();
	};

	virtual ~ISceneLoader() {}

	virtual bool Load( const wchar_t * pwFilename, IStatusCallback * pCallback ) = 0;
	virtual const wchar_t * GetFilename() const = 0;

	virtual const float3   * GetVertexPos( int i ) const = 0;
	virtual const float2   * GetVertexUV( int i ) const = 0;
	virtual const float3   * GetVertexNormal( int i ) const = 0;
	virtual const Triangle * GetTriangle( int face ) const = 0;
	virtual int              GetNumTriangles() const = 0;
	virtual const Material * GetMaterial( int i ) const = 0;
	virtual int              GetNumMaterials() const = 0;
};



class IScene {
public:
	virtual ~IScene() {}

	virtual void Build( const ISceneLoader * pLoader, IStatusCallback * pCallback ) = 0;
	virtual bool IntersectRay( const Ray & ray, IntersectResult & hit ) const = 0;
	virtual bool IsEmpty() const = 0;
	virtual AABB GetAABB() const = 0;

	enum {
		eBarycentric = 1,
		eUV = 2,
		eNormal = 4,
		eTangent = 8
	};
	virtual void InterpolateTriangleAttributes( IntersectResult & hit, int flags ) = 0;
};




struct Environment {
	int	  GIBounces, GISamples;
	int   SunSamples;
	float SunDiskAngle;
	float SunAngle1, SunAngle2;
	float SunStrength;
	float3 SunColor;

	float3 SkyColor;
	float  SkyStrength;

	Environment() :
		GIBounces(0), GISamples(20),
		SunSamples(10), SunDiskAngle( 0.52f ), SunAngle1(90.0f), SunAngle2(15.0f),
		SunColor(1,1,1), SunStrength(10),
		SkyColor(0.75f,0.9f,1), SkyStrength(2)
	{}
};

class IShader;

class IRaytracer {
public:
	virtual ~IRaytracer() {}
	virtual bool   TraceRay( Ray & V, IntersectResult & result ) = 0;
	virtual float3 RenderRay( Ray & V, IShader * pShader, void * context ) = 0;
};

class IShader {
public:
	virtual ~IShader() {}
	virtual void   BeginShading( const Environment & env ) = 0;
	virtual float3 Shade( Ray & V, IntersectResult & result, const Environment & env, IRaytracer * pRaytracer ) = 0;
	virtual void*  CreateContext() = 0;
};



ISceneLoader * CreateObjLoader();
IScene * CreateKDTree( int maxTrianglesPerNode );

#endif