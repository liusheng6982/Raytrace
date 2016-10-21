#ifndef __INC_NANOCORE_JOBS
#define __INC_NANOCORE_JOBS

#include "Common.h"



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
	virtual void StartNewFrame( int frame ) = 0;
	virtual void Stop() = 0;
};



class IJobManager
{
public:
	virtual ~IJobManager() {}

	virtual void Init( int numThreads, int maxTypes ) = 0;
	virtual IJobFrame * CreateJobFrame() = 0;
	virtual bool IsRunning() = 0;
	virtual void PrintStats( IJobFrame * p ) = 0;

	static IJobManager * Create();
};

#endif