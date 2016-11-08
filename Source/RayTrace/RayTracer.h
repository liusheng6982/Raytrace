#ifndef ___INC_RAYTRACE_RAYTRACER
#define ___INC_RAYTRACE_RAYTRACER

#include "Common.h"
#include "Camera.h"
#include "KDTree.h"
#include "Image.h"



class Raytracer
{
public:
	int m_ScreenTileSizePow2;

	enum EShading {
		ePreviewShading_ColoredCube,
		ePreviewShading_ColoredCubeShadowed,
		ePreviewShading_TriangleID,
		ePreviewShading_Checker,
	};

	Raytracer();
	~Raytracer();

	void LoadMaterials( IObjectFileLoader & loader );
	void Render( Camera & camera, Image & image, KDTree & kdTree );
	bool IsRendering();
	void Stop();

	void RaytracePixel( int x, int y, int * pixel );

	void GetStatus( std::wstring & status );

	mutable int m_PixelCompleteCount;
	int m_TotalPixelCount;
	EShading m_Shading;

	int m_GIBounces;
	int m_SunSamples;
	float m_SunDiskAngle;
	int m_NumThreads;

	int m_SelectedTriangle;

	Image  * m_pImage;
	Camera * m_pCamera;
	KDTree * m_pKDTree;

	struct Material {
		Image Md, Ms, Mb;
		float Tr;
		float3 Kd, Ks, Ke;
	};
	std::vector<Material> m_Materials;
};

#endif