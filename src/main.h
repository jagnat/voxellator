#pragma once

#include "thirdparty/j_threedee.h"

#include "platform.h"

typedef struct
{
	float topSpeed, accelFactor;

	float pitchDelta, yawDelta;
	float pitch, yaw;

	int moveDirX, moveDirY, moveDirZ;

	union { JVec3 velocity; JVec3 vel; };
	union { JVec3 position; JVec3 pos; };
} Movement;

typedef struct
{
	bool forward, backward, left, right, up, down;
	float screenDeltaX, screenDeltaY;
} Controls;

typedef struct
{
	Controls controls;
	Movement movement;
} SimState;
