#ifndef __INC_NANOCORE_JOBS
#define __INC_NANOCORE_JOBS

#include "Common.h"



namespace NanoCore {


	
class IJobManager;



class IJob {
public:
	IJob( int type ) : m_pJobManager( NULL ), m_type( type ) {}
	virtual ~IJob() {}
	virtual void Execute() = 0;

	int GetType() const { return m_type; }
	void SetType( int type );

	void SetJobManager( IJobManager * ptr );
	IJobManager * GetJobManager() const { return m_pJobManager; }

private:
	int m_type;
	IJobManager * m_pJobManager;
};



class IJobManager {
public:
	virtual ~IJobManager() {}

	virtual void Init( int numThreads, int maxTypes ) = 0;
	virtual void AddJob( IJob * pJob, int typeToWait = -1 ) = 0;
	virtual void ClearPending() = 0;  // remove all tasks, except the running ones
	virtual void Wait() = 0;
	virtual bool IsRunning() = 0;

	virtual void PrintStats() = 0;

	static IJobManager * Create();
};



}
#endif