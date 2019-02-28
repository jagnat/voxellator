#pragma once
#ifndef _VOX_NOISE_H_
#define _VOX_NOISE_H_

typedef struct
{
	uint8 indices[512];
	uint64 xor_x;
} Perlin3;

void seedPerlin3(Perlin3 *perl, uint64 seed);
float perlin3(Perlin3 *perl, float x, float y, float z);

#endif // _VOX_NOISE_H_
