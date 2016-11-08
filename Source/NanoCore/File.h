#ifndef ___INC_NANOCORE_FILE
#define ___INC_NANOCORE_FILE

#include "Common.h"
#include "String.h"

namespace NanoCore {

class IFile {
public:
	virtual ~IFile() {}
	virtual uint32 Read( void * ptr, uint32 size ) = 0;
	virtual uint32 Write( void * ptr, uint32 size ) = 0;
	virtual uint64 Tell() = 0;
	virtual void   Seek( uint64 pos ) = 0;
	virtual uint64 GetSize() = 0;
	virtual const wchar_t * GetName() = 0;
	virtual int GetOpenMode() = 0;
};

class TextFile {
public:
	TextFile( IFile * file );
	virtual ~TextFile();

	bool   EndOfFile();
	uint32 ReadLine( char * buf, int maxSize );
	uint32 ReadLine( std::string & line );
	uint32 Write( const char * fmt, ... );
	int    GetOpenMode();

private:
	IFile * m_pFile;
	char  * m_pBuffer;
	int     m_BufferSize, m_Position;
};

namespace FS {
	enum {
		efRead  = 1 << 0,
		efWrite = 1 << 1,
		efTrunc = 1 << 2,
	};

	IFile * Open( const wchar_t * name, int mode );
	IFile * Open( const char * name, int mode );
}

}
#endif