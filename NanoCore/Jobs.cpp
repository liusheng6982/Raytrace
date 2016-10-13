#include "Jobs.h"



class WorkerThread : public ncThread
{
public:
	WorkerThread( JobManager * pJobManager ):m_pActiveJob(NULL), m_pJobManager(pJobManager) {}

	virtual void Run( void* );

private:
	IJob * m_pActiveJob;
	JobManager * m_pJobManager;
};

class JobFrame : public IJobFrame
{
public:
	JobFrame( JobManager * ptr, int max_types ) : m_pJobManager( ptr ), m_frame(0), m_jobsCount(0) {
		m_pJobTypes = new JobType[max_types];
		m_maxTypes = max_types;
	}
	~JobFrame() {
		delete[] m_pJobTypes;
	}

	void StartNewFrame( int frame ) { m_frame = frame; }

	void AddJob( IJob * pJob, int typeToWait = -1 );

	int m_frame;
	mutable int m_jobsCount;
	JobManager  * m_pJobManager;

	struct JobType {
		int count;
		vector<IJob*> dependant_jobs;
		ncCriticalSection * cs;

		JobType():count(0){
			cs = ncCriticalSectionCreate();
		}
		~JobType() {
			ncCriticalSectionDelete( cs );
		}
	};
	JobType * m_pJobTypes;
	int       m_maxTypes;
};

class JobManager
{
public:
	JobManager( int numThreads );
	~JobManager();

	void AddJob( IJob * p, int type );
	void AddJobs( IJob ** p, int count, int type );
	IJob * GetJob();

	mutable int m_jobsCount;

	ncCriticalSection   * m_csAvailable;
	deque<IJob*>          m_available;
	vector<WorkerThread*> m_threads;
};

void JobFrame::AddJob( IJob * pJob, int typeToWait )
{
	pJob->m_pFrame = this;

	JobType * pJobType = m_pJobTypes + pJob->GetType();

	//ncCriticalSectionScope cs( pJobType->cs );
	pJobType->count++;

	if( typeToWait == -1 ) {
		m_pJobManager->AddJob( pJob );
	} else {
		ncCriticalSectionScope cs( m_pJobTypes[typeToWait].cs );
		m_pJobTypes[typeToWait].dependant_jobs.push_back( pJob );
		m_jobsCount++;
	}
}

void WorkerThread::Run( void* )
{
	ncDebugOutput( "Worker thread %X started.\n", this );
	for( ;; ) {
		IJob * pJob = m_pJobManager->GetJob();
		if( !pJob ) {
			ncSleep( 1 );
			continue;
		}

		m_pActiveJob = pJob;
		pJob->Execute();

		int type = pJob->GetType();

		JobFrame * frame = pJob->GetFrame();
		JobFrame::JobType * ptr = &frame->m_pJobTypes[type];

		{
			ncCriticalSectionScope cs( ptr->cs );
			if( --ptr->count == 0 ) {
				const int jc = ptr->dependant_jobs.size();
				if( jc ) {
					m_pJobManager->AddJobs( &ptr->dependant_jobs[0], jc );
					ptr->dependant_jobs.clear();
					frame->m_jobsCount -= jc;
				}
			}
		}
		ncCriticalSectionScope cs( m_pJobManager->m_csAvailable );
		m_pJobManager->m_jobsCount--;
	}
}

JobManager::JobManager( int numThreads )
{
	m_jobsCount = 0;
	m_csAvailable = ncCriticalSectionCreate();
	for( int i=0; i<numThreads; ++i ) {
		WorkerThread * p = new WorkerThread( this );
		m_threads.push_back( p );
		p->Start( NULL );
	}
}

JobManager::~JobManager()
{
	ncCriticalSectionDelete( m_csAvailable );
	for( size_t i=0; i<m_threads.size(); ++i )
		delete m_threads[i];
}

void JobManager::AddJob( IJob * p )
{
	ncCriticalSectionScope cs( m_csAvailable ); 
	m_available.push_back( p );
	m_jobsCount++;
}

void JobManager::AddJobs( IJob ** p, int count )
{
	ncCriticalSectionScope cs( m_csAvailable );
	for( int i=0; i<count; ++i )
		m_available.push_back( p[i] );
	m_jobsCount += count;
}



IJob * JobManager::GetJob()
{
	ncCriticalSectionScope cs( m_csAvailable );

	if( m_available.empty())
		return NULL;

	IJob * p = m_available.front();
	m_available.pop_front();
	return p;
}