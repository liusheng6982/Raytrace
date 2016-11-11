#ifndef __INC_NANOCORE_MATH
#define __INC_NANOCORE_MATH

#include "Common.h"

#define M_PI 3.14159265359f
#define DEG2RAD(x) ((x)*M_PI/180.0f)
#define RAD2DEG(x) ((x)*180.0f/M_PI)



float ncSqrt( float x );
float ncTan( float x );
float ncSin( float x );
float ncCos( float x );
void  ncSinCos( float x, float & s, float & c );
float ncFloor( float x );


struct float2
{
	union {
		float v[2];
		struct {
			float x,y;
		};
	};

	float2() {}
	float2( float x, float y ) : x(x), y(y) {}
	float2( const float2 & f3 ) { x = f3.x; y = f3.y; }

	friend float2 operator + ( float2 a, float2 b ) {
		return float2( a.x + b.x, a.y + b.y );
	}
	friend float2 operator - ( float2 a, float2 b ) {
		return float2( a.x - b.x, a.y - b.y );
	}
	friend float2 operator * ( float2 a, float k ) {
		return float2( a.x*k, a.y*k );
}
};

struct float3
{
	union {
		float v[3];
		struct {
			float x,y,z;
		};
	};

	float3() {}
	float3( float _x ) { x = y = z = _x; }
	float3( float _x, float _y, float _z ) { x = _x; y = _y; z = _z; }
	float3( const float3 & f3 ) { x = f3.x; y = f3.y; z = f3.z; }

	float & operator[] ( int i ) { return v[i]; }
	float operator[]( int i ) const { return v[i]; }

	void operator += ( float3 v ) {
		x += v.x; y += v.y; z += v.z;
	}
	void operator += ( float k ) {
		x += k; y += k; z += k;
	}
	void operator *= ( float k ) {
		x *= k; y *= k; z *= k;
	}

	friend float3 operator + ( float3 a, float3 b ) {
		return float3( a.x + b.x, a.y + b.y, a.z + b.z );
	}
	friend float3 operator - ( float3 a, float3 b ) {
		return float3( a.x - b.x, a.y - b.y, a.z - b.z );
	}
	friend float3 operator * ( float3 a, float k ) {
		return float3( a.x*k, a.y*k, a.z*k );
	}
	friend float3 operator * ( float k, float3 a ) {
		return a * k;
	}
	friend float3 operator / ( float3 a, float3 b ) {
		return float3( a.x / b.x, a.y / b.y, a.z / b.z );
	}
	friend float3 operator - ( float3 a ) {
		return float3( -a.x, -a.y, -a.z );
	}
	friend float dot( float3 a, float3 b ) {
		return a.x*b.x + a.y*b.y + a.z*b.z;
	}
	friend float3 cross( float3 a, float3 b ) {
		return float3( a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x );
	}
	friend float3 reflect( float3 v, float3 n ) {
		return v - n * dot(v,n)*2;
	}
	friend float len( float3 v ) {
		return ncSqrt( v.x*v.x + v.y*v.y + v.z*v.z );
	}
	friend float3 normalize( float3 v ) {
		float k = 1.f / len( v );
		return float3( v.x * k, v.y * k, v.z * k );
	}
};

struct aabb {
	float3 min, max;

	aabb() {}
	aabb( float3 min, float3 max ) : min(min), max(max) {}

	void reset() {
		min = float3(1000000.0f,1000000.0f,1000000.0f);
		max = float3(-1000000.0f,-1000000.0f,-1000000.0f);
	}
	void operator += ( float3 a ) {
		if( a.x < min.x ) min.x = a.x; else if( a.x > max.x ) max.x = a.x;
		if( a.y < min.y ) min.y = a.y; else if( a.y > max.y ) max.y = a.y;
		if( a.z < min.z ) min.z = a.z; else if( a.z > max.z ) max.z = a.z;
	}
	float3 GetSize() const {
		return max - min;
	}
};

struct matrix  // row-major
{
	float v[4][4];

	matrix() {}

	friend float3 operator * ( matrix a, float3 b ) {
		return float3(
			a.v[0][0]*b.x + a.v[0][1]*b.y + a.v[0][2]*b.z + a.v[0][3],
			a.v[1][0]*b.x + a.v[1][1]*b.y + a.v[1][2]*b.z + a.v[1][3],
			a.v[2][0]*b.x + a.v[2][1]*b.y + a.v[2][2]*b.z + a.v[2][3]
		);
	}
	friend matrix operator * ( matrix a, matrix b ) {
		matrix c;
		for( int i=0; i<4; ++i ) {
			for( int j=0; j<4; ++j ) {
				c.v[i][j] = a.v[i][0]*b.v[0][j] + a.v[i][1]*b.v[1][j] + a.v[i][2]*b.v[2][j] + a.v[i][3]*b.v[3][j];
			}
		}
		return c;
	}
	void setRotationAxis( float3 axis, float angle ) {
		float3 u = normalize( axis );
		float cosa, sina;
		ncSinCos( angle, sina, cosa );
		v[0][0] = cosa + u.x*u.x*(1.f - cosa);
		v[0][1] = u.x*u.y*(1.f-cosa) - u.z*sina;
		v[0][2] = u.x*u.z*(1.f-cosa) + u.y*sina;
		v[0][3] = 0.0f;

		v[1][0] = u.y*u.x*(1.0f-cosa) + u.z*sina;
		v[1][1] = cosa + u.y*u.y*(1-cosa);
		v[1][2] = u.y*u.z*(1-cosa) - u.x*sina;
		v[1][3] = 0.0f;

		v[2][0] = u.z*u.x*(1.0f - cosa) - u.y*sina;
		v[2][1] = u.z*u.y*(1.0f - cosa) + u.x*sina;
		v[2][2] = cosa + u.z*u.z*(1.0f - cosa);
		v[2][3] = 0.0f;

		v[3][0] = v[3][1] = v[3][2] = 0.0f;
		v[3][3] = 1.0f;
	}
	void setTranslation( float3 a ) {
		setIdentity();
		v[0][3] = a.x;
		v[1][3] = a.y;
		v[2][3] = a.z;
	}
	void setIdentity() {
		for( int i=0; i<4; ++i )
			for( int j=0; j<4; ++j )
				v[i][j] = (i==j) ? 1.0f : 0.0f;
	}
};

#endif