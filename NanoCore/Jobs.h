#ifndef __INC_NANOCORE_JOBS
#define __INC_NANOCORE_JOBS

#include "Threads.h"

#include <vector>
#include <deque>

using namespace std;

class IJobFrame;

class IJob
{
public:
	IJob( int type ) : m_pFrame( NULL ), m_type( type ) {}
	virtual ~IJob() {}
	virtual void Execute() = 0;

	int GetType() const { return m_type; }
	IJobFrame * GetFrame() const { return m_pFrame; }

	void SetType( int type );
	void SetFrame( IJobFrame * p );

private:
	int m_type;
	IJobFrame * m_pFrame;
};

class IJobFrame
{
public:
	virtual ~IJobFrame() {}

	virtual void AddJob( IJob * pJob, int typeToWait = -1 ) = 0;

	virtual void AddJob( IJob * pJob, int type, int typeToWait ) = 0;

	virtual void StartNewFrame( int frame ) = 0;
};

void        ncJobsInit( int numThreads );
void        ncJobsShutdown();
IJobFrame * ncJobsCreateFrame();
bool        ncJobsAreRunning();
void        ncJobsAdd( IJob * pJob, IJobFrame * pFrame, int type, int typeToWait = -1 );

#endif