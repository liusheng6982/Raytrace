#include <NanoCore/Threads.h>
#include <NanoCore/Jobs.h>
#include <vector>

using namespace std;


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
			GetFrame()->AddJob( &cbjobs[i] );
		GetFrame()->AddJob( &rjob, JOBTYPE_CREATECB );
	}
};
MergeCulledJob mcjob;


struct MainJob : public IJob
{
	MainJob() : IJob( JOBTYPE_MAIN ) {}
	virtual void Execute() {
		ncSleep( 200 );
		for( size_t i=0; i<fcjobs.size(); ++i )
			GetFrame()->AddJob( &fcjobs[i] );
		GetFrame()->AddJob( &mcjob, JOBTYPE_FRUSTUMCULL );
		ncDebugOutput( "Main job on thread %X.\n", ncGetCurrentThreadId() );
	}
};
MainJob mjob;


int main()
{
	IJobManager * jm = IJobManager::Create();
	jm->Init( 0, JOBTYPE_MAX );
	IJobFrame * jf = jm->CreateJobFrame();

	jf->AddJob( &mjob );

	while( jm->IsRunning() ) {
		ncSleep( 5 );
	}

	jm->PrintStats( jf );

	delete jf;
	delete jm;

	return 0;
}