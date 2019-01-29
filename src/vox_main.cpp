#include "vox_main.h"

#include "vox_platform.h"
#include "vox_render.h"
#include "vox_world.h"
#include "vox_mesher.h"
#include "vox_noise.h"
#include "vox_jobs.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>

#define J_THREEDEE_IMPLEMENTATION
#include "thirdparty/j_threedee.h"
#undef J_THREEDEE_IMPLEMENTATION

PlatformState *platform;
SimState *sim;

const int chunkSize = 3;
const int numChunks = chunkSize * chunkSize * chunkSize;

void init(PlatformState *plat)
{
	platform = plat;
	sim = (SimState*)calloc(1, sizeof(SimState));
	sim->movement.pos.x = -16;
	sim->movement.pos.z = -16;
	sim->movement.pos.y = 80;
	sim->movement.yaw = M_PI + M_PI / 4;
	sim->movement.pitch = -M_PI / 5;

	initJobSystem(platform->info.logicalCores - 1);

	sim->world.init(49839594734698ul);

	initRender();

	for (int x = 0; x < chunkSize; x++)
		for (int z = 0; z < chunkSize; z++)
			for (int y = 0; y < chunkSize; y++)
			{
				sim->world.getOrCreateChunk(x, y, z);
			}
}

void handleEvents();

void buildMovementFromControls();

int timer = 0;

void update()
{
	handleEvents();

	buildMovementFromControls();

	sim->world.update();

	setCam(sim->movement);

	ChunkEntry *entry = sim->world.loadedChunks;
	while (entry)
	{
		if (entry->chunk.hasMesh)
			renderChunkMesh(entry->chunk.mesh);
		entry = entry->next;
	}

	timer++;
	if (timer == 400)
	{
		// Unload all chunks here
		for (int x = 0; x < chunkSize; x++)
			for (int z = 0; z < chunkSize; z++)
				for (int y = 0; y < chunkSize; y++)
				{
					//sim->world.unloadChunkAt(x,y,z);
				}
	}
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
					case 'l':
					if (e.key.state == BUTTON_PRESSED)
						setMouseState(!(platform->flags & MOUSE_LOCKED));
					break;
					case 'W':
					case 'w':
					controls->forward = e.key.state == BUTTON_PRESSED;
					break;
					case 'A':
					case 'a':
					controls->left = e.key.state == BUTTON_PRESSED;
					break;
					case 'S':
					case 's':
					controls->backward = e.key.state == BUTTON_PRESSED;
					break;
					case 'D':
					case 'd':
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
				resizeRender(e.resize.width, e.resize.height);
				break;

				default: break;
			}
		}
	}
}

#include "vox_render.cpp"
#include "vox_noise.cpp"
#include "vox_mesher.cpp"
#include "vox_world.cpp"
#include "vox_jobs.cpp"

