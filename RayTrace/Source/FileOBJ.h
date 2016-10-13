#ifndef __INC_RAYTRACE_FILEOBJ
#define __INC_RAYTRACE_FILEOBJ

#include "Common.h"

class FileOBJ
{
public:
	FileOBJ();
	~FileOBJ();

	bool Load( const char * pcFilename );
	int  GetLoadingProgress() const { return m_Progress; }

	int GetNumTriangles() const { return m_numTris; }
	const Triangle * GetTriangles() const { return m_pTris; }

private:
	Triangle * m_pTris;
	int        m_numTris;
	int        m_Progress;
};

#endif