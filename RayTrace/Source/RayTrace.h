#include <math.h>
#include <Windows.h>
#include <vector>
#include "Common.h"
#include "Math.h"



#pragma warning( disable : 4996 )



struct RayInfo
{
	Vec3 pos, dir;
	float hitlen;
	Vec3  n;
	const Triangle * tri;

	RayInfo() : hitlen(1000000.0f), tri(0) {}

	Vec3 GetHit() const {
		if( tri )
			return pos + dir * hitlen + n * 0.001f;
		return pos + dir * hitlen;
	}
};

struct Camera
{
	Vec3 pos, at,up,right;
	float fovy;
	int width, height;

	void InitRay( int x, int y, RayInfo & ray );
	void Orthogonalize();
	void GetAxes( Vec3 & at, Vec3 & up, Vec3 & right );
};

struct KDTreeNode
{
	Vec3 min, max;
	int numTris, axis;
	const Triangle * pTris;
	KDTreeNode *left, *right;

	bool IntersectRay( RayInfo & ray );
};

struct KDTreeInfo
{
	Triangle * pTris;
	int numNodes;
	int maxTrianglesPerNode;
};



uint32 GetTime();

KDTreeNode * BuildKDTree( KDTreeInfo * info, int l, int r );

int LoadObjFile( const char * pcFile, Triangle ** pTris );
int GetLoadingProgress();
int WriteImage( const uint8 * pImage, int width, int height, const char * pcPathFileName );

void ComputeGridDestribution( int w, int h, int * order );

struct RenderTask
{
	int numThreads;
	int render_width, render_height;
	int cell_width, cell_height;
	int * cell_destribution;

	Camera camera;
	KDTreeNode * pKDTree;
	int numKDTreeNodes;
	int numSamples, maxBounces;
	int numTriangles;
	int gatherIndex;

	uint8 * pRenderBuffer;
	uint8 * pHalfResBuffer;
	uint8 ** pRenderBuffers;

	int mip, maxMip;

	int pixelsComplete;

	void TraceRay( int x, int y, int threadId );
	void TracePreviewRay( int x, int y, int threadId );
	void GatherRenderBuffer();
	void FinishCurrentMip();
	void InitFromHalfResImage( uint8 * dest, uint8 * src, int src_width, int src_height );

	void SetResolution( int w, int h );

	float ComputeError( int x, int y );
};

void InitRenderTask( int numThreads, RenderTask & task );
void RunRenderTask( RenderTask & task );
void StopRenderTask();
bool IsRenderTaskComplete();

typedef void (*TaskFunc)( int );

void RunTask( TaskFunc fn, int param ); 
bool IsTaskComplete();

void TraceRay( RayInfo & ri, KDTreeNode * pTree, Vec3 & rightPix, Vec3 & upPix, uint8 & r, uint8 & g, uint8 & b );