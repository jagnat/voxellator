#pragma once
#ifndef _VOX_WORLD_H_
#define _VOX_WORLD_H_

#include "vox_noise.h"
#include "vox_platform.h"

#define CHUNK_SIZE 64

struct ChunkMesh;

struct Chunk
{
	uint8 *data;
	bool empty;
	int filledVoxels;
	int x, y, z;
	Color color;

	bool hasMesh;
	ChunkMesh *mesh;

	volatile int generated;

	bool coordsEqual(int x, int y, int z);
	void setCoords(int x, int y, int z);
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

struct ChunkEntry
{
	Chunk chunk;
	bool dirty; // Chunk has been used
	bool used; // Chunk is currently in use
	ChunkEntry *next, *prev;
};

#define CHUNK_TABLE_LEN 128
struct World
{
	GenContext gen;
	ChunkEntry chunkTable[CHUNK_TABLE_LEN] = {};

	ChunkEntry *loadingChunks;
	ChunkEntry *loadedChunks;

	// NEW API
	void init(uint64 seed);
	void update();

	Chunk* getOrCreateChunk(int x, int y, int z);
	void unloadChunkAt(int x, int y, int z);

private:
	int linearProbe(int x, int y, int z, int* firstEmpty);

	void addChunkToList(ChunkEntry **list, ChunkEntry *entry);
	void removeChunkFromList(ChunkEntry **list, ChunkEntry *entry);
};

//void initWorld(World *world, uint64 seed);

//Chunk* createEmptyChunk();
void allocateChunkData(Chunk *chunk);
//void freeChunk(Chunk *chunk);
//void setChunkCoords(Chunk *chunk, int x, int y, int z);

//Chunk* createPerlinChunk(int x, int y, int z);
void fillPerlinChunk(Chunk *c);

void addPerlinChunkJob(Chunk *c);

uint8 chunk_getBlockUnchecked(Chunk *chunk, int x, int y, int z);
uint8 chunk_getBlockChecked(Chunk *chunk, int x, int y, int z);
void chunk_setBlockUnchecked(Chunk *chunk, uint8 val, int x, int y, int z);
void chunk_setBlockChecked(Chunk *chunk, uint8 val, int x, int y, int z);

#endif // _VOX_WORLD_H_
