#ifndef ___INC_RAYTRACE_RAYTRACER
#define ___INC_RAYTRACE_RAYTRACER

#include <NanoCore/Image.h>
#include "Common.h"
#include "Camera.h"

#include <vector>
#include <map>
#include <string>




class Raytracer : public IRaytracer
{
public:
	int m_ScreenTileSizePow2;

	Raytracer();
	virtual ~Raytracer();

	virtual bool   TraceRay( Ray & V, IntersectResult & result );
	virtual float3 RenderRay( Ray & V, IShader * pShader, void * context );
	virtual const IScene * GetScene() const { return m_pScene; }

	void LoadMaterials( ISceneLoader * pLoader, IStatusCallback * pCallback );
	void Render( Camera & camera, NanoCore::Image & image, IScene * pScene, const Environment & env, IShader * pShader, IStatusCallback * pCallback );
	bool IsRendering();
	void Stop();

	void RaytracePixel( int x, int y, int * pixel );

	int GetProgress() const { return m_PixelCompleteCount * 100 / m_TotalPixelCount; }

	volatile int m_PixelCompleteCount;
	int          m_TotalPixelCount;

	int m_NumThreads;

	int m_SelectedTriangle, m_DebugX, m_DebugY;

	NanoCore::Image * m_pImage;
	Camera * m_pCamera;
	IScene * m_pScene;

	std::vector<Material> m_Materials;
	std::map<std::wstring,Texture::Ptr> m_TextureMaps;

private:
	Texture::Ptr LoadTexture( std::wstring path, std::string file );

	int m_ImageCountLoaded;
	int m_ImageSizeLoaded;

	const Environment * m_pEnv;
	IShader * m_pShader;
};

#endif