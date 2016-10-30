//#include <windows.h>
//#include <stdlib.h>
//#include <string.h>
//#include <tchar.h>
//#include <stdio.h>


#include <NanoCore/Threads.h>
#include <NanoCore/MainWindow.h>
#include "ObjectFileLoader.h"
#include "KDTree.h"
#include "RayTracer.h"
#include <memory>

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

class MainWnd : public ncMainWindow
{
public:
	MainWnd()
	{
		m_pModel = std::auto_ptr<IObjectFileLoader>( IObjectFileLoader::Create() );
		m_csStatus = ncCriticalSectionCreate();
		m_pLoadingThread = NULL;
		m_Raytracer.m_pKDTree = &m_KDTree;
		m_Raytracer.m_pImage = &m_Image;
		m_Raytracer.m_pCamera = &m_Camera;
		m_bInvalidate = false;
	}
	~MainWnd()
	{
		//delete m_pModel;
		ncCriticalSectionDelete( m_csStatus );
	}
	virtual void OnKey( int key )
	{
		switch( key ) {
			case 79:
			case 76:
				LoadModel();
				break;
			case 80:
				
			case 32:
				if( !m_Raytracer.IsRendering())
					m_Raytracer.Render();
				break;
			case 90:
				CenterCamera();
				break;
		}
	}
	virtual void OnMouse( int x, int y, int btn_down, int btn_up, int btn_dblclick, int wheel )
	{
		if( wheel ) {
			m_Camera.pos += wheel>0 ? m_Camera.at : -m_Camera.at;
			m_bInvalidate = true;
		}

		static bool bDrag = false;
		static int dragX, dragY;
		static Camera dragCam;

		if( btn_down & 1 ) {
			bDrag = true;
			dragX = x;
			dragY = y;
			dragCam = m_Camera;
		}
		if( btn_up & 1 ) {
			bDrag = false;
		}
		if( bDrag ) {
			m_Camera = dragCam;
			m_Camera.Rotate( (dragY - y) * 3.1415f / 200.0f, (x - dragX) * 3.1415f / 200.0f );
			m_bInvalidate = true;
		}
	}
	virtual void OnSize( int w, int h )
	{
		m_Raytracer.Stop();
		m_Image.Init( w/4, h/4, 24 );
		m_bInvalidate = true;
	}
	virtual void OnDraw()
	{
		if( m_Image.GetWidth())
		//if( m_Raytracer.IsRendering()) {
			DrawImage( 0, 0, GetWidth(), GetHeight(), m_Image.GetImageAt(0,0), m_Image.GetWidth(), m_Image.GetHeight(), 24 );
		//}
	}
	virtual void OnUpdate()
	{
		if( m_pLoadingThread ) {
			if( m_pLoadingThread->IsRunning()) {
				std::wstring str;
				m_pLoadingThread->GetStatus( str );
				SetStatus( str.c_str() );
			} else {
				delete m_pLoadingThread;
				m_pLoadingThread = NULL;
				SetStatus( NULL );
				CenterCamera();
			}
		} else {
			if( m_Raytracer.IsRendering()) {
				std::wstring str;
				m_Raytracer.GetStatus( str );
				SetStatus( str.c_str());
			} else
				SetStatus( NULL );

			if( m_bInvalidate && !m_Raytracer.IsRendering()) {
				m_Raytracer.Render();
				m_bInvalidate = false;
			}

			Redraw();
		}
	}
	void LoadModel() {
		std::wstring wFolder = ncGetCurrentFolder();
		std::wstring wFile = ChooseFile( wFolder.c_str(), L"Wavefront object files (*.obj)\0*.obj\0", L"Load model", true );
		if( !wFile.empty()) {
			m_pLoadingThread = new LoadingThread( wFile, &m_KDTree );
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
	void CenterCamera()
	{
		if( !m_KDTree.Empty() )
		{
			float3 min, max;
			m_KDTree.GetBBox( min, max );

			float3 center = (min + max) * 0.5f;
			float L = len( min - center )*0.7f;

			m_Camera.fovy = 60.0f * 3.1415f / 180.0f;
			m_Camera.world_up = float3(0,-1,0);
			m_Camera.LookAt( center, float3(1,0,-1)*L );
		}
	}

	std::auto_ptr<IObjectFileLoader> m_pModel;
	ncCriticalSection * m_csStatus;
	std::string m_strStatus;
	LoadingThread * m_pLoadingThread;
	KDTree          m_KDTree;
	Image           m_Image;
	Camera          m_Camera;
	Raytracer       m_Raytracer;
	bool            m_bInvalidate;
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
