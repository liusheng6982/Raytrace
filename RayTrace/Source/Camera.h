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
	int w,h;
};


void CoverageQTree( int x, int y, int w, int h, int mask )
{
	if( mask & 1 )
		AddPixel( x, y );
	if( mask & 2 )
		AddPixel( x+w-1, y );
	if( mask & 3 )
		AddPixel( x, y+h-1 );

	if( w > 2 ) {
		CoverageQTree( x, y, w/2, h/2, 0 );
		CoverageQTree( x+w/2, y+h/2, w/2, h/2, 7 );
		CoverageQTree( x+w/2, y, w/2, h/2, 1 );
		CoverageQTree( x, y+h/2, w/2, h/2, 1 );
	} else
		AddPixel( x+w-1, y+h-1 );
}



class ImageTile
{
	Image * pImage;
	int x,y,w,h;





};

#endif