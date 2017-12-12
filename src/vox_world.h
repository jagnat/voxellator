#pragma once
#ifndef _VOX_WORLD_H_
#define _VOX_WORLD_H_

#include "vox_noise.h"

#if !1
load()
{
	sim->world = createWorld(/* seed, genmode, name goes here maybe*/);
}

update()
{
	for chunk in sim->world.loadedChunks:
		updatesim(chunk, getEntitiesIn(chunk.coords))
	
	if (player->chunkPos != player->prevChunkPos)
		sim->world.updateChunkTargets(player->chunkPos)
	

}

render()
{
	for chunkmesh in sim->renderstate.loadedChunkMeshes
		render(chunkmesh)
}

#endif

#include "vox_platform.h"

#define CHUNK_SIZE 64

struct Chunk
{
	bool hasData;
	bool empty;
	int filledVoxels;
	uint8 *data;
	int x, y, z;
	Color color;

	// For free list
	Chunk* next;
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

#define NUM_ALLOCATED_CHUNKS 32
struct World
{
	int centerX, centerY, centerZ;
	GenContext gen;
	Chunk chunkList[NUM_ALLOCATED_CHUNKS];
	Chunk *freeChunks;
	uint8 *dataBlocks;
};

void initWorld(World *world, uint64 seed);

Chunk* createEmptyChunk();
void allocateChunkData(Chunk *chunk);
void freeChunk(Chunk *chunk);
void setChunkCoords(Chunk *chunk, int x, int y, int z);

Chunk* createPerlinChunk(int x, int y, int z);
void fillPerlinChunk(Chunk *c);

void addPerlinChunkJob(int xc, int yzc, int zc);

uint8 chunk_getBlockUnchecked(Chunk *chunk, int x, int y, int z);
uint8 chunk_getBlockChecked(Chunk *chunk, int x, int y, int z);
void chunk_setBlockUnchecked(Chunk *chunk, uint8 val, int x, int y, int z);
void chunk_setBlockChecked(Chunk *chunk, uint8 val, int x, int y, int z);

#endif // _VOX_WORLD_H_

