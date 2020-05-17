#pragma once

#include "noise.h"
#include "platform.h"
#include "render.h"
#include "thirdparty/j_threedee.h"

#define CHUNK_SIZE 64
#define NUM_VOXELS_IN_CHUNK (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)

typedef enum
{
	GEN_PERL3D,
	GEN_PERL2D
} GenMode;

typedef struct
{
	GenMode mode;
	NoiseSapling noise;
} GenContext;

// Voxel definitions - may only go up to 255
typedef enum
{
	VOXEL_AIR = 0,
	VOXEL_DIRT = 1,
	VOXEL_GRASS = 2,
	VOXEL_STONE = 3,
} VoxelType;

typedef struct _Chunk Chunk;
struct _Chunk
{
	int in_use;
	int x, y, z;
	int empty;
	int set_voxel_count;
	volatile int generated;
	volatile int meshed;
	Chunk *next;
	ChunkMesh *mesh;
	Chunk* neighbors[6]; // +x, -x, +y, -y, +z, -z
	uint8 voxels[NUM_VOXELS_IN_CHUNK];
};

#define CHUNK_COORD_TO_INDEX(x, y, z) ((x) + CHUNK_SIZE * ((z) + (y) * CHUNK_SIZE))

#define FOR_EACH_VOXEL_IN_CHUNK(vx, vy, vz) for(int vz = 0; vz < CHUNK_SIZE; vz++) \
for(int vy = 0; vy < CHUNK_SIZE; vy++) \
for (int vx = 0; vx < CHUNK_SIZE; vx++)

bool coord_in_chunk_bounds(Chunk *chunk, int x, int y, int z);
VoxelType chunk_get_voxel_at(Chunk *chunk, int x, int y, int z);
void chunk_set_voxel(Chunk *chunk, VoxelType val, int x, int y, int z);

void init_world(uint64 seed);
void update_world(JVec3 player_loc);
