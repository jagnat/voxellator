#include "vox_jobs.h"
#include "vox_platform.h"

#include <stdio.h>
#include <stdlib.h>

JobManager *jobManager;

static Job extractJob(JobManager *jm);

static void jobThreadProc(void* data)
{
	JobManager *jm = (JobManager*)data;
	while (1)
	{
		Job job = extractJob(jm);
		if (!job.jobProc) // Heap was empty
		{
			sleepMs(1);
			continue;
		}
		job.jobProc(job.args);
	}
}

void initJobSystem(int maxJobs)
{
	jobManager = (JobManager*)calloc(1, sizeof(JobManager));

	jobManager->maxJobs = maxJobs;
	printf("max threads: %d\n", maxJobs);

	jobManager->heapSize = 1024;
	jobManager->jobsQueued = 0;
	jobManager->jobHeap = (Job*)calloc(jobManager->heapSize, sizeof(Job));
	jobManager->heapLock = createMutex();
	if (!jobManager->heapLock)
		printf("Err creating heap lock\n"); // TODO: Fix
	
	for (int i = 0; i < jobManager->maxJobs; i++)
	{
		int r = createThread(jobThreadProc, jobManager);
		if (!r)
			printf("Failed to create thread!\n");
	}
}

// Extract top job from heap
static Job extractJob(JobManager *jm)
{
	lockMutex(jm->heapLock);
	Job *heap = jm->jobHeap;
	Job top = {0};

	if (jm->jobsQueued > 0)
	{
		top = heap[0];
		jm->jobsQueued--;
		heap[0] = heap[jm->jobsQueued]; // Replace top with last

		// Sift down
		int index = 0;
		while (index * 2 + 1 < jm->jobsQueued)
		{
			// Decide which leaf to sift down
			int swapChild = index * 2 + 1;
			if (swapChild + 1 < jm->jobsQueued && heap[swapChild + 1].priority > heap[swapChild].priority)
				swapChild++;
	
			// Sift
			if (heap[index].priority < heap[swapChild].priority)
			{
				Job swap = heap[swapChild];
				heap[swapChild] = heap[index];
				heap[index] = swap;
				index = swapChild;
			}
			else break;
		}
	}

	unlockMutex(jm->heapLock);

	return top;
}

// Thread procedure for a job
#if 0
void startJob(void *arg)
{
	Job *job = (Job*)arg;
	job->jobProc(job->args);
	//atomicIncrement(&job->done);
}
#endif

// Process any pending jobs, assign to threads if available
#if 0
void processJobs()
{
	JobManager *jm = jobManager;

	freeThreads(jm);
	
	int availableThreads = jm->maxThreads - jm->jobsActive;
	if (availableThreads <= 0)
		return;
	
	int jobsSpawned = 0;
	
	// Pull jobs from heap and run them
	while (jobsSpawned < availableThreads && jm->jobsQueued != 0)
	{
		Job *freeJob = jm->freeJobs;
		jm->freeJobs = freeJob->next;

		*freeJob = extractJob(jm);
		createThread(startJob, freeJob);
		jm->jobsActive++;
		jobsSpawned++;
	}
}
#endif

// Add job to job queue
void addJob(Job job)
{
	JobManager *jm = jobManager;
	lockMutex(jm->heapLock);
	if (jm->jobsQueued >= jm->heapSize) // Grow heap
	{
		jm->heapSize *= 2;
		jm->jobHeap = (Job*)realloc(jm->jobHeap, jm->heapSize * sizeof(Job));
	}

	Job *heap = jm->jobHeap;
	heap[jm->jobsQueued++] = job; // Place job at end of heap

	int index = jm->jobsQueued - 1;
	if (index == 0) // Skip sifting if no other jobs
	{
		unlockMutex(jm->heapLock);
		return;
	}

	// Sift up
	while ((index - 1) / 2 >= 0)
	{
		int parent = (index - 1) / 2;
		if (heap[parent].priority < heap[index].priority)
		{
			heap[index] = heap[parent];
			heap[parent] = job;
			index = parent;
		}
		else break;
	}
	unlockMutex(jm->heapLock);
}

