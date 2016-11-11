#ifndef ___INC_RAYTRACE_RAYTRACER
#define ___INC_RAYTRACE_RAYTRACER

#include <NanoCore/Image.h>
#include "Common.h"
#include "Camera.h"
#include "KDTree.h"




class Raytracer
{
public:
	int m_ScreenTileSizePow2;

	enum EShading {
		eShading_ColoredCube,
		eShading_ColoredCubeShadowed,
		eShading_TriangleID,
		eShading_Checker,
		eShading_Diffuse,
		eShading_Previews,
		eShading_Photo,
	};

	Raytracer();
	~Raytracer();

	void LoadMaterials( IObjectFileLoader & loader );
	void Render( Camera & camera, NanoCore::Image & image, KDTree & kdTree );
	bool IsRendering();
	void Stop();

	void RaytracePixel( int x, int y, int * pixel );

	void GetStatus( std::wstring & status );

	volatile int m_PixelCompleteCount;
	int          m_TotalPixelCount;

	EShading m_Shading;

	int m_GIBounces;
	int m_SunSamples;
	float m_SunDiskAngle;
	int m_NumThreads;

	int m_SelectedTriangle, m_DebugX, m_DebugY;

	NanoCore::Image * m_pImage;
	Camera * m_pCamera;
	KDTree * m_pKDTree;

	struct Material {
		std::string name;
		NanoCore::Image mapDiffuse, mapSpecular, mapBump, mapAlpha;
		float Tr;
		float3 Kd, Ks, Ke;
	};
	std::vector<Material> m_Materials;

private:
	void LoadImage( std::wstring path, std::string file, NanoCore::Image & img );
	void RaytracePreviewPixel( int x, int y, int * pixel );

	int m_ImageCountLoaded;
	int m_ImageSizeLoaded;
};

#endif