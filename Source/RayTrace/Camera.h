#ifndef ___INC_RAYTRACE_CAMERA
#define ___INC_RAYTRACE_CAMERA

#include <NanoCore/Mathematics.h>

class Camera {
public:
	float3 pos, at,up,right;
	float fovy;
	int width, height;

	float3 world_up;

	void LookAt( float3 lookat_pos, float3 at_vec );
	void Orthonormalize();
	void GetAxes( float3 & _at, float3 & _up, float3 & _right );
	void Rotate( float pitch, float yaw );
};

#endif