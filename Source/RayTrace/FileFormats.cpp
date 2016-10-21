#include <stdio.h>
#include "RayTrace.h"

#pragma warning( disable : 4996 )



int WriteImage( const uint8 * pImage, int width, int height, const char * pcPathFileName )
{
	char pc[512];
	strcpy( pc, pcPathFileName );
	strcat( pc, ".bmp" );
	FILE * fp = fopen( pc, "wb" );
	if( !fp ) return 0;

	char sig[2] = {'B','M'};
	fwrite( sig, 1, 2, fp );

	int pw = width*3;
	if( pw % 4 ) pw += 4 - pw%4;

	int size = 14 + 40 + pw*height;
	fwrite( &size, 1, 4, fp );

	int zero = 0;
	fwrite( &zero, 1, 4, fp );

	int offset = 14+40;
	fwrite( &offset, 1, 4, fp );

	BITMAPINFOHEADER bmpih = { 40, width, height, 1, 24, 0, pw*height, 0, 0, 0, 0 };
	fwrite( &bmpih, 1, 40, fp );

	uint8 pad=0;
	for( int i=0; i<height; ++i ) {
		fwrite( pImage + i*width*3, 1, width*3, fp );
		for( int j=0; j<pw-width*3; ++j )
			fwrite( &pad, 1, 1, fp );
	}
	fclose( fp );
	return 1;
}