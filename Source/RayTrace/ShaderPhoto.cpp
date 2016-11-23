#include "ShaderPhoto.h"



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



void ShaderPhoto::BeginShading( const Environment & env ) {
	matrix m1, m2;
	m1.setRotationAxis( float3(1,0,0), DEG2RAD(env.SunAngle1) );
	m2.setRotationAxis( float3(0,0,1), DEG2RAD(env.SunAngle2) );
	m_SunDir = m2 * m1 * float3(0,0,-1);
}


static float3 BRDF( float3 V, float3 L, float3 N, float3 LightColor, const Material & M, float2 uv ) {
	float3 Albedo = M.pDiffuseMap->GetTexel( uv ) * M.Kd;
	float3 Diffuse = LightColor * Albedo * Max( dot( N, L ), 0.0f );

	float3 Contrib(0,0,0);

	Contrib += Diffuse;

	if( M.Ns > 0.0f || M.pRoughnessMap ) {
		float3 H = normalize( V + L );
		float spec = Max( dot( N, H ), 0.0f );

		if( M.pRoughnessMap ) {
			float r = M.pRoughnessMap->GetTexel( uv ).x;
			float power = (1.0f - r);
			power = ncPow( power, 2.0f );
			power *= 1000.0f;

			spec = ncPow( spec, power ) * (power + 8.0f) / (M_PI*8.0f);
		} else {
			spec = ncPow( spec, M.Ns );
		}

		float3 SpecMap(0,0,0); // = M.Ks;
		if( M.pSpecularMap )
			SpecMap = M.pSpecularMap->GetTexel( uv );
		else
			SpecMap = Albedo;
		Contrib += SpecMap * LightColor * spec;
	}
	return Contrib;
}


float3 ShaderPhoto::Shade( Ray & ray, IntersectResult & result, const Environment & env, IRaytracer * pRaytracer, void * context ) {

	float3 Sky = env.SkyColor * env.SkyStrength;

	if( !result.triangle )
		return Sky;

	pRaytracer->GetScene()->InterpolateTriangleAttributes( result, IntersectResult::eNormal | IntersectResult::eUV | IntersectResult::eTangentSpace );

	float3 V = -ray.dir;

	float sunDiskTan = tan( DEG2RAD(env.SunDiskAngle) * 0.5f );

	Material mtlWhite;
	mtlWhite.Kd = float3(0.5,0.5,0.5);

	float3 Sun = env.SunColor * env.SunStrength;
	float3 Contrib(0,0,0);

	float2 UV = result.GetUV();
	const Material & M = *result.material;
	float3 hit = result.hit;

	float3 N = result.GetInterpolatedNormal(); //ComputeNormal( ri, M, UV );

	for( int i=0; i<env.SunSamples; ++i ) {
		Ray rs( hit, m_SunDir );
		if( i ) {
			rs.dir += randUnitSphere() * sunDiskTan;
			rs.dir = normalize( rs.dir );
		}
		IntersectResult hitTest;
		if( !pRaytracer->TraceRay( rs, hitTest ))
			Contrib += BRDF( V, m_SunDir, N, Sun, M, UV );
	}
	for( int i=0; i<env.GISamples; ++i ) {
		Ray rs( hit, randUnitSphere() );
		if( dot( rs.dir, result.n ) < 0.0f )
			rs.dir = reflect( rs.dir, result.n );
		IntersectResult hitTest;
		if( !pRaytracer->TraceRay( rs, hitTest ))
			Contrib += BRDF( V, rs.dir, N, Sky, M, UV );
		else if( (int)context < env.GIBounces ) {



			// trace further here....
		}
	}
	Contrib *= 1.0f / float( env.SunSamples + env.GISamples );
	return Contrib;

}

