#include "Jobs.h"
#include "Threads.h"
#include <vector>
#include <deque>

#define MAX_THREADS 32



namespace NanoCore {



class JobManager;

class WorkerThread : public Thread {
public:
	WorkerThread( JobManager * pJobManager ) : m_pActiveJob(NULL), m_pJobManager(pJobManager) {}

	virtual void Run( void* );

private:
	IJob * m_pActiveJob;
	JobManager * m_pJobManager;
};

class JobManager : public IJobManager {
public:
	JobManager();
	virtual ~JobManager();

	virtual void Init( int numThreads, int maxTypes );
	virtual void AddJob( IJob * pJob, int typeToWait = -1 );
	virtual void ClearPending();
	virtual void Wait();

	virtual bool IsRunning();
	virtual void PrintStats();

	void AddImmediateJobs( IJob ** p, int count );
	IJob * GetSingleJob();

	volatile int  m_jobsCount;
	volatile bool m_bSuspendedThreads;

	CriticalSection            m_csAvailable;
	std::deque<IJob*>          m_available;
	std::vector<WorkerThread*> m_threads;
	int                        m_maxTypes;

	struct JobType {
		volatile int count;
		std::vector<IJob*> dependant_jobs;
		CriticalSection cs;
		JobType() : count(0) {}
	};
	JobType * m_pJobTypes;
};


void IJob::SetType( int type ) {
	m_type = type;
}

void IJob::SetJobManager( IJobManager * ptr ) {
	m_pJobManager = ptr;
}


void JobManager::AddJob( IJob * pJob, int typeToWait ) {
	pJob->SetJobManager( this );
	JobType * pJobType = m_pJobTypes + pJob->GetType();

	//CriticalSectionScope cs( pJobType->cs );
	pJobType->count++;

	if( typeToWait == -1 ) {
		AddImmediateJobs( &pJob, 1 );
	} else {
		csScope cs( m_pJobTypes[typeToWait].cs );
		m_pJobTypes[typeToWait].dependant_jobs.push_back( pJob );
		m_jobsCount++;
	}
}

void WorkerThread::Run( void* ) {
	DebugOutput( "Worker thread %X started.\n", this );
	for( ;; ) {
		IJob * pJob = m_pJobManager->GetSingleJob();
		if( !pJob ) {
			Suspend();
			//Sleep( 1 );
			continue;
		}

		m_pActiveJob = pJob;
		pJob->Execute();

		const int type = pJob->GetType();
		JobManager::JobType * ptr = &m_pJobManager->m_pJobTypes[type];

		{
			csScope cs( ptr->cs );
			if( --ptr->count == 0 ) {
				const int jc = ptr->dependant_jobs.size();
				if( jc ) {
					m_pJobManager->AddImmediateJobs( &ptr->dependant_jobs[0], jc );
					ptr->dependant_jobs.clear();
					m_pJobManager->m_jobsCount -= jc;
				}
			}
		}
		csScope cs( m_pJobManager->m_csAvailable );
		m_pJobManager->m_jobsCount--;
	}
}

JobManager::JobManager() : m_jobsCount( 0 ), m_pJobTypes(NULL) {
}

JobManager::~JobManager() {
	for( size_t i=0; i<m_threads.size(); ++i )
		delete m_threads[i];
	delete[] m_pJobTypes;
}

void JobManager::Init( int numThreads, int maxTypes ) {
	if( m_pJobTypes ) delete[] m_pJobTypes;
	m_pJobTypes = new JobType[maxTypes];
	m_maxTypes = maxTypes;

	if( numThreads <= 0 ) {
		SystemInfo si;
		GetSystemInfo( &si );
		numThreads = (numThreads == 0) ? si.ProcessorCount-1 : si.ProcessorCount;
	} else if( numThreads > MAX_THREADS ) {
		numThreads = MAX_THREADS;
		// warning
	}
	for( int i=0; i<numThreads; ++i ) {
		WorkerThread * p = new WorkerThread( this );
		m_threads.push_back( p );
		p->Start();
	}
}

bool JobManager::IsRunning() {
	return m_jobsCount > 0;
}

void JobManager::AddImmediateJobs( IJob ** p, int count ) {
	csScope cs( m_csAvailable );
	for( int i=0; i<count; ++i )
		m_available.push_back( p[i] );
	m_jobsCount += count;
	for( int i=0, n=m_threads.size(); i<n; ++i )
		m_threads[i]->Resume();
}

IJob * JobManager::GetSingleJob() {
	csScope cs( m_csAvailable );
	if( m_available.empty())
		return NULL;
	IJob * p = m_available.front();
	m_available.pop_front();
	return p;
}

void JobManager::ClearPending() {
	for( int i=0; i<m_maxTypes; ++i ) {
		csScope cs( m_pJobTypes[i].cs );
		m_pJobTypes[i].dependant_jobs.clear();
		m_pJobTypes[i].count = 0;
	}
	csScope cs( m_csAvailable );
	m_available.clear();
	m_jobsCount = 0;
}

void JobManager::Wait() {
	while( true ) {
		bool bRunning = false;
		for( int i=0,n=m_threads.size(); i<n; ++i )
			if( m_threads[i]->IsRunning()) bRunning = true;
		if( bRunning || m_jobsCount ) {
			Sleep( 1 );
		} else {
			break;
		}
	}
}

void JobManager::PrintStats() {
	DebugOutput( "Job manager CS wait: %ld us\n", TickToMicroseconds( m_csAvailable.GetWaitTicks( CriticalSection::eWaitTick_Total )));
	uint64 u = 0;
	for( int i=0; i<m_maxTypes; ++i )
		u += m_pJobTypes[i].cs.GetWaitTicks( CriticalSection::eWaitTick_Total );
	DebugOutput( "Job frame CS wait: %ld us\n", u );
}

IJobManager * IJobManager::Create() {
	return new JobManager();
}

}