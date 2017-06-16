
// This file contains platform-agnostic function prototypes and datatypes.
// The platform layers must fully populate the PlatformState structure
// and call the functions exposed in this header.
// The platform layer shouldn't know anything about the common codebase
// except for what exists in this header file.

#pragma once
#ifndef _VOX_PLATFORM_H_
#define _VOX_PLATFORM_H_

#include <stdint.h>
#include <stdbool.h>

typedef int8_t int8;
typedef int16_t int16;
//typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
//typedef uint32_t uint32;
typedef uint64_t uint64;

typedef enum
{
	EVENT_KEY,
	EVENT_MOUSE_CLICK,
	EVENT_MOUSE_MOVE,
	EVENT_RESIZE,
	EVENTTYPE_LENGTH
} EventType;

typedef struct
{
	uint keyCode;
	bool pressed;
	bool released;
} KeyEvent;

typedef struct
{
	int mouseButton;
	bool pressed;
	bool released;
} MouseClickEvent;

typedef struct
{
	int x, y;
} MouseMoveEvent;

typedef struct
{
	EventType type;
	KeyEvent key;
	MouseClickEvent mouseClick;
	MouseMoveEvent mouseMove;
} Event;

typedef struct
{
	bool running;
	double targetDelta;

	#define EVENT_QUEUE_SIZE 64
	Event eventQueue[EVENT_QUEUE_SIZE];
	int eventIndex;
} PlatformState;

void tick(double delta);
void init(PlatformState *platform);

#endif //_VOX_PLATFORM_H_

