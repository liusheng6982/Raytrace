#ifndef ___INC_NANOCORE_WINDOW
#define ___INC_NANOCORE_WINDOW

class ncWindow
{
public:
	ncWindow( int w, int h );
	virtual ~ncWindow() {}

	virtual void OnKey( int key ) {}
	virtual void OnMouse( int x, int y, int btn_state ) {}
	virtual void OnSize( int w, int h ) {}
	virtual void OnDraw() {}

	int GetWidth();
	int GetHeight();

	bool ChooseFile( const char * pcFolder, bool bLoad );
	void DrawImage( int x, int y, int w, int h, const uint32 * pImage, int iw, int ih );
	void DrawText( int x, int y, const char * pcText );
};

#endif