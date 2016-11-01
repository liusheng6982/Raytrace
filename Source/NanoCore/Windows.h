#ifndef ___INC_NANOCORE_MAINWINDOW
#define ___INC_NANOCORE_MAINWINDOW

#include <string>
#include <vector>
#include "Common.h"

namespace NanoCore {

class WindowMain
{
public:
	WindowMain();
	virtual ~WindowMain();

	bool Init( int w, int h );
	bool Update();
	void SetCaption( const wchar_t * pwCaption );
	void Redraw();
	void Exit();

	virtual void OnKey( int key ) {}
	virtual void OnMouse( int x, int y, int btn_down, int btn_up, int btn_dblclick, int wheel ) {}
	virtual void OnSize( int w, int h ) {}
	virtual void OnDraw() {}
	virtual void OnUpdate() {}
	virtual void OnMenu( int id ) {}
	virtual void OnInit() {}
	virtual void OnQuit() {}

	int GetWidth();
	int GetHeight();

	std::wstring ChooseFile( const wchar_t * pwFolder, const wchar_t * pwFilter, const wchar_t * pwCaption, bool bLoad );
	void  DrawImage( int x, int y, int w, int h, const uint8 * pImage, int iw, int ih, int bpp );
	void  DrawText( int x, int y, const char * pcText );

	int CreateMenu();
	void AddSubmenu( int menu, const wchar_t * pcName, int submenu );
	void AddMenuItem( int menu, const wchar_t * pcName, int id );
};

class WindowInputDialog {
public:
	WindowInputDialog();
	virtual ~WindowInputDialog();

	void Show( const wchar_t * caption );

	struct Impl;
	Impl * m_pImpl;

	virtual void OnOK() {}

protected:
	void Add( const wchar_t * pName, std::string & str );
	void Add( const wchar_t * name, float & f );
	void Add( const wchar_t * name, int & i );
	void Add( const wchar_t * pName, std::vector<std::wstring> & combo, int & item );
};

}
#endif