#include "RayTrace.h"

void InitRenderTask( int numThreads, RenderTask & task )
{
	if( !task.render_width || !task.render_height )
		return;

	if( !numThreads ) {
		SYSTEM_INFO si;
		GetSystemInfo( &si );
		numThreads = si.dwNumberOfProcessors;
	}
	if( !task.cell_width || !task.cell_height ) {
		task.cell_width = task.cell_height = 10;
	}
	task.cell_destribution = new int[task.cell_width * task.cell_height];
	ComputeGridDestribution( task.cell_width, task.cell_height, task.cell_destribution );

	const int size = task.render_width*task.render_height*3;
	if( !task.pRenderBuffer ) {
		task.pRenderBuffer = new uint8[size];
		memset( task.pRenderBuffer, 0, size );
	}
	task.pRenderBuffers = new uint8*[numThreads];
	for( int i=0; i<numThreads; ++i ) {
		task.pRenderBuffers[i] = new uint8[size];
		memset( task.pRenderBuffers[i], 0, size );
	}
	task.pixelsComplete = 0;
	task.numThreads = numThreads;
	task.gatherIndex = 0;
	task.pHalfResBuffer = 0;
	task.mip = 0;
}

static RenderTask        * s_pRenderTask = 0;
static std::vector<HANDLE> s_Threads;

static DWORD WINAPI RenderThreadProc( LPVOID param )
{
	int threadId = (int)param;
	int cw = s_pRenderTask->cell_width;
	int ch = s_pRenderTask->cell_height;
	int w = s_pRenderTask->render_width / cw;
	int h = s_pRenderTask->render_height / ch;

	Vec3 at, up, right;
	s_pRenderTask->camera.GetAxes( at, up, right );

	right *= 1.0f / s_pRenderTask->render_width;
	up *= 1.0f / s_pRenderTask->render_height;

	int step = w*h / s_pRenderTask->numThreads / s_pRenderTask->numThreads * s_pRenderTask->numThreads;

	const int rw = s_pRenderTask->render_width;

	for( int i=0; i<cw*ch; ++i ) {
		for( int j = threadId; j < w*h; j += s_pRenderTask->numThreads ) {
			int cell = j; //(j + threadId*step) % (w*h);
			int x = (cell % w)*cw + s_pRenderTask->cell_destribution[i] % cw;
			int y = (cell / w)*ch + s_pRenderTask->cell_destribution[i] / cw;

			if( s_pRenderTask->pHalfResBuffer ) {
				if( s_pRenderTask->ComputeError( x, y ) < 20.0f ) {
					s_pRenderTask->pRenderBuffers[threadId][(x+y*rw)*3] = s_pRenderTask->pHalfResBuffer[(x/2 + (y/2)*rw/2)*3];
					s_pRenderTask->pRenderBuffers[threadId][(x+y*rw)*3+1] = s_pRenderTask->pHalfResBuffer[(x/2 + (y/2)*rw/2)*3+1];
					s_pRenderTask->pRenderBuffers[threadId][(x+y*rw)*3+2] = s_pRenderTask->pHalfResBuffer[(x/2 + (y/2)*rw/2)*3+2];
					s_pRenderTask->pixelsComplete++;
					continue;
				}
			}
			s_pRenderTask->TracePreviewRay( x, y, threadId );
			s_pRenderTask->pixelsComplete++;
		}
	}
	s_Threads[threadId] = 0;
	ExitThread( 0 );
}

void RunRenderTask( RenderTask & task )
{
	if( ! IsRenderTaskComplete() )
		return;

	s_pRenderTask = &task;
	s_Threads.clear();
	s_Threads.resize( task.numThreads );

	for( int i=0; i<task.numThreads; ++i )
	{
		s_Threads[i] = CreateThread( 0, 0, &RenderThreadProc, (void*)i, 0, 0 );
	}
}

void StopRenderTask()
{
	if( s_Threads.empty())
		return;
	for( size_t i=0; i<s_Threads.size(); ++i ) {
		if( s_Threads[i] )
			TerminateThread( s_Threads[i], 0 );
	}
	s_Threads.clear();
}

bool IsRenderTaskComplete()
{
	for( size_t i=0; i<s_Threads.size(); ++i )
		if( s_Threads[i] )
			return false;
	return true;
}

static HANDLE   s_hTask = 0;
static TaskFunc s_TaskFunc = 0;
static int      s_TaskParam;

static DWORD WINAPI TaskProc( LPVOID param )
{
	s_TaskFunc( s_TaskParam );
	s_hTask = 0;
	ExitThread( 0 );
}

void RunTask( TaskFunc fn, int param )
{
	if( s_hTask ) return;
	s_TaskFunc = fn;
	s_TaskParam = param;
	s_hTask = CreateThread( 0, 0, &TaskProc, 0, 0, 0 );
}

bool IsTaskComplete()
{
	return s_hTask == 0;
}

void RenderTask::GatherRenderBuffer()
{
	if( !pRenderBuffer ) return;
	const int size = render_width*render_height*3;
	const uint8 * p = pRenderBuffers[gatherIndex];
	for( int i=0; i<size; ++i ) {
		pRenderBuffer[i] = max( pRenderBuffer[i], p[i] );
	}
	gatherIndex = (gatherIndex+1) % numThreads;
}