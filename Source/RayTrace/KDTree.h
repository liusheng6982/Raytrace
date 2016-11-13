#ifndef ___INC_RAYTRACE_KDTREE
#define ___INC_RAYTRACE_KDTREE

#include <vector>
#include "ObjectFileLoader.h"
#include "Camera.h"



#define BARYCENTRIC_DATA_TRIANGLES
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

struct RayInfo {
	float3 pos, dir;
	float hitlen;
	float3  n;
	const Triangle * tri;
	float2 barycentric;
	float3 hit;

	RayInfo() : hitlen(1000000.0f), tri(0) {}

	void Init( int x, int y, const Camera & cam, int width, int height );

	void Clear() {
		tri = NULL;
	}

	float3 GetHit() const {
		if( tri )
			return pos + dir * hitlen + n * 0.001f;
		return pos + dir * hitlen;
	}
};

class KDTree {
public:
	struct Node {
		float3 min, max;
		int axis;
		int startTriangle, numTriangles;
		int left, right;
	};

	KDTree();
	~KDTree();

	void Build( IObjectFileLoader * pModel, int maxTrianglesPerNode, IStatusCallback * pCallback );
	void Intersect( RayInfo & ray );
	bool Empty();
	aabb GetBBox();

private:
	int  BuildTree( int l, int r );
	void Intersect_r( int node, RayInfo & ray );

	std::vector<Triangle> m_Triangles;
	std::vector<Node> m_Tree;
	int m_maxTrianglesPerNode;
};

#endif