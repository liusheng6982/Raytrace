#include "RayTrace.h"

#define EPSILON 0.000001

KDTreeNode * BuildKDTree( KDTreeInfo * pInfo, int l, int r )
{
	if( l >= r ) return 0;

	pInfo->numNodes++;

	Vec3 min = pInfo->pTris[l].pos[0], max = pInfo->pTris[l].pos[0];
	for( int i=l; i<r; ++i ) {
		const Triangle & t = pInfo->pTris[i];
		for( int j=0; j<3; ++j ) {
			const Vec3 & v = t.pos[j];
			if( v.x < min.x ) min.x = v.x; else if( v.x > max.x ) max.x = v.x;
			if( v.y < min.y ) min.y = v.y; else if( v.y > max.y ) max.y = v.y;
			if( v.z < min.z ) min.z = v.z; else if( v.z > max.z ) max.z = v.z;
		}
	}

	Vec3 size = max-min;
	int axis = 0;
	if( size.y > size.x ) axis = 1;
	if( size.z > size.x && size.z > size.y ) axis = 2;

	const float axislen = max[axis] - min[axis];
	const float separator = min[axis] + axislen * 0.5f;

	int l1 = l, l2 = r-1;

	int nL=0, nR=0, nC=0;

	if( r-l <= pInfo->maxTrianglesPerNode ) {
		l1 = r;
	} else {
		for( int i=l; i<=l2; ++i ) {
			Triangle & t = pInfo->pTris[i];
			float tmin = t.pos[0][axis], tmax = t.pos[0][axis];

			float p2 = t.pos[1][axis], p3 = t.pos[2][axis];
			if( p2 < tmin ) tmin = p2; else if( p2 > tmax ) tmax = p2;
			if( p3 < tmin ) tmin = p3; else if( p3 > tmax ) tmax = p3;

			if( tmax < separator + axislen*0.3f ) {  // left
				nL++;
			} else if( tmin > separator - axislen*0.3f ) {  // right
				Triangle temp = t;
				t = pInfo->pTris[l2];
				pInfo->pTris[l2] = temp;
				l2--;
				i--;
				nR++;
			} else {  // stay in node
				if( l1 < i ) {
					Triangle temp = t;
					t = pInfo->pTris[l1];
					pInfo->pTris[l1] = temp;
				}
				l1++;
				nC++;
			}
		}
	}

	KDTreeNode * node = new KDTreeNode();
	node->min = min;
	node->max = max;
	node->axis = axis;
	node->numTris = l1-l;
	node->pTris = pInfo->pTris + l;

	node->left = BuildKDTree( pInfo, l1, l2+1 );
	node->right = BuildKDTree( pInfo, l2+1, r );

	return node;
}

int64 rays_traced = 0;

bool KDTreeNode::IntersectRay( RayInfo & ray )
{
	// intersect ray with node's box

	//rays_traced++;

	if( ray.pos.x < min.x || ray.pos.y < min.y || ray.pos.z < min.z ||
		ray.pos.x > max.x || ray.pos.y > max.y || ray.pos.z > max.z )
	{
		Vec3 dir = ray.dir;
		Vec3 bs( dir.x > 0 ? min.x : max.x, dir.y > 0 ? min.y : max.y, dir.z > 0 ? min.z : max.z );
		Vec3 len = bs - ray.pos;

		for( int i=0; i<3; ++i )
			len[i] = fabsf( dir[i] ) > 0.0001f ? len[i] / dir[i] : -10000.0f;

		int axis = 0;
		if( len.y > len.x ) axis = 1;
		if( len.z > len[axis] ) axis = 2;

		if( len[axis] <= 0 ) return false;

		Vec3 i = ray.pos + dir * len[axis];
		switch( axis ) {
			case 0: if( i.y < min.y || i.y > max.y || i.z < min.z || i.z > max.z ) return false; break;
			case 1: if( i.z < min.z || i.z > max.z || i.x < min.x || i.x > max.x ) return false; break;
			case 2: if( i.x < min.x || i.x > max.x || i.y < min.y || i.y > max.y ) return false; break;
		}

		// already have a hit, that is closer compared to the box intersection
		if( ray.tri && ray.hitlen < len[axis] ) return false;
	}

	const int count = numTris;
	const Triangle * ptr = pTris;
	for( int i=0; i<count; ++i ) {
		const Triangle & t = ptr[i];

#ifdef COMPACT_TRIANGLES
		Vec3 e1, e2, P, Q, T;
		float det, inv_det, u, v;
		float k;
 
		//Find vectors for two edges sharing V0
		e1 = t.pos[1] - t.pos[0];
		e2 = t.pos[2] - t.pos[0];
		//Begin calculating determinant - also used to calculate u parameter
		P = cross( ray.dir, e2 );
		//if determinant is near zero, ray lies in plane of triangle
		det = dot( e1, P );
		//NOT CULLING
		if( det > -EPSILON && det < EPSILON ) continue;
		inv_det = 1.f / det;
 
		//calculate distance from V0 to ray origin
		T = ray.pos - t.pos[0];
 
		//Calculate u parameter and test bound
		u = dot( T, P ) * inv_det;
		//The intersection lies outside of the triangle
		if( u < 0.f || u > 1.f ) continue;
 
		//Prepare to test v parameter
		Q = cross( T, e1 );
 
		//Calculate V parameter and test bound
		v = dot( ray.dir, Q ) * inv_det;
		//The intersection lies outside of the triangle
		if( v < 0.f || u + v  > 1.f ) continue;
 
		k = dot( e2, Q ) * inv_det;
 
		if( k > EPSILON && k < ray.hitlen ) { //ray intersection
			ray.hitlen = k;
			ray.n = cross( e1, e2 );
			ray.n.Normalize();
			ray.tri = &t;
		}

#else
		//(px + t.vx)*A + (py + t.vy)*B + (pz + t.vz)*C = -D
		//t = (-D - dot(p,N)) / dot(v,N)

		float NdotPos = dot( t.n, ray.pos );
		float NdotDir = dot( t.n, ray.dir );

		if( NdotDir > -0.0001f ) continue;

		float k = (-t.d - NdotPos) / NdotDir;

		if( k > ray.hitlen || k < 0 ) continue;

		Vec3 hit = ray.pos + ray.dir * k;

#ifdef BARYCENTRIC_DATA_TRIANGLES
		Vec3 v0 = t.v0;
		Vec3 v1 = t.v1;
		float dot00 = t.dot00;
		float dot01 = t.dot01;
		float dot11 = t.dot11;
		float invDenom = t.invDenom;
#else
		Vec3 v0 = t.pos[1] - t.pos[0];
		Vec3 v1 = t.pos[2] - t.pos[0];
		float dot00 = dot( v0, v0 );
		float dot01 = dot( v0, v1 );
		float dot11 = dot( v1, v1 );
		float invDenom = 1.f / (dot00 * dot11 - dot01 * dot01);
#endif
		Vec3 v2 = hit - t.pos[0];
		float dot02 = dot( v0, v2 );
		float dot12 = dot( v1, v2 );

		// Compute barycentric coordinates

		float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
		float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

		// Check if point is in triangle

		if( u < -EPSILON || v < -EPSILON || u+v > 1+EPSILON ) continue;

		if( k < ray.hitlen ) {
			ray.hitlen = k;
			ray.n = t.n;
			ray.tri = &t;
		}
#endif
	}
	if( left ) left->IntersectRay( ray );
	if( right ) right->IntersectRay( ray );

	return ray.tri != 0;
}