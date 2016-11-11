#ifndef ___INC_RAYTRACE_RAYTRACER
#define ___INC_RAYTRACE_RAYTRACER

#include <NanoCore/Image.h>
#include "Common.h"
#include "Camera.h"
#include "KDTree.h"

#include <vector>
#include <map>
#include <string>




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

	struct Context {
		EShading Shading;
		int	  GIBounces, GISamples;
		int   SunSamples;
		float SunDiskAngle;

		Context() : Shading(eShading_ColoredCube), GIBounces(1), GISamples(10), SunSamples(10), SunDiskAngle( 0.52f ) {}
	};

	void LoadMaterials( IObjectFileLoader & loader );
	void Render( Camera & camera, NanoCore::Image & image, KDTree & kdTree, const Context & context );
	bool IsRendering();
	void Stop();

	void RaytracePixel( int x, int y, int * pixel );

	void GetStatus( std::wstring & status );

	volatile int m_PixelCompleteCount;
	int          m_TotalPixelCount;

	int m_NumThreads;

	int m_SelectedTriangle, m_DebugX, m_DebugY;

	NanoCore::Image * m_pImage;
	Camera * m_pCamera;
	KDTree * m_pKDTree;

	struct Material {
		std::string name;
		NanoCore::Image::Ptr pDiffuseMap, pSpecularMap, pBumpMap, pAlphaMap;
		float Tr;
		float3 Kd, Ks, Ke;
	};
	std::vector<Material> m_Materials;
	std::map<std::wstring,NanoCore::Image::Ptr> m_TextureMaps;

private:
	NanoCore::Image::Ptr LoadImage( std::wstring path, std::string file );
	void RaytracePreviewPixel( int x, int y, int * pixel );

	int m_ImageCountLoaded;
	int m_ImageSizeLoaded;
	Context m_Context;
};

#endif