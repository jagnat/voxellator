#include "vox_platform.h"

#include <stdio.h>

#include "vox_gl_triangle.h"

PlatformState *platform;

void init(PlatformState *plat)
{
	platform = plat;
	initGlTriangle();
}

void tick(double delta)
{
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

				break;

				case EVENT_KEY:

				break;

				case EVENT_RESIZE:
				
				break;
			}
		}
	}

	drawGlTriangle(0, 0, 0, 0);
}

#include "vox_gl_triangle.c"

