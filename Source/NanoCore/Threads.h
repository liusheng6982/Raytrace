#ifndef __INC_NANOCORE_THREADS
#define __INC_NANOCORE_THREADS

#include <string>
#include "Common.h"



namespace NanoCore {



class CriticalSection {
public:
	CriticalSection();
	~CriticalSection();

	void Enter();
	void Leave();

	enum EWaitTickType {
		eWaitTick_Min,
		eWaitTick_Max,
		eWaitTick_Total,
		eWaitTick_Average
	};
	uint64 GetWaitTicks( EWaitTickType type );

private:
	void operator = ( const CriticalSection & other ) {}
	CriticalSection( CriticalSection & other ) {}

	struct Impl;
	Impl * m_pImpl;
};



class Thread {
public:
	Thread();
	virtual ~Thread();

	bool   Start( void * params = NULL );
	bool   IsRunning();
	void   Wait();
	void   Terminate();
	uint32 GetId();
	void   Resume();
	void   Suspend();

	void SetName( const wchar_t * name );
	const wchar_t * GetName() const;

	struct Impl;
	Impl * m_pImpl;

	virtual void Run( void * params ) = 0;
	virtual void OnTerminate() {}
};


struct csScope {
	csScope( CriticalSection & cs ) : cs(cs) {
		cs.Enter();
	}
	~csScope() {
		cs.Leave();
	}
private:
	CriticalSection & cs;
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

int32 AtomicInc( volatile int32 * ptr );
int32 AtomicDec( volatile int32 * ptr );

}
#endif