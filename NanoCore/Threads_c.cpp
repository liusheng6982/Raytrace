#include <windows.h>

struct ncThread_t
{
	HANDLE hThread;
	ncThreadFunc func;
	void * params;
};

static DWORD WINAPI ThreadFunc( void * ptr )
{
	ncThread_t * p = (ncThread_t*)ptr;
	p->func( p->params );
	p->hThread = NULL;
	return 0;
}

ncThread_t * ncThreadStart( ncThreadFunc func, void * params )
{
	ncThread_t * p = new ncThread_t();
	p->func = func;
	p->params = params;
	p->hThread = CreateThread( NULL, 0, ThreadFunc, p, 0, NULL );
	return p;
}

bool ncThreadIsRunning( ncThread_t * p )
{
	return p && p->hThread != NULL;
}

void ncThreadWait( ncThread_t * p )
{
	if( p ) {
		HANDLE h = p->hThread;
		if( h )
			WaitForSingleObject( h, 0 );
	}
}

void ncThreadTerminate( ncThread_t * p )
{
	if( p ) {
		HANDLE h = p->hThread;
		TerminateThread( h );
		p->hThread = NULL;
	}
}

int ncThreadGetId( ncThread_t * p )
{
	if( p ) {
		HANDLE h = p->hThread;
		return GetThreadId( h );
	}
	return -1;
}

struct ncCriticalSection_t;

ncCriticalSection_t * ncCriticalSectionCreate();
void                  ncCriticalSectionDelete( ncCriticalSection_t * p );
void                  ncCriticalSectionEnter( ncCriticalSection_t * p );
void                  ncCriticalSectionLeave( ncCriticalSection_t * p );