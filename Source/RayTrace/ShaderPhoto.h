#include "Common.h"

class ShaderPhoto : public IShader {
public:
	virtual void   BeginShading( const Environment & env );
	virtual float3 Shade( Ray & V, IntersectResult & result, const Environment & env, IRaytracer * pRaytracer );
	virtual void*  CreateContext() { return NULL; }

private:
	float3 m_SunDir;
};