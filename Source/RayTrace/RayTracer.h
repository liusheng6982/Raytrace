#ifndef ___INC_RAYTRACE_RAYTRACER
#define ___INC_RAYTRACE_RAYTRACER

#include "Common.h"
#include "Camera.h"
#include "KDTree.h"
#include "Image.h"



class Raytracer
{
public:
	Image  * m_pImage;
	Camera * m_pCamera;
	KDTree * m_pKDTree;

	int     m_ScreenTileSizePow2;
	bool    m_bPreview;

	Raytracer();
	~Raytracer();

	void Render();
	bool IsRendering();
	void Stop();

	void RaytracePixel( int x, int y, int * pixel );

	void GetStatus( std::wstring & status );

	mutable int m_PixelCompleteCount;
	int m_TotalPixelCount;
};

#endif