#include "ShaderPhoto.h"

void ShaderPhoto::BeginShading( const Environment & env ) {
	matrix m1, m2;
	m1.setRotationAxis( float3(1,0,0), DEG2RAD(env.SunAngle1) );
	m2.setRotationAxis( float3(0,0,1), DEG2RAD(env.SunAngle2) );
	m_SunDir = m2 * m1 * float3(0,0,-1);
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


float3 ShaderPhoto::Shade( Ray & V, IntersectResult & result, const Environment & env, IRaytracer * pRaytracer ) {


	float3 Sky = m_Context.SkyColor * m_Context.SkyStrength;


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

