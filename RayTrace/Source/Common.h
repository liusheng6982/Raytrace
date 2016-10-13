#ifndef __INC_RAYTRACE_COMMON
#define __INC_RAYTRACE_COMMON

#include <Math.h>
#include <NanoCore/Common.h>


//#define COMPACT_TRIANGLES
//#define MATERIAL_TRIANGLES
#define BARYCENTRIC_DATA_TRIANGLES

struct Triangle
{
	ncFloat3 pos[3];

#ifndef COMPACT_TRIANGLES
	ncFloat3 n;
	float    d;

	#ifdef BARYCENTRIC_DATA_TRIANGLES
		ncFloat3 v0, v1;
		float dot00, dot01, dot11, invDenom;
	#endif
#endif

#ifdef MATERIAL_TRIANGLES
	ncFloat2  uv[3];
	int       mtl;
#endif
};

#endif