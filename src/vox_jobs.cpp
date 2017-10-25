#include "vox_jobs.h"

JobManager *jobManager;

void initJobSystem(int maxThreads)
{
	jobManager = (JobManager*)calloc(1, sizeof(JobManager));

	jobManager->maxThreads = maxThreads;
	jobManager->runningJobs = (Job*)calloc(jobManager->maxThreads, sizeof(Job));
	jobManager->freeJobs = jobManager->runningJobs;
	for (int i = 0; i < jobManager->maxThreads - 1; i++)
		jobManager->runningJobs[i].next = jobManager->runningJobs + i + 1;

	jobManager->heapSize = 1024;
	jobManager->jobHeap = (Job*)calloc(jobManager->heapSize, sizeof(Job));

}

static void freeThreads(JobManager *jm)
{
	for (int i = 0; i < jm->maxThreads; i++)
	{
		if (jm->runningJobs[i].done)
		{
			jm->runningJobs[i].completionProc(jm->runningJobs[i].args);

			jm->runningJobs[i] = {0};
			jm->runningJobs[i].next = jm->freeJobs;
			jm->freeJobs = jm->runningJobs + i;
			jm->jobsActive--;
		}
	}
}

static Job extractJob(JobManager *jm)
{
	Job *heap = jm->jobHeap;
	Job top = heap[0];
	heap[0] = heap[--jm->jobsQueued];

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

	return top;
}

void startJob(void *arg)
{
	Job *job = (Job*)arg;
	job->jobProc(job->args);
	atomicIncrement(&job->done);
}

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

void addJob(Job job)
{
	JobManager *jm = jobManager;
	if (jm->jobsQueued >= jm->heapSize) // Grow heap
	{
		jm->heapSize *= 2;
		jm->jobHeap = (Job*)realloc(jm->jobHeap, jm->heapSize * sizeof(Job));
	}

	Job *heap = jm->jobHeap;
	heap[jm->jobsQueued++] = job;

	int index = jm->jobsQueued - 1;
	if (index == 0)
		return;
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
}
