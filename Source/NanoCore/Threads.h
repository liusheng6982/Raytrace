#ifndef __INC_NANOCORE_THREADS
#define __INC_NANOCORE_THREADS

#include <string>
#include "Common.h"



namespace NanoCore {

struct CriticalSection;
CriticalSection * CriticalSectionCreate();
void CriticalSectionDelete( CriticalSection * cs );
void CriticalSectionEnter( CriticalSection * cs );
void CriticalSectionLeave( CriticalSection * cs );
uint64 CriticalSectionGetWaitTicks( CriticalSection * cs );

class Thread {
public:
	Thread();
	virtual ~Thread();

	bool   Start( void * params );
	bool   IsRunning();
	void   Wait();
	void   Terminate();
	uint32 GetId();

	struct Impl;
	Impl * m_pImpl;

	virtual void Run( void * params ) = 0;
	virtual void OnTerminate() {}
};

struct CriticalSectionScope {
	CriticalSectionScope( CriticalSection * cs ) : m_pCS(cs) {
		CriticalSectionEnter( cs );
	}
	~CriticalSectionScope() {
		CriticalSectionLeave( m_pCS );
	}
private:
	CriticalSection * m_pCS;
};

struct SystemInfo
{
	int ProcessorCount;
};

void   Sleep( int ms );
uint64 GetTicks();
uint64 TickToMicroseconds( uint64 ticks );
void   DebugOutput( const char * pcFormat, ... );
void   GetSystemInfo( SystemInfo * pInfo );
uint32 GetCurrentThreadId();
std::wstring GetCurrentFolder();

}
#endif