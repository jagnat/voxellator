#pragma once
#ifndef _VOX_WORLD_H_
#define _VOX_WORLD_H_

#include "vox_noise.h"
#include "vox_platform.h"
#include "thirdparty/j_threedee.h"

#define CHUNK_SIZE 64

typedef struct
{
	uint8 *data;
	int empty;
	int numSet;
	int x, y, z;
	Color color;

	int hasMesh;
	struct ChunkMesh *mesh;

	volatile int generated;
} Chunk;

typedef enum
{
	GEN_PERL3D,
	GEN_PERL2D
} GenMode;

typedef struct
{
	GenMode mode;
	Perlin3 perlin;
	uint64 seed;
} GenContext;

void initWorld(uint64 seed);
void updateWorld();

void loadChunk(int x, int y, int z);

void allocateChunkData(Chunk *chunk);

void fillPerlinChunk(Chunk *c);

void addPerlinChunkJob(Chunk *c);

uint8 chunk_getBlockUnchecked(Chunk *chunk, int x, int y, int z);
uint8 chunk_getBlockChecked(Chunk *chunk, int x, int y, int z);
void chunk_setBlockUnchecked(Chunk *chunk, uint8 val, int x, int y, int z);
void chunk_setBlockChecked(Chunk *chunk, uint8 val, int x, int y, int z);

#endif // _VOX_WORLD_H_
