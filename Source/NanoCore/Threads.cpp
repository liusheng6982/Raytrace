#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "Threads.h"



struct ncThread::Impl
{
	HANDLE hThread;
	void * params;
	uint32 id;

	Impl() : hThread(0), params(0), id(0) {}
};

ncThread::ncThread() {
	m_pImpl = new Impl();
}

ncThread::~ncThread() {
	if( m_pImpl->hThread )
		Terminate();
	delete m_pImpl;
}

static DWORD WINAPI StartThreadProc( void * params )
{
	ncThread * pThread = (ncThread*)params;
	pThread->Run( pThread->m_pImpl->params );
	pThread->m_pImpl->hThread = NULL;
	return 0;
}

bool ncThread::Start( void * params )
{
	if( m_pImpl->hThread )
		return false;

	m_pImpl->params = params;
	m_pImpl->hThread = CreateThread( NULL, 0, StartThreadProc, this, 0, &m_pImpl->id );
	return true;
}

bool ncThread::IsRunning()
{
	return m_pImpl->hThread != NULL;
}

void ncThread::Terminate()
{
	if( m_pImpl->hThread ) {
		TerminateThread( m_pImpl->hThread, 0 );
		m_pImpl->hThread = NULL;
		OnTerminate();
	}
}

uint32 ncThread::GetId()
{
	return m_pImpl->id;
}

void ncThread::Wait()
{
	if( m_pImpl->hThread && GetId() != ncGetCurrentThreadId() )
		WaitForSingleObject( m_pImpl->hThread, 0 );
}




struct ncCriticalSection
{
	CRITICAL_SECTION cs;
	uint64 wait_ticks;

	~ncCriticalSection() {
		DeleteCriticalSection( &cs );
	}
};

ncCriticalSection * ncCriticalSectionCreate()
{
	ncCriticalSection * p = new ncCriticalSection();
	InitializeCriticalSection( &p->cs );
	p->wait_ticks = 0;
	return p;
}

void ncCriticalSectionDelete( ncCriticalSection * cs )
{
	if( cs )
		delete cs;
}

void ncCriticalSectionEnter( ncCriticalSection * cs )
{
	if( cs ) {
		uint64 t0 = ncGetTicks();
		EnterCriticalSection( &cs->cs );
		cs->wait_ticks += ncGetTicks() - t0;
	}
}

void ncCriticalSectionLeave( ncCriticalSection * cs )
{
	if( cs )
		LeaveCriticalSection( &cs->cs );
}

uint64 ncCriticalSectionGetWaitTicks( ncCriticalSection * cs )
{
	return cs ? cs->wait_ticks : 0;
}



void ncSleep( int ms )
{
	Sleep( ms );
}

uint64 ncGetTicks()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter( &li );
	return li.QuadPart;
}

uint64 ncTickToMicroseconds( uint64 ticks )
{
	LARGE_INTEGER li;
	QueryPerformanceFrequency( &li );
	return ticks * 1000000 / li.QuadPart;
}

void ncDebugOutput( const char * pcFormat, ... )
{
	char buf[2048];
	va_list args;
	va_start( args, pcFormat );
	vsprintf_s( buf, sizeof(buf), pcFormat, args );
	va_end( args );
	OutputDebugStringA( buf );
}

uint32 ncGetCurrentThreadId()
{
	return GetCurrentThreadId();
}

void ncGetSystemInfo( ncSystemInfo * pInfo )
{
	if( !pInfo ) return;
	SYSTEM_INFO si;
	GetSystemInfo( &si );
	pInfo->ProcessorCount = si.dwNumberOfProcessors;
}

std::wstring ncGetCurrentFolder()
{
	wchar_t wPath[MAX_PATH];
	GetCurrentDirectory( MAX_PATH, wPath );
	return std::wstring(wPath);
}