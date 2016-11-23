#include <windows.h>
#include "Mathematics.h"
#include "File.h"
#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "3rdparty/stb/stb_image.h"



namespace NanoCore {

Image::Image() : m_pBuffer(NULL), m_width(0), m_height(0), m_bpp(0) {}

Image::Image( int w, int h, int bpp ) : m_pBuffer(NULL) {
	Init( w, h, bpp );
}

Image::Image( const Image & img ) : m_width(0), m_height(0), m_bpp(0), m_pBuffer(NULL) {
	Init( img.m_width, img.m_height, img.m_bpp );
	if( m_pBuffer )
		memcpy( m_pBuffer, img.m_pBuffer, GetWidth()*GetHeight()*(GetBpp()/8) );
}

void Image::Init( int w, int h, int bpp ) {
	if( bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32 )
		return;
	delete[] m_pBuffer;
	m_width = w;
	m_height = h;
	m_bpp = bpp;
	if( w && h && bpp )
		m_pBuffer = new uint8[w*h*bpp/8];
	else
		m_pBuffer = NULL;
}

void Image::Fill( uint32 color ) {
	if( m_pBuffer ) {
		uint8 rgb[4] = { color&0xFF, (color>>8)&0xFF, (color>>16)&0xFF, (color>>24) };
		for( int i=0, n=m_width*m_height; i<n; ++i ) {
			switch( m_bpp ) {
				case 8:
					m_pBuffer[i] = rgb[0]; break;
				case 16:
					m_pBuffer[i*2] = rgb[0]; m_pBuffer[i*2+1] = rgb[1]; break;
				case 24:
					m_pBuffer[i*3] = rgb[0]; m_pBuffer[i*3+1] = rgb[1]; m_pBuffer[i*3+2] = rgb[2]; break;
				case 32:
					((uint32*)(m_pBuffer + i*4))[0] = color; break;
			}
		}
	}
}

int Image::WriteAsBMP( const wchar_t * name ) {
	NanoCore::IFile::Ptr fp = NanoCore::FS::Open( name, NanoCore::FS::efWrite | NanoCore::FS::efTrunc );
	if( !fp )
		return 0;

	char sig[2] = {'B','M'};
	fp->Write( sig, 2 );

	int pw = m_width * (m_bpp/8);
	if( pw % 4 ) pw += 4 - pw%4;

	int hdr[] = { 14 + 40 + pw*m_height, 0, 14+40 };
	fp->Write( hdr, sizeof(hdr) );

	BITMAPINFOHEADER bmpih = { 40, m_width, m_height, 1, GetBpp(), 0, pw*m_height, 0, 0, 0, 0 };
	fp->Write( &bmpih, 40 );

	uint32 pad = 0;
	uint32 rowSize = GetWidth() * (GetBpp()/8);

	for( int i=0; i<m_height; ++i ) {
		fp->Write( m_pBuffer + i*rowSize, rowSize );
		fp->Write( &pad, pw - rowSize );
	}
	return 1;
}

const uint8 * Image::GetImageAt( int x, int y ) const {
	return m_pBuffer ? m_pBuffer + (x + y*m_width)*m_bpp/8 : NULL;
}

uint8 * Image::GetImageAt( int x, int y ) {
	return m_pBuffer ? m_pBuffer + (x + y*m_width)*m_bpp/8 : NULL;
}

void Image::GetPixel( int x, int y, int * out ) const {
	const uint8 * p = GetImageAt( x, y );
	out[0] = p[0];
	if( m_bpp > 8 ) {
		out[1] = p[1];
		if( m_bpp > 16 ) {
			out[2] = p[2];
			if( m_bpp > 24 )
				out[3] = p[3];
		}
	}
}

void Image::SetPixel( int x, int y, int * pix ) {
	uint8 * p = GetImageAt( x, y );
	switch( m_bpp ) {
		case 8: p[0] = pix[0]; break;
		case 16: p[0] = pix[0]; p[1] = pix[1]; break;
		case 24: p[0] = pix[0]; p[1] = pix[1]; p[2] = pix[2]; break;
		case 32: p[0] = pix[0]; p[1] = pix[1]; p[2] = pix[2]; p[3] = pix[3]; break;
	}
}

void Image::GetPixel( float u, float v, int * pix ) const {
	u -= ncFloor( u );
	v -= ncFloor( v );

	int rw = GetWidth()-1, rh = GetHeight()-1;

	u *= rw;
	v *= rh;

	int x = Clamp( int(u), 0, rw ), y = Clamp( int(v), 0, rh );
	GetPixel( x, y, pix );
}

static void Interpolate( int * p0, int * p1, int coef1kRange, int * result ) {
	result[0] = (p0[0]*(1024-coef1kRange) + p1[0]*coef1kRange) >> 10;
	result[1] = (p0[1]*(1024-coef1kRange) + p1[1]*coef1kRange) >> 10;
	result[2] = (p0[2]*(1024-coef1kRange) + p1[2]*coef1kRange) >> 10;
}

void Image::BilinearFilterRect( int x, int y, int w, int h ) {
	if( m_bpp != 24 )
		return;

	int p00[3], p01[3], p10[3], p11[3];

	GetPixel( x, y, p00 );
	GetPixel( x+w-1, y, p10 );
	GetPixel( x, y+h-1, p01 );
	GetPixel( x+w-1, y+h-1, p11 );

	for( int yi = y; yi < y+h; ++yi )
	{
		int p0[3], p1[3];
		Interpolate( p00, p01, (yi-y)*1024 / h, p0 );
		Interpolate( p10, p11, (yi-y)*1024 / h, p1 );

		if( yi > y && yi < y+h-1 ) {
			SetPixel( x, yi, p0 );
			SetPixel( x+w-1, yi, p1 );
		}
		for( int xi = 1; xi < w-1; ++xi ) {
			int pix[3];
			Interpolate( p0, p1, xi*1024/w, pix );
			SetPixel( x + xi, yi, pix );
		}
	}
}

bool Image::Load( const wchar_t * name ) {
	Clear();

	int n;
	std::string file = StrWcsToMbs( name );
	uint8 * data = stbi_load( file.c_str(), &m_width, &m_height, &n, 0 );

	if( data ) {
		m_bpp = n*8;
		m_pBuffer = new uint8[m_width * m_height * n];
		memcpy( m_pBuffer, data, m_width*m_height*n );
		stbi_image_free( data );

		return true;
	}
	return false;
}

void Image::Clear() {
	delete[] m_pBuffer;
	m_pBuffer = NULL;
	m_width = m_height = m_bpp = 0;
}

void Image::Average( const Image & img ) {
	if( GetWidth() > img.GetWidth() || GetHeight() > img.GetHeight() || m_bpp != 24 || img.GetBpp() != 24 )
		return;

	int w = img.GetWidth(), h = img.GetHeight();

	for( int y=0; y<m_height; ++y ) {
		for( int x=0; x<m_width; ++x ) {
			int x1 = x * w / m_width;
			int y1 = y * h / m_height;
			int x2 = (x+1) * w / m_width;
			int y2 = (y+1) * h / m_height;

			int pix[3] = {0};
			int count = 0;
			for( int i=y1; i<y2; ++i ) {
				const uint8 * ptr = img.GetImageAt( x1, i );
				for( int j=0; j<x2-x1; ++j, ptr += 3 ) {
					if( ptr[0] || ptr[1] || ptr[2] ) {
						pix[0] += ptr[0];
						pix[1] += ptr[1];
						pix[2] += ptr[2];
						count++;
					}
				}
			}
			if( count ) {
				pix[0] /= count;
				pix[1] /= count;
				pix[2] /= count;
			}
			SetPixel( x, y, pix );
		}
	}
}

void Image::Stretch( const Image & img ) {
	if( GetBpp() != img.GetBpp())
		return;
	int w = img.GetWidth(), h = img.GetHeight();

	for( int y=0; y<m_height; ++y ) {
		for( int x=0; x<m_width; ++x ) {
			int x1 = x * w / m_width;
			int y1 = y * h / m_height;
			int x2 = (x+1) * w / m_width;
			int y2 = (y+1) * h / m_height;

			if( x1 == x2 ) x2++;
			if( y1 == y2 ) y2++;

			int pix[4] = {0};
			for( int i=y1; i<y2; ++i ) {
				const uint8 * ptr = img.GetImageAt( x1, i );
				int step = m_bpp/8;
				for( int j=0; j<x2-x1; ++j, ptr += step ) {
					switch( m_bpp ) {
						case 32: pix[3] += ptr[3];
						case 24: pix[2] += ptr[2];
						case 16: pix[1] += ptr[1];
						case  8: pix[0] += ptr[0];
					}
				}
			}
			int div = (x2-x1)*(y2-y1);
			pix[0] /= div;
			pix[1] /= div;
			pix[2] /= div;
			pix[3] /= div;
			SetPixel( x, y, pix );
		}
	}
}

}