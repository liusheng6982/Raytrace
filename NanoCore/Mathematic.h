#ifndef __INC_NANOCORE_MATH
#define __INC_NANOCORE_MATH

#include "Common.h"



float ncSqrt( float x );



struct ncVec3
{
	union {
		float v[3];
		struct {
			float x,y,z;
		};
	};

	ncVec3() {}
	ncVec3( float _x ) { x = y = z = _x; }
	ncVec3( float _x, float _y, float _z ) { x = _x; y = _y; z = _z; }
	ncVec3( ncFloat3 f3 ) { x = f3.x; y = f3.y; z = f3.z; }

	float & operator[] ( int i ) { return v[i]; }
	float operator[]( int i ) const { return v[i]; }

	void operator += ( ncVec3 v ) {
		x += v.x; y += v.y; z += v.z;
	}
	void operator += ( float k ) {
		x += k; y += k; z += k;
	}
	void operator *= ( float k ) {
		x *= k; y *= k; z *= k;
	}

	friend ncVec3 operator + ( ncVec3 a, ncVec3 b ) {
		return ncVec3( a.x + b.x, a.y + b.y, a.z + b.z );
	}
	friend ncVec3 operator - ( ncVec3 a, ncVec3 b ) {
		return ncVec3( a.x - b.x, a.y - b.y, a.z - b.z );
	}
	friend ncVec3 operator * ( ncVec3 a, float k ) {
		return ncVec3( a.x*k, a.y*k, a.z*k );
	}
	friend ncVec3 operator * ( float k, ncVec3 a ) {
		return a * k;
	}
	friend ncVec3 operator / ( ncVec3 a, ncVec3 b ) {
		return ncVec3( a.x / b.x, a.y / b.y, a.z / b.z );
	}
	friend float dot( ncVec3 a, ncVec3 b ) {
		return a.x*b.x + a.y*b.y + a.z*b.z;
	}
	friend ncVec3 cross( ncVec3 a, ncVec3 b ) {
		return ncVec3( a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x );
	}
	friend ncVec3 reflect( ncVec3 v, ncVec3 n ) {
		return v - n * dot(v,n)*2;
	}
	friend float len( ncVec3 v ) {
		return ncSqrt( v.x*v.x + v.y*v.y + v.z*v.z );
	}
	friend ncVec3 normalize( ncVec3 v ) {
		float k = 1.f / len( v );
		return ncVec3( v.x * k, v.y * k, v.z * k );
	}
};

#endif