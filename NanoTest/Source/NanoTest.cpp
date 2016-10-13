#include <NanoCore/Jobs.h>

enum {
	JOBTYPE_MAIN,
	JOBTYPE_FRUSTUMCULL,
	JOBTYPE_MERGECULLED,
	JOBTYPE_CREATECB,
	JOBTYPE_RENDERCB,
	JOBTYPE_MAX
};

struct FrustumCullJob : public IJob
{
	FrustumCullJob() : IJob( JOBTYPE_FRUSTUMCULL ) {}
	virtual void Execute() {
		ncSleep( 30 );
		ncDebugOutput( "Frustum cull job on thread %X.\n", ncGetCurrentThreadId() );
	}
};
vector<FrustumCullJob> fcjobs(160);

struct CreateCBJob : public IJob
{
	CreateCBJob() : IJob( JOBTYPE_CREATECB ) {}
	virtual void Execute() {
		ncSleep( 10 );
		ncDebugOutput( "CreateCB job on thread %X.\n", ncGetCurrentThreadId() );
	}
};
vector<CreateCBJob> cbjobs(160);

struct RenderJob : public IJob
{
	RenderJob() : IJob( JOBTYPE_RENDERCB ) {}
	virtual void Execute() {
		ncSleep( 30 );
		ncDebugOutput( "RenderCB job on thread %X.\n", ncGetCurrentThreadId() );
	}
};
RenderJob rjob;

struct MergeCulledJob : public IJob
{
	MergeCulledJob() : IJob( JOBTYPE_MERGECULLED ) {}
	virtual void Execute() {
		ncSleep( 3 );
		ncDebugOutput( "MergeCulled jobon thread %X.\n", ncGetCurrentThreadId() );
		for( size_t i=0; i<cbjobs.size(); ++i )
			m_pFrame->AddJob( &cbjobs[i] );
		m_pFrame->AddJob( &rjob, JOBTYPE_CREATECB );
	}
};
MergeCulledJob mcjob;


struct MainJob : public IJob
{
	MainJob() : IJob( JOBTYPE_MAIN ) {}
	virtual void Execute() {
		ncSleep( 200 );
		for( size_t i=0; i<fcjobs.size(); ++i )
			m_pFrame->AddJob( &fcjobs[i] );
		m_pFrame->AddJob( &mcjob, JOBTYPE_FRUSTUMCULL );
		ncDebugOutput( "Main job on thread %X.\n", ncGetCurrentThreadId() );
	}
};
MainJob mjob;


int main()
{
	JobManager * jm = new JobManager( 4 );
	JobFrame * jf = new JobFrame( jm, JOBTYPE_MAX );
	jf->AddJob( &mjob );

	while( jf->m_jobsCount || jm->m_jobsCount ) {
		ncSleep( 5 );
	}

	ncDebugOutput( "Job manager CS wait: %ld us\n", ncTickToMicroseconds( ncCriticalSectionGetWaitTicks( jm->m_csAvailable )));

	uint64 u = 0;
	for( int i=0; i<jf->m_maxTypes; ++i )
		u += ncCriticalSectionGetWaitTicks( jf->m_pJobTypes[i].cs );
	ncDebugOutput( "Job frame CS wait: %ld us\n", u );

	return 0;
}