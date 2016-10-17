#include <stdio.h>
#include <vector>
#include "ObjectFileLoader.h"

#define BUCKET_SIZE 1024

using namespace std;

#pragma warning( disable : 4996 )



class ObjectFileLoader : public IObjectFileLoader
{
public:
	ObjectFileLoader() : m_Progress(0) {}
	virtual ~ObjectFileLoader() {
		EraseContainer( m_Positions );
		EraseContainer( m_UVs );
		EraseContainer( m_Triangles );
	}

	virtual bool Load( const char * pcFilename );
	virtual int  GetLoadingProgress() {
		return m_Progress;
	}
	virtual const float3 * GetVertexPos( int i ) {
		return &m_Positions[i/BUCKET_SIZE][i%BUCKET_SIZE];
	}
	virtual const float2 * GetVertexUV( int i ) {
		return &m_UVs[i/BUCKET_SIZE][i%BUCKET_SIZE];
	}
	virtual const Triangle * GetTriangle( int i ) {
		return &m_Triangles[i/BUCKET_SIZE][i%BUCKET_SIZE];
	}
	virtual int GetNumTriangles() {
		return m_TriangleCount;
	}

	template< typename T > void AddElement( T & el, vector<T*> & container, int & count ) {
		if( count % BUCKET_SIZE == 0 )
			container.push_back( new T[BUCKET_SIZE] );
		container.back()[ count % BUCKET_SIZE ] = el;
		count++;
	}
	template< typename T > void EraseContainer( vector<T*> & container ) {
		for( size_t i=0; i<container.size(); ++i )
			delete[] container[i];
		container.clear();
	}

	vector<float3*> m_Positions;
	vector<float2*> m_UVs;
	vector<Triangle*> m_Triangles;
	int m_PositionCount, m_UVCount, m_TriangleCount;

	mutable int m_Progress;
};



struct TextFileReadStream
{
	int  bufp, bufsize;
	char buf[1024];
	FILE * fp;
	int size;

	TextFileReadStream( const char * pc ) {
		fp = fopen( pc, "rt" );
		bufsize = bufp = 0;
		fseek( fp, 0, SEEK_END );
		size = ftell( fp );
		fseek( fp, 0, SEEK_SET );
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
	int GetOffset() {
		return ftell(  fp );
	}
	int GetSize() {
		return size;
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


bool ObjectFileLoader::Load( const char * pcFilename )
{
	TextFileReadStream f( pcFilename );
	if( !f.Exist())
		return false;

	m_Progress = 0;
	m_PositionCount = m_UVCount = m_TriangleCount = 0;

	char line[1024];
	while( !f.EndOfFile()) {

		f.ReadLine( line );

		switch( line[0] ) {
			case 'v':
				if( line[1] == 't') {
					float2 uv;
					sscanf( line+3, "%f %f", &uv.x, &uv.y );
					AddElement( uv, m_UVs, m_UVCount );
				} else {
					float3 pos;
					sscanf( line+3, "%f %f %f", &pos.x, &pos.y, &pos.z );
					AddElement( pos, m_Positions, m_PositionCount );
				}
				break;
			case 'f': {
				Triangle f;
				char * p = line+2;
				for( int i=0; p; ) {
					sscanf( p, "%d/%d", f.pos + i, f.uv + i );
					f.pos[i]--;
					f.uv[i]--;

					if( f.pos[i] >= (int)m_Positions.size())
						break;
					if( f.uv[i] >= (int)m_UVs.size())
						f.uv[i] = 0;

					// add a face per each vertex after first 2 - create a fan of triangles
					if( i == 2 ) {
						AddElement( f, m_Triangles, m_TriangleCount );
						f.pos[1] = f.pos[2];
						f.uv[1] = f.uv[1];
					} else
						i++;

					p = strchr( p+1, ' ' );
					if( !p || !strchr( p, '/' ))
						break;
				}
				break;
			}
		}
		m_Progress = f.GetOffset() * 100 / f.GetSize();
	}
	m_Progress = 100;
	return true;
}

IObjectFileLoader * IObjectFileLoader::Create()
{
	return new ObjectFileLoader();
}