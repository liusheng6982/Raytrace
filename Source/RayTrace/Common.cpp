#include <Windows.h>
#include "RayTrace.h"


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