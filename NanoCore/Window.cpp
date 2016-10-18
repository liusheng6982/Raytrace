#include <Windows.h>
#include "Window.h"

static LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	



}

ncWindow::ncWindow( int w, int h )
{
	WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance, 0, 0, (HBRUSH)(COLOR_WINDOW+1), 0, szWindowClass, 0 };
	wcex.hIcon          = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_APPLICATION) );
	wcex.hCursor        = LoadCursor( NULL, IDC_ARROW );
	wcex.hIconSm        = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_APPLICATION) );
	if( ! RegisterClassEx( &wcex ))
		return 1;

	s_hInstance = hInstance;
	s_hWnd = CreateWindow( szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 820, 650, NULL, NULL, hInstance, NULL );
	if( !s_hWnd )
		return 1;

	ShowWindow( s_hWnd, nCmdShow );
	UpdateWindow( s_hWnd );	
}