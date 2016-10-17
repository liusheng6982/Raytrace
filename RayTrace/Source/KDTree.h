#ifndef ___INC_RAYTRACE_KDTREE
#define ___INC_RAYTRACE_KDTREE

#include <vector>
#include "ObjectFileLoader.h"
#include <NanoCore/Mathematic.h>

using namespace std;

#define BARYCENTRIC_DATA_TRIANGLES

struct Triangle
{
	ncFloat3 pos[3];
	ncFloat3 n;
	float    d;

#ifdef BARYCENTRIC_DATA_TRIANGLES
	ncFloat3 v0, v1;
	float dot00, dot01, dot11, invDenom;
#endif

	ncFloat2  uv[3];
	int       mtl;
};

struct RayInfo
{
	ncVec3 pos, dir;
	float hitlen;
	ncVec3  n;
	const Triangle * tri;

	RayInfo() : hitlen(1000000.0f), tri(0) {}

	ncVec3 GetHit() const {
		if( tri )
			return pos + dir * hitlen + n * 0.001f;
		return pos + dir * hitlen;
	}
};

class KDTree
{
public:
	struct KDTreeNode
	{
		ncVec3 min, max;
		int numTris, axis;
		const Triangle * pTris;
		KDTreeNode *left, *right;

		bool IntersectRay( RayInfo & ray );
	};

	KDTree();
	~KDTree();

	void Build( IObjectFileLoader * pModel, int maxTrianglesPerNode );

private:
	KDTreeNode * BuildTree( int l, int r );

	KDTreeNode * m_pRoot;
	vector<Triangle> m_Triangles;
	int m_numNodes;
	int m_maxTrianglesPerNode;
};

#endif