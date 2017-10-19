#include "vox_main.h"

#include "vox_platform.h"
#include "vox_render.h"
#include "vox_world.h"
#include "vox_mesher.h"
#include "vox_noise.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>

#define J_THREEDEE_IMPLEMENTATION
#include "thirdparty/j_threedee.h"
#undef J_THREEDEE_IMPLEMENTATION

PlatformState *platform;
SimState *sim;

const int chunkSize = 8;
const int numChunks = chunkSize * chunkSize * chunkSize;
Chunk *chunks[numChunks];

void init(PlatformState *plat)
{
	platform = plat;
	sim = (SimState*)calloc(1, sizeof(SimState));
	sim->movement.pos.x = -16;
	sim->movement.pos.z = -16;
	sim->movement.pos.y = 80;
	sim->movement.yaw = M_PI + M_PI / 4;
	sim->movement.pitch = -M_PI / 5;

	initThreadManager();

	initRender();

	seedPerlin3(349278650234309);
	finishedMeshes = (ChunkMesh**)calloc(numChunks, sizeof(ChunkMesh*));

	for (int x = 0; x < chunkSize; x++)
		for (int z = 0; z < chunkSize; z++)
			for (int y = 0; y < chunkSize; y++)
			{
				//chunks[x * chunkSize * chunkSize + z * chunkSize + y] = createPerlinChunk(x, y, z);
				addPerlinChunkJob(x, y, z);
			}
	
	/*
	for (int i = 0; i < numChunks; i++)
	{
		//Color c = {50, (uint8)(100 + rand() % 50), 50};
		Color c = {50, 100, 50};
		chunks[i]->color = c;
		addGreedyJob(chunks[i]);
	}
	*/
}

/// {
void initThreadManager()
{
	ThreadManager *tm = &sim->threading;
	//tm->maxThreads = platform->info.logicalCores - 1;
	tm->maxThreads = 32;
	tm->activeJobs = (ThreadJob*)calloc(tm->maxThreads, sizeof(ThreadJob));
	tm->freeJobList = tm->activeJobs;
	// Last job slot already points to null
	for (int i = 0; i < tm->maxThreads - 1; i++)
	{
		tm->activeJobs[i].next = &tm->activeJobs[i + 1];
	}
}

void startJob(void *arg)
{
	ThreadJob *job = (ThreadJob*)arg;
	job->jobProc(job->args);
	atomicIncrement(&job->done);
}

bool addJob(ThreadJob job)
{
	if (sim->threading.jobsQueued >= JOB_QUEUE_LEN)
		return false;
	
	sim->threading.jobQueue[sim->threading.jobsQueued++] = job;
	return true;
}

void processJobs()
{
	ThreadManager *tm = &sim->threading;

	// Iterate through running threads and erase old ones
	for (int i = 0; i < tm->maxThreads; i++)
	{
		if (tm->activeJobs[i].done)
		{
			tm->activeJobs[i].completionProc(tm->activeJobs[i].args);

			// Zero out and add to free list
			memset((void*)(tm->activeJobs + i), 0, sizeof(ThreadJob)); // Might be unnecessary
			tm->activeJobs[i].next = tm->freeJobList;
			tm->freeJobList = tm->activeJobs + i;
			tm->jobsActive--;
		}
	}

	int availableThreads = tm->maxThreads - tm->jobsActive;
	if (availableThreads <= 0)
		return;

	int jobsSpawned = 0;
	
	// Spawn new threads if we have room
	for (int i = 0; i < tm->jobsQueued; i++)
	{
		if (i >= availableThreads)
			break;
		// Remove first entry from free list
		ThreadJob *freeJob = tm->freeJobList;
		tm->freeJobList = tm->freeJobList->next;
		memcpy(freeJob, tm->jobQueue + i, sizeof(ThreadJob));
		// create thread
		createThread(startJob, freeJob);
		//startJob(freeJob);
		tm->jobsActive++;
		jobsSpawned++;
	}

	for (int i = jobsSpawned; i < tm->jobsQueued; i++)
	{
		memcpy(tm->jobQueue + i - jobsSpawned, tm->jobQueue + i, sizeof(ThreadJob));
	}

	tm->jobsQueued -= jobsSpawned;
}
/// }

void handleEvents();

void buildMovementFromControls();

void update()
{
	handleEvents();
	processJobs();

	buildMovementFromControls();

	setCam(sim->movement);

	for (int i = 0; i < numFinishedMeshes; i++)
		renderChunkMesh(finishedMeshes[i]);
}

void render(double updateInterval)
{

}

void buildMovementFromControls()
{
	Controls *con = &sim->controls;
	Movement *mov = &sim->movement;
	
	const float MOUSE_FACTOR = 0.75;
	float yawDelta = con->screenDeltaX * MOUSE_FACTOR;
	float pitchDelta = con->screenDeltaY * MOUSE_FACTOR;
	
	mov->pitch -= pitchDelta;
	if (mov->pitch > (M_PI / 2) - 0.001)
		mov->pitch = (M_PI / 2) - 0.001;
	if (mov->pitch < 0.001 - (M_PI / 2))
		mov->pitch = 0.001 - (M_PI / 2);
	
	mov->yaw = fmod(mov->yaw - yawDelta, 2 * M_PI);

	const float MV_DELTA = 0.2;
	if (con->forward)
	{
		mov->pos.z -= MV_DELTA * cos(mov->yaw);
		mov->pos.x -= MV_DELTA * sin(mov->yaw);
	}
	if (con->backward)
	{
		mov->pos.z += MV_DELTA * cos(mov->yaw);
		mov->pos.x += MV_DELTA * sin(mov->yaw);
	}
	if (con->left)
	{
		mov->pos.z -= MV_DELTA * cos(mov->yaw + M_PI / 2);
		mov->pos.x -= MV_DELTA * sin(mov->yaw + M_PI / 2);
	}
	if (con->right)
	{
		mov->pos.z += MV_DELTA * cos(mov->yaw + M_PI / 2);
		mov->pos.x += MV_DELTA * sin(mov->yaw + M_PI / 2);
	}
	if (con->up)
		mov->pos.y += MV_DELTA;
	if (con->down)
		mov->pos.y -= MV_DELTA;
	
	// TODO: Add this to text rendering, once we have that
	//if (con->forward || con->backward || con->left || con->right || con->up || con->down)
	//	printf("pos = x:%.2f y:%.2f z:%.2f\n", mov->pos.x, mov->pos.y, mov->pos.z);
}

void handleEvents()
{
	Controls *controls = &sim->controls;
	controls->screenDeltaX = 0;
	controls->screenDeltaY = 0;
	if (platform->filledEvents != 0)
	{
		for (int i = 0; i < platform->filledEvents; i++)
		{
			Event e = platform->eventQueue[i];
			switch (e.type)
			{
				case EVENT_MOUSE_CLICK:

				break;

				case EVENT_MOUSE_MOVE:
				// Use deltas
				if (e.mouseMove.locked)
				{
					// Accum deltas
					controls->screenDeltaX += e.mouseMove.dx / (float)platform->viewportWidth;
					controls->screenDeltaY += e.mouseMove.dy / (float)platform->viewportHeight;
				}
				// Use absolute position
				else
				{
				}
				break;

				case EVENT_KEY:
				{
				switch(e.key.keyCode)
				{
					case 'L':
					if (e.key.state == BUTTON_PRESSED)
						setMouseState(!(platform->flags & MOUSE_LOCKED));
					break;
					case 'W':
					controls->forward = e.key.state == BUTTON_PRESSED;
					break;
					case 'A':
					controls->left = e.key.state == BUTTON_PRESSED;
					break;
					case 'S':
					controls->backward = e.key.state == BUTTON_PRESSED;
					break;
					case 'D':
					controls->right = e.key.state == BUTTON_PRESSED;
					break;
					// TODO: Make special keys platform-agnostic
					case 0xa0: // Left Shift
					controls->down = e.key.state == BUTTON_PRESSED;
					break;
					case ' ':
					controls->up = e.key.state == BUTTON_PRESSED;
					break;
				}
				}
				break;

				case EVENT_RESIZE:
				// TODO: Handle GL viewport resize
				break;
			}
		}
	}
}

#include "vox_render.cpp"
#include "vox_noise.cpp"
#include "vox_mesher.cpp"
#include "vox_world.cpp"

