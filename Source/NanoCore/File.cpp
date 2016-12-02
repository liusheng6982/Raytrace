#include <windows.h>
#include <stdarg.h>
#include <string>
#include "File.h"



namespace NanoCore {



class File : public IFile {
public:
	virtual ~File();

	virtual uint32 Read( void * ptr, uint32 size );
	virtual uint32 Write( void * ptr, uint32 size );
	virtual uint64 Tell();
	virtual void   Seek( uint64 pos );
	virtual uint64 GetSize();

	virtual const wchar_t * GetName() { return m_Name.c_str(); }
	virtual int GetOpenMode() { return m_OpenMode; }

	HANDLE m_hFile;
	std::wstring m_Name;
	int m_OpenMode;
};

static HANDLE CommonOpen( const wchar_t * name, int mode ) {
	DWORD dwAccess = ((mode & FS::efRead) ? GENERIC_READ : 0) + ((mode & FS::efWrite) ? GENERIC_WRITE : 0);
	DWORD dwSharing = 0;
	DWORD dwCreate = (mode & FS::efTrunc) ? CREATE_ALWAYS : OPEN_EXISTING;
	return ::CreateFile( name, dwAccess, dwSharing, NULL, dwCreate, 0, NULL );
}

IFile::Ptr FS::Open( const char * name, int mode ) {
	wchar_t wname[512];
	size_t conv;
	mbstowcs_s( &conv, wname, 512, name, strlen(name) );
	HANDLE h = CommonOpen( wname, mode );
	if( !h )
		return IFile::Ptr();
	File * f = new File();
	f->m_hFile = h;
	f->m_Name = wname;
	f->m_OpenMode= mode;
	return IFile::Ptr( f );
}

IFile::Ptr FS::Open( const wchar_t * name, int mode ) {
	HANDLE h = CommonOpen( name, mode );
	if( h == INVALID_HANDLE_VALUE )
		return IFile::Ptr();
	File * f = new File();
	f->m_hFile = h;
	f->m_Name = name;
	f->m_OpenMode= mode;
	return IFile::Ptr( f );
}

uint32 File::Read( void * buf, uint32 size ) {
	DWORD dwRead = 0;
	::ReadFile( m_hFile, buf, size, &dwRead, NULL );
	return dwRead;
}

uint32 File::Write( void * buf, uint32 size ) {
	DWORD dwWritten = 0;
	::WriteFile( m_hFile, buf, size, &dwWritten, NULL );
	return dwWritten;
}

uint64 File::Tell() {
	LONG hi = 0;
	uint32 low = ::SetFilePointer( m_hFile, 0, &hi, FILE_CURRENT );
	return uint64(low) + (uint64(hi) << 32U);
}

void File::Seek( uint64 pos ) {
	::SetFilePointer( m_hFile, (uint32)pos, ((LONG*)&pos) + 1, FILE_BEGIN );
}

uint64 File::GetSize() {
	uint64 size;
	uint32 lw = ::GetFileSize( m_hFile, ((DWORD*)&size) + 1 );
	size |= lw;
	return size;
}

File::~File() {
	::CloseHandle( m_hFile );
}



TextFile::TextFile( IFile::Ptr file ) : m_pFile(file) {
	m_pBuffer = new char[1024];
	m_BufferSize = m_Position = 1024;
	m_Ident = 0;
	m_bNewLine = true;
}

TextFile::~TextFile() {
	delete m_pBuffer;
}

bool TextFile::EndOfFile() {
	return m_BufferSize < 1024 && m_Position == m_BufferSize;
}

uint32 TextFile::ReadLine( char * buf, int maxSize ) {
	for( int i=0;; ++m_Position ) {
		if( m_Position == m_BufferSize ) {
			m_BufferSize = m_pFile->Read( m_pBuffer, 1024 );
			m_Position = 0;
			if( !m_BufferSize ) {
				buf[i] = 0;
				return i;
			}
		}
		if( m_pBuffer[m_Position] == '\n' || m_pBuffer[m_Position] == '\r' ) {
			if( i ) {
				buf[i] = 0;
				m_Position++;
				return i;
			}
		} else
			buf[i++] = m_pBuffer[m_Position];
	}
}

uint32 TextFile::ReadLine( std::string & line ) {
	line.clear();
	for( int i=0;; ++m_Position ) {
		if( m_Position == m_BufferSize ) {
			m_BufferSize = m_pFile->Read( m_pBuffer, 1024 );
			m_Position = 0;
			if( !m_BufferSize ) {
				return (uint32)line.size();
			}
		}
		if( m_pBuffer[m_Position] == '\n' || m_pBuffer[m_Position] == '\r' ) {
			if( i ) {
				m_Position++;
				return (uint32)line.size();
			}
		} else {
			line.push_back( m_pBuffer[m_Position] );
			i++;
		}
	}
	return 0;
}

uint32 TextFile::Write( const char * fmt, ... ) {
	char buf[2048];
	va_list args;
	va_start( args, fmt );
	int len = vsprintf_s( buf, fmt, args );
	va_end( args );

	int ident = m_Ident < 0 ? 0 : m_Ident;
	if( !ident )
		return (uint32)m_pFile->Write( buf, len );

	char buf2[2048];
	int  len2 = 0;
	for( int i=0; i<len; ++i ) {
		if( m_bNewLine ) {
			for( int k=0; k<ident; ++k )
				buf2[len2+k] = ' ';
			len2 += ident;
			m_bNewLine = false;
		}
		buf2[len2++] = buf[i];
		if( buf[i] == '\n' ) m_bNewLine = true;
	}
	buf2[len2] = 0;
	return (uint32)m_pFile->Write( buf2, len2 );
}

int TextFile::GetOpenMode() {
	return m_pFile->GetOpenMode();
}

uint64 TextFile::Tell() {
	return m_pFile->Tell();
}

uint64 TextFile::GetSize() {
	return m_pFile->GetSize();
}

int TextFile::ChangeIdentation( int delta ) {
	int curr = m_Ident;
	m_Ident += delta;
	return curr;
}

}