#include <stdio.h>
#include "FileOBJ.h"

#pragma warning( disable : 4996 )



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
		return fp != NULL;
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


bool FileOBJ::Load( const char * pcFilename )
{
	TextFileReadStream f( pcFilename );
	if( !f.Exist())
		return false;

	uint32 t0 = GetTime();

	m_Progress = 0;

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

	*m_pTris = new Triangle[faces];
	m_numTris = faces;

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
					Triangle & t = (*m_pTris)[tri];
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
			m_Progress = tri * 100 / faces;
			break;
							}
		}
	}
	m_Progress = 100;
	return true;
}