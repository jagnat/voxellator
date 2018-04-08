#pragma once
#ifndef _VOX_WORLD_H_
#define _VOX_WORLD_H_

#include "vox_noise.h"
#include "vox_platform.h"

#define CHUNK_SIZE 64

struct Chunk
{
	uint8 *data;
	bool hasData;
	bool used;
	bool empty;
	int filledVoxels;
	int x, y, z;
	Color color;

	volatile int generated;
};

enum GenMode
{
	GEN_PERL3D,
	GEN_PERL2D
};

struct GenContext
{
	GenMode mode;
	Perlin3 perlin;
	uint64 seed;
};

#define NUM_ALLOCATED_CHUNKS 512
struct World
{
	GenContext gen;
	Chunk chunkList[NUM_ALLOCATED_CHUNKS];
	int loadedChunks;
	uint8 *dataBlocks;

	// NEW API
	void init(uint64 seed);

	Chunk* getOrCreateChunk(int x, int y, int z);
	void unloadChunkAt(int x, int y, int z);
};

//void initWorld(World *world, uint64 seed);

//Chunk* createEmptyChunk();
//void allocateChunkData(Chunk *chunk);
//void freeChunk(Chunk *chunk);
//void setChunkCoords(Chunk *chunk, int x, int y, int z);

//Chunk* createPerlinChunk(int x, int y, int z);
//void fillPerlinChunk(Chunk *c);

void addPerlinChunkJob(int xc, int yzc, int zc);

uint8 chunk_getBlockUnchecked(Chunk *chunk, int x, int y, int z);
uint8 chunk_getBlockChecked(Chunk *chunk, int x, int y, int z);
void chunk_setBlockUnchecked(Chunk *chunk, uint8 val, int x, int y, int z);
void chunk_setBlockChecked(Chunk *chunk, uint8 val, int x, int y, int z);

#endif // _VOX_WORLD_H_

