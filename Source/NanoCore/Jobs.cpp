#include "Jobs.h"
#include "Threads.h"
#include <vector>
#include <deque>

#define MAX_THREADS 32

//#define Log NanoCore::DebugOutput
#define Log



namespace NanoCore {



class WorkerThread : public Thread {
public:
	WorkerThread() : m_job(NULL) {}

	virtual void Run( void* );

private:
	IJob * m_job;
};




void   AddImmediateJobs( IJob ** p, int count );
IJob * GetSingleJob();
void   ClearPending();

static volatile int32 s_jobsCount;
static volatile bool  s_bEnableJobs = true;
static volatile int32 s_numJobs = 0;

static CriticalSection            s_csAvailable;
static std::deque<IJob*>          s_available;
static std::vector<WorkerThread*> s_threads;
static int                        s_maxTypes;

struct JobType {
	volatile int32 count;
	std::vector<IJob*> dependant_jobs;
	CriticalSection cs;
	JobType() : count(0) {}
};
static JobType * s_pJobTypes;



void IJob::SetType( int type ) {
	m_type = type;
}

void WorkerThread::Run( void* ) {
	Log( "Worker thread %ls started.\n", GetName() );
	for( ;; ) {
		IJob * pJob = GetSingleJob();
		if( !pJob ) {
			Log( "Worker thread %ls SUSPENDED.\n", GetName() );
			Suspend();
			Log( "Worker thread %ls RESUMED.\n", GetName() );
			continue;
		}
		m_job = pJob;
		pJob->Execute();

		AtomicInc( &s_numJobs );

		const int type = pJob->GetType();
		JobType * ptr = &s_pJobTypes[type];

		{
			int32 ret = AtomicDec( &ptr->count );
			if( ret == 0 ) {
				csScope cs( ptr->cs );
				const int jc = ptr->dependant_jobs.size();
				if( jc ) {
					AddImmediateJobs( &ptr->dependant_jobs[0], jc );
					ptr->dependant_jobs.clear();
				}
			}
		}
		AtomicDec( &s_jobsCount );
	}
}

void JobManager::Init( int numThreads, int maxTypes ) {
	if( s_pJobTypes )
		delete[] s_pJobTypes;
	s_pJobTypes = new JobType[maxTypes];
	s_maxTypes = maxTypes;
	s_bEnableJobs = true;

	if( numThreads <= 0 ) {
		SystemInfo si;
		GetSystemInfo( &si );
		numThreads = (numThreads == 0) ? si.ProcessorCount-1 : si.ProcessorCount;
	} else if( numThreads > MAX_THREADS ) {
		numThreads = MAX_THREADS;
		// warning
	}
	for( int i=0; i<numThreads; ++i ) {
		WorkerThread * p = new WorkerThread();

		wchar_t buf[64];
		swprintf_s( buf, L"jmThread %d", i );
		p->SetName( buf );

		s_threads.push_back( p );
		p->Start();
	}
}

void JobManager::Done() {
	for( size_t i=0; i<s_threads.size(); ++i )
		delete s_threads[i];
	s_threads.clear();
	delete[] s_pJobTypes;
	s_pJobTypes = NULL;
	s_available.clear();
}

void JobManager::AddJob( IJob * pJob, int typeToWait ) {
	if( !s_bEnableJobs )
		return;
	JobType * pJobType = s_pJobTypes + pJob->GetType();

	AtomicInc( &pJobType->count );
	AtomicInc( &s_jobsCount );

	if( typeToWait == -1 ) {
		AddImmediateJobs( &pJob, 1 );
	} else {
		csScope cs( s_pJobTypes[typeToWait].cs );
		if( s_pJobTypes[typeToWait].count == 0 )
			AddImmediateJobs( &pJob, 1 );
		else
			s_pJobTypes[typeToWait].dependant_jobs.push_back( pJob );
	}
}

bool JobManager::IsRunning() {
	return s_jobsCount > 0;
}

void AddImmediateJobs( IJob ** p, int count ) {
	csScope cs( s_csAvailable );
	for( int i=0; i<count; ++i )
		s_available.push_back( p[i] );
	for( int i=0, n=s_threads.size(); i<n; ++i )
		s_threads[i]->Resume();
}

IJob * GetSingleJob() {
	csScope cs( s_csAvailable );
	if( s_available.empty())
		return NULL;
	IJob * p = s_available.front();
	s_available.pop_front();
	return p;
}

void ClearPending() {
	for( int i=0; i<s_maxTypes; ++i ) {
		csScope cs( s_pJobTypes[i].cs );
		s_pJobTypes[i].dependant_jobs.clear();
		s_pJobTypes[i].count = 0;
	}
	csScope cs( s_csAvailable );
	s_available.clear();
	s_jobsCount = 0;
}

void JobManager::Wait( int flags ) {

	if( flags & efDisableJobAddition )
		s_bEnableJobs = false;
	if( flags & efClearPendingJobs )
		ClearPending();

	while( true ) {
		bool bRunning = false;
		for( int i=0,n=s_threads.size(); i<n; ++i )
			if( s_threads[i]->IsRunning()) bRunning = true;
		if( bRunning || s_jobsCount > 0 ) {
			Sleep( 1 );
		} else {
			break;
		}
	}
	for( int i=0; i<s_maxTypes; ++i ) {
		s_pJobTypes[i].count = 0;
		s_pJobTypes[i].dependant_jobs.clear();
	}
	s_jobsCount = 0;
	s_bEnableJobs = true;
}

int JobManager::GetNumThreads() {
	return (int)s_threads.size();
}

void JobManager::ResetStats() {

}

uint64 JobManager::GetStats( EStats stats ) {
	switch( stats ) {
		case eNumThreads: return s_threads.size();
		case eNumJobs: return s_numJobs;
		case eThreadIdleTime: {
			uint64 t=0;
			for( size_t i=0; i<s_threads.size(); ++i )
				t += s_threads[i]->GetIdleTicks();
			return t;
		}
		case eThreadWorkTime: {
			uint64 t=0;
			for( size_t i=0; i<s_threads.size(); ++i )
				t += s_threads[i]->GetWorkTicks();
			return t;
		}
		case eCriticalSectionsWaitTime: {
			uint64 t = s_csAvailable.GetWaitTicks( CriticalSection::eTotal );
			for( int i=0; i<s_maxTypes; ++i )
				t += s_pJobTypes[i].cs.GetWaitTicks( CriticalSection::eTotal );
			return t;
		}
	}
	return 0;
}

void JobManager::PrintStats() {
	DebugOutput( "Job manager CS wait: %ld us\n", TickToMicroseconds( s_csAvailable.GetWaitTicks( CriticalSection::eTotal )));
	uint64 u = 0;
	for( int i=0; i<s_maxTypes; ++i )
		u += s_pJobTypes[i].cs.GetWaitTicks( CriticalSection::eTotal );
	DebugOutput( "Job frame CS wait: %ld us\n", u );
}

}