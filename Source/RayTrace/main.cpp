#include <stdarg.h>
#include <NanoCore/Threads.h>
#include <NanoCore/Windows.h>
#include <NanoCore/File.h>
#include <NanoCore/Serialize.h>
#include <NanoCore/Image.h>
#include "Common.h"
#include "RayTracer.h"
#include "ShaderPreview.h"
#include "ShaderPhoto.h"
#include "UIOptionsDialog.h"



class LoadingThread : public NanoCore::Thread {
public:
	LoadingThread() : m_pScene(NULL), m_pLoader(NULL), m_pStatusCallback(NULL) {}
	void Init( std::wstring wFile, IScene * pScene, Raytracer * pRaytracer, IStatusCallback * pCallback ) {
		m_wFile = wFile;
		m_pScene = pScene;
		m_pLoader = NULL;
		m_pRaytracer = pRaytracer;
		m_pStatusCallback = pCallback;
	}
	virtual void Run( void* ) {
		m_pLoader = CreateObjLoader();
		m_pLoader->Load( m_wFile.c_str(), m_pStatusCallback );
		m_pScene->Build( m_pLoader, m_pStatusCallback );
		m_pRaytracer->LoadMaterials( m_pLoader, m_pStatusCallback );
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
	IScene * m_pScene;
	ISceneLoader * m_pLoader;
	Raytracer * m_pRaytracer;
	IStatusCallback * m_pStatusCallback;
};



class MainWnd : public NanoCore::WindowMain, public IStatusCallback
{
	const static int IDC_FILE_OPEN = 1001;
	const static int IDC_FILE_SAVE_IMAGE = 1002;
	const static int IDC_FILE_EXIT = 1003;
	const static int IDC_VIEW_CENTER_CAMERA = 1100;
	const static int IDC_VIEW_CAPTURE_CURRENT_CAMERA = 1101;

	const static int IDC_VIEW_PREVIEWMODE_FIRST = 1200;
	const static int IDC_VIEW_PREVIEWMODE_COLOREDCUBE = 1200;
	const static int IDC_VIEW_PREVIEWMODE_COLOREDCUBESHADOWED = 1201;
	const static int IDC_VIEW_PREVIEWMODE_TRIANGLEID = 1202;
	const static int IDC_VIEW_PREVIEWMODE_CHECKER = 1203;
	const static int IDC_VIEW_PREVIEWMODE_DIFFUSE = 1204;
	const static int IDC_VIEW_PREVIEWMODE_SPECULAR = 1205;
	const static int IDC_VIEW_PREVIEWMODE_BUMP = 1206;
	const static int IDC_VIEW_OPTIONS = 1102;
	const static int IDC_OPTIONS_OK = 1103;
	const static int IDC_CAMERAS_FIRST = 2000;



	enum EState {
		STATE_LOADING,
		STATE_PREVIEW,
		STATE_RENDERING,
	};

	enum ESerializeType { eSave, eLoad };

	void Serialize( std::wstring wFile, ESerializeType eST ) {
		NanoCore::IFile::Ptr fp = NanoCore::FS::Open( wFile.c_str(), eST == eSave ? NanoCore::FS::efWriteTrunc : NanoCore::FS::efRead );
		if( !fp )
			return;

		NanoCore::TextFile tf( fp );
		NanoCore::XmlNode * scene = new NanoCore::XmlNode( "Scene" );

		if( eST == eLoad ) scene->Load( tf );

		NanoCore::Serialize( scene, "Environment", m_Options );
		NanoCore::Serialize( scene, "Cameras", m_Cameras );

		if( eST == eSave )
			scene->Save( tf );
		else {
			ClearMenu( m_CamerasMenu );
			for( size_t i=0; i<m_Cameras.size(); ++i ) {
				wchar_t name[64];
				swprintf( name, L"Camera %d", i+1 );
				AddMenuItem( m_CamerasMenu, name, IDC_CAMERAS_FIRST + i );
			}
		}
		delete scene;
	}

public:
	MainWnd() {
		m_bInvalidate = false;
		m_State = STATE_PREVIEW;
		m_PreviewResolution = 200;

		m_UpdateMs = 20;
		m_bCtrlKey = false;
		m_strBottomHelpLine = "Press Space to open file";
		m_MainThreadId = NanoCore::GetCurrentThreadId();

		m_pScene = CreateKDTree( 8 );

		m_Options.push_back( NanoCore::KeyValuePtr( "Preview resolution", m_PreviewResolution ));
		m_Options.push_back( NanoCore::KeyValuePtr( "Raytrace threads", m_Raytracer.m_NumThreads ));
		m_Options.push_back( NanoCore::KeyValuePtr( "GI bounces", m_Environment.GIBounces ));
		m_Options.push_back( NanoCore::KeyValuePtr( "GI samples", m_Environment.GISamples ));
		m_Options.push_back( NanoCore::KeyValuePtr( "Sun samples", m_Environment.SunSamples ));
		m_Options.push_back( NanoCore::KeyValuePtr( "Sun disk angle", m_Environment.SunDiskAngle ));
		m_Options.push_back( NanoCore::KeyValuePtr( "Sun angle 1", m_Environment.SunAngle1 ));
		m_Options.push_back( NanoCore::KeyValuePtr( "Sun angle 2", m_Environment.SunAngle2 ));
		m_Options.push_back( NanoCore::KeyValuePtr( "Sun strength", m_Environment.SunStrength ));
		m_Options.push_back( NanoCore::KeyValuePtr( "Sky strength", m_Environment.SkyStrength ));
	}
	~MainWnd() {
	}
	virtual void OnKey( int key, bool bDown ) {
		switch( key ) {
			case 32:
				if( m_pScene->IsEmpty()) {
					m_State = STATE_LOADING;
					LoadModel();
				} else {
					if( m_State == STATE_PREVIEW && bDown ) {
						m_State = STATE_RENDERING;
						m_Image.Init( GetWidth(), GetHeight(), 24 );
						m_Raytracer.Render( m_Camera, m_Image, m_pScene, m_Environment, &m_ShaderPreview, this );
						m_strBottomHelpLine = "Press Esc to cancel the rendering";
					}
				}
				break;
			case 13:
				if( m_State == STATE_PREVIEW && bDown ) {
					m_State = STATE_RENDERING;
					m_Image.Init( GetWidth(), GetHeight(), 24 );
					m_Raytracer.Render( m_Camera, m_Image, m_pScene, m_Environment, &m_ShaderPhoto, this );
					m_strBottomHelpLine = "Press Esc to cancel the rendering";
				}
				break;
			case 27:
				if( m_State == STATE_RENDERING ) {
					m_Raytracer.Stop();
					m_State = STATE_PREVIEW;
					m_bInvalidate = true;
					m_UpdateMs = 20;
					SetStatus( NULL );
				}
				break;
			case 17:
				m_bCtrlKey = bDown;
				break;
		}
	}
	virtual void OnMouse( int x, int y, int btn_down, int btn_up, int btn_dblclick, int wheel ) {
		if( m_State != STATE_PREVIEW )
			return;

		if( wheel ) {
			float step = len( m_pScene->GetAABB().GetSize() ) / (m_bCtrlKey ? 60.0f : 30.0f);
			m_Camera.pos += (wheel>0 ? m_Camera.at : -m_Camera.at) * step;
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
			m_Camera.Rotate( DEG2RAD(y - dragY) * 0.5f, DEG2RAD(x - dragX) * 0.5f );
			m_bInvalidate = true;
		}
		if( btn_down & 2 ) {
			RightClick( x, y );
		}
	}
	virtual void OnSize( int w, int h ) {
		if( !w || !h )
			return;
		if( m_Image.GetWidth() != w || m_Image.GetHeight() != h ) {
			m_Raytracer.Stop();
			m_Image.Init( m_PreviewResolution, m_PreviewResolution * h / w, 24 );
			m_Image.Fill( 0x303540 );
			m_bInvalidate = true;
		}
	}
	virtual void OnDraw()
	{
		if( m_Image.GetWidth()) {

			int percent = m_Raytracer.GetProgress();

			if( percent > 95 || m_State == STATE_PREVIEW ) {
				DrawImage( 0, 0, GetWidth(), GetHeight(), m_Image.GetImageAt(0,0), m_Image.GetWidth(), m_Image.GetHeight(), 24 );
			} else {
				static int lastWidth = 0;
				percent = Max( percent, 1 );
				int lw = m_Image.GetWidth()*percent / 100;
				if( lw % 4 ) lw += 4 - (lw % 4);

				if( lastWidth != lw ) {
					lastWidth = lw;
					m_LowresImage.Init( lw, m_Image.GetHeight() * percent / 100, 24 );
					m_LowresImage.Average( m_Image );
					DrawImage( 0, 0, GetWidth(), GetHeight(), m_LowresImage.GetImageAt(0,0), m_LowresImage.GetWidth(), m_LowresImage.GetHeight(), 24 );
				}
			}
			if( !m_strBottomHelpLine.empty()) {
				DrawText( 11, GetHeight() - 19, m_strBottomHelpLine.c_str(), 0x000000 );
				DrawText( 10, GetHeight() - 20, m_strBottomHelpLine.c_str(), 0xFFFFFF );
			}
		}
	}
	virtual void OnUpdate()
	{
		{
			NanoCore::csScope cs( m_csStatus );
			if( !m_strStatus.empty()) {
				SetCaption( m_strStatus.c_str());
				m_strStatus.clear();
			}
		}
		switch( m_State ) {
			case STATE_LOADING:
				if( ! m_LoadingThread.IsRunning()) {
					CenterCamera();
					m_State = STATE_PREVIEW;
					m_UpdateMs = 100;
				}
				break;
			case STATE_PREVIEW:
				if( m_bInvalidate ) {
					if( m_Image.GetWidth() != m_PreviewResolution ) {
						m_Image.Init( m_PreviewResolution, m_PreviewResolution * GetHeight() / GetWidth(), 24 );
					}
					m_Raytracer.Render( m_Camera, m_Image, m_pScene, m_Environment, &m_ShaderPreview, this );
					m_bInvalidate = false;
				}
				Redraw();
				break;

			case STATE_RENDERING:
				if( m_Raytracer.IsRendering()) {
					Redraw();
				} else {
					if( m_bInvalidate ) {
						m_bInvalidate = false;
						m_Raytracer.Render( m_Camera, m_Image, m_pScene, m_Environment, &m_ShaderPhoto, this );
					} else {
						m_State = STATE_PREVIEW;
						m_UpdateMs = 20;
						SetStatus( NULL );
					}
				}
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
			AddMenuItem( fileMenu, L"Save image", IDC_FILE_SAVE_IMAGE );
			AddMenuItem( fileMenu, L"Exit", IDC_FILE_EXIT );
		AddSubmenu( mainMenu, L"View", viewMenu );
			AddMenuItem( viewMenu, L"Center camera", IDC_VIEW_CENTER_CAMERA );
			AddMenuItem( viewMenu, L"Capture current camera", IDC_VIEW_CAPTURE_CURRENT_CAMERA );
			AddSubmenu( viewMenu, L"Preview mode", previewMenu );
				AddMenuItem( previewMenu, L"Colored cube", IDC_VIEW_PREVIEWMODE_COLOREDCUBE );
				AddMenuItem( previewMenu, L"Colored cube shadowed", IDC_VIEW_PREVIEWMODE_COLOREDCUBESHADOWED );
				AddMenuItem( previewMenu, L"Triangle ID", IDC_VIEW_PREVIEWMODE_TRIANGLEID );
				AddMenuItem( previewMenu, L"Checker", IDC_VIEW_PREVIEWMODE_CHECKER );
				AddMenuItem( previewMenu, L"Diffuse map", IDC_VIEW_PREVIEWMODE_DIFFUSE );
				AddMenuItem( previewMenu, L"Specular map", IDC_VIEW_PREVIEWMODE_SPECULAR );
				AddMenuItem( previewMenu, L"Bump map", IDC_VIEW_PREVIEWMODE_BUMP );
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
			case IDC_FILE_SAVE_IMAGE:
				SaveImage();
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
				m_pOptionsDialog = std::auto_ptr<OptionsDialog>( new OptionsDialog( this, IDC_OPTIONS_OK, m_Environment, m_PreviewResolution, m_Raytracer.m_NumThreads ));
				m_pOptionsDialog->m_Items = m_Options;
				m_pOptionsDialog->Show( L"Options" );
				break;
			}
			case IDC_OPTIONS_OK:
				m_Image.Init( m_PreviewResolution, m_PreviewResolution * GetHeight() / GetWidth(), 24 );
				Serialize( m_wFile + L".xml", eSave );
				break;
			case IDC_VIEW_PREVIEWMODE_COLOREDCUBE:
			case IDC_VIEW_PREVIEWMODE_COLOREDCUBESHADOWED:
			case IDC_VIEW_PREVIEWMODE_TRIANGLEID:
			case IDC_VIEW_PREVIEWMODE_CHECKER:
			case IDC_VIEW_PREVIEWMODE_DIFFUSE:
			case IDC_VIEW_PREVIEWMODE_SPECULAR:
			case IDC_VIEW_PREVIEWMODE_BUMP:
				m_ShaderPreview.m_Shader = ShaderPreview::EShader( ShaderPreview::eFirst + (id - IDC_VIEW_PREVIEWMODE_FIRST) );
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
	void RightClick( int x, int y ) {
		m_Raytracer.m_DebugX = x;
		m_Raytracer.m_DebugY = y;

		float3 dir = m_Camera.ConstructRay( x, GetHeight() - y, GetWidth(), GetHeight() );

		IntersectResult result;
		if( m_pScene->IntersectRay( Ray( m_Camera.pos, dir ), result ) ) {
			char pc[128];
			sprintf_s( pc, "Picked material '%s' (%d)", m_Raytracer.m_Materials[result.materialId].name.c_str(), result.materialId );
			NanoCore::DebugOutput( pc );
			m_strBottomHelpLine = pc;
		} else {
			m_Raytracer.m_SelectedTriangle = -1;
			m_strBottomHelpLine = "Nothing there";
		}
		m_bInvalidate = true;
	}
	void AddCurrentCamera() {
		wchar_t w[128];
		swprintf( w, 128, L"Camera %d", m_Cameras.size()+1 );
		AddMenuItem( m_CamerasMenu, w, IDC_CAMERAS_FIRST + (int)m_Cameras.size());
		m_Cameras.push_back( m_Camera );
		Serialize( m_wFile + L".xml", eSave );
	}
	void LoadModel() {
		std::wstring wFolder = NanoCore::GetExecutableFolder();
		std::wstring wFile = ChooseFile( wFolder.c_str(), L"Wavefront object files (*.obj)\0*.obj\0", L"Load model", true );
		if( !wFile.empty()) {
			m_LoadingThread.Init( wFile, m_pScene, &m_Raytracer, this );
			m_LoadingThread.Start( NULL );

			m_wFile = wFile;
			std::wstring w = NanoCore::StrGetFilename( wFile );
			m_sFilename = NanoCore::StrWcsToMbs( w.c_str() );
			size_t p = m_wFile.find_last_of( L'.' );
			if( p != std::wstring::npos )
				m_wFile.erase( p );

			Serialize( m_wFile + L".xml", eLoad );
		}
	}
	void SaveImage() {
		std::wstring wFolder = NanoCore::GetCurrentFolder();
		std::wstring wFile = ChooseFile( wFolder.c_str(), L"Bitmap(*.bmp)\0*.bmp\0", L"Save image", false );
		if( !wFile.empty()) {
			m_Image.WriteAsBMP( wFile.c_str() );
		}
	}
	virtual void SetStatus( const char * pcFormat, ... ) {
#ifdef NDEBUG
		const char * pcPlatform = (sizeof(void*) == 8) ? "Release(64 bit)" : "Release(32 bit)";
#else
		const char * pcPlatform = (sizeof(void*) == 8) ? "Debug(64 bit)" : "Debug(32 bit)";
#endif
		char buf[256];
		int len = sprintf_s( buf, "Raytracer | %s | https://github.com/sergeiam/ | %s", m_sFilename.c_str(), pcPlatform );
		if( pcFormat ) {
			strcat_s( buf, " | " );
			len += 3;
			va_list args;
			va_start( args, pcFormat );
			vsprintf_s( buf + len, sizeof(buf)-len, pcFormat, args );
			va_end( args );
		}
		if( NanoCore::GetCurrentThreadId() == m_MainThreadId )
			SetCaption( buf );
		else {
			NanoCore::csScope cs( m_csStatus );			
			m_strStatus = buf;
		}
	}
	void CenterCamera() {
		if( m_pScene->IsEmpty())
			return;

		AABB box = m_pScene->GetAABB();

		float3 center = (box.min + box.max) * 0.5f;
		float L = len( box.min - center );

		m_Camera.fovy = 60.0f;
		m_Camera.world_up = float3(0,1,0);
		m_Camera.LookAt( center, float3(1,-1,-1)*L );
		m_bInvalidate = true;
	}

	std::wstring    m_wFile;
	std::string     m_sFilename;
	std::string     m_strStatus, m_strBottomHelpLine;
	LoadingThread   m_LoadingThread;
	IScene*         m_pScene;
	NanoCore::Image m_Image, m_LowresImage;
	Camera          m_Camera;
	Raytracer       m_Raytracer;
	bool            m_bInvalidate;
	EState          m_State;
	int             m_CamerasMenu;
	std::vector<Camera> m_Cameras;
	int             m_PreviewResolution;
	int             m_UpdateMs;
	bool            m_bCtrlKey;
	Environment     m_Environment;
	uint32          m_MainThreadId;
	NanoCore::CriticalSection m_csStatus;
	ShaderPreview   m_ShaderPreview;
	ShaderPhoto     m_ShaderPhoto;

	std::vector<NanoCore::KeyValuePtr>  m_Options;
	std::auto_ptr<OptionsDialog> m_pOptionsDialog;
};


int Main()
{
	MainWnd * pWnd = new MainWnd();
	pWnd->Init( 800, 600 );
	pWnd->SetStatus( NULL );

	while( pWnd->Update()) {
		NanoCore::Sleep( pWnd->m_UpdateMs );
	}
	delete pWnd;
	return 0;
}
