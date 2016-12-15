#include <string>
#include <NanoCore/File.h>
#include <NanoCore/Windows.h>
#include "Camera.h"
#include "Common.h"

using namespace std;

#define EPSILON 0.00001f
//#define BARYCENTRIC_DATA_TRIANGLES
//#define KEEP_TRIANGLE_ID



struct Triangle {
	float3  pos[3], n;
	float   d;

#ifdef BARYCENTRIC_DATA_TRIANGLES
	float3 v0, v1;
	float dot00, dot01, dot11, invDenom;
#endif

	float2 uv[3];
	float3 normal[3];
	int    mtl;

#ifdef KEEP_TRIANGLE_ID
	int triangleID;
#endif

	float2 GetUV( float2 barycentric_pos ) const {
		float u = barycentric_pos.x;
		float v = barycentric_pos.y;
		float w = 1.0f - u - v;
		return uv[0]*u + uv[1]*v + uv[2]*w;
	}
	float3 GetNormal( float2 barycentric_pos ) const {
		float u = barycentric_pos.x;
		float v = barycentric_pos.y;
		float w = 1.0f - u - v;
		return normalize( normal[0]*u + normal[1]*v + normal[2]*w );
	}
};



class KDTree : public IScene {
public:
	struct Node {
		float3 min, max;
		int axis;
		int startTriangle, numTriangles;
		int left, right;
	};

	KDTree( int maxTrianglesPerNode );
	~KDTree();

	virtual void Build( const ISceneLoader * pLoader, IStatusCallback * pCallback );
	virtual bool IntersectRay( const Ray & ray, IntersectResult & hit ) const;
	virtual bool IsEmpty() const;
	virtual AABB GetAABB() const;
	virtual void InterpolateTriangleAttributes( IntersectResult & hit, int flags ) const;

	virtual void SetCamera( const Camera & cam, const NanoCore::Image & image );

	virtual float ComputeMipMapCoef( IntersectResult & hit, int x, int y );
	virtual float ComputeMipMapCoef( IntersectResult & hit );

private:
	int  BuildTree( int l, int r );
	void Intersect_r( int node, Ray & ray, IntersectResult & hit ) const;
	void ComputeBarycentricCoordinates( const float3 & v, const Triangle & tri, float3 & bc ) const;

	std::vector<Triangle> m_Triangles;
	std::vector<Node> m_Tree;
	int m_maxTrianglesPerNode;

	const Camera * m_pCamera;
	const NanoCore::Image * m_pImage;
	float m_fPixelDistCoef;
};

IScene * CreateKDTree( int maxTrianglesPerNode ) {
	return new KDTree( maxTrianglesPerNode );
}

KDTree::KDTree( int maxTrianglesPerNode ) : m_maxTrianglesPerNode(maxTrianglesPerNode) {
}

KDTree::~KDTree() {
}
void KDTree::Build( const ISceneLoader * pLoader, IStatusCallback * pCallback ) {
	m_Tree.clear();

	wstring wFile = pLoader->GetFilename();
	wFile += L".kdtree";

	NanoCore::IFile::Ptr fp = NanoCore::FS::Open( wFile.c_str(), NanoCore::FS::efRead );
	if( fp ) {
		if( pCallback ) pCallback->SetStatus( "Loading cached KD-tree" );

		int numTris, numNodes, mtpn;
		fp->Read( &numTris, sizeof(numTris) );
		fp->Read( &numNodes, sizeof(numNodes) );
		fp->Read( &mtpn, sizeof(mtpn) );

		if( m_maxTrianglesPerNode == mtpn ) {
			m_Triangles.resize( numTris );
			m_Tree.resize( numNodes );
			for( int i=0; i<numTris; ++i )
				fp->Read( &m_Triangles[i], sizeof(Triangle) );
			for( int i=0; i<numNodes; ++i )
				fp->Read( &m_Tree[i], sizeof(Node) );

			if( pCallback ) pCallback->SetStatus( NULL );
			return;
		}
	}

	if( pCallback ) pCallback->SetStatus( "Processing geometry for KD-tree" );

	const int numTris = pLoader->GetNumTriangles();

	AABB bx;
	bx.reset();

	m_Triangles.resize( numTris );
	for( int i=0; i<numTris; ++i ) {
		Triangle & t = m_Triangles[i];
		const ISceneLoader::Triangle * p = pLoader->GetTriangle( i );
		t.mtl = p->material;

		for( int j=0; j<3; ++j ) {
			t.pos[j] = *pLoader->GetVertexPos( p->pos[j] );
			bx += t.pos[j];
		}
		t.n = cross( t.pos[1] - t.pos[0], t.pos[2] - t.pos[0] );
		t.n = normalize( t.n );
		t.d = -dot( t.n, t.pos[0] );

		for( int j=0; j<3; ++j ) {
			const float2 * pUV = pLoader->GetVertexUV( p->uv[j] );
			if( pUV ) t.uv[j] = *pUV;
			if( p->normal[j] >= 0 ) {
				const float3 * pNormal = pLoader->GetVertexNormal( p->normal[j] );
				t.normal[j] = *pNormal;
			} else {
				t.normal[j] = t.n;
			}
		}

#ifdef BARYCENTRIC_DATA_TRIANGLES
		t.v0 = t.pos[1] - t.pos[0];
		t.v1 = t.pos[2] - t.pos[0];
		t.dot00 = dot( t.v0, t.v0 );
		t.dot01 = dot( t.v0, t.v1 );
		t.dot11 = dot( t.v1, t.v1 );
		t.invDenom = 1.f / (t.dot00 * t.dot11 - t.dot01 * t.dot01);
#endif
	}

	if( pCallback ) pCallback->SetStatus( "Building KD-tree" );

	BuildTree( 0, numTris );

	if( NanoCore::WindowMain::MsgBox( L"Warning", L"Should we cache the KD-tree for faster loading?", true )) {
		fp = NanoCore::FS::Open( wFile.c_str(), NanoCore::FS::efWriteTrunc );
		if( fp ) {
			if( pCallback )
				pCallback->SetStatus( "Caching KD-tree" );
			int numTris = (int)m_Triangles.size();
			int numNodes = (int)m_Tree.size();
			fp->Write( &numTris, sizeof(numTris) );
			fp->Write( &numNodes, sizeof(numNodes) );
			fp->Write( &m_maxTrianglesPerNode, sizeof(m_maxTrianglesPerNode) );
			for( int i=0; i<numTris; ++i )
				fp->Write( &m_Triangles[i], sizeof(Triangle) );
			for( int i=0; i<numNodes; ++i )
				fp->Write( &m_Tree[i], sizeof(Node) );
		}
	}
	if( pCallback ) pCallback->SetStatus( NULL );
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

void KDTree::Intersect_r( int node_index, Ray & ray, IntersectResult & result ) const {
	const Node & node = m_Tree[node_index];

	float3 origin = ray.origin;
	float3 dir = ray.dir;
	float hitlen = ray.hitlen;

#if 1
// if the ray origin is OUTSIDE the AABB, find its nearest corner, compute its three plane intersections and choose the furthest - see if it is inside the AABB
	if( origin.x < node.min.x || origin.y < node.min.y || origin.z < node.min.z ||
		origin.x > node.max.x || origin.y > node.max.y || origin.z > node.max.z )
	{
		float3 bs( dir.x > 0 ? node.min.x : node.max.x, dir.y > 0 ? node.min.y : node.max.y, dir.z > 0 ? node.min.z : node.max.z );
		float3 len = bs - origin;

		int axis = -1;
		float max_k = 0.0f;

		for( int i=0; i<3; ++i ) {
			if( dir[i] > -EPSILON && dir[i] < EPSILON ) continue;

			float k = len[i] / dir[i];
			if( k > max_k ) {
				max_k = k;
				axis = i;
			}
		}
		// already have a hit, that is closer compared to the box intersection
		if( axis == -1 || hitlen < max_k ) return;

		float3 i = origin + dir * max_k;
		switch( axis ) {
			case 0: if( i.y < node.min.y || i.y > node.max.y || i.z < node.min.z || i.z > node.max.z ) return; break;
			case 1: if( i.z < node.min.z || i.z > node.max.z || i.x < node.min.x || i.x > node.max.x ) return; break;
			case 2: if( i.x < node.min.x || i.x > node.max.x || i.y < node.min.y || i.y > node.max.y ) return; break;
		}
	}
#endif

	const int count = node.numTriangles;
	if( count ) {

		const Triangle * best_triangle = NULL;
		float3 best_bary, best_hit;

		const Triangle * ptr = &m_Triangles[0] + node.startTriangle;
		for( int i=0; i<count; ++i ) {
			const Triangle & t = ptr[i];
			//(px + t.vx)*A + (py + t.vy)*B + (pz + t.vz)*C = -D
			//t = (-D - dot(p,N)) / dot(v,N)

			float NdotPos = dot( t.n, origin );
			float NdotDir = dot( t.n, dir );

			if( NdotDir > -0.0001f && NdotDir < 0.0001f ) continue;

			//if( NdotDir > -0.0001f ) continue;

			float k = (-t.d - NdotPos) / NdotDir;

			if( k > hitlen || k < 0 ) continue;

			float3 hit = origin + dir * k;

			float3 bary;
			ComputeBarycentricCoordinates( hit, t, bary );

			// Check if point is in triangle
			if( bary.x < -EPSILON || bary.y < -EPSILON || bary.w < -EPSILON ) continue;

			if( k < ray.hitlen ) {
				hitlen = k;
				best_triangle = &t;
				best_hit = hit;
				best_bary = bary;
			}
		}
		if( best_triangle ) {
			ray.hitlen = hitlen;
			result.hit = best_hit;
			result.barycentric = best_bary;
			result.triangle = best_triangle;
			result.materialId = best_triangle->mtl;
			result.n = best_triangle->n;
		}
	}

#if 1
	int a, b;
	float axis = (node.min[node.axis] + node.max[node.axis])*0.5f;
	if( origin[node.axis] < axis )
		a = node.left, b = node.right;
	else
		a = node.right, b = node.left;

	if( a ) Intersect_r( a, ray, result );
	if( b ) Intersect_r( b, ray, result );

#else
	if( node.left )
		Intersect_r( node.left, ray, result );
	if( node.right )
		Intersect_r( node.right, ray, result );
#endif

}

bool KDTree::IntersectRay( const Ray & ray, IntersectResult & result ) const {
	if( m_Tree.empty()) return false;

	Ray r(ray);
	Intersect_r( 0, r, result );
	return result.triangle != NULL;
}

bool KDTree::IsEmpty() const {
	return m_Tree.empty();
}

AABB KDTree::GetAABB() const {
	if( IsEmpty() )
		return AABB( float3(0,0,0), float3(0,0,0) );
	return AABB( m_Tree[0].min, m_Tree[0].max );
}

static void ComputeTangentBasis( const Triangle & tri, float3 & tangent, float3 & bitangent ) {

	const float3 * pos = tri.pos;
	const float2 * uv = tri.uv;

//  pure geometric method - find a point with the same U-coordinate on the edge21 as the U0, this is tangent

	float2 duv21 = uv[2] - uv[1];
	float2 duv20 = uv[2] - uv[0];
	float2 duv10 = uv[1] - uv[0];

	float3 edge20 = pos[2] - pos[0];
	float3 edge21 = pos[2] - pos[1];

	float3 t,b;

	if( duv21.x > -0.001f && duv21.x < 0.001f ) {
		t = pos[0] + edge20 * duv10.x / duv20.x - pos[1];
		if( duv10.y > 0 ) t = -t;
		b = pos[1] + edge21 * -duv10.y / duv21.y - pos[0];
		if( duv10.x > 0 ) b = -b;
	} else {
		t = pos[1] + edge21 * -duv10.x / duv21.x - pos[0];
		if( duv10.y < 0 ) t = -t;
		b = pos[0] + edge20 * duv10.y / duv20.y - pos[1];
		if( duv10.x < 0 ) b = -b;
	}
	tangent = normalize( t );
	bitangent = normalize( b );
}

void KDTree::InterpolateTriangleAttributes( IntersectResult & result, int flags ) const {
	if( !result.triangle )
		return;

	flags &= ~result.GetFlags();

	const Triangle * t = (const Triangle*)result.triangle;
	if( flags & IntersectResult::eUV ) {
		result.SetUV( t->uv[0]*result.barycentric.x + t->uv[1]*result.barycentric.y + t->uv[2]*result.barycentric.z );
	}
	if( flags & (IntersectResult::eNormal|IntersectResult::eTangentSpace) ) {
		result.SetInterpolatedNormal( normalize( t->normal[0]*result.barycentric.x + t->normal[1]*result.barycentric.y + t->normal[2]*result.barycentric.z ));
	}
	if( flags & IntersectResult::eTangentSpace ) {
		float3 T, B;
		ComputeTangentBasis( *t, T, B );

		float3 n = result.GetInterpolatedNormal();

		float3 b = normalize( cross( n, T ));
		if( dot( B, b ) < 0.0f ) b = -b;

		float3 t = normalize( cross( b, n ));
		if( dot( T, t ) < 0.0f ) t = -t;

		result.SetTangentSpace( t, b );
	}
	if( flags & IntersectResult::eMipMapCoef ) {

	}
}

void KDTree::ComputeBarycentricCoordinates( const float3 & pt, const Triangle & t, float3 & bc ) const {

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
	float3 v2 = pt - t.pos[0];
	float dot02 = dot( v0, v2 );
	float dot12 = dot( v1, v2 );

	float v = (dot11 * dot02 - dot01 * dot12) * invDenom;
	float w = (dot00 * dot12 - dot01 * dot02) * invDenom;
	bc.x = 1.0f - v - w;
	bc.y = v;
	bc.z = w;
}

void KDTree::SetCamera( const Camera & cam, const NanoCore::Image & image ) {
	m_pCamera = &cam;
	m_pImage = &image;
	m_fPixelDistCoef = ncTan( cam.fovy*0.5f ) * 2.0f / image.GetHeight();
}

float KDTree::ComputeMipMapCoef( IntersectResult & ir, const Camera & cam ) {
	float dist = len( ir.hit - cam.pos );
	dist *= m_fPixelDistCoef;

	const Triangle * tri = (const Triangle*)ir.triangle;

	float dUV1 = len(tri->uv[1] - tri->uv[0]) * dist / len(tri->pos[1] - tri->pos[0]);
	float dUV2 = len(tri->uv[2] - tri->uv[0]) * dist / len(tri->pos[2] - tri->pos[0]);
	float dUV = Max( dUV1, dUV2 );
	dUV = Clamp( dUV, 0.0001f, 1.0f );

	return ncLn( 1.0f / dUV ) * LN_2;
}