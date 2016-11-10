#include "KDTree.h"
#include <string>
#include <NanoCore/File.h>
#include <NanoCore/Windows.h>

using namespace std;

#define EPSILON 0.000001



KDTree::KDTree() {
}

KDTree::~KDTree() {
}

void KDTree::Build( IObjectFileLoader * pModel, int maxTrianglesPerNode ) {
	m_Tree.clear();

	wstring wFile = pModel->GetFilename();
	wFile += L".kdtree";

	NanoCore::IFile::Ptr fp = NanoCore::FS::Open( wFile.c_str(), NanoCore::FS::efRead );
	if( fp ) {
		int numTris, numNodes;
		fp->Read( &numTris, sizeof(numTris) );
		fp->Read( &numNodes, sizeof(numNodes) );
		fp->Read( &m_maxTrianglesPerNode, sizeof(m_maxTrianglesPerNode) );

		if( m_maxTrianglesPerNode == maxTrianglesPerNode ) {
			m_Triangles.resize( numTris );
			m_Tree.resize( numNodes );
			for( int i=0; i<numTris; ++i )
				fp->Read( &m_Triangles[i], sizeof(Triangle) );
			for( int i=0; i<numNodes; ++i )
				fp->Read( &m_Tree[i], sizeof(Node) );
			return;
		}
	}

	const int numTris = pModel->GetNumTriangles();

	aabb bx;
	bx.reset();

	m_Triangles.resize( numTris );
	for( int i=0; i<numTris; ++i ) {
		Triangle & t = m_Triangles[i];
		const IObjectFileLoader::Triangle * p = pModel->GetTriangle( i );
		t.triangleID = i;
		t.mtl = p->material;

		for( int j=0; j<3; ++j ) {
			t.pos[j] = *pModel->GetVertexPos( p->pos[j] );
			const float2 * pUV = pModel->GetVertexUV( p->uv[j] );
			if( pUV ) t.uv[j] = *pUV;

			float x = t.pos[j].x;

			bx += t.pos[j];
		}

		t.n = cross( t.pos[1] - t.pos[0], t.pos[2] - t.pos[0] );
		//if( len( t.n ) < 0.00001f )
		//	break;
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
	}
	m_maxTrianglesPerNode = maxTrianglesPerNode;
	BuildTree( 0, numTris );

	if( NanoCore::WindowMain::MsgBox( L"Warning", L"Should we cache the KD-tree for faster loading?", true )) {
		fp = NanoCore::FS::Open( wFile.c_str(), NanoCore::FS::efWriteTrunc );
		if( fp ) {
			size_t numTris = m_Triangles.size();
			size_t numNodes = m_Tree.size();
			fp->Write( &numTris, sizeof(numTris) );
			fp->Write( &numNodes, sizeof(numNodes) );
			fp->Write( &m_maxTrianglesPerNode, sizeof(m_maxTrianglesPerNode) );
			for( int i=0; i<numTris; ++i )
				fp->Write( &m_Triangles[i], sizeof(Triangle) );
			for( int i=0; i<numNodes; ++i )
				fp->Write( &m_Tree[i], sizeof(Node) );
		}
	}
}

int KDTree::BuildTree( int l, int r ) {
	if( l >= r )
		return 0;

	float3 min = m_Triangles[l].pos[0], max = m_Triangles[l].pos[0];
	for( int i=l; i<r; ++i ) {
		const Triangle & t = m_Triangles[i];
		for( int j=0; j<3; ++j ) {
			const float3 & v = t.pos[j];
			if( v.x < min.x ) min.x = v.x; else if( v.x > max.x ) max.x = v.x;
			if( v.y < min.y ) min.y = v.y; else if( v.y > max.y ) max.y = v.y;
			if( v.z < min.z ) min.z = v.z; else if( v.z > max.z ) max.z = v.z;
		}
	}

	float3 size = max-min;
	int axis = 0;
	if( size.y > size.x ) axis = 1;
	if( size.z > size.x && size.z > size.y ) axis = 2;

	const float axislen = max[axis] - min[axis];
	const float separator = min[axis] + axislen * 0.5f;

	int l1 = l, l2 = r-1;

	int nL=0, nR=0, nC=0;

	if( r-l <= m_maxTrianglesPerNode ) {
		l1 = r;
	} else {
		for( int i=l; i<=l2; ++i ) {
			Triangle & t = m_Triangles[i];
			float tmin = t.pos[0][axis], tmax = t.pos[0][axis];

			float p2 = t.pos[1][axis], p3 = t.pos[2][axis];
			if( p2 < tmin ) tmin = p2; else if( p2 > tmax ) tmax = p2;
			if( p3 < tmin ) tmin = p3; else if( p3 > tmax ) tmax = p3;

			if( tmax < separator + axislen*0.3f ) {  // left
				nL++;
			} else if( tmin > separator - axislen*0.3f ) {  // right
				Triangle temp = t;
				t = m_Triangles[l2];
				m_Triangles[l2] = temp;
				l2--;
				i--;
				nR++;
			} else {  // stay in node
				if( l1 < i ) {
					Triangle temp = t;
					t = m_Triangles[l1];
					m_Triangles[l1] = temp;
				}
				l1++;
				nC++;
			}
		}
	}

	int node = (int)m_Tree.size();
	m_Tree.push_back(Node());

	int left_node = BuildTree(l1, l2+1);
	int right_node = BuildTree(l2+1, r);

	m_Tree[node].min = min;
	m_Tree[node].max = max;
	m_Tree[node].axis = axis;
	m_Tree[node].startTriangle = l;
	m_Tree[node].numTriangles = l1-l;
	m_Tree[node].left = left_node;
	m_Tree[node].right = right_node;
	return node;
}

int64 rays_traced = 0;

void KDTree::Intersect_r( int node_index, RayInfo & ray ) {
	const Node & node = m_Tree[node_index];

#if 1

	if( ray.pos.x < node.min.x || ray.pos.y < node.min.y || ray.pos.z < node.min.z ||
		ray.pos.x > node.max.x || ray.pos.y > node.max.y || ray.pos.z > node.max.z )
	{
		float3 dir = ray.dir;
		float3 bs( dir.x > 0 ? node.min.x : node.max.x, dir.y > 0 ? node.min.y : node.max.y, dir.z > 0 ? node.min.z : node.max.z );
		float3 len = bs - ray.pos;

		for( int i=0; i<3; ++i )
			len[i] = fabsf( dir[i] ) > 0.0001f ? len[i] / dir[i] : -10000.0f;

		int axis = 0;
		if( len.y > len.x ) axis = 1;
		if( len.z > len[axis] ) axis = 2;

		if( len[axis] <= 0 ) return;

		float3 i = ray.pos + dir * len[axis];
		switch( axis ) {
			case 0: if( i.y < node.min.y || i.y > node.max.y || i.z < node.min.z || i.z > node.max.z ) return; break;
			case 1: if( i.z < node.min.z || i.z > node.max.z || i.x < node.min.x || i.x > node.max.x ) return; break;
			case 2: if( i.x < node.min.x || i.x > node.max.x || i.y < node.min.y || i.y > node.max.y ) return; break;
		}

		// already have a hit, that is closer compared to the box intersection
		if( ray.tri && ray.hitlen < len[axis] ) return;
	}
#endif

	const int count = node.numTriangles;
	const Triangle * ptr = &m_Triangles[0] + node.startTriangle;
	for( int i=0; i<count; ++i ) {
		const Triangle & t = ptr[i];
		//(px + t.vx)*A + (py + t.vy)*B + (pz + t.vz)*C = -D
		//t = (-D - dot(p,N)) / dot(v,N)

		float NdotPos = dot( t.n, ray.pos );
		float NdotDir = dot( t.n, ray.dir );

		//if( NdotDir > -0.0001f ) continue;

		float k = (-t.d - NdotPos) / NdotDir;

		if( k > ray.hitlen || k < 0 ) continue;

		float3 hit = ray.pos + ray.dir * k;

#ifdef BARYCENTRIC_DATA_TRIANGLES
		float3 v0 = t.v0;
		float3 v1 = t.v1;
		float dot00 = t.dot00;
		float dot01 = t.dot01;
		float dot11 = t.dot11;
		float invDenom = t.invDenom;
#else
		float3 v0 = t.pos[1] - t.pos[0];
		float3 v1 = t.pos[2] - t.pos[0];
		float dot00 = dot( v0, v0 );
		float dot01 = dot( v0, v1 );
		float dot11 = dot( v1, v1 );
		float invDenom = 1.f / (dot00 * dot11 - dot01 * dot01);
#endif
		float3 v2 = hit - t.pos[0];
		float dot02 = dot( v0, v2 );
		float dot12 = dot( v1, v2 );

		// Compute barycentric coordinates

		float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
		float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

		// Check if point is in triangle

		if( u < -EPSILON || v < -EPSILON || u+v > 1+EPSILON ) continue;

		if( k < ray.hitlen ) {
			ray.n = t.n;
			ray.barycentric = float2( u, v );
			ray.hit = hit;
			ray.hitlen = k;
			ray.tri = &t;
		}
	}
	if( node.left )
		Intersect_r( node.left, ray );
	if( node.right )
		Intersect_r( node.right, ray );
}

void KDTree::Intersect( RayInfo & ray ) {
	ray.Clear();
	if( !m_Tree.empty())
		Intersect_r( 0, ray );
}

void RayInfo::Init( int x, int y, const Camera & cam, int width, int height ) {
	pos = cam.pos;
	hitlen = 1000000000.f;
	tri = 0;
	float kx = (x*2.f/width - 1) * tanf( cam.fovy * 0.5f ) * float(width) / height;
	float ky = (y*2.f/height - 1) * tanf( cam.fovy * 0.5f );
	dir = normalize( cam.at + cam.right * kx + cam.up * ky );
}

bool KDTree::Empty() {
	return m_Tree.empty();
}

aabb KDTree::GetBBox() {
	if( !Empty() ) {
		return aabb( m_Tree[0].min, m_Tree[0].max );
	} else {
		return aabb( float3(0,0,0), float3(0,0,0));
	}
}