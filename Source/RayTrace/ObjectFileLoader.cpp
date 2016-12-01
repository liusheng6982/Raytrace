#include <vector>
#include <map>
#include <string>
#include <NanoCore/File.h>
#include <NanoCore/Threads.h>
#include <NanoCore/Windows.h>
#include "Common.h"




#define BUCKET_SIZE_SHIFT 12



#define BUCKET_SIZE (1 << BUCKET_SIZE_SHIFT)
#define BUCKET_DIV(i) ((i)>>BUCKET_SIZE_SHIFT)
#define BUCKET_MOD(i) ((i)&(BUCKET_SIZE-1))

using namespace std;

#pragma warning( disable : 4996 )

class ObjectFileLoader : public ISceneLoader {
public:
	ObjectFileLoader() : m_TriangleCount(0), m_PositionCount(0), m_UVCount(0) {}
	virtual ~ObjectFileLoader() {
		EraseContainer( m_Positions );
		EraseContainer( m_UVs );
		EraseContainer( m_Triangles );
	}

	virtual bool Load( const wchar_t * pwFilename, IStatusCallback * pCallback );

	virtual const wchar_t * GetFilename() const {
		return m_wFilename.c_str();
	}
	virtual const float3 * GetVertexPos( int i ) const {
		return (i >= 0 && i < m_PositionCount) ? &m_Positions[BUCKET_DIV(i)][BUCKET_MOD(i)] : NULL;
	}
	virtual const float2 * GetVertexUV( int i ) const {
		return (i >= 0 && i < m_UVCount) ? &m_UVs[BUCKET_DIV(i)][BUCKET_MOD(i)] : NULL;
	}
	virtual const float3 * GetVertexNormal( int i ) const {
		return (i >= 0 && i < m_NormalCount) ? &m_Normals[BUCKET_DIV(i)][BUCKET_MOD(i)] : NULL;
	}
	virtual const Triangle * GetTriangle( int i ) const {
		return (i >= 0 && i < m_TriangleCount) ? &m_Triangles[BUCKET_DIV(i)][BUCKET_MOD(i)] : NULL;
	}
	virtual int GetNumTriangles() const {
		return m_TriangleCount;
	}
	virtual const Material * GetMaterial( int i ) const {
		return &m_Materials[i];
	}
	virtual int GetNumMaterials() const {
		return (int)m_Materials.size();
	}

	void Save( const wchar_t * pwFilename, IStatusCallback * pCallback );
	void LoadMaterialLibrary( const wchar_t * name );
	void ShowStats( const wchar_t * name );

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
	vector<float3*>   m_Normals;
	vector<Triangle*> m_Triangles;
	vector<Material>  m_Materials;
	map<string,int>   m_MaterialToIndex;

	int m_PositionCount, m_UVCount, m_NormalCount, m_TriangleCount;
	wstring m_wFilename;
	string m_Mtllib;
};

void ObjectFileLoader::LoadMaterialLibrary( const wchar_t * name ) {
	NanoCore::IFile::Ptr fp = NanoCore::FS::Open( name, NanoCore::FS::efRead );
	if( !fp )
		return;

	NanoCore::TextFile tf( fp );

	m_Materials.clear();

	while( !tf.EndOfFile() ) {
		char buf[512];
		tf.ReadLine( buf, sizeof(buf) );
		strlwr( buf );

		Material * mtl = m_Materials.empty() ? NULL : &m_Materials.back();

		std::vector<std::string> words;
		NanoCore::StrSplit( buf, " \t", words );
		if( words.empty())
			continue;

#define READ_VEC3(v) v.x = (float)atof(words[1].c_str()), v.y = (float)atof(words[2].c_str()), v.z = (float)atof(words[3].c_str())

		if( !strcmp( words[0].c_str(), "newmtl" )) {
			std::string s( buf+7 );
			NanoCore::StrTrim( s );
			NanoCore::StrLwr( s );
			if( m_MaterialToIndex.find( s ) != m_MaterialToIndex.end()) {
				NanoCore::DebugOutput( "Warning: duplicate material found: %s\n", s.c_str());
			}
			m_MaterialToIndex[ s ] = (int)m_Materials.size();
			m_Materials.push_back(Material());
			m_Materials.back().name = s;
		} else if( !strcmp( words[0].c_str(), "map_kd" ))
			mtl->mapKd = words[ words.size()-1 ].c_str();
		else if( !strcmp( words[0].c_str(), "map_ks" ))
			mtl->mapKs = words[ words.size()-1 ].c_str();
		else if( !strcmp( words[0].c_str(), "map_bump" ))
			mtl->mapBump = words[ words.size()-1 ].c_str();
		else if( !strcmp( words[0].c_str(), "map_d" ))
			mtl->mapAlpha = words[ words.size()-1 ].c_str();
		else if( !strcmp( words[0].c_str(), "bump" ))
			mtl->mapBump = words[ words.size()-1 ].c_str();
		else if( !strcmp( words[0].c_str(), "map_ns" ))
			mtl->mapNs = words[ words.size()-1 ].c_str();
		else if( words[0] == "kd" )
		{READ_VEC3(mtl->Kd);}
		else if( words[0] == "ks" )
		{READ_VEC3(mtl->Ks);}
		else if( words[0] == "ke" )
		{READ_VEC3(mtl->Ke);}
		else if( words[0] == "tr" )
			mtl->Transparency = (float)atof( words[1].c_str() );
		else if( words[0] == "ns" )
			mtl->Ns = (float)atof( words[1].c_str() );
	}
}

void ObjectFileLoader::ShowStats( const wchar_t * name ) {
	NanoCore::DebugOutput( "OBJ file: %ls\n", name );
	NanoCore::DebugOutput( "\t%d vertices\n", m_PositionCount );
	NanoCore::DebugOutput( "\t%d UV coords\n", m_UVCount );
	NanoCore::DebugOutput( "\t%d normals\n", m_NormalCount );
	NanoCore::DebugOutput( "\t%d triangles\n", m_TriangleCount );
}

bool ObjectFileLoader::Load( const wchar_t * pwFilename, IStatusCallback * pCallback ) {
	m_wFilename = pwFilename;

	wstring wFilenameCached( pwFilename );
	wFilenameCached += L".cached";

	EraseContainer( m_Positions );
	EraseContainer( m_UVs );
	EraseContainer( m_Normals );
	EraseContainer( m_Triangles );
	m_PositionCount = m_UVCount = m_NormalCount = m_TriangleCount = 0;

	NanoCore::IFile::Ptr fp = NanoCore::FS::Open( wFilenameCached.c_str(), NanoCore::FS::efRead );
	if( fp ) {
		if( pCallback )
			pCallback->SetStatus( "Loading cached model" );

		int len;
		fp->Read( &len, sizeof(len) );

		m_Mtllib.resize( len );
		fp->Read( &m_Mtllib[0], len );

		fp->Read( &m_PositionCount, sizeof(m_PositionCount) );
		fp->Read( &m_UVCount, sizeof(m_UVCount) );
		fp->Read( &m_NormalCount, sizeof(m_NormalCount) );
		fp->Read( &m_TriangleCount, sizeof(m_TriangleCount) );

		for( int count = m_PositionCount; count > 0; count -= BUCKET_SIZE ) {
			m_Positions.push_back( new float3[BUCKET_SIZE] );
			fp->Read( m_Positions.back(), Min( BUCKET_SIZE, count )*sizeof(float3) );
		}
		for( int count = m_UVCount; count > 0; count -= BUCKET_SIZE ) {
			m_UVs.push_back( new float2[BUCKET_SIZE] );
			fp->Read( m_UVs.back(), Min( BUCKET_SIZE, count )*sizeof(float2) );
		}
		for( int count = m_NormalCount; count > 0; count -= BUCKET_SIZE ) {
			m_Normals.push_back( new float3[BUCKET_SIZE] );
			fp->Read( m_Normals.back(), Min( BUCKET_SIZE, count )*sizeof(float3) );
		}
		for( int count = m_TriangleCount; count > 0; count -= BUCKET_SIZE ) {
			m_Triangles.push_back( new Triangle[BUCKET_SIZE] );
			fp->Read( m_Triangles.back(), Min( BUCKET_SIZE, count )*sizeof(Triangle) );
		}
		ShowStats( pwFilename );

		wstring name = NanoCore::StrGetPath( pwFilename );
		name += NanoCore::StrMbsToWcs( m_Mtllib.c_str());
		LoadMaterialLibrary( name.c_str() );

		if( pCallback )
			pCallback->SetStatus( NULL );

		return true;
	}

	fp = NanoCore::FS::Open( pwFilename, NanoCore::FS::efRead );
	if( !fp )
		return false;
	NanoCore::TextFile tf( fp );

	int materialID = 0, lineNum = 0;
	AABB box;

	char line[1024];
	while( !tf.EndOfFile()) {
		tf.ReadLine( line, 1024 );
		lineNum++;

		if( lineNum % 256 == 0 && pCallback )
			pCallback->ShowLoadingProgress( "Parsing model (%d %%)", fp );

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
					std::string s( line+7 );
					NanoCore::StrTrim( s );
					NanoCore::StrLwr( s );
					if( m_MaterialToIndex.find( s ) == m_MaterialToIndex.end()) {
						NanoCore::DebugOutput( "Warning: material '%s' not found in material library!\n", s.c_str());
						materialID = 0;
					} else
						materialID = m_MaterialToIndex[s];
				}
				break;
			case 'v':
				if( line[1] == 't') {
					float2 uv;
					sscanf( line+3, "%f %f", &uv.x, &uv.y );
					//uv.x = 1.0f - uv.x;
					uv.y = 1.0f - uv.y;
					AddElement( uv, m_UVs, m_UVCount );
				} else if( line[1] == 'n' ) {
					float3 n;
					sscanf( line+3, "%f %f %f", &n.x, &n.y, &n.z );
					AddElement( n, m_Normals, m_NormalCount );
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
				assert( m_Materials.empty() || materialID >= 0 && materialID < m_Materials.size() );
				char * p = line+2;

				const char * sep1 = strchr( p, '/' );
				const char * sep2 = sep1 ? strchr(sep1+1, '/') : NULL;

				bool bNormals = sep2 != NULL;
				bool bUVs = sep1 != NULL;

				for( int i=0; p; ) {

					f.uv[i] = f.normal[i] = -1;
					if( bNormals ) {
						if( sscanf( p, "%d/%d/%d", f.pos+i, f.uv+i, f.normal+i ) != 3 )
							break;
					} else if( bUVs ) {
						if( sscanf( p, "%d/%d", f.pos + i, f.uv + i ) != 2 )
							break;
					} else {
						if( sscanf( p, "%d", f.pos + i ) != 1 )
							break;
					}

					f.pos[i] += (f.pos[i] > 0) ? -1 : m_PositionCount;
					if( bUVs ) {
						f.uv[i] += (f.uv[i] > 0) ? -1 : m_UVCount;
					}
					if( bNormals ) {
						f.normal[i] += (f.normal[i] > 0) ? -1 : m_NormalCount;
					}

					if( f.pos[i] >= m_PositionCount )
						break;

					// add a face per each vertex after first 2 - create a fan of triangles
					if( i == 2 ) {
						AddElement( f, m_Triangles, m_TriangleCount );
						f.pos[1] = f.pos[2];
						f.uv[1] = f.uv[2];
						f.normal[1] = f.normal[2];
					} else
						i++;

					p = strchr( p+1, ' ' );
					if( !p )
						break;
				}
				break;
			}
		}
	}
	if( pCallback ) pCallback->SetStatus( NULL );
	ShowStats( pwFilename );
	NanoCore::DebugOutput( "\tbox min: %0.3f, %0.3f, %0.3f\n\tbox max: %0.3f, %0.3f, %0.3f\n", box.min.x, box.min.y, box.min.z, box.max.x, box.max.y, box.max.z );
	if( NanoCore::WindowMain::MsgBox( L"Warning", L"Should we cache the OBJ file into binary for faster loading?", true ))
		Save( wFilenameCached.c_str(), pCallback );
	return true;
}

void ObjectFileLoader::Save( const wchar_t * pwFilename, IStatusCallback * pCallback ) {
	NanoCore::IFile::Ptr fp = NanoCore::FS::Open( pwFilename, NanoCore::FS::efWriteTrunc );
	if( !fp )
		return;

	if( pCallback )
		pCallback->SetStatus( "Caching model" );

	int len = (int)m_Mtllib.size();
	fp->Write( &len, sizeof(len) );
	fp->Write( &m_Mtllib[0], len );

	fp->Write( &m_PositionCount, sizeof(m_PositionCount) );
	fp->Write( &m_UVCount, sizeof(m_UVCount) );
	fp->Write( &m_NormalCount, sizeof(m_NormalCount) );
	fp->Write( &m_TriangleCount, sizeof(m_TriangleCount) );

	for( int i=0, count = m_PositionCount; count > 0; ++i, count -= BUCKET_SIZE ) {
		fp->Write( m_Positions[i], Min( BUCKET_SIZE, count )*sizeof(float3) );
	}
	for( int i=0, count = m_UVCount; count > 0; ++i, count -= BUCKET_SIZE ) {
		fp->Write( m_UVs[i], Min( BUCKET_SIZE, count )*sizeof(float2) );
	}
	for( int i=0, count = m_NormalCount; count > 0; ++i, count -= BUCKET_SIZE ) {
		fp->Write( m_Normals[i], Min( BUCKET_SIZE, count )*sizeof(float2) );
	}
	for( int i=0, count = m_TriangleCount; count > 0; ++i, count -= BUCKET_SIZE ) {
		fp->Write( m_Triangles[i], Min( BUCKET_SIZE, count )*sizeof(Triangle) );
	}
	if( pCallback )
		pCallback->SetStatus( NULL );
}

ISceneLoader * CreateObjLoader() {
	return new ObjectFileLoader();
}

ISceneLoader::Material::Material() {
	Ns = 0.0f;
	Transparency = 0.0f;
	Ka = float3(0,0,0);
	Kd = float3(0,0,0);
	Ks = float3(0,0,0);
	Ke = float3(0,0,0);
}