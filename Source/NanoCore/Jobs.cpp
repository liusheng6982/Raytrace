#include "Jobs.h"
#include "Threads.h"
#include <vector>
#include <deque>

using namespace std;
class JobManager;


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

	virtual void StartNewFrame( int frame ) { m_frame = frame; }
	virtual void AddJob( IJob * pJob, int typeToWait = -1 );
	virtual void Stop();

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

class WorkerThread : public ncThread
{
public:
	WorkerThread( JobManager * pJobManager ):m_pActiveJob(NULL), m_pJobManager(pJobManager) {}

	virtual void Run( void* );

private:
	IJob * m_pActiveJob;
	JobManager * m_pJobManager;
};

class JobManager : public IJobManager
{
public:
	JobManager();
	virtual ~JobManager();

	virtual void Init( int numThreads, int maxTypes );
	virtual IJobFrame * CreateJobFrame();
	virtual bool IsRunning();
	virtual void PrintStats( IJobFrame * p );

	void AddJob( IJob * p );
	void AddJobs( IJob ** p, int count );
	IJob * GetJob();

	mutable int m_jobsCount;

	ncCriticalSection   * m_csAvailable;
	deque<IJob*>          m_available;
	vector<WorkerThread*> m_threads;
	int                   m_maxTypes;
};


void IJob::SetType( int type )
{
	m_type = type;
}
void IJob::SetFrame( IJobFrame * p )
{
	m_pFrame = p;
}
void JobFrame::AddJob( IJob * pJob, int typeToWait )
{
	pJob->SetFrame( this );

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
IJobManager * IJobManager::Create()
{
	return new JobManager();
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

		JobFrame * frame = (JobFrame*)pJob->GetFrame();
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
JobManager::JobManager()
{
	m_jobsCount = 0;
	m_csAvailable = ncCriticalSectionCreate();
}
JobManager::~JobManager()
{
	ncCriticalSectionDelete( m_csAvailable );
	for( size_t i=0; i<m_threads.size(); ++i )
		delete m_threads[i];
}
void JobManager::Init( int numThreads, int maxTypes )
{
	if( numThreads <= 0 ) {
		ncSystemInfo si;
		ncGetSystemInfo( &si );
		numThreads = (numThreads == 0) ? si.ProcessorCount-1 : si.ProcessorCount;
	}
	for( int i=0; i<numThreads; ++i ) {
		WorkerThread * p = new WorkerThread( this );
		m_threads.push_back( p );
		p->Start( NULL );
	}
	m_maxTypes = maxTypes;
}
IJobFrame * JobManager::CreateJobFrame()
{
	return new JobFrame( this, m_maxTypes );
}
bool JobManager::IsRunning()
{
	return m_jobsCount > 0;
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
void JobManager::PrintStats( IJobFrame * p )
{
	ncDebugOutput( "Job manager CS wait: %ld us\n", ncTickToMicroseconds( ncCriticalSectionGetWaitTicks( m_csAvailable )));
	JobFrame * jf = (JobFrame*)p;
	uint64 u = 0;
	for( int i=0; i<jf->m_maxTypes; ++i )
		u += ncCriticalSectionGetWaitTicks( jf->m_pJobTypes[i].cs );
	ncDebugOutput( "Job frame CS wait: %ld us\n", u );
}
void JobFrame::Stop()
{
	for( size_t i=0; i<m_pJobManager->m_threads.size(); ++i ) {
		m_pJobManager->m_threads[i]->Terminate();
	}

	for( int i=0; i<m_maxTypes; ++i ) {
		m_pJobTypes[i].dependant_jobs.clear();
		m_pJobTypes[i].count = 0;
	}

	ncCriticalSectionLeave( m_pJobManager->m_csAvailable );
	m_pJobManager->m_available.clear();

	for( size_t i=0; i<m_pJobManager->m_threads.size(); ++i ) {
		m_pJobManager->m_threads[i]->Start( NULL );
	}
}