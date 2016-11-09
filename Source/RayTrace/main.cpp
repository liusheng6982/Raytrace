#include <NanoCore/Threads.h>
#include <NanoCore/Windows.h>
#include <NanoCore/File.h>
#include <NanoCore/Serialize.h>
#include "ObjectFileLoader.h"
#include "KDTree.h"
#include "RayTracer.h"
#include <memory>



class LoadingThread : public NanoCore::Thread
{
public:
	LoadingThread() : m_pTree(NULL), m_pLoader(NULL) {}
	void Init( std::wstring wFile, KDTree * pTree, Raytracer * pRaytracer ) {
		m_wFile = wFile;
		m_pTree = pTree;
		m_pLoader = NULL;
		m_pRaytracer = pRaytracer;
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
		m_pRaytracer->LoadMaterials( *m_pLoader );
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
	Raytracer * m_pRaytracer;
	bool m_bLoading;
};

class MainWnd : public NanoCore::WindowMain
{
	const static int IDC_FILE_OPEN = 1001;
	const static int IDC_FILE_SAVE_IMAGE = 1002;
	const static int IDC_FILE_EXIT = 1003;
	const static int IDC_VIEW_CENTER_CAMERA = 1100;
	const static int IDC_VIEW_CAPTURE_CURRENT_CAMERA = 1101;
	const static int IDC_VIEW_PREVIEWMODE_COLOREDCUBE = 1200;
	const static int IDC_VIEW_PREVIEWMODE_COLOREDCUBESHADOWED = 1201;
	const static int IDC_VIEW_PREVIEWMODE_TRIANGLEID = 1202;
	const static int IDC_VIEW_PREVIEWMODE_CHECKER = 1203;
	const static int IDC_VIEW_PREVIEWMODE_DIFFUSE = 1204;
	const static int IDC_VIEW_OPTIONS = 1102;
	const static int IDC_CAMERAS_FIRST = 2000;

	enum EState {
		STATE_LOADING,
		STATE_PREVIEW,
		STATE_RENDERING,
	};

	struct OptionItem {
		std::string name;
		int * i;
		float * f;
		std::string * str;
		std::vector<std::string> combo;

		OptionItem(){}
		OptionItem( const char * name, int & i ) : name(name), i(&i), f(0), str(0) {}
		OptionItem( const char * name, float & f ) : name(name), i(0), f(&f), str(0) {}
		OptionItem( const char * name, std::string & str ) : name(name), i(0), f(0), str(&str) {}
	};
	std::vector<OptionItem> m_OptionItems;

	void PersistOptions( bool bLoad ) {
		std::wstring wFile = m_wFile + L".options";
		FILE * fp = _wfopen( wFile.c_str(), bLoad ? L"rt" : L"wt" );
		if( !fp )
			return;

		for( size_t i=0; i<m_OptionItems.size(); ++i ) {
			OptionItem & it = m_OptionItems[i];
			if( bLoad ) {
				char buf[512] = "";
				fgets( buf, 512, fp );

				char * nl = strchr( buf, '\n' );
				if( nl ) nl[0] = 0;
				nl = strchr( buf, '\r' );
				if( nl ) nl[0] = 0;

				const char * p = strchr( buf, '=' );
				if( p ) {
					if( it.i )
						*it.i = atol(p+1);
					else if( it.f )
						*it.f = (float)atof(p+1);
					else
						*it.str = p+1;
				}
			} else {
				fprintf( fp, "%s=", it.name.c_str() );
				if( it.i )
					fprintf( fp, "%d\n", *it.i );
				else if( it.f )
					fprintf( fp, "%0.5f\n", *it.f );
				else
					fprintf( fp, "%s\n", it.str->c_str() );
			}
		}
		fclose( fp );
	}

	class OptionsWnd : public NanoCore::WindowInputDialog {
	public:
		MainWnd & wnd;
		OptionsWnd( MainWnd & wnd ) : wnd(wnd) {
			for( size_t i=0; i<wnd.m_OptionItems.size(); ++i ) {
				OptionItem & it = wnd.m_OptionItems[i];
				if( it.i )
					Add( it.name.c_str(), *it.i );
				else if( it.f )
					Add( it.name.c_str(), *it.f );
				else if( it.str )
					Add( it.name.c_str(), *it.str );
			}
		}
	protected:
		virtual void OnOK() {
			wnd.m_Image.Init( wnd.m_PreviewResolution, wnd.m_PreviewResolution * wnd.GetHeight() / wnd.GetWidth(), 24 );
			wnd.PersistOptions( false );
		}
	};

public:
	MainWnd() {
		m_bInvalidate = false;
		m_State = STATE_PREVIEW;
		m_Raytracer.m_Shading = Raytracer::ePreviewShading_ColoredCube;
		m_PreviewResolution = 200;

		m_OptionItems.push_back( OptionItem( "Preview resolution", m_PreviewResolution ));
		m_OptionItems.push_back( OptionItem( "Raytrace threads", m_Raytracer.m_NumThreads ));
		m_OptionItems.push_back( OptionItem( "GI bounces", m_Raytracer.m_GIBounces ));
		m_OptionItems.push_back( OptionItem( "Sun samples", m_Raytracer.m_SunSamples ));
		m_OptionItems.push_back( OptionItem( "Sun disk angle", m_Raytracer.m_SunDiskAngle ));

		m_UpdateMs = 20;
	}
	~MainWnd() {
	}
	virtual void OnKey( int key ) {
		switch( key ) {
			case 32:
				if( m_State == STATE_PREVIEW ) {
					m_State = STATE_RENDERING;
					m_Image.Init( GetWidth(), GetHeight(), 24 );
					m_Raytracer.Render( m_Camera, m_Image, m_KDTree );
				}
				break;
		}
	}
	virtual void OnMouse( int x, int y, int btn_down, int btn_up, int btn_dblclick, int wheel )
	{
		if( m_State != STATE_PREVIEW )
			return;

		if( wheel ) {
			float step = len( m_KDTree.GetBBox().GetSize() ) / 20.0f;
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
			m_Camera.Rotate( (dragY - y) * 3.1415f / 200.0f, (x - dragX) * 3.1415f / 200.0f );
			m_bInvalidate = true;
		}
		if( btn_down & 2 ) {
			RightClick( x, y );
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
					m_UpdateMs = 100;
				}
				break;
			case STATE_PREVIEW:
				if( m_bInvalidate ) {
					if( m_Image.GetWidth() != m_PreviewResolution ) {
						m_Image.Init( m_PreviewResolution, m_PreviewResolution * GetHeight() / GetWidth(), 24 );
					}
					m_Raytracer.Render( m_Camera, m_Image, m_KDTree );
					m_bInvalidate = false;
				}
				Redraw();
				break;

			case STATE_RENDERING:
				if( m_Raytracer.IsRendering()) {
					std::wstring str;
					m_Raytracer.GetStatus( str );
					SetStatus( str.c_str());
					Redraw();
				} else {
					if( m_bInvalidate ) {
						m_bInvalidate = false;
						m_Raytracer.Render( m_Camera, m_Image, m_KDTree );
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
			case IDC_VIEW_PREVIEWMODE_DIFFUSE:
				m_Raytracer.m_Shading = Raytracer::ePreviewShading_Diffuse;
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
		RayInfo ri;
		ri.Init( x, GetHeight() - y, m_Camera, GetWidth(), GetHeight() );
		m_KDTree.Intersect( ri );

		m_Raytracer.m_DebugX = x;
		m_Raytracer.m_DebugY = y;

		NanoCore::DebugOutput( "Triangle picker:\n" );
		if( ri.tri ) {
			NanoCore::DebugOutput( "  triangleID: %d\n\n", ri.tri->triangleID );
			m_Raytracer.m_SelectedTriangle = ri.tri->triangleID;
		} else {
			m_Raytracer.m_SelectedTriangle = -1;
		}
		m_bInvalidate = true;
	}
	void AddCurrentCamera() {
		wchar_t w[128];
		swprintf( w, 128, L"Camera %d", m_Cameras.size()+1 );
		AddMenuItem( m_CamerasMenu, w, IDC_CAMERAS_FIRST + m_Cameras.size());
		m_Cameras.push_back( m_Camera );
		SaveCameras();
	}
	void SaveCameras() {
		NanoCore::XmlNode * root = new NanoCore::XmlNode( "Cameras" );
		for( size_t i=0; i<m_Cameras.size(); ++i ) {
			root->AddChild( new NanoCore::XmlNode( "Camera" ));
			m_Cameras[i].Serialize( root->GetChild( i ));
		}
		std::wstring name = m_wFile + L".cameras.xml";
		NanoCore::IFile * f = NanoCore::FS::Open( name.c_str(), NanoCore::FS::efWrite | NanoCore::FS::efTrunc );
		if( f ) {
			NanoCore::TextFile tf( f );
			root->Save( tf );
			delete root;
		}
	}
	void LoadCameras() {
		m_Cameras.clear();
		ClearMenu( m_CamerasMenu );
		std::wstring name = m_wFile + L".cameras.xml";
		NanoCore::IFile * f = NanoCore::FS::Open( name.c_str(), NanoCore::FS::efRead );
		if( f ) {
			NanoCore::TextFile tf( f );
			NanoCore::XmlNode * root = new NanoCore::XmlNode();
			root->Load( tf );
			for( int i=0; i<root->GetNumChildren(); ++i ) {
				NanoCore::XmlNode * camNode = root->GetChild( i );
				if( !strcmp( camNode->GetName(), "Camera" )) {
					m_Cameras.push_back( Camera() );
					m_Cameras.back().Serialize( camNode );

					wchar_t w[128];
					swprintf( w, 128, L"Camera %d", i+1 );
					AddMenuItem( m_CamerasMenu, w, IDC_CAMERAS_FIRST + i);
				}
			}
			delete root;
		}
	}
	void LoadModel() {
		std::wstring wFolder = NanoCore::GetExecutableFolder();
		std::wstring wFile = ChooseFile( wFolder.c_str(), L"Wavefront object files (*.obj)\0*.obj\0", L"Load model", true );
		if( !wFile.empty()) {
			m_LoadingThread.Init( wFile, &m_KDTree, &m_Raytracer );
			m_LoadingThread.Start( NULL );

			m_wFile = wFile;
			size_t p = m_wFile.find_last_of( L'.' );
			if( p != std::wstring::npos )
				m_wFile.erase( p );

			PersistOptions( true );
			LoadCameras();
		}
	}
	void SaveImage() {
		std::wstring wFolder = NanoCore::GetCurrentFolder();
		std::wstring wFile = ChooseFile( wFolder.c_str(), L"Bitmap(*.bmp)\0*.bmp\0", L"Save image", false );
		if( !wFile.empty()) {
			m_Image.WriteAsBMP( wFile.c_str() );
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

		aabb box = m_KDTree.GetBBox();

		float3 center = (box.min + box.max) * 0.5f;
		float L = len( box.min - center );

		m_Camera.fovy = 60.0f * 3.1415f / 180.0f;
		m_Camera.world_up = float3(0,-1,0);
		m_Camera.LookAt( center, float3(1,-1,-1)*L );
		m_bInvalidate = true;
	}

	std::wstring    m_wFile;
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
	int             m_UpdateMs;
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
