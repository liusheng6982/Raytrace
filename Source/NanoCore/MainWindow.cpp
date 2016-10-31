#include <windows.h>
#include <tchar.h>
#include <vector>
#include "MainWindow.h"



extern int Main();



static TCHAR szWindowClass[] = _T("MainWindow");

HINSTANCE g_hInstance;
HWND g_hWnd;

static HDC  s_hDrawDC;
static int s_CmdShow;
static NanoCore::MainWindow * s_pMainWindow;
static bool s_bQuit;
static int s_Width, s_Height, s_MouseX, s_MouseY;



static LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	int mouseup = 0, mousedown = 0, mousedblclick = 0, mousewheel = 0;

	switch( message ) {
		case WM_KEYDOWN:
			s_pMainWindow->OnKey( wParam );
			break;

		case WM_MOUSEMOVE:
			s_MouseX = LOWORD(lParam);
			s_MouseY = HIWORD(lParam);
			s_pMainWindow->OnMouse( s_MouseX, s_MouseY, 0, 0, 0, 0 );
			break;
		case WM_LBUTTONDOWN: mousedown = 1; break;
		case WM_LBUTTONUP: mouseup = 1; break;
		case WM_RBUTTONDOWN: mousedown = 2; break;
		case WM_RBUTTONUP: mouseup = 2; break;
		case WM_MOUSEWHEEL: mousewheel = GET_WHEEL_DELTA_WPARAM(wParam); break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			s_hDrawDC = BeginPaint( hWnd, &ps );
			s_pMainWindow->OnDraw();
			EndPaint( hWnd, &ps );
			s_hDrawDC = NULL;
			break;
		}
		case WM_ERASEBKGND:
			break;
		case WM_SIZE: {
			s_Width = LOWORD(lParam);
			s_Height = HIWORD(lParam);

			RECT rc;
			GetClientRect( hWnd, &rc );

			s_Width = rc.right - rc.left;
			s_Height = rc.bottom - rc.top;

			s_pMainWindow->OnSize( s_Width, s_Height );
			break;
		}
		case WM_COMMAND:
			if( HIWORD(wParam) == 0 ) {
				s_pMainWindow->OnMenu( LOWORD(wParam) );
				break;
			}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_QUIT:
		case WM_CLOSE:
			s_pMainWindow->OnQuit();
			s_bQuit = true;
			break;
		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
	}
	if( mouseup || mousedown || mousedblclick || mousewheel )
		s_pMainWindow->OnMouse( s_MouseX, s_MouseY, mousedown, mouseup, mousedblclick, mousewheel );

	return 0;
}

namespace NanoCore {

MainWindow::MainWindow()
{
	s_pMainWindow = this;
	s_bQuit = false;
}
MainWindow::~MainWindow()
{
	s_pMainWindow = NULL;
}

bool MainWindow::Init( int w, int h )
{
	WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, g_hInstance, 0, 0, (HBRUSH)(COLOR_WINDOW+1), 0, szWindowClass, 0 };
	wcex.hIcon          = LoadIcon( g_hInstance, MAKEINTRESOURCE(IDI_APPLICATION) );
	wcex.hCursor        = LoadCursor( NULL, IDC_ARROW );
	wcex.hIconSm        = LoadIcon( g_hInstance, MAKEINTRESOURCE(IDI_APPLICATION) );
	if( ! RegisterClassEx( &wcex ))
		return false;

	g_hWnd = CreateWindow( szWindowClass, _T(""), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 820, 650, NULL, NULL, g_hInstance, NULL );
	if( !g_hWnd )
		return false;

	OnInit();
	ShowWindow( g_hWnd, s_CmdShow );
	UpdateWindow( g_hWnd );	

	return true;
}

bool MainWindow::Update()
{
	MSG msg;
	while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE )) {
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
	s_pMainWindow->OnUpdate();
	return !s_bQuit;
}

void MainWindow::SetCaption( const wchar_t * pwCaption )
{
	SetWindowText( g_hWnd, pwCaption );
}

void MainWindow::Redraw()
{
	InvalidateRect( g_hWnd, NULL, TRUE );
	UpdateWindow( g_hWnd );
}

void MainWindow::Exit()
{
	::PostQuitMessage( 0 );
}

int MainWindow::GetWidth()
{
	return s_Width;
}

int MainWindow::GetHeight()
{
	return s_Height;
}

std::wstring MainWindow::ChooseFile( const wchar_t * pwFolder, const wchar_t * pwFilter, const wchar_t * pwCaption, bool bLoad )
{
	wchar_t wBuffer[1024] = {0};

	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = g_hWnd;
	ofn.lpstrTitle = pwCaption;
	ofn.lpstrFile = wBuffer;
	ofn.nMaxFile = 1024;
	ofn.lpstrInitialDir = pwFolder;
	ofn.lpstrFilter = pwFilter;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_ENABLESIZING;
	if( bLoad )
		ofn.Flags |= OFN_FILEMUSTEXIST;

	BOOL ret = bLoad ? GetOpenFileName( &ofn ) : GetSaveFileName( &ofn );
	if( !ret ) wBuffer[0] = L'\0';
	return std::wstring(wBuffer);
}

void MainWindow::DrawImage( int x, int y, int w, int h, const uint8 * pImage, int iw, int ih, int bpp )
{
	if( !s_hDrawDC )
		return;
	static BITMAPINFO bmp = { { sizeof(BITMAPINFOHEADER), 0, 0, 1, bpp, BI_RGB, 0, 0, 0, 0, 0 } };
	bmp.bmiHeader.biWidth = iw;
	bmp.bmiHeader.biHeight = ih;
	int ret = StretchDIBits( s_hDrawDC, x, y, w, h, 0, 0, iw, ih, pImage, &bmp, DIB_RGB_COLORS, SRCCOPY );
}

void MainWindow::DrawText( int x, int y, const char * pcText )
{
	if( !s_hDrawDC )
		return;
	TextOutA( s_hDrawDC, x, y, pcText, (int)strlen(pcText) );
}

static std::vector<HMENU> s_Menus;

int MainWindow::CreateMenu()
{
	s_Menus.push_back( ::CreateMenu() );
	if( s_Menus.size() == 1 ) {
		::SetMenu( g_hWnd, s_Menus[0] );
	}
	return int(s_Menus.size()-1);
}

void MainWindow::AddMenuItem( int menu, const wchar_t * pcName, int id )
{
	if( menu<0 || menu >= s_Menus.size())
		return;
	::AppendMenu( s_Menus[menu], MF_STRING, (UINT_PTR)id, pcName );
	::DrawMenuBar( g_hWnd );
}

void MainWindow::AddSubmenu( int menu, const wchar_t * pcName, int submenu )
{
	if( menu<0 || menu >= s_Menus.size())
		return;
	if( submenu < 0 || submenu >= s_Menus.size())
		return;
	::AppendMenu( s_Menus[menu], MF_STRING | MF_POPUP, (UINT_PTR)s_Menus[submenu], pcName );
	::DrawMenuBar( g_hWnd );
}

} //NanoCore

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	g_hInstance = hInstance;
	s_CmdShow = nCmdShow;

	return Main();
}