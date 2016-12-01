#include "ShaderPreview.h"

ShaderPreview::ShaderPreview() : m_Shader( ShaderPreview::eColoredCube ) {
}

void ShaderPreview::BeginShading( const Environment & env ) {
	matrix m1, m2;
	m1.setRotationAxis( float3(1,0,0), DEG2RAD(env.SunAngle1) );
	m2.setRotationAxis( float3(0,0,1), DEG2RAD(env.SunAngle2) );
	m_SunDir = m2 * m1 * float3(0,0,-1);
}

float3 ShaderPreview::Shade( Ray & V, IntersectResult & result, const Environment & env, IRaytracer * pRaytracer, void * context ) {

	if( result.triangle == NULL )
		return 0.0f;

	float shade = 1.0f;
	switch( m_Shader ) {
		case eColoredCubeShadowed: {
			Ray ray( result.hit, m_SunDir );
			IntersectResult rSun;
			if( pRaytracer->TraceRay( ray, rSun ))
				shade = 0.2f;
		}
		case eColoredCube:
			return float3( (result.n + 1.001f)*shade*0.49f );
		case eTriangleID: {
			uint32 c = (uint32)result.triangle;
			return float3( float(c&0xFF)/255.0f, float((c>>8)&0xFF)/255.0f, float((c>>16)&0xFF)/255.0f );
		}
		case eChecker: {
			float3 hit = result.hit;
			shade = Max( dot( result.n, m_SunDir ), 0.0f );
			shade = shade*0.5f + 0.5f;
			int c = int(hit.x) + int(hit.y) + int(hit.z);
			return float3( ((c&1) ? 1.0f : 0.2f)*shade );
		}
		case eDiffuse: {
			if( result.material && result.material->pDiffuseMap ) {
				pRaytracer->GetScene()->InterpolateTriangleAttributes( result, IntersectResult::eUV );
				return result.material->pDiffuseMap->GetTexel( result.GetUV() )* result.material->Kd;
			}
			break;
		}
		case eSpecular: {
			if( result.material && result.material->pSpecularMap ) {
				pRaytracer->GetScene()->InterpolateTriangleAttributes( result, IntersectResult::eUV );
				return result.material->pSpecularMap->GetTexel( result.GetUV() );
			}
			break;
		}
		case eBump: {
			if( result.material && result.material->pBumpMap ) {
				pRaytracer->GetScene()->InterpolateTriangleAttributes( result, IntersectResult::eUV );
				return result.material->pBumpMap->GetTexel( result.GetUV() );
			}
			break;
		}
	}
	return float3(0.0f);
}