#ifndef __INC_NANOCORE_THREADS_C
#define __INC_NANOCORE_THREADS_C

struct ncThread_t;

typedef void (*ncThreadFunc)( void * params );

ncThread_t * ncThreadStart( ncThreadFunc func, void * params );
bool         ncThreadIsRunning( ncThread_t * p );
void         ncThreadWait( ncThread_t * p );
void         ncThreadTerminate( ncThread_t * p );
int          ncThreadGetId( ncThread_t * p );

struct ncCriticalSection_t;

ncCriticalSection_t * ncCriticalSectionCreate();
void                  ncCriticalSectionDelete( ncCriticalSection_t * p );
void                  ncCriticalSectionEnter( ncCriticalSection_t * p );
void                  ncCriticalSectionLeave( ncCriticalSection_t * p );

class ncCriticalSectionScope {
	ncCriticalSection * cs;
public:
	ncCriticalSectionScope( ncCriticalSection_t * p ) : cs(p) {
		cs->Enter();
	}
	~ncCriticalSectionScope() {
		cs->Leave();
	}
};

#endif