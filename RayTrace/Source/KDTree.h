#ifndef ___INC_RAYTRACE_KDTREE
#define ___INC_RAYTRACE_KDTREE

#include <vector>
#include "ObjectFileLoader.h"



#define BARYCENTRIC_DATA_TRIANGLES



struct Triangle
{
	float3  pos[3], n;
	float   d;

#ifdef BARYCENTRIC_DATA_TRIANGLES
	float3 v0, v1;
	float dot00, dot01, dot11, invDenom;
#endif

	float2  uv[3];
	int       mtl;
};

struct RayInfo
{
	float3 pos, dir;
	float hitlen;
	float3  n;
	const Triangle * tri;

	RayInfo() : hitlen(1000000.0f), tri(0) {}

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
		int numTris, axis;
		const Triangle * pTris;
		Node *left, *right;

		bool IntersectRay( RayInfo & ray );
	};

	KDTree();
	~KDTree();

	void Build( IObjectFileLoader * pModel, int maxTrianglesPerNode );
	void Intersect( RayInfo & ray );

private:
	Node * BuildTree( int l, int r );

	Node * m_pRoot;
	std::vector<Triangle> m_Triangles;
	int m_numNodes;
	int m_maxTrianglesPerNode;
};

#endif