#include <stdio.h>
#include <vector>
#include <map>
#include <string>
#include <NanoCore/File.h>
#include <NanoCore/Threads.h>
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
		return (i >= 0 && i < m_PositionCount) ? &m_Positions[i/BUCKET_SIZE][i%BUCKET_SIZE] : NULL;
	}
	virtual const float2 * GetVertexUV( int i ) {
		return (i >= 0 && i < m_UVCount) ? &m_UVs[i/BUCKET_SIZE][i%BUCKET_SIZE] : NULL;
	}
	virtual const Triangle * GetTriangle( int i ) {
		return (i >= 0 && i < m_TriangleCount) ? &m_Triangles[i/BUCKET_SIZE][i%BUCKET_SIZE] : NULL;
	}
	virtual int GetNumTriangles() {
		return m_TriangleCount;
	}
	virtual const Material * GetMaterial( int i ) {
		return &m_Materials[i];
	}
	virtual int GetNumMaterials() {
		return m_Materials.size();
	}

	void Save( const wchar_t * pwFilename );
	void LoadMaterialLibrary( const wchar_t * name );

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

	vector<float3*>   m_Positions;
	vector<float2*>   m_UVs;
	vector<Triangle*> m_Triangles;
	vector<Material>  m_Materials;
	map<string,int>   m_MaterialToIndex;

	int m_PositionCount, m_UVCount, m_TriangleCount;
	wstring m_wFilename;
	string m_Mtllib;

	volatile int m_Progress;
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

static void TrimLineEnding( char * buf ) {
	while( *buf ) {
		if( *buf == '\n' || *buf == '\r' ) {
			*buf = 0;
			break;
		} else
			buf++;
	}
}

void ObjectFileLoader::LoadMaterialLibrary( const wchar_t * name ) {
	FILE * fp = _wfopen( name, L"rt" );
	if( !fp )
		return;

	m_Materials.clear();

	while( !feof( fp )) {
		char buf[512];
		fgets( buf, 512, fp );
		strlwr( buf );
		TrimLineEnding( buf );

		const char * ptr;

		Material * mtl = m_Materials.empty() ? NULL : &m_Materials.back();

		if( !strncmp( buf, "newmtl", 6 )) {
			m_MaterialToIndex[ buf+7 ] = m_Materials.size();
			m_Materials.push_back(Material());
		} else if( ptr = strstr( buf, "map_kd" ))
			mtl->mapKd = ptr+7;
		else if( ptr = strstr( buf, "map_ks" ))
			mtl->mapKs = ptr+7;
		else if( ptr = strstr( buf, "map_bump" ))
			mtl->mapBump = ptr+9;
		else if( ptr = strstr( buf, "map_d" ))
			mtl->mapAlpha = ptr+6;
		else if( ptr = strstr( buf, "bump" ))
			mtl->mapBump = ptr+5;
		else if( ptr = strstr( buf, "kd" ))
			sscanf( ptr+3, "%f %f %f", &mtl->Kd.x, &mtl->Kd.y, &mtl->Kd.z );
		else if( ptr = strstr( buf, "ks" ))
			sscanf( ptr+3, "%f %f %f", &mtl->Ks.x, &mtl->Ks.y, &mtl->Ks.z );
		else if( ptr = strstr( buf, "ke" ))
			sscanf( ptr+3, "%f %f %f", &mtl->Ke.x, &mtl->Ke.y, &mtl->Ke.z );
		else if( ptr = strstr( buf, "tr" ))
			mtl->Transparency = (float)atof( buf+3 );
		else if( ptr = strstr( buf, "ns" ))
			mtl->Ns = (float)atof( ptr+3 );
	}
	fclose( fp );
}

bool ObjectFileLoader::Load( const wchar_t * pwFilename )
{
	m_Progress = 0;
	m_wFilename = pwFilename;

	wstring wFilenameCached( pwFilename );
	wFilenameCached += L".cached";

	FILE * fp = _wfopen( wFilenameCached.c_str(), L"rb" );
	if( fp ) {

		int len;
		fread( &len, sizeof(len), 1, fp );

		m_Mtllib.resize( len );
		fread( &m_Mtllib[0], len, 1, fp );

		fread( &m_PositionCount, sizeof(m_PositionCount), 1, fp );
		fread( &m_UVCount, sizeof(m_UVCount), 1, fp );
		fread( &m_TriangleCount, sizeof(m_TriangleCount), 1, fp );

		EraseContainer( m_Positions );
		for( int count = m_PositionCount; count > 0; count -= BUCKET_SIZE ) {
			m_Positions.push_back( new float3[BUCKET_SIZE] );
			fread( m_Positions.back(), Min( BUCKET_SIZE, count )*sizeof(float3), 1, fp );
		}
		EraseContainer( m_UVs );
		for( int count = m_UVCount; count > 0; count -= BUCKET_SIZE ) {
			m_UVs.push_back( new float2[BUCKET_SIZE] );
			fread( m_UVs.back(), Min( BUCKET_SIZE, count )*sizeof(float2), 1, fp );
		}
		EraseContainer( m_Triangles );
		for( int count = m_TriangleCount; count > 0; count -= BUCKET_SIZE ) {
			m_Triangles.push_back( new Triangle[BUCKET_SIZE] );
			fread( m_Triangles.back(), Min( BUCKET_SIZE, count )*sizeof(Triangle), 1, fp );
		}
		fclose( fp );
		NanoCore::DebugOutput( "OBJ file: %ls\n\t%d vertices\n\t%d UV coords\n\t%d triangles\n", pwFilename, m_PositionCount, m_UVCount, m_TriangleCount );

		wstring name = NanoCore::StrGetPath( pwFilename );
		name += NanoCore::StrMbsToWcs( m_Mtllib.c_str());
		LoadMaterialLibrary( name.c_str() );

		return true;
	}

	TextFileReadStream f( pwFilename );
	if( !f.Exist())
		return false;

	m_PositionCount = m_UVCount = m_TriangleCount = 0;

	int materialID = 0, lineNum = 0;

	aabb box;

	char line[1024];
	while( !f.EndOfFile()) {

		f.ReadLine( line );
		lineNum++;

		switch( line[0] ) {
			case 'm':
				if( !strnicmp( line, "mtllib ", 7 )) {
					m_Mtllib = line + strlen( "mtllib " );
					std::wstring name = NanoCore::StrGetPath( pwFilename ) + NanoCore::StrMbsToWcs( m_Mtllib.c_str() );
					LoadMaterialLibrary( name.c_str() );
				}
				break;
			case 'u':
				if( !strncmp( line, "usemtl ", 7 )) {
					materialID = m_MaterialToIndex[line+7];
				}
				break;
			case 'v':
				if( line[1] == 't') {
					float2 uv;
					sscanf( line+3, "%f %f", &uv.x, &uv.y );
					AddElement( uv, m_UVs, m_UVCount );
				} else if( line[1] == ' ' ) {
					float3 pos;
					sscanf( line+2, "%f %f %f", &pos.x, &pos.y, &pos.z );

					if( !m_PositionCount )
						box.min = box.max = pos;
					else
						box += pos;
					AddElement( pos, m_Positions, m_PositionCount );
				}
				break;
			case 'f': {
				Triangle f;
				f.material = materialID;
				assert( materialID >= 0 && materialID < m_Materials.size() );
				char * p = line+2;

				bool bTexCoords = strchr( p, '/' ) != NULL;

				for( int i=0; p; ) {

					if( bTexCoords ) {
						if( sscanf( p, "%d/%d", f.pos + i, f.uv + i ) != 2 )
							break;
					} else {
						if( sscanf( p, "%d", f.pos + i ) != 1 )
							break;
						f.uv[0] = f.uv[1] = f.uv[2] = -1;
					}

					if( f.pos[i] > 0 )
						f.pos[i]--;
					else
						f.pos[i] = m_PositionCount + f.pos[i];

					if( bTexCoords ) {
						if( f.uv[i] > 0 )
							f.uv[i]--;
						else
							f.uv[i] = m_UVCount + f.uv[i];
					}

					if( f.pos[i] >= m_PositionCount )
						break;
					if( bTexCoords && f.uv[i] >= m_UVCount )
						f.uv[i] = 0;

					// add a face per each vertex after first 2 - create a fan of triangles
					if( i == 2 ) {
						AddElement( f, m_Triangles, m_TriangleCount );
						f.pos[1] = f.pos[2];
						f.uv[1] = f.uv[2];
					} else
						i++;

					p = strchr( p+1, ' ' );
					if( !p )
						break;
				}
				break;
			}
		}
		m_Progress = int( int64(f.GetOffset()) * 100 / int64(f.GetSize()));
	}
	NanoCore::DebugOutput( "OBJ file: %ls\n\t%d vertices\n\t%d UV coords\n\t%d triangles\n", pwFilename, m_PositionCount, m_UVCount, m_TriangleCount );
	NanoCore::DebugOutput( "\tbox min: %0.3f, %0.3f, %0.3f\n\tbox max: %0.3f, %0.3f, %0.3f\n", box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z );
	m_Progress = 100;
	Save( wFilenameCached.c_str());
	return true;
}

void ObjectFileLoader::Save( const wchar_t * pwFilename )
{
	FILE * fp = _wfopen( pwFilename, L"wb" );
	if( !fp )
		return;

	int len = m_Mtllib.size();
	fwrite( &len, sizeof(len), 1, fp );
	fwrite( &m_Mtllib[0], len, 1, fp );

	fwrite( &m_PositionCount, sizeof(m_PositionCount), 1, fp );
	fwrite( &m_UVCount, sizeof(m_UVCount), 1, fp );
	fwrite( &m_TriangleCount, sizeof(m_TriangleCount), 1, fp );

	for( int i=0, count = m_PositionCount; count > 0; ++i, count -= BUCKET_SIZE ) {
		fwrite( m_Positions[i], Min( BUCKET_SIZE, count )*sizeof(float3), 1, fp );
	}
	for( int i=0, count = m_UVCount; count > 0; ++i, count -= BUCKET_SIZE ) {
		fwrite( m_UVs[i], Min( BUCKET_SIZE, count )*sizeof(float2), 1, fp );
	}
	for( int i=0, count = m_TriangleCount; count > 0; ++i, count -= BUCKET_SIZE ) {
		fwrite( m_Triangles[i], Min( BUCKET_SIZE, count )*sizeof(Triangle), 1, fp );
	}
	fclose( fp );
}

IObjectFileLoader * IObjectFileLoader::Create()
{
	return new ObjectFileLoader();
}

IObjectFileLoader::Material::Material() {
	Ns = 0.0f;
	Transparency = 0.0f;
	Ka = float3(0,0,0);
	Kd = float3(0,0,0);
	Ks = float3(0,0,0);
	Ke = float3(0,0,0);
}