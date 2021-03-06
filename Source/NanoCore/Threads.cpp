#include <Windows.h>
#include <stdarg.h>
#include "Threads.h"
#include "String.h"


namespace NanoCore {



struct Thread::Impl {
	volatile HANDLE hThread;
	volatile bool bRunning;

	void * params;
	uint32 id;
	HANDLE hSuspendEvent;
	std::wstring name;

	uint64 idleTicks, workTicks, startTick;

	Impl() : hThread(0), params(0), id(0), bRunning(false), hSuspendEvent(NULL), idleTicks(0), workTicks(0), startTick(0) {}
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
	pThread->m_pImpl->startTick = GetTicks();
	pThread->Run( pThread->m_pImpl->params );
	pThread->m_pImpl->workTicks += GetTicks() - pThread->m_pImpl->startTick;
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
		uint64 t1 = GetTicks();
		m_pImpl->workTicks += t1 - m_pImpl->startTick;
		::WaitForSingleObject( m_pImpl->hSuspendEvent, INFINITE );
		uint64 t2 = GetTicks();
		m_pImpl->idleTicks += t2 - t1;
		m_pImpl->startTick = t2;
		m_pImpl->bRunning = true;
	}
}

void Thread::Resume() {
	//assert( GetId() != GetCurrentThreadId() );
	if( m_pImpl->hThread && !m_pImpl->bRunning ) {
		::SetEvent( m_pImpl->hSuspendEvent );
	}
}

void Thread::SetName( const wchar_t * name ) {
	m_pImpl->name = name;
}

const wchar_t * Thread::GetName() const {
	return m_pImpl->name.c_str();
}

uint64 Thread::GetIdleTicks() const {
	return m_pImpl->idleTicks;
}

uint64 Thread::GetWorkTicks() const {
	return m_pImpl->workTicks;
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
		case eMin: return m_pImpl->min_ticks;
		case eMax: return m_pImpl->max_ticks;
		case eTotal: return m_pImpl->total_ticks;
		case eAverage: return m_pImpl->num_calls ? m_pImpl->total_ticks / m_pImpl->num_calls : 0;
	}
	return 0;
}

void CriticalSection::ResetWaitTicks() {
	m_pImpl->min_ticks = m_pImpl->max_ticks = m_pImpl->total_ticks = 0;
	m_pImpl->num_calls = 0;
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

static CriticalSection csDebugOutput;

void DebugOutput( const char * pcFormat, ... ) {
	csScope cs( csDebugOutput );
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

std::wstring GetExecutableFolder() {
	CriticalSection cs;
	csScope scope( cs );

	static std::wstring folder;
	if( folder.empty()) {
		HMODULE hModule = ::GetModuleHandleW( NULL );
		wchar_t path[MAX_PATH];
		GetModuleFileNameW( hModule, path, MAX_PATH );
		folder = path;
		folder = StrGetPath( folder );
	}
	return folder;
}

int32 AtomicInc( volatile int32 * ptr ) {
	return ::InterlockedIncrement( ptr );
}

int32 AtomicDec( volatile int32 * ptr ) {
	return ::InterlockedDecrement( ptr );
}

int32 AtomicCompareAndSwap( volatile int32 * ptr, int32 compare, int32 swap ) {
	return ::InterlockedCompareExchange( ptr, swap, compare );
}

}