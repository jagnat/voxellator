
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
//typedef int32_t int;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint;
typedef uint64_t uint64;

// Used to query state for keyboard/mouse events
#define BUTTON_RELEASED 0
#define BUTTON_PRESSED 1

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
	uint8 keyCode;
	uint8 state;
} KeyEvent;

#define MOUSE_BUTTON_LEFT 0
#define MOUSE_BUTTON_RIGHT 1

typedef struct
{
	uint8 mouseButton;
	uint8 state;
} MouseClickEvent;

typedef struct
{
	int x, y;
} MouseMoveEvent;

typedef struct
{
	int width, height;
} ResizeEvent;

typedef struct
{
	EventType type;
	KeyEvent key;
	MouseClickEvent mouseClick;
	MouseMoveEvent mouseMove;
	ResizeEvent resize;
} Event;

typedef struct
{
	bool running;
	double targetDelta;

	#define EVENT_QUEUE_SIZE 64
	Event eventQueue[EVENT_QUEUE_SIZE];
	int filledEvents;
} PlatformState;

void tick(double delta);
void init(PlatformState *plat);

#endif //_VOX_PLATFORM_H_

