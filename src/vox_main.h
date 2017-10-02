#ifndef _VOX_MAIN_C_
#define _VOX_MAIN_C_

#include "thirdparty/j_threedee.h"

struct Movement
{
	float topSpeed, accelFactor;

	float pitchDelta, yawDelta;
	float pitch, yaw;

	int moveDirX, moveDirY, moveDirZ;

	union { JVec3 velocity; JVec3 vel; };
	union { JVec3 position; JVec3 pos; };
};

struct Controls
{
	bool forward, backward, left, right, up, down;
	float screenDeltaX, screenDeltaY;
};

// ------ Begin thread stuff ------
// TODO: Move this all into its own file

struct ThreadJob
{
	void (*jobProc) (void*);
	void (*completionProc) (void*);
	void *args;
	volatile int done;
	ThreadJob *next; // Implementation-only
};

struct ThreadManager
{
	int maxThreads;
	// Circular queue
	// TODO: Move to datastructure code
#define JOB_QUEUE_LEN 256
	ThreadJob jobQueue[JOB_QUEUE_LEN];
	ThreadJob *activeJobs;
	ThreadJob *freeJobList;
	int jobsQueued;
	int jobsFinished;
	volatile int jobsActive;
};

void initThreadManager();
void processJobs();
bool addJob(ThreadJob *job);
// ------ End thread stuff ------

struct SimState
{
	ThreadManager threading;
	Controls controls;
	Movement movement;
};

#endif // _VOX_MAIN_C_

