#include "Camera.h"

void Camera::GetAxes( float3 & _at, float3 & _up, float3 & _right )
{
	_right = right * (ncTan( fovy * 3.1415f / 180.0f / 2 ) * width / height);
	_up = up * ncTan( fovy * 3.1415f / 180.0f / 2 );
	_at = at;
}

void Camera::Orthonormalize() {
	at = normalize( at );
	right = normalize( cross( up, at ));
	up = normalize( cross( right, at ));
}

void Camera::LookAt( float3 lookat_pos, float3 at_vec )
{
	pos = lookat_pos - at_vec;
	at = normalize( at_vec );
	up = normalize( world_up );
	Orthonormalize();
}

void Camera::Rotate( float pitch, float yaw )
{
	matrix m1, m2, m;
	m1.setRotationAxis( world_up, yaw );

	float3 nr = m1 * right;

	m2.setRotationAxis( nr, pitch );

	m = m2 * m1;

	at = m * at;
	right = m * right;
	up = m * up;
}