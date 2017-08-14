#pragma once
#ifndef _VOX_WORLD_H_
#define _VOX_WORLD_H_

#define CHUNK_SIZE 64

typedef struct
{
	bool empty;
	int filledVoxels;
	uint8 *data;
	int x, y, z;
} Chunk;

typedef struct
{
	int currentX, currentY, currentZ;
	Chunk** loaded;
	Chunk** free;
} ChunkSet;

#endif // _VOX_WORLD_H_

