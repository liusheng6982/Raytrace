#ifndef __INC_NANOCORE_THREADS
#define __INC_NANOCORE_THREADS

#include <string>
#include "Common.h"



struct ncCriticalSection;
ncCriticalSection * ncCriticalSectionCreate();
void ncCriticalSectionDelete( ncCriticalSection * cs );
void ncCriticalSectionEnter( ncCriticalSection * cs );
void ncCriticalSectionLeave( ncCriticalSection * cs );
uint64 ncCriticalSectionGetWaitTicks( ncCriticalSection * cs );

class ncThread
{
public:
	ncThread();
	virtual ~ncThread();

	bool   Start( void * params );
	bool   IsRunning();
	void   Wait();
	void   Terminate();
	uint32 GetId();

	struct Impl;
	Impl * m_pImpl;

	virtual void Run( void * params ) = 0;
};

struct ncCriticalSectionScope
{
	ncCriticalSectionScope( ncCriticalSection * cs ) : m_pCS(cs) {
		ncCriticalSectionEnter( cs );
	}
	~ncCriticalSectionScope() {
		ncCriticalSectionLeave( m_pCS );
	}
private:
	ncCriticalSection * m_pCS;
};

struct ncSystemInfo
{
	int ProcessorCount;
};

void   ncSleep( int ms );
uint64 ncGetTicks();
uint64 ncTickToMicroseconds( uint64 ticks );
void   ncDebugOutput( const char * pcFormat, ... );
void   ncGetSystemInfo( ncSystemInfo * pInfo );
uint32 ncGetCurrentThreadId();
std::wstring ncGetCurrentFolder();


#endif