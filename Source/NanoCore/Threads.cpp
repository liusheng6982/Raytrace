#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "Threads.h"



namespace NanoCore {



struct Thread::Impl {
	volatile HANDLE hThread;
	volatile bool bRunning;

	void * params;
	uint32 id;
	HANDLE hSuspendEvent;
	std::wstring name;

	Impl() : hThread(0), params(0), id(0), bRunning(false), hSuspendEvent(NULL) {}
};

Thread::Thread() {
	m_pImpl = new Impl();
}

Thread::~Thread() {
	if( m_pImpl->hThread )
		Terminate();
	delete m_pImpl;
}

static DWORD WINAPI StartThreadProc( void * params ) {
	Thread * pThread = (Thread*)params;
	pThread->m_pImpl->bRunning = true;
	pThread->Run( pThread->m_pImpl->params );
	pThread->m_pImpl->hThread = NULL;
	pThread->m_pImpl->bRunning = false;
	return 0;
}

bool Thread::Start( void * params ) {
	if( m_pImpl->hThread )
		return false;

	m_pImpl->hSuspendEvent = ::CreateEvent( NULL, FALSE, FALSE, L"ThreadSuspendEvent" );
	m_pImpl->params = params;
	m_pImpl->hThread = ::CreateThread( NULL, 0, StartThreadProc, this, 0, &m_pImpl->id );
	return true;
}

bool Thread::IsRunning() {
	return m_pImpl->hThread != NULL && m_pImpl->bRunning;
}

void Thread::Terminate() {
	if( m_pImpl->hThread ) {
		::TerminateThread( m_pImpl->hThread, 0 );
		m_pImpl->hThread = NULL;
		m_pImpl->bRunning = false;
		if( m_pImpl->hSuspendEvent ) {
			::CloseHandle( m_pImpl->hSuspendEvent );
			m_pImpl->hSuspendEvent = NULL;
		}
		OnTerminate();
	}
}

uint32 Thread::GetId() {
	return m_pImpl->id;
}

void Thread::Wait() {
	if( m_pImpl->hThread && GetId() != GetCurrentThreadId() && m_pImpl->bRunning )
		::WaitForSingleObject( m_pImpl->hThread, 0 );
}

void Thread::Suspend() {
	if( m_pImpl->hThread && GetId() == GetCurrentThreadId()) {
		m_pImpl->bRunning = false;
		::WaitForSingleObject( m_pImpl->hSuspendEvent, INFINITE );
	}
}

void Thread::Resume() {
	//assert( GetId() != GetCurrentThreadId() );
	if( m_pImpl->hThread && !m_pImpl->bRunning ) {
		::SetEvent( m_pImpl->hSuspendEvent );
		m_pImpl->bRunning = true;
	}
}

void Thread::SetName( const wchar_t * name ) {
	m_pImpl->name = name;
}

const wchar_t * Thread::GetName() const {
	return m_pImpl->name.c_str();
}



struct CriticalSection::Impl {
	CRITICAL_SECTION cs;
	uint64 min_ticks, max_ticks, total_ticks, num_calls;
	Impl() {
		::InitializeCriticalSection( &cs );
		min_ticks = max_ticks = total_ticks = num_calls = 0;
	}
	~Impl() {
		::DeleteCriticalSection( &cs );
	}
};

CriticalSection::CriticalSection() {
	m_pImpl = new Impl();
}

CriticalSection::~CriticalSection() {
	delete m_pImpl;
}

void CriticalSection::Enter() {
	uint64 t = GetTicks();
	::EnterCriticalSection( &m_pImpl->cs );
	t = GetTicks() - t;

	m_pImpl->total_ticks += t;
	if( m_pImpl->max_ticks < t ) m_pImpl->max_ticks = t;
	if( m_pImpl->min_ticks > t || !m_pImpl->num_calls ) m_pImpl->min_ticks = t;
	m_pImpl->num_calls++;
}

void CriticalSection::Leave() {
	::LeaveCriticalSection( &m_pImpl->cs );
}

uint64 CriticalSection::GetWaitTicks( EWaitTickType type ) {
	switch( type ) {
		case eWaitTick_Min: return m_pImpl->min_ticks;
		case eWaitTick_Max: return m_pImpl->max_ticks;
		case eWaitTick_Total: return m_pImpl->total_ticks;
		case eWaitTick_Average: return m_pImpl->num_calls ? m_pImpl->total_ticks / m_pImpl->num_calls : 0;
	}
	return 0;
}



void Sleep( int ms ) {
	::Sleep( ms );
}

uint64 GetTicks() {
	LARGE_INTEGER li;
	::QueryPerformanceCounter( &li );
	return li.QuadPart;
}

uint64 TickToMicroseconds( uint64 ticks ) {
	LARGE_INTEGER li;
	::QueryPerformanceFrequency( &li );
	return ticks * 1000000 / li.QuadPart;
}

void DebugOutput( const char * pcFormat, ... ) {
	char buf[2048];
	va_list args;
	va_start( args, pcFormat );
	vsprintf_s( buf, sizeof(buf), pcFormat, args );
	va_end( args );
	::OutputDebugStringA( buf );
}

uint32 GetCurrentThreadId() {
	return ::GetCurrentThreadId();
}

void GetSystemInfo( SystemInfo * pInfo ) {
	if( !pInfo ) return;
	SYSTEM_INFO si;
	::GetSystemInfo( &si );
	pInfo->ProcessorCount = si.dwNumberOfProcessors;
}

std::wstring GetCurrentFolder() {
	wchar_t wPath[MAX_PATH];
	::GetCurrentDirectory( MAX_PATH, wPath );
	return std::wstring(wPath);
}

}