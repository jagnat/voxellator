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
	int filledVoxels;
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

typedef struct ChunkEntry
{
	Chunk chunk;
	int dirty; // Chunk has been used
	int used; // Chunk is currently in use
	struct ChunkEntry *next, *prev;
} ChunkEntry;

#define CHUNK_TABLE_LEN 128
#define LOADED_RAD 2
typedef struct
{
	GenContext gen;
	ChunkEntry chunkTable[CHUNK_TABLE_LEN];

	ChunkEntry *loadingChunks;
	ChunkEntry *loadedChunks;

	int cx, cy, cz; // Chunk coords to be centered around

} World;

void initWorld(World *world, uint64 seed);
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
