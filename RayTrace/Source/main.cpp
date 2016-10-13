#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <stdio.h>
#include "RayTrace.h"

#define SPONZA

#define OBJ_FILE "..\\data\\sponza\\sponza.obj"
//#define OBJ_FILE "..\\data\\teapot\\teapot.obj"
//#define OBJ_FILE "..\\data\\san-miguel.obj"

static TCHAR szWindowClass[] = _T("win32app");
static TCHAR szTitle[] = _T("RayTrace by Zemedelec");

static HINSTANCE  s_hInstance;
static HWND       s_hWnd;
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

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint( hWnd, &ps );
			static BITMAPINFO bmp = { { sizeof(BITMAPINFOHEADER), 0, 0, 1, 24, BI_RGB, 0, 0, 0, 0, 0 } };
			bmp.bmiHeader.biWidth = s_rt.render_width;
			bmp.bmiHeader.biHeight = s_rt.render_height;
			s_rt.GatherRenderBuffer();
			if( s_rt.pRenderBuffer ) {
				int ret = StretchDIBits( hdc, 0, 0, 800, 600, 0, 0, s_rt.render_width, s_rt.render_height, s_rt.pRenderBuffer, &bmp, DIB_RGB_COLORS, SRCCOPY );
			}
			TextOutA( hdc, 5, 5, s_pcInfo, strlen(s_pcInfo) );
			EndPaint( hWnd, &ps );
			break;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc( hWnd, message, wParam, lParam );
    }
    return 0;
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
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
	SetTimer( s_hWnd, 0, 50, 0 );

	s_State = STATE_LOAD;

    MSG msg;
    while( GetMessage( &msg, NULL, 0, 0 )) {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    return (int) msg.wParam;
}