#include "InputDialog.h"
#include <windows.h>

#pragma optimize( "", off )

extern HINSTANCE g_hInstance;
extern HWND g_hWnd;

namespace NanoCore {

struct Item {
	std::wstring name;
	std::string * pStr;
	float * f;
	int * i;
	std::vector<std::wstring> combo;
	HWND hWnd;

	Item():pStr(NULL),f(NULL),i(NULL){}
};

static std::vector<Item> items;
static HWND hWnd;

void InputDialog::Add( const wchar_t * name, int & i )
{
	Item it;
	it.name = name;
	it.i = &i;
	items.push_back( it );
}

static LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message )
	{
	case WM_CREATE: {
		int y = 10, i = 1;
		for( auto it = items.begin(); it != items.end(); ++it, ++i ) {
			char buf[64];
			if( it->pStr )
				strcpy( buf, it->pStr->c_str() );
			else if( it->f )
				sprintf( buf, "%0.4f", *it->f );
			else if( it->i )
				sprintf( buf, "%d", *it->i );

			CreateWindow( L"STATIC", it->name.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT, 10, y, 120, 20, hWnd, 0, g_hInstance, NULL );
			it->hWnd = CreateWindowA( "EDIT", buf, WS_CHILD | WS_VISIBLE, 130, y, 50, 20, hWnd, (HMENU)i, g_hInstance, NULL );
			y += 20;
		}
		CreateWindow( L"BUTTON", L"Ok", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, y, 100, 20, hWnd, (HMENU)IDOK, g_hInstance, NULL );
		CreateWindow( L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 120, y, 100, 20, hWnd, (HMENU)IDCANCEL, g_hInstance, NULL );
		break;
	}
	case WM_COMMAND:
		switch( LOWORD(wParam)) {
			case IDOK: {

				if( HIWORD(wParam) == BN_CLICKED ) {

					int i=1;
					for( auto it = items.begin(); it != items.end(); ++it, ++i ) {
						char buf[128];
						SendDlgItemMessageA( hWnd, i, WM_GETTEXT, 128, (LPARAM)buf );
						if( it->i ) {
							*it->i = atol( buf );
						}
					}
					items.clear();
					DestroyWindow( hWnd );
					UnregisterClass( L"NanoCoreInputDialog", g_hInstance );
					return 0;
				}
				break;
			}
			case IDCANCEL:
				if( HIWORD(wParam) == BN_CLICKED ) {
					items.clear();
					DestroyWindow( hWnd );
					UnregisterClass( L"NanoCoreInputDialog", g_hInstance );
					return 0;
				}
				break;
		}
	default:
		return DefWindowProc( hWnd, message, wParam, lParam );
	}
	return 0;
}

void InputDialog::Run( const wchar_t * pCaption )
{
	int w = 800, h = items.size() * 20 + 40 + 50;

	WNDCLASSEXW wcex = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, g_hInstance, 0, 0, (HBRUSH)(COLOR_WINDOW+1), 0, L"NanoCoreInputDialog", 0 };
	RegisterClassEx( &wcex );

	hWnd = CreateWindow( L"NanoCoreInputDialog", pCaption, WS_OVERLAPPEDWINDOW, 100, 100, w, h, g_hWnd, NULL, g_hInstance, NULL );
	ShowWindow( hWnd, SW_SHOW );
}


















}