#ifndef _VOX_JOBS_H_
#define _VOX_JOBS_H_

typedef struct
{
	void (*jobProc)(void*);
	//void (*completionProc)(void*);
	void *args;
	//volatile int done;
	int priority;
	//Job *next;
} Job;

typedef struct
{
	Job *jobHeap;
	int jobsQueued;
	int heapSize;
	void *heapLock;

	int maxJobs;
} JobManager;

void initJobSystem(int maxJobs);
//void processJobs();
void addJob(Job job);

#endif //_VOX_JOBS_H_
