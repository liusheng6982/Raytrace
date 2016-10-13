#include <Windows.h>
#include "RayTrace.h"

void Camera::InitRay( int x, int y, RayInfo & ray ) {
	ray.pos = pos;
	ray.hitlen = 1000000000.f;
	ray.tri = 0;
	float kx = (x*2.f/width - 1) * tanf( fovy * 3.1415f / 180.0f / 2 ) * width / height;
	float ky = (y*2.f/height - 1) * tanf( fovy * 3.1415f / 180.0f / 2 );
	ray.dir = normalize( at + right * kx + up * ky );
}

void Camera::GetAxes( Vec3 & _at, Vec3 & _up, Vec3 & _right )
{
	_right = right * (tanf( fovy * 3.1415f / 180.0f / 2 ) * width / height);
	_up = up * tanf( fovy * 3.1415f / 180.0f / 2 );
	_at = at;
}

void Camera::Orthogonalize() {
	at = normalize( at );
	right = normalize( cross( up, at ));
	up = normalize( cross( right, at ));
}

uint32 GetTime()
{
	return timeGetTime();
}

void ComputeGridDestribution( int w, int h, int * order )
{
	int cx=0, cy=0;
	float * grid = new float[w*h];
	for( int i=0; i<w*h; grid[i++] = 0 );

	for( int i=0; i<w*h; ++i ) {
		grid[cx+cy*w] = 0;
		order[i] = cx+cy*w;

		int nx=0,ny=0;
		float maxd2 = 0;
		for( int y=0; y<h; ++y ) {
			int dy = cy-y;
			if( dy > h/2 ) dy -= h; else if( dy <= -h/2 ) dy += h;
			for( int x=0; x<w; ++x ) {
				int dx = cx-x;
				if( dx > w/2 ) dx -= w; else if( dx < -w/2 ) dx += w;
				float d2 = sqrtf( (float)dx*dx + dy*dy );

				if( !i )
					grid[x+y*w] = d2;
				else if( grid[x+y*w] > 0 )
					grid[x+y*w] = min( grid[x+y*w]*0.9999f, d2 );

				if( grid[x+y*w] > maxd2 ) {
					maxd2 = grid[x+y*w];
					nx = x;
					ny = y;
				}
			}
		}
		cx = nx;
		cy = ny;
	}
}