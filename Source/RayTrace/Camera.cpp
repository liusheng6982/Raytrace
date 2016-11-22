#include "Camera.h"
#include <NanoCore/Serialize.h>

void Camera::Orthonormalize() {
	at = normalize( at ); // z
	right = normalize( cross( up, at ));  // x
	up = normalize( cross( at, right ));  // y
}

void Camera::LookAt( float3 lookat_pos, float3 at_vec ) {
	pos = lookat_pos - at_vec;
	at = normalize( at_vec );
	up = normalize( world_up );
	Orthonormalize();
}

void Camera::Rotate( float pitch, float yaw ) {
	matrix m1, m2, m;
	m1.setRotationAxis( world_up, yaw );

	float3 nr = m1 * right;

	m2.setRotationAxis( nr, pitch );

	m = m2 * m1;

	at = m * at;
	right = m * right;
	up = m * up;
}

float3 Camera::ConstructRay( int x, int y, int width, int height ) {
	float angle = DEG2RAD(fovy) * 0.5f;
	float tan = ncTan( angle );
	float kx = (x*2.f/width - 1) * tan * float(width) / height;
	float ky = (y*2.f/height - 1) * tan;
	return normalize( at + right * kx + up * ky );
}

void Serialize( float3 & f, NanoCore::XmlNode * node ) {
	node->SerializeAttrib( "x", f.x );
	node->SerializeAttrib( "y", f.y );
	node->SerializeAttrib( "z", f.z );
}

void Camera::Serialize( NanoCore::XmlNode * node ) {
	::Serialize( pos, node->SerializeChild( "pos" ));
	::Serialize( at, node->SerializeChild( "at" ));
	::Serialize( up, node->SerializeChild( "up" ));
	::Serialize( right, node->SerializeChild( "right" ));
	::Serialize( world_up, node->SerializeChild( "world_up" ));

	NanoCore::XmlNode * n = node->SerializeChild( "fovy" );
	if( !n->Get( fovy ))
		n->Set( fovy );
}