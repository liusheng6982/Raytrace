#ifndef ___INC_RAYTRACE_KDTREE
#define ___INC_RAYTRACE_KDTREE

#include <vector>
#include "ObjectFileLoader.h"
#include "Camera.h"



#define BARYCENTRIC_DATA_TRIANGLES



struct Triangle
{
	float3  pos[3], n;
	float   d;

#ifdef BARYCENTRIC_DATA_TRIANGLES
	float3 v0, v1;
	float dot00, dot01, dot11, invDenom;
#endif

	float2 uv[3];
	int    mtl;
	int    triangleID;

	float2 GetUV( float2 barycentric_pos ) const {
		float2 uv10 = uv[1] - uv[0];
		float2 uv20 = uv[2] - uv[0];
		return uv10 * barycentric_pos.x + uv20 * barycentric_pos.y + uv[0];
	}
};

struct RayInfo
{
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

class KDTree
{
public:
	struct Node {
		float3 min, max;
		int axis;
		int startTriangle, numTriangles;
		int left, right;
	};

	KDTree();
	~KDTree();

	void Build( IObjectFileLoader * pModel, int maxTrianglesPerNode );
	void Intersect( RayInfo & ray );
	bool Empty();
	aabb GetBBox();

private:
	int BuildTree( int l, int r );
	void Intersect_r( int node, RayInfo & ray );

	std::vector<Triangle> m_Triangles;
	std::vector<Node> m_Tree;
	int m_maxTrianglesPerNode;
};

#endif