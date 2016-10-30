#include <stdio.h>
#include <vector>
#include "ObjectFileLoader.h"

#define BUCKET_SIZE 1024

using namespace std;

#pragma warning( disable : 4996 )



class ObjectFileLoader : public IObjectFileLoader
{
public:
	ObjectFileLoader() : m_Progress(0), m_TriangleCount(0), m_PositionCount(0), m_UVCount(0) {}
	virtual ~ObjectFileLoader() {
		EraseContainer( m_Positions );
		EraseContainer( m_UVs );
		EraseContainer( m_Triangles );
	}

	virtual const wchar_t * GetFilename() const {
		return m_wFilename.c_str();
	}

	virtual bool Load( const wchar_t * pwFilename );

	virtual int  GetLoadingProgress() {
		return m_Progress;
	}
	virtual const float3 * GetVertexPos( int i ) {
		return (i < m_PositionCount) ? &m_Positions[i/BUCKET_SIZE][i%BUCKET_SIZE] : NULL;
	}
	virtual const float2 * GetVertexUV( int i ) {
		return (i < m_UVCount) ? &m_UVs[i/BUCKET_SIZE][i%BUCKET_SIZE] : NULL;
	}
	virtual const Triangle * GetTriangle( int i ) {
		return (i < m_TriangleCount) ? &m_Triangles[i/BUCKET_SIZE][i%BUCKET_SIZE] : NULL;
	}
	virtual int GetNumTriangles() {
		return m_TriangleCount;
	}

	void Save( const wchar_t * pwFilename );

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
	wstring m_wFilename;

	mutable int m_Progress;
};



struct TextFileReadStream
{
	int  bufp, bufsize;
	char buf[1024];
	FILE * fp;
	int size;

	TextFileReadStream( const wchar_t * pw ) {
		fp = _wfopen( pw, L"rb" );
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


bool ObjectFileLoader::Load( const wchar_t * pwFilename )
{
	m_Progress = 0;
	m_wFilename = pwFilename;

	wstring wFilenameCached( pwFilename );
	wFilenameCached += L".cached";

	FILE * fp = _wfopen( wFilenameCached.c_str(), L"rb" );
	if( fp ) {
		fread( &m_PositionCount, sizeof(m_PositionCount), 1, fp );
		fread( &m_UVCount, sizeof(m_UVCount), 1, fp );
		fread( &m_TriangleCount, sizeof(m_TriangleCount), 1, fp );

		EraseContainer( m_Positions );
		for( int count = m_PositionCount; count > 0; count -= BUCKET_SIZE ) {
			m_Positions.push_back( new float3[BUCKET_SIZE] );
			fread( m_Positions.back(), min( BUCKET_SIZE, count )*sizeof(float3), 1, fp );
		}
		EraseContainer( m_UVs );
		for( int count = m_UVCount; count > 0; count -= BUCKET_SIZE ) {
			m_UVs.push_back( new float2[BUCKET_SIZE] );
			fread( m_UVs.back(), min( BUCKET_SIZE, count )*sizeof(float2), 1, fp );
		}
		EraseContainer( m_Triangles );
		for( int count = m_TriangleCount; count > 0; count -= BUCKET_SIZE ) {
			m_Triangles.push_back( new Triangle[BUCKET_SIZE] );
			fread( m_Triangles.back(), min( BUCKET_SIZE, count )*sizeof(Triangle), 1, fp );
		}
		fclose( fp );
		return true;
	}

	TextFileReadStream f( pwFilename );
	if( !f.Exist())
		return false;

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
					if( f.pos[i] > 0 )
						f.pos[i]--;
					else
						f.pos[i] = m_PositionCount + f.pos[i];

					if( f.uv[i] > 0 )
						f.uv[i]--;
					else
						f.uv[i] = m_UVCount + f.uv[i];

					if( f.pos[i] >= m_PositionCount )
						break;
					if( f.uv[i] >= m_UVCount )
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
		m_Progress = int( int64(f.GetOffset()) * 100 / int64(f.GetSize()));
	}
	m_Progress = 100;
	Save( wFilenameCached.c_str());
	return true;
}

void ObjectFileLoader::Save( const wchar_t * pwFilename )
{
	FILE * fp = _wfopen( pwFilename, L"wb" );
	if( !fp )
		return;

	fwrite( &m_PositionCount, sizeof(m_PositionCount), 1, fp );
	fwrite( &m_UVCount, sizeof(m_UVCount), 1, fp );
	fwrite( &m_TriangleCount, sizeof(m_TriangleCount), 1, fp );

	for( int i=0, count = m_PositionCount; count > 0; ++i, count -= BUCKET_SIZE ) {
		fwrite( m_Positions[i], min( BUCKET_SIZE, count )*sizeof(float3), 1, fp );
	}
	for( int i=0, count = m_UVCount; count > 0; ++i, count -= BUCKET_SIZE ) {
		fwrite( m_UVs[i], min( BUCKET_SIZE, count )*sizeof(float2), 1, fp );
	}
	for( int i=0, count = m_TriangleCount; count > 0; ++i, count -= BUCKET_SIZE ) {
		fwrite( m_Triangles[i], min( BUCKET_SIZE, count )*sizeof(Triangle), 1, fp );
	}
	fclose( fp );
}

IObjectFileLoader * IObjectFileLoader::Create()
{
	return new ObjectFileLoader();
}