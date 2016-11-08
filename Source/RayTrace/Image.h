#ifndef __INC_RAYTRACE_IMAGE
#define __INC_RAYTRACE_IMAGE

#include <NanoCore/Common.h>

class Image
{
public:
	Image();
	Image( int w, int h, int bpp );
	Image( const Image & img );
	~Image() { delete[] m_pBuffer; }



	void Init( int w, int h, int bpp );
	void Fill( uint32 color );
	void Clear();
	void BilinearFilterRect( int x, int y, int w, int h );

	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }
	int GetBitsPerPixel() const { return m_bpp; }

	const uint8 * GetImageAt( int x, int y ) const;
	uint8 * GetImageAt( int x, int y );

	void    GetPixel( int x, int y, int * pix ) const;
	void    SetPixel( int x, int y, int * pix );

	bool Load( const wchar_t * name );
	int  WriteAsBMP( const wchar_t * name );

private:
	int     m_width,m_height,m_bpp;
	uint8 * m_pBuffer;
};

#endif