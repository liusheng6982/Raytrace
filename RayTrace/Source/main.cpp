//#include <windows.h>
//#include <stdlib.h>
//#include <string.h>
//#include <tchar.h>
//#include <stdio.h>
//#include "RayTrace.h"

#include <NanoCore/Threads.h>
#include <NanoCore/MainWindow.h>
#include "ObjectFileLoader.h"
#include "KDTree.h"
#include <memory>

#define SPONZA

#define OBJ_FILE "..\\data\\sponza\\sponza.obj"
//#define OBJ_FILE "..\\data\\teapot\\teapot.obj"
//#define OBJ_FILE "..\\data\\san-miguel.obj"

/*

static RenderTask s_rt, s_rtPreview;
static DWORD s_t0 = 0, s_t1 = 0;
enum {
	STATE_LOAD,
	STATE_LOADING,
	STATE_RENDERING,
	STATE_FINISH_FRAME
};

static int  s_State = 0;
static char s_pcInfo[512] = "";

static void LoadScene( int param )
{
	Triangle * pTris;
	int numTris = LoadObjFile( OBJ_FILE, &pTris );

	KDTreeInfo kdti;
	kdti.pTris = pTris;
	kdti.maxTrianglesPerNode = 14;
	s_rt.pKDTree = BuildKDTree( &kdti, 0, numTris );
	s_rt.numKDTreeNodes = kdti.numNodes;
	s_rt.numTriangles = numTris;
}
*/

/*
extern int64 rays_traced;

LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch (message)
    {
		case WM_TIMER:
			switch( s_State ) {
				case STATE_LOAD:
					RunTask( &LoadScene, 0 );
					s_State = STATE_LOADING;
					break;
				case STATE_LOADING:
					if( ! IsTaskComplete()) {
						sprintf( s_pcInfo, "Loading... %d%%", GetLoadingProgress());
						InvalidateRect( hWnd, 0, TRUE );
						UpdateWindow( hWnd );
					} else {
						s_State = STATE_RENDERING;

						Vec3 vSize = s_rt.pKDTree->max - s_rt.pKDTree->min;
						float size = max( vSize.x, max( vSize.y, vSize.z ));

						s_rtPreview.camera.pos = (s_rt.pKDTree->min + s_rt.pKDTree->max) * 0.5f;
#ifdef SPONZA
						s_rtPreview.camera.at = normalize( Vec3( 1, 0, 0 ));
						s_rtPreview.camera.pos = s_rt.camera.pos - s_rt.camera.at * size * 0.5f;
						s_rtPreview.camera.pos = s_rt.camera.pos + Vec3(1,-1,1);
#else
						s_rtPreview.camera.at = normalize( Vec3( 1, 0, 1 ));
						s_rtPreview.camera.pos = s_rt.camera.pos - s_rt.camera.at * size * 0.1f;
						s_rtPreview.camera.pos = s_rt.camera.pos + Vec3(0,-1,0);
#endif
						s_rtPreview.camera.up = Vec3( 0, -1, 0 );
						s_rtPreview.camera.Orthogonalize();
						s_rtPreview.camera.fovy = 70;

						// 2:09
						s_rt.render_width = 320;
						s_rt.render_height = 200;
						s_rt.numSamples = 100;
						s_rt.maxMip = 2;

						s_rtPreview.render_width = 160;
						s_rtPreview.render_height = 100;
						//s_rtPreview.camera

						s_rt.numSamples = 400;
						s_rt.maxMip = 0;

						s_rt.camera.width = s_rt.render_width;
						s_rt.camera.height = s_rt.render_height;
						s_rt.maxBounces = 2;
						InitRenderTask( 1, s_rt );
						RunRenderTask( s_rt );
						s_t0 = GetTime();
					}
					break;

				case STATE_RENDERING:
					if( !IsRenderTaskComplete()) {
						s_t1 = GetTime();
						int sec = int((s_t1-s_t0) * 0.001f);
						sprintf( s_pcInfo, "%dx%d, %d:%02d m, threads: %d, kd-nodes: %d (x%d), tris: %d (x%d), samples: %d, progress: %d%%, %d rps",
							s_rt.render_width, s_rt.render_height, sec/60, sec%60, s_rt.numThreads, s_rt.numKDTreeNodes, sizeof(KDTreeNode), s_rt.numTriangles, sizeof(Triangle),
							s_rt.numSamples, s_rt.pixelsComplete*100 / (s_rt.render_width*s_rt.render_height), int(rays_traced / 1000000 / (sec ? sec : 1)) );
						GdiFlush();
						InvalidateRect( hWnd, 0, TRUE );
						UpdateWindow( hWnd );
					} else {
						s_State = STATE_FINISH_FRAME;
					}
					break;
				case STATE_FINISH_FRAME:
					if( s_rt.mip < s_rt.maxMip ) {
						s_rt.mip++;
						s_rt.FinishCurrentMip();
						RunRenderTask( s_rt );
						s_State = STATE_RENDERING;
					} else {
						s_rt.GatherRenderBuffer();
						WriteImage( s_rt.pRenderBuffer, s_rt.render_width, s_rt.render_height, "Final" );
					}
					break;
			}
			return 0;
}
*/

class LoadingThread : public ncThread
{
public:
	LoadingThread( std::wstring wFile, KDTree * pTree ) {
		m_wFile = wFile;
		m_pTree = pTree;
		m_pLoader = NULL;
	}
	void GetStatus( std::wstring & status ) {
		if( m_pLoader ) {
			wchar_t buf[64];
			swprintf( buf, L"Loading %d %%", m_pLoader->GetLoadingProgress());
			status += buf;
		} else
			status += L"Building KD-tree";
	}
	virtual void Run( void* )
	{
		m_pLoader = IObjectFileLoader::Create();
		m_pLoader->Load( m_wFile.c_str() );
		IObjectFileLoader * p = m_pLoader;
		m_pLoader = NULL;
		m_pTree->Build( p, 40 );
		delete p;
	}

private:
	std::wstring m_wFile;
	KDTree * m_pTree;
	IObjectFileLoader * m_pLoader;
};

struct TestThread : public ncThread
{
	Image * pImage;
	virtual void Run(void*)
	{
		pImage = new Image();
		pImage->w = pImage->h = 128;
		pImage->pBuffer = new uint8[128*128*3];
		memset( pImage->pBuffer, 0, 128*128*3 );

		int size2 = 7, size = 1 << size2;

		for( int i=size2; i>=0; --i ) {
			int step = 1<<i;
			for( int y=0; y<size; y += step )
				for( int x=0; x<size; x += step ) {
					if( (x & step) || (y & step) ) {  // only compute pixels, that don't have BOTH coordinates are bigger pow2 - those are already computed
						uint8 * p = pImage->pBuffer + (x + y*128)*3;
						p[0] = p[1] = p[2] = 255;
					} else {
						uint8 * p = pImage->pBuffer + (x + y*128)*3;
						p[0] = 255; p[1] = p[2] = 0;
					}
					ncSleep( 5 );
				}
		}
	}
};


class MainWnd : public ncMainWindow
{
public:
	TestThread * m_pTest;

	MainWnd()
	{
		m_pModel = std::auto_ptr<IObjectFileLoader>( IObjectFileLoader::Create() );
		m_csStatus = ncCriticalSectionCreate();
		m_pLoadingThread = NULL;
		m_pTest = NULL;
	}
	~MainWnd()
	{
		//delete m_pModel;
		ncCriticalSectionDelete( m_csStatus );
	}

	virtual void OnKey( int key )
	{
		LoadModel();
	}
	virtual void OnMouse( int x, int y, int btn_down, int btn_up, int btn_dblclick, int wheel )
	{
		if( btn_down && !m_pTest ) {
			m_pTest = new TestThread(); 
			m_pTest->Start(NULL);
		}
	}
	virtual void OnSize( int w, int h ) {}
	virtual void OnDraw()
	{
		if( m_pTest ) {
			DrawImage( 0, 0, 128, 128, m_pTest->pImage->pBuffer, 128, 128, 24 );
		}
	}
	virtual void OnUpdate()
	{
		if( m_pTest ) {
			Redraw();
		}
		if( m_pLoadingThread ) {
			if( m_pLoadingThread->IsRunning()) {
				std::wstring str;
				m_pLoadingThread->GetStatus( str );
				SetStatus( str.c_str() );
			} else {
				delete m_pLoadingThread;
				m_pLoadingThread = NULL;
				SetStatus( NULL );
			}
		}
	}
	void LoadModel() {
		std::wstring wFolder = ncGetCurrentFolder();
		std::wstring wFile = ChooseFile( wFolder.c_str(), L"Object files(*.obj)\0*.obj\0", L"Load model", true );
		if( !wFile.empty()) {
			m_pLoadingThread = new LoadingThread( wFile, &m_kdTree );
			m_pLoadingThread->Start( NULL );
		}
	}
	void SetStatus( const wchar_t * pcStatus ) {
		std::wstring s = L"Raytracer | by Sergey Miloykov";
		if( pcStatus && *pcStatus ) {
			s += L" | ";
			s += pcStatus;
		}
		SetCaption( s.c_str());
	}

	std::auto_ptr<IObjectFileLoader> m_pModel;
	ncCriticalSection * m_csStatus;
	std::string m_strStatus;
	LoadingThread * m_pLoadingThread;
	KDTree          m_kdTree;
};

int Main()
{
	MainWnd * pWnd = new MainWnd();
	pWnd->Init( 800, 600 );
	pWnd->SetStatus( NULL );

	while( pWnd->Update()) {
		ncSleep( 10 );
	}
	delete pWnd;
	return 0;
}