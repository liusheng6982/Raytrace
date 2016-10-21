#ifndef __INC_NANOCORE_MATH
#define __INC_NANOCORE_MATH

#include "Common.h"



float ncSqrt( float x );
float ncTan( float x );


struct float2
{
	union {
		float v[2];
		struct {
			float x,y;
		};
	};

	float2() {}
	float2( const float2 & f3 ) { x = f3.x; y = f3.y; }
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

#endif