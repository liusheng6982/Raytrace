#include <NanoCore/Threads.h>
#include <NanoCore/Jobs.h>
#include <NanoCore/Serialize.h>
#include <vector>

using namespace std;
using namespace NanoCore;


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
		Sleep( 30 );
		DebugOutput( "Frustum cull job on thread %X.\n", GetCurrentThreadId() );
	}
};
vector<FrustumCullJob> fcjobs(160);

struct CreateCBJob : public IJob
{
	CreateCBJob() : IJob( JOBTYPE_CREATECB ) {}
	virtual void Execute() {
		Sleep( 10 );
		DebugOutput( "CreateCB job on thread %X.\n", GetCurrentThreadId() );
	}
};
vector<CreateCBJob> cbjobs(160);

struct RenderJob : public IJob
{
	RenderJob() : IJob( JOBTYPE_RENDERCB ) {}
	virtual void Execute() {
		Sleep( 30 );
		DebugOutput( "RenderCB job on thread %X.\n", GetCurrentThreadId() );
	}
};
RenderJob rjob;

struct MergeCulledJob : public IJob
{
	MergeCulledJob() : IJob( JOBTYPE_MERGECULLED ) {}
	virtual void Execute() {
		Sleep( 3 );
		DebugOutput( "MergeCulled jobon thread %X.\n", GetCurrentThreadId() );
		for( size_t i=0; i<cbjobs.size(); ++i )
			JobManager::AddJob( &cbjobs[i] );
		JobManager::AddJob( &rjob, JOBTYPE_CREATECB );
	}
};
MergeCulledJob mcjob;


struct MainJob : public IJob
{
	MainJob() : IJob( JOBTYPE_MAIN ) {}
	virtual void Execute() {
		Sleep( 200 );
		for( size_t i=0; i<fcjobs.size(); ++i )
			JobManager::AddJob( &fcjobs[i] );
		JobManager::AddJob( &mcjob, JOBTYPE_FRUSTUMCULL );
		DebugOutput( "Main job on thread %X.\n", GetCurrentThreadId() );
	}
};
MainJob mjob;


int main()
{
	int a,b;
	int len = sscanf( "138879/83984", "%d/%d", &a, &b );

	std::string name, attr, attr_val;
	if( StrPatternMatch( "   <name x=\"5\"  />", "\\s*<%\\s+%=\"%\"\\s*/\\s*>", &name, &attr, &attr_val )) {
		printf( "Match!\n\tname = %s\n\tattribute = %s\n\tvalue = %s\n", name.c_str(), attr.c_str(), attr_val.c_str() );
	} else {
		printf( "Not matched!" );
	}

	JobManager::Init( 0, JOBTYPE_MAX );
	JobManager::AddJob( &mjob );
	while( JobManager::IsRunning() ) {
		Sleep( 5 );
	}
	JobManager::PrintStats();
	return 0;
}