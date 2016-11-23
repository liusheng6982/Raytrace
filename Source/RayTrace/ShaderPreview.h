#ifndef ___INC_RAYTRACER_SHADERPREVIEW
#define ___INC_RAYTRACER_SHADERPREVIEW

#include "Common.h"

class ShaderPreview : public IShader {
public:
	enum EShader {
		eFirst,
		eColoredCube = eFirst,
		eColoredCubeShadowed,
		eTriangleID,
		eChecker,

		eDiffuse,
		eSpecular,
		eBump,
	};

	EShader m_Shader;

	ShaderPreview();

	virtual void   BeginShading( const Environment & env );
	virtual float3 Shade( Ray & V, IntersectResult & hit, const Environment & env, IRaytracer * pRaytracer, void * context );
	virtual void*  CreateContext() { return NULL; }

private:
	float3 m_SunDir;
};

#endif