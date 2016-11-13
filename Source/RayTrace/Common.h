#ifndef __INC_RAYTRACE_COMMON
#define __INC_RAYTRACE_COMMON

#include <NanoCore/Common.h>
#include <NanoCore/Mathematics.h>
#include <NanoCore/File.h>

class IStatusCallback {
public:
	virtual ~IStatusCallback() {}
	virtual void SetStatus( const char * pcFormat, ... ) = 0;

	void ShowLoadingProgress( const char * pcFormat, NanoCore::IFile::Ptr & pFile ) {
		SetStatus( pcFormat, int(pFile->Tell()*100 / pFile->GetSize()));
	}
};

#endif