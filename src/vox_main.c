#include "vox_main.h"

#include "vox_platform.h"
#include "vox_render.h"
#include "vox_world.h"
#include "vox_mesher.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>

#define J_THREEDEE_IMPLEMENTATION
#include "thirdparty/j_threedee.h"
#undef J_THREEDEE_IMPLEMENTATION

PlatformState *platform;
SimState *sim;

ChunkMesh *meshes[3];
Chunk *chunks[3];

void init(PlatformState *plat)
{
	platform = plat;
	sim = (SimState*)calloc(1, sizeof(SimState));
	sim->movement.pos.x = -16;
	sim->movement.pos.z = -16;
	sim->movement.pos.y = 80;
	sim->movement.yaw = M_PI + M_PI / 4;
	sim->movement.pitch = -M_PI / 5;
	initRender();

	for (int i = 0; i < 3; i++)
	{
		chunks[i] = createPerlinChunk(0, 0, 0);
		chunks[i]->z = i;
		meshes[i] = createChunkMesh(20000000);
	}
	meshVanillaNaive(chunks[0], meshes[0]);
	meshVanillaCull(chunks[1], meshes[1]);
	meshVanillaGreedy(chunks[2], meshes[2]);
	for (int i = 0; i < 3; i++)
		uploadChunkMesh(meshes[i]);
}

void handleEvents();

void buildMovementFromControls();

void tick(double delta)
{
	handleEvents();
	buildMovementFromControls();

	setCam(sim->movement);
	for (int i = 0; i < 3; i++)
		renderChunkMesh(meshes[i]);
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

	const float MV_DELTA = 0.5;
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

#include "vox_render.c"
#include "vox_noise.c"
#include "vox_mesher.c"
#include "vox_world.c"

