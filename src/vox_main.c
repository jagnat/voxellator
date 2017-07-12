#include "vox_main.h"

#include "vox_platform.h"

#include <stdio.h>

#include "vox_gl_triangle.h"

#define J_THREEDEE_IMPLEMENTATION
#include "thirdparty/j_threedee.h"
#undef J_THREEDEE_IMPLEMENTATION

PlatformState *platform;

void init(PlatformState *plat)
{
	platform = plat;
	initGlTriangle();
}

void tick(double delta)
{
	Controls controls = {0};

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
					controls.screenDeltaX += e.mouseMove.dx / (float)platform->viewportWidth;
					controls.screenDeltaY += e.mouseMove.dy / (float)platform->viewportHeight;
					if (e.mouseMove.dx != 0 || e.mouseMove.dy != 0)
					{
						printf("dx: %d vw: %d\n", e.mouseMove.dx, platform->viewportWidth);
						printf("dy: %d vh: %d\n", e.mouseMove.dy, platform->viewportHeight);
					}
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
					controls.forward = e.key.state == BUTTON_PRESSED ? true : false;
					break;
					case 'A':
					controls.left = e.key.state == BUTTON_PRESSED ? true : false;
					break;
					case 'S':
					controls.backward = e.key.state == BUTTON_PRESSED ? true : false;
					break;
					case 'D':
					controls.right = e.key.state == BUTTON_PRESSED ? true : false;
					break;
					//TODO: Add ascend/descend
				}
				}
				break;

				case EVENT_RESIZE:
				
				break;
			}
		}
	}

	//drawGlTriangle(movement);
}

#include "vox_gl_triangle.c"

