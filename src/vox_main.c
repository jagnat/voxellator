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
	Movement movement = {0};

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
					movement.screenDeltaX += e.mouseMove.dx / (float)platform->viewportWidth;
					movement.screenDeltaY += e.mouseMove.dy / (float)platform->viewportHeight;
				}
				// Use absolute position
				else
				{
				}
				break;

				case EVENT_KEY:
				{
					if (e.key.state == BUTTON_PRESSED)
					{
						switch(e.key.keyCode)
						{
							case 'L':
								setMouseState(!(platform->flags & MOUSE_LOCKED));
							break;
							case 'W':
							break;
							case 'A':
							break;
							case 'S':
							break;
							case 'D':
							break;
							//TODO: Add ascend/descend
						}
					}
					else // BUTTON_RELEASED
					{

					}
				}
				break;

				case EVENT_RESIZE:
				
				break;
			}
		}
	}

	drawGlTriangle(movement);
}

#include "vox_gl_triangle.c"

