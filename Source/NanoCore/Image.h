#ifndef __INC_NANOCORE_IMAGE
#define __INC_NANOCORE_IMAGE

#include "Common.h"



namespace NanoCore {

class Image {
public:
	typedef RefCountPtr<Image> Ptr;

	Image();
	Image( int w, int h, int bpp );
	Image( const Image & img );
	~Image() { delete[] m_pBuffer; }

	void Init( int w, int h, int bpp );
	void Fill( uint32 color );
	void Clear();
	void BilinearFilterRect( int x, int y, int w, int h );

	void Average( const Image & img );

	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }
	int GetBpp() const { return m_bpp; }
	int GetSize() const { return m_width*m_height*(m_bpp/8); }

	const uint8 * GetImageAt( int x, int y ) const;
	uint8 * GetImageAt( int x, int y );

	void GetPixel( float u, float v, int * pix ) const;
	void GetPixel( int x, int y, int * pix ) const;
	void SetPixel( int x, int y, int * pix );

	bool Load( const wchar_t * name );
	int  WriteAsBMP( const wchar_t * name );

private:
	int     m_width,m_height,m_bpp;
	uint8 * m_pBuffer;
};

}

#endif