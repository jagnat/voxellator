#include "vox_platform.h"

#include <stdio.h>

PlatformState *platform;

void init(PlatformState *plat)
{
	platform = plat;
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
#if 0
				case EVENT_MOUSE_CLICK:
				printf("mouse click: %d, %d\n", e.mouseClick.mouseButton, e.mouseClick.state);
				break;

				case EVENT_MOUSE_MOVE:
				printf("move: %d,%d\n", e.mouseMove.x, e.mouseMove.y);
				break;

				case EVENT_KEY:
				printf("key %s: %c\n", e.key.state ? "pressed" : "released", e.key.keyCode);
				break;
#endif

				case EVENT_RESIZE:
				printf("resize w:%d h:%d\n", e.resize.width, e.resize.height);
				break;
			}
		}
	}
}

