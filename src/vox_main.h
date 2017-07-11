#ifndef _VOX_MAIN_C_
#define _VOX_MAIN_C_

#include "thirdparty/j_threedee.h"

typedef struct
{
	float topSpeed, accelFactor;

	float screenDeltaX, screenDeltaY;
	float pitchDelta, yawDelta;
	float pitch, yaw;

	int moveDirX, moveDirY, moveDirZ;

	union { JVec3 velocity; JVec3 vel; };
	union { JVec3 position; JVec3 pos; };
} Movement;

#endif // _VOX_MAIN_C_

