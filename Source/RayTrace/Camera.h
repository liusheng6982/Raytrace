#ifndef ___INC_RAYTRACE_CAMERA
#define ___INC_RAYTRACE_CAMERA

#include <NanoCore/Mathematics.h>

class Camera {
public:
	float3 pos, at,up,right;
	float fovy;
	int width, height;

	void Orthonormalize();
	void GetAxes( float3 & _at, float3 & _up, float3 & _right );
};

class Image
{
public:
	int w,h;
	uint8 * pBuffer;

	Image() : pBuffer(NULL), w(0), h(0) {}
	~Image() { delete[] pBuffer; }

	void Fill( uint32 color );
};





/*int size2 = 6, size = 1 << size2;

for( int i=size2; i>=0; ++i ) {
	int step = 1<<i;
	for( int y=0; y<size; y += step )
		for( int x=0; x<size; x += step ) {
			if( (x & step) || (y & step) ) {  // only compute pixels, that don't have BOTH coordinates are bigger pow2 - those are already computed

			} else {

			}
		}
}*/

#endif