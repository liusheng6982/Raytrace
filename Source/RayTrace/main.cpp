//#include <windows.h>
//#include <stdlib.h>
//#include <string.h>
//#include <tchar.h>
//#include <stdio.h>


#include <NanoCore/Threads.h>
#include <NanoCore/Windows.h>
#include "ObjectFileLoader.h"
#include "KDTree.h"
#include "RayTracer.h"
#include <memory>

class LoadingThread : public NanoCore::Thread
{
public:
	LoadingThread() : m_pTree(NULL), m_pLoader(NULL) {}
	void Init( std::wstring wFile, KDTree * pTree ) {
		m_wFile = wFile;
		m_pTree = pTree;
		m_pLoader = NULL;
	}
	void GetStatus( std::wstring & status ) {
		if( m_bLoading && m_pLoader ) {
			wchar_t buf[64];
			swprintf( buf, 64, L"Loading %d %%", m_pLoader->GetLoadingProgress());
			status += buf;
		} else
			status += L"Building KD-tree";
	}
	virtual void Run( void* ) {
		m_pLoader = IObjectFileLoader::Create();
		m_bLoading = true;
		m_pLoader->Load( m_wFile.c_str() );
		m_bLoading = false;
		m_pTree->Build( m_pLoader, 40 );
		OnTerminate();
	}
	virtual void OnTerminate() {
		if( m_pLoader ) {
			delete m_pLoader;
			m_pLoader = NULL;
		}
	}

private:
	std::wstring m_wFile;
	KDTree * m_pTree;
	IObjectFileLoader * m_pLoader;
	bool m_bLoading;
};

class MainWnd : public NanoCore::WindowMain
{
	const static int IDC_FILE_OPEN = 1001;
	const static int IDC_FILE_EXIT = 1002;
	const static int IDC_VIEW_CENTER_CAMERA = 1003;
	const static int IDC_VIEW_CAPTURE_CURRENT_CAMERA = 1004;
	const static int IDC_VIEW_PREVIEWMODE_COLOREDCUBE = 1100;
	const static int IDC_VIEW_PREVIEWMODE_COLOREDCUBESHADOWED = 1101;
	const static int IDC_VIEW_PREVIEWMODE_TRIANGLEID = 1102;
	const static int IDC_VIEW_PREVIEWMODE_CHECKER = 1103;
	const static int IDC_VIEW_OPTIONS = 1005;
	const static int IDC_CAMERAS_FIRST = 2000;

	enum EState {
		STATE_LOADING,
		STATE_PREVIEW,
		STATE_RENDERING,
	};

	class OptionsWnd : public NanoCore::WindowInputDialog {
	public:
		MainWnd & wnd;
		OptionsWnd( MainWnd & wnd ) : wnd(wnd) {
			Add( L"Preview resolution", wnd.m_PreviewResolution );
			Add( L"GI bounces", wnd.m_Raytracer.m_GIBounces );
			Add( L"Sun samples", wnd.m_Raytracer.m_SunSamples );
			Add( L"Sun disk angle", wnd.m_Raytracer.m_SunDiskAngle );
		}
	protected:
		virtual void OnOK() {
			wnd.m_Image.Init( wnd.m_PreviewResolution, wnd.m_PreviewResolution * wnd.GetHeight() / wnd.GetWidth(), 24 );
		}
	};

public:
	MainWnd() {
		m_Raytracer.m_pKDTree = &m_KDTree;
		m_Raytracer.m_pImage = &m_Image;
		m_Raytracer.m_pCamera = &m_Camera;
		m_bInvalidate = false;
		m_State = STATE_PREVIEW;
		m_Raytracer.m_Shading = Raytracer::ePreviewShading_ColoredCube;
		m_PreviewResolution = 200;
	}
	~MainWnd() {
	}
	virtual void OnKey( int key ) {
		switch( key ) {
			case 80:
			case 32:
				if( !m_Raytracer.IsRendering())
					m_Raytracer.Render();
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
		m_Image.Init( m_PreviewResolution, m_PreviewResolution * h / w, 24 );
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
		switch( m_State ) {
			case STATE_LOADING:
				if( m_LoadingThread.IsRunning()) {
					std::wstring str;
					m_LoadingThread.GetStatus( str );
					SetStatus( str.c_str() );
				} else {
					SetStatus( NULL );
					CenterCamera();
					m_State = STATE_PREVIEW;
				}
				break;
			case STATE_PREVIEW:
				if( m_bInvalidate ) {
					m_Raytracer.Render();
					m_bInvalidate = false;
				}
				Redraw();
				break;

			case STATE_RENDERING:
				if( m_Raytracer.IsRendering()) {
					std::wstring str;
					m_Raytracer.GetStatus( str );
					SetStatus( str.c_str());
				} else
					SetStatus( NULL );
				break;
		}
	}
	virtual void OnInit() {
		int mainMenu = CreateMenu();
		int fileMenu = CreateMenu();
		int viewMenu = CreateMenu();
		m_CamerasMenu = CreateMenu();
		int previewMenu = CreateMenu();
		AddSubmenu( mainMenu, L"File", fileMenu );
			AddMenuItem( fileMenu, L"Open", IDC_FILE_OPEN );
			AddMenuItem( fileMenu, L"Exit", IDC_FILE_EXIT );
		AddSubmenu( mainMenu, L"View", viewMenu );
			AddMenuItem( viewMenu, L"Center camera", IDC_VIEW_CENTER_CAMERA );
			AddMenuItem( viewMenu, L"Capture current camera", IDC_VIEW_CAPTURE_CURRENT_CAMERA );
			AddSubmenu( viewMenu, L"Preview mode", previewMenu );
				AddMenuItem( previewMenu, L"Colored cube", IDC_VIEW_PREVIEWMODE_COLOREDCUBE );
				AddMenuItem( previewMenu, L"Colored cube shadowed", IDC_VIEW_PREVIEWMODE_COLOREDCUBESHADOWED );
				AddMenuItem( previewMenu, L"Triangle ID", IDC_VIEW_PREVIEWMODE_TRIANGLEID );
				AddMenuItem( previewMenu, L"Checker", IDC_VIEW_PREVIEWMODE_CHECKER );
			AddMenuItem( viewMenu, L"Options", IDC_VIEW_OPTIONS );
		AddSubmenu( mainMenu, L"Cameras", m_CamerasMenu );
	}
	virtual void OnQuit() {
		m_Raytracer.Stop();
	}
	virtual void OnMenu( int id ) {
		switch( id ) {
			case IDC_FILE_OPEN:
				if( m_State == STATE_PREVIEW ) {
					m_State = STATE_LOADING;
					LoadModel();
				}
				break;
			case IDC_FILE_EXIT:
				m_Raytracer.Stop();
				Exit();
				break;
			case IDC_VIEW_CENTER_CAMERA:
				if( m_State == STATE_PREVIEW )
					CenterCamera();
				break;
			case IDC_VIEW_CAPTURE_CURRENT_CAMERA:
				AddCurrentCamera();
				break;
			case IDC_VIEW_OPTIONS: {
				m_OptionsWnd = std::auto_ptr<OptionsWnd>( new OptionsWnd( *this ) );
				m_OptionsWnd->Show( L"Options" );
				break;
			}
			case IDC_VIEW_PREVIEWMODE_COLOREDCUBE:
				m_Raytracer.m_Shading = Raytracer::ePreviewShading_ColoredCube;
				m_bInvalidate = true;
				break;
			case IDC_VIEW_PREVIEWMODE_COLOREDCUBESHADOWED:
				m_Raytracer.m_Shading = Raytracer::ePreviewShading_ColoredCubeShadowed;
				m_bInvalidate = true;
				break;
			case IDC_VIEW_PREVIEWMODE_TRIANGLEID:
				m_Raytracer.m_Shading = Raytracer::ePreviewShading_TriangleID;
				m_bInvalidate = true;
				break;
			case IDC_VIEW_PREVIEWMODE_CHECKER:
				m_Raytracer.m_Shading = Raytracer::ePreviewShading_Checker;
				m_bInvalidate = true;
				break;
			default:
				if( id >= IDC_CAMERAS_FIRST ) {
					m_Camera = m_Cameras[id - IDC_CAMERAS_FIRST];
					m_bInvalidate = true;
				}
				break;
		}
	}
	void AddCurrentCamera() {
		wchar_t w[128];
		swprintf( w, 128, L"Camera %d", m_Cameras.size()+1 );
		AddMenuItem( m_CamerasMenu, w, IDC_CAMERAS_FIRST + m_Cameras.size());
		m_Cameras.push_back( m_Camera );
	}
	void LoadModel() {
		std::wstring wFolder = NanoCore::GetCurrentFolder();
		std::wstring wFile = ChooseFile( wFolder.c_str(), L"Wavefront object files (*.obj)\0*.obj\0", L"Load model", true );
		if( !wFile.empty()) {
			m_LoadingThread.Init( wFile, &m_KDTree );
			m_LoadingThread.Start( NULL );
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
	void CenterCamera() {
		if( m_KDTree.Empty())
			return;

		float3 min, max;
		m_KDTree.GetBBox( min, max );

		float3 center = (min + max) * 0.5f;
		float L = len( min - center );

		m_Camera.fovy = 60.0f * 3.1415f / 180.0f;
		m_Camera.world_up = float3(0,-1,0);
		m_Camera.LookAt( center, float3(1,-1,-1)*L );
		m_bInvalidate = true;
	}

	std::string     m_strStatus;
	LoadingThread   m_LoadingThread;
	KDTree          m_KDTree;
	Image           m_Image;
	Camera          m_Camera;
	Raytracer       m_Raytracer;
	bool            m_bInvalidate;
	EState          m_State;
	int             m_CamerasMenu;
	std::vector<Camera> m_Cameras;
	Raytracer::EShading m_PreviewShading;
	int             m_PreviewResolution;
	std::auto_ptr<OptionsWnd> m_OptionsWnd;
};

int Main()
{
	MainWnd * pWnd = new MainWnd();
	pWnd->Init( 800, 600 );
	pWnd->SetStatus( NULL );

	while( pWnd->Update()) {
		NanoCore::Sleep( 20 );
	}
	delete pWnd;
	return 0;
}
