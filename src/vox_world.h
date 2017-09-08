#pragma once
#ifndef _VOX_WORLD_H_
#define _VOX_WORLD_H_

#include "vox_platform.h"

#define CHUNK_SIZE 64

struct Chunk
{
	bool empty;
	int filledVoxels;
	uint8 *data;
	int x, y, z;
	Color color;
};

struct ChunkSet
{
	int currentX, currentY, currentZ;
	Chunk** loaded;
	Chunk** free;
};

struct GenContext
{
	uint64 seed;
};

struct World
{
	GenContext gen;
	ChunkSet set;
};

Chunk* createEmptyChunk();
void allocateChunkData(Chunk *chunk);
void freeChunk(Chunk *chunk);
void setChunkCoords(Chunk *chunk, int x, int y, int z);

Chunk* createPerlinChunk(int x, int y, int z);

uint8 chunk_getBlockUnchecked(Chunk *chunk, int x, int y, int z);
uint8 chunk_getBlockChecked(Chunk *chunk, int x, int y, int z);
void chunk_setBlockUnchecked(Chunk *chunk, uint8 val, int x, int y, int z);
void chunk_setBlockChecked(Chunk *chunk, uint8 val, int x, int y, int z);

#endif // _VOX_WORLD_H_
