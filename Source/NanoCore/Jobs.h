#ifndef __INC_NANOCORE_JOBS
#define __INC_NANOCORE_JOBS

// FIXME: adding a dependant job, where the type it depends on doesn't present any task - it would hang up forever, since we consider dependent tasks only when parent task got finished

/*
	Each jobs can depend on 'type' (an integer constant) or be completely independent.
	The typical use-case is to assign each job to a given type and then chart the dependencies between the types.
	No individual job to job dependencies are supported for now.
*/

#include "Common.h"



namespace NanoCore {



class IJob {
public:
	IJob( int type ) : m_type( type ) {}
	virtual ~IJob() {}
	virtual void Execute() = 0;
	virtual const wchar_t * GetName() { return L"*IJob*"; }

	int GetType() const { return m_type; }
	void SetType( int type );

private:
	int m_type;
};



struct JobManager {
	enum {
		efClearPendingJobs = 1,
		efDisableJobAddition = 2,
	};

	static void Init( int numThreads, int maxTypes );
	static void Done();
	static void AddJob( IJob * pJob, int typeToWait = -1 );
	static void Wait( int flags );
	static bool IsRunning();
	static int  GetNumThreads();

	enum EStats {
		eNumThreads,
		eNumJobs,
		eThreadWorkTime,
		eThreadIdleTime,
		eCriticalSectionsWaitTime,
	};

	static void   ResetStats();
	static uint64 GetStats( EStats stats );

	static void PrintStats();
};



}
#endif