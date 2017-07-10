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

				}
				// Use absolute position
				else
				{

				}
				break;

				case EVENT_KEY:
				switch(e.key.keyCode)
				{
					case 'L':
					if (e.key.state == BUTTON_PRESSED)
						setMouseState(!(platform->flags & MOUSE_LOCKED));
					break;
				}
				break;

				case EVENT_RESIZE:
				
				break;
			}
		}
	}

	Movement movement = {0};
	drawGlTriangle(movement);
}

#include "vox_gl_triangle.c"

