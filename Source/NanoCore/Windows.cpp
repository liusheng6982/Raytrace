#include "Windows.h"
#include <windows.h>
#include <tchar.h>
#include <vector>
#include <map>


#undef DrawText

extern int Main();



static TCHAR szWindowClass[] = _T("WindowMain");

HINSTANCE g_hInstance;
HWND g_hWnd;

static HDC  s_hDrawDC;
static int s_CmdShow;
static NanoCore::WindowMain * s_pWindowMain;
static bool s_bQuit;
static int s_Width, s_Height, s_MouseX, s_MouseY;



static LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	int mouseup = 0, mousedown = 0, mousedblclick = 0, mousewheel = 0;

	switch( message ) {
		case WM_KEYDOWN:
			s_pWindowMain->OnKey( (int)wParam, message == WM_KEYDOWN );
			break;

		case WM_MOUSEMOVE:
			s_MouseX = LOWORD(lParam);
			s_MouseY = HIWORD(lParam);
			s_pWindowMain->OnMouse( s_MouseX, s_MouseY, 0, 0, 0, 0 );
			break;
		case WM_LBUTTONDOWN: mousedown = 1; break;
		case WM_LBUTTONUP: mouseup = 1; break;
		case WM_RBUTTONDOWN: mousedown = 2; break;
		case WM_RBUTTONUP: mouseup = 2; break;
		case WM_MOUSEWHEEL: mousewheel = GET_WHEEL_DELTA_WPARAM(wParam); break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			s_hDrawDC = BeginPaint( hWnd, &ps );
			s_pWindowMain->OnDraw();
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

			s_pWindowMain->OnSize( s_Width, s_Height );
			break;
		}
		case WM_COMMAND:
			if( HIWORD(wParam) == 0 ) {
				s_pWindowMain->OnMenu( LOWORD(wParam) );
				break;
			}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_QUIT:
		case WM_CLOSE:
			s_pWindowMain->OnQuit();
			s_bQuit = true;
			break;
		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
	}
	if( mouseup || mousedown || mousedblclick || mousewheel )
		s_pWindowMain->OnMouse( s_MouseX, s_MouseY, mousedown, mouseup, mousedblclick, mousewheel );

	return 0;
}

namespace NanoCore {

WindowMain::WindowMain()
{
	s_pWindowMain = this;
	s_bQuit = false;
}
WindowMain::~WindowMain()
{
	s_pWindowMain = NULL;
}

bool WindowMain::Init( int w, int h )
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

bool WindowMain::Update()
{
	MSG msg;
	while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE )) {
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
	s_pWindowMain->OnUpdate();
	return !s_bQuit;
}

void WindowMain::SetCaption( const char * caption ) {
	SetWindowTextA( g_hWnd, caption );
}

void WindowMain::Redraw() {
	InvalidateRect( g_hWnd, NULL, TRUE );
	UpdateWindow( g_hWnd );
}

void WindowMain::Exit() {
	::PostQuitMessage( 0 );
}

int WindowMain::GetWidth() {
	return s_Width;
}

int WindowMain::GetHeight() {
	return s_Height;
}

std::wstring WindowMain::ChooseFile( const wchar_t * pwFolder, const wchar_t * pwFilter, const wchar_t * pwCaption, bool bLoad ) {
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

void WindowMain::DrawImage( int x, int y, int w, int h, const uint8 * pImage, int iw, int ih, int bpp ) {
	if( !s_hDrawDC )
		return;
	static BITMAPINFO bmp = { { sizeof(BITMAPINFOHEADER), 0, 0, 1, bpp, BI_RGB, 0, 0, 0, 0, 0 } };
	bmp.bmiHeader.biWidth = iw;
	bmp.bmiHeader.biHeight = ih;
	StretchDIBits( s_hDrawDC, x, y, w, h, 0, 0, iw, ih, pImage, &bmp, DIB_RGB_COLORS, SRCCOPY );
}

void WindowMain::DrawText( int x, int y, const char * pcText, uint32 color ) {
	if( !s_hDrawDC )
		return;
	SetBkMode( s_hDrawDC, TRANSPARENT );
	SetTextColor( s_hDrawDC, color );
	TextOutA( s_hDrawDC, x, y, pcText, (int)strlen(pcText) );
}

static std::vector<HMENU> s_Menus;

int WindowMain::CreateMenu() {
	s_Menus.push_back( ::CreateMenu() );
	if( s_Menus.size() == 1 ) {
		::SetMenu( g_hWnd, s_Menus[0] );
	}
	return int(s_Menus.size()-1);
}

void WindowMain::AddMenuItem( int menu, const wchar_t * pcName, int id ) {
	if( menu<0 || menu >= (int)s_Menus.size())
		return;
	::AppendMenu( s_Menus[menu], MF_STRING, (UINT_PTR)id, pcName );
	::DrawMenuBar( g_hWnd );
}

void WindowMain::AddSubmenu( int menu, const wchar_t * pcName, int submenu ) {
	if( menu<0 || menu >= (int)s_Menus.size())
		return;
	if( submenu < 0 || submenu >= (int)s_Menus.size())
		return;
	::AppendMenu( s_Menus[menu], MF_STRING | MF_POPUP, (UINT_PTR)s_Menus[submenu], pcName );
	::DrawMenuBar( g_hWnd );
}

void WindowMain::ClearMenu( int menu ) {
	while( ::GetMenuItemCount( s_Menus[menu] ) > 0 ) {
		::DeleteMenu( s_Menus[menu], 0, MF_BYPOSITION );
	}
}

bool WindowMain::MsgBox( const wchar_t * caption, const wchar_t * text, bool bOkCancel ) {
	return ::MessageBox( g_hWnd, text, caption, bOkCancel ? MB_OKCANCEL : MB_OK ) == IDOK;
}

struct Item {
	std::string name;
	std::string * pStr;
	float * f;
	int * i;
	std::vector<std::wstring> combo;
	HWND hWnd;

	Item():pStr(NULL),f(NULL),i(NULL){}
};

struct WindowInputDialog::Impl
{
	std::vector<Item> items;
};

WindowInputDialog::WindowInputDialog()
{
	m_pImpl = new Impl();
}

WindowInputDialog::~WindowInputDialog()
{
	delete m_pImpl;
}

static std::map<HWND,WindowInputDialog*> s_wid;
static WindowInputDialog * s_pWid;

void WindowInputDialog::Add( const char * name, int & i ) {
	Item it;
	it.name = name;
	it.i = &i;
	m_pImpl->items.push_back( it );
}

void WindowInputDialog::Add( const char * name, float & f ) {
	Item it;
	it.name = name;
	it.f = &f;
	m_pImpl->items.push_back( it );
}

void WindowInputDialog::Add( const char * name, std::string & str ) {
	Item it;
	it.name = name;
	it.pStr = &str;
	m_pImpl->items.push_back( it );
}

static void CreateDialogElements( HWND hWnd )
{
	WindowInputDialog * pDialog = s_pWid;
	int y = 10, id = 1000;
	for( auto it = pDialog->m_pImpl->items.begin(); it != pDialog->m_pImpl->items.end(); ++it, ++id ) {
		char buf[64];
		if( it->pStr )
			strcpy_s( buf, it->pStr->c_str() );
		else if( it->f )
			sprintf_s( buf, "%0.4f", *it->f );
		else if( it->i )
			sprintf_s( buf, "%d", *it->i );

		CreateWindowA( "STATIC", it->name.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT, 10, y, 220, 20, hWnd, 0, g_hInstance, NULL );
		it->hWnd = CreateWindowA( "EDIT", buf, WS_CHILD | WS_VISIBLE, 230, y, 150, 20, hWnd, (HMENU)id, g_hInstance, NULL );
		y += 20;
	}
	y += 20;
	CreateWindow( L"BUTTON", L"Ok", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, y, 100, 20, hWnd, (HMENU)IDOK, g_hInstance, NULL );
	CreateWindow( L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 120, y, 100, 20, hWnd, (HMENU)IDCANCEL, g_hInstance, NULL );

	UpdateWindow( hWnd );
}

static void ReadDialogElements( WindowInputDialog * pDialog, HWND hWnd ) {
	int id=1000;
	for( auto it = pDialog->m_pImpl->items.begin(); it != pDialog->m_pImpl->items.end(); ++it, ++id ) {
		char buf[128];
		SendDlgItemMessageA( hWnd, id, WM_GETTEXT, 128, (LPARAM)buf );
		if( it->i ) {
			*it->i = atol( buf );
		} else if( it->f ) {
			*it->f = (float)atof( buf );
		} else if( it->pStr ) {
			*it->pStr = buf;
		}
	}
}

static LRESULT CALLBACK InputDialogProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) {
	WindowInputDialog * pDialog = s_wid[hWnd];
	switch( message ) {
		case WM_CREATE:
			CreateDialogElements( hWnd );
			break;
		case WM_COMMAND:
			switch( LOWORD(wParam) ) {
				case IDCANCEL:
				case IDOK: {
					if( HIWORD(wParam) == BN_CLICKED ) {
						if( LOWORD(wParam) == IDOK ) {
							ReadDialogElements( pDialog, hWnd );
							pDialog->OnOK();
						}
						pDialog->m_pImpl->items.clear();
						EnableWindow( g_hWnd, TRUE );
						DestroyWindow( hWnd );
						UnregisterClass( L"NanoCoreWindowInputDialog", g_hInstance );
						UpdateWindow( g_hWnd );
						ShowWindow( g_hWnd, SW_SHOW );
						s_wid.erase( s_wid.find( hWnd ));
						return 0;
					}
					break;
				}
				break;
			}
			break;
		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
	}
	return 0;
}

void WindowInputDialog::Show( const wchar_t * pCaption )
{
	int w = 800, h = (int)m_pImpl->items.size() * 20 + 40 + 150;

	WNDCLASSEXW wcex = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, InputDialogProc, 0, 0, g_hInstance, 0, 0, (HBRUSH)(COLOR_WINDOW+1), 0, L"NanoCoreWindowInputDialog", 0 };
	RegisterClassEx( &wcex );

	EnableWindow( g_hWnd, FALSE );

	s_pWid = this;

	HWND hWnd = CreateWindow( L"NanoCoreWindowInputDialog", pCaption, WS_OVERLAPPED | WS_CAPTION, CW_USEDEFAULT, CW_USEDEFAULT, w, h, g_hWnd, NULL, g_hInstance, NULL );
	s_wid[hWnd] = this;
	InvalidateRect( hWnd, NULL, TRUE );
	UpdateWindow( hWnd );
	ShowWindow( hWnd, SW_SHOW );
}

} //NanoCore

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	g_hInstance = hInstance;
	s_CmdShow = nCmdShow;

	return Main();
}