#ifndef __INC_RAYTRACE_OBJECTFILELOADER
#define __INC_RAYTRACE_OBJECTFILELOADER

#include "Common.h"

class IObjectFileLoader {
public:
	virtual ~IObjectFileLoader() {}

	virtual bool Load( const wchar_t * pwFilename, IStatusCallback * pCallback ) = 0;
	virtual const wchar_t * GetFilename() const = 0;

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

	virtual const float3   * GetVertexPos( int i ) = 0;
	virtual const float2   * GetVertexUV( int i ) = 0;
	virtual const float3   * GetVertexNormal( int i ) = 0;
	virtual const Triangle * GetTriangle( int face ) = 0;
	virtual int              GetNumTriangles() = 0;
	virtual const Material * GetMaterial( int i ) = 0;
	virtual int              GetNumMaterials() = 0;

	static IObjectFileLoader * Create();
};

#endif