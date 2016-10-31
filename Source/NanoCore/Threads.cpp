#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "Threads.h"

namespace NanoCore {

struct Thread::Impl
{
	HANDLE hThread;
	void * params;
	uint32 id;

	Impl() : hThread(0), params(0), id(0) {}
};

Thread::Thread() {
	m_pImpl = new Impl();
}

Thread::~Thread() {
	if( m_pImpl->hThread )
		Terminate();
	delete m_pImpl;
}

static DWORD WINAPI StartThreadProc( void * params )
{
	Thread * pThread = (Thread*)params;
	pThread->Run( pThread->m_pImpl->params );
	pThread->m_pImpl->hThread = NULL;
	return 0;
}

bool Thread::Start( void * params )
{
	if( m_pImpl->hThread )
		return false;

	m_pImpl->params = params;
	m_pImpl->hThread = CreateThread( NULL, 0, StartThreadProc, this, 0, &m_pImpl->id );
	return true;
}

bool Thread::IsRunning()
{
	return m_pImpl->hThread != NULL;
}

void Thread::Terminate()
{
	if( m_pImpl->hThread ) {
		TerminateThread( m_pImpl->hThread, 0 );
		m_pImpl->hThread = NULL;
		OnTerminate();
	}
}

uint32 Thread::GetId()
{
	return m_pImpl->id;
}

void Thread::Wait()
{
	if( m_pImpl->hThread && GetId() != GetCurrentThreadId() )
		WaitForSingleObject( m_pImpl->hThread, 0 );
}




struct CriticalSection
{
	CRITICAL_SECTION cs;
	uint64 wait_ticks;

	~CriticalSection() {
		DeleteCriticalSection( &cs );
	}
};

CriticalSection * CriticalSectionCreate()
{
	CriticalSection * p = new CriticalSection();
	InitializeCriticalSection( &p->cs );
	p->wait_ticks = 0;
	return p;
}

void CriticalSectionDelete( CriticalSection * cs )
{
	if( cs )
		delete cs;
}

void CriticalSectionEnter( CriticalSection * cs )
{
	if( cs ) {
		uint64 t0 = GetTicks();
		EnterCriticalSection( &cs->cs );
		cs->wait_ticks += GetTicks() - t0;
	}
}

void CriticalSectionLeave( CriticalSection * cs )
{
	if( cs )
		LeaveCriticalSection( &cs->cs );
}

uint64 CriticalSectionGetWaitTicks( CriticalSection * cs )
{
	return cs ? cs->wait_ticks : 0;
}



void Sleep( int ms )
{
	::Sleep( ms );
}

uint64 GetTicks()
{
	LARGE_INTEGER li;
	::QueryPerformanceCounter( &li );
	return li.QuadPart;
}

uint64 TickToMicroseconds( uint64 ticks )
{
	LARGE_INTEGER li;
	::QueryPerformanceFrequency( &li );
	return ticks * 1000000 / li.QuadPart;
}

void DebugOutput( const char * pcFormat, ... )
{
	char buf[2048];
	va_list args;
	va_start( args, pcFormat );
	vsprintf_s( buf, sizeof(buf), pcFormat, args );
	va_end( args );
	::OutputDebugStringA( buf );
}

uint32 GetCurrentThreadId()
{
	return ::GetCurrentThreadId();
}

void GetSystemInfo( SystemInfo * pInfo )
{
	if( !pInfo ) return;
	SYSTEM_INFO si;
	::GetSystemInfo( &si );
	pInfo->ProcessorCount = si.dwNumberOfProcessors;
}

std::wstring GetCurrentFolder()
{
	wchar_t wPath[MAX_PATH];
	::GetCurrentDirectory( MAX_PATH, wPath );
	return std::wstring(wPath);
}

}