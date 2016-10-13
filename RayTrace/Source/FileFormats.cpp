#include <stdio.h>
#include "RayTrace.h"

#pragma warning( disable : 4996 )

static int s_Progress = 0;

struct TextFileReadStream
{
	int  bufp, bufsize;
	char buf[1024];
	FILE * fp;

	TextFileReadStream( const char * pc ) {
		fp = fopen( pc, "rt" );
		bufsize = bufp = 0;
	}
	~TextFileReadStream() {
		if( fp ) fclose( fp );
	}
	bool Exist() const {
		return fp;
	}
	bool EndOfFile() {
		return feof( fp ) && bufp == bufsize;
	}
	void Reset() {
		fseek( fp, 0, SEEK_SET );
		bufp = bufsize = 0;
	}
	void ReadLine( char * pcLine ) {
		int i = 0;
		for( ;; ) {
			if( bufp == bufsize ) {
				bufp = 0;
				bufsize = fread( buf, 1, 1024, fp );
				if( !bufsize ) break;
			}
			char c = buf[bufp++];
			if( c > 13 )
				pcLine[i++] = c;
			else if( i ) break;
		}
		pcLine[i] = 0;
	}
};

int GetLoadingProgress()
{
	return s_Progress;
}

int RT_LoadObjFile( const char * pcFile, Triangle ** pTris )
{
	TextFileReadStream f( pcFile );
	if( !f.Exist())
		return 0;

	uint32 t0 = GetTime();

	s_Progress = 0;

	int faces = 0;
	char line[1024];
	while( !f.EndOfFile()) {
		f.ReadLine( line );
		if( line[0] == 'f') {
			for( char * p = line; p; p = strchr( p+1, '/' ), ++faces );
			faces -= 3;
		}
	}

	std::vector<Vec3> v;
	std::vector<Vec2> vt;

	*pTris = new Triangle[faces];
	int tri = 0;
	int curLine = 0;

	f.Reset();
	while( !f.eof()) {
		Vec3 pos;
		Vec2 uv;
		int ipos[3], iuv[3];
		f.ReadLine( line );
		curLine++;
		switch( line[0] ) {
			case 'v':
				if( line[1] == 't') {
					sscanf( line+3, "%f %f", &uv.x, &uv.y );
					vt.push_back( uv );
				} else {
					sscanf( line+3, "%f %f %f", &pos.x, &pos.y, &pos.z );
					v.push_back( pos );
				}
				break;
			case 'f': {
				char * p = line+2;
				for( int i=0; p; ) {
					sscanf( p, "%d/%d", ipos+i, iuv+i );
					if( ipos[i] > (int)v.size())
						break;
					if( iuv[i] > (int)vt.size())
						iuv[i] = 1;
					if( i == 2 ) {
						Triangle & t = (*pTris)[tri];
						for( int j=0; j<3; ++j ) {
							t.pos[j] = v[ ipos[j]-1 ];
#ifdef MATERIAL_TRIANGLES
							t.uv[j] = vt[ iuv[j]-1 ];
#endif
						}

#ifndef COMPACT_TRIANGLES
						t.n = cross( t.pos[1] - t.pos[0], t.pos[2] - t.pos[0] );
						if( len( t.n ) < 0.00001f )
							break;
						t.n = normalize( t.n );
						t.d = -dot( t.n, t.pos[0] );

#ifdef BARYCENTRIC_DATA_TRIANGLES
						t.v0 = t.pos[1] - t.pos[0];
						t.v1 = t.pos[2] - t.pos[0];
						t.dot00 = dot( t.v0, t.v0 );
						t.dot01 = dot( t.v0, t.v1 );
						t.dot11 = dot( t.v1, t.v1 );
						t.invDenom = 1.f / (t.dot00 * t.dot11 - t.dot01 * t.dot01);
#endif
#endif
						ipos[1] = ipos[2];
						iuv[1] = iuv[2];

						tri++;
					} else
						i++;
					p = strchr( p+1, ' ' );
					if( p && !strchr( p, '/' )) p = 0;
				}
				s_Progress = tri * 100 / faces;
				break;
			}
		}
	}
	s_Progress = 100;
	return tri;
}

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