#pragma once

#include "platform.h"

typedef struct
{
	uint8 indices[512];
	uint64 xor_x;
	uint64 original_seed;
} NoiseSapling;

void seed_noise(NoiseSapling *noise, uint64 seed);
float perlin3(NoiseSapling *noise, float x, float y, float z);
