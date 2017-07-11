
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

// TODO: Define keycodes for better agnosticity
typedef struct
{
	uint8 keyCode;
	uint8 state;
} KeyEvent;

#define MOUSE_BUTTON_LEFT 0
#define MOUSE_BUTTON_RIGHT 1

typedef struct
{
	uint mouseButton;
	uint state;
} MouseClickEvent;

typedef struct
{
	int x, y;
	int dx, dy;

	bool locked;
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

// NOTE: The platform must serialize/deserialize this at init/exit
typedef struct
{
	int renderMode;
	float mouseSensitivity;
} Config;

typedef struct
{
	bool running;
	double targetDelta;

	#define EVENT_QUEUE_SIZE 64
	Event eventQueue[EVENT_QUEUE_SIZE];
	int filledEvents;

	#define MOUSE_LOCKED 0x01
	int flags;

	int viewportWidth, viewportHeight;
} PlatformState;

// Functions the platform calls
void tick(double delta);
void init(PlatformState *plat);

// Functions the platform must implement
void setMouseState(bool locked);
bool getPlatformFlag(int flag);

#endif //_VOX_PLATFORM_H_

