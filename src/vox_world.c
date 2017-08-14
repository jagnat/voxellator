#include "vox_world.h"

#include "vox_noise.h"

Chunk* createEmptyChunk()
{
	Chunk *c = calloc(1, sizeof(Chunk));
	c->empty = true;
	return c;
}

void allocateChunkData(Chunk *chunk)
{
	if (chunk->data)
		return;
	// TODO: error checking
	chunk->data = calloc(1, sizeof(uint8) * CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);
	chunk->empty = false;
}

void freeChunk(Chunk *chunk)
{
	if (!chunk)
		return;
	if (!chunk->empty)
		free(chunk->data);
	free(chunk);
}

void setChunkCoords(Chunk *chunk, int x, int y, int z)
{ chunk->x = x; chunk->y = y; chunk->z = z; }

// TODO: Use a generation context instead of a hardcoded seed
Chunk* createPerlinChunk(int xc, int yc, int zc)
{
	Chunk *c = createEmptyChunk();
	allocateChunkData(c);
	setChunkCoords(c, xc, yc, zc);

	seedPerlin3(429579272525743);
	xc *= CHUNK_SIZE; yc *= CHUNK_SIZE; zc *= CHUNK_SIZE;
	for (int x = 0; x < CHUNK_SIZE; x++)
		for (int y = 0; y < CHUNK_SIZE; y++)
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				float p = perlin3((xc + x) / 30.0, (yc + y) / 30.0, (zc + z) / 30.0);
				if (p > 0)
				{
					chunk_setBlockUnchecked(c, 255, x, y, z);
					c->filledVoxels++;
				}
			}
	
	return c;
}

uint8 chunk_getBlockUnchecked(Chunk *chunk, int x, int y, int z)
{ return chunk->data[x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE]; }


uint8 chunk_GetBlockChecked(Chunk *chunk, int x, int y, int z)
{
	if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE)
		return 0;
	else
		return chunk->data[x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE];
}

void chunk_setBlockUnchecked(Chunk *chunk, uint8 val, int x, int y, int z)
{ chunk->data[x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE] = val; }

void chunk_setBlockChecked(Chunk *chunk, uint8 val, int x, int y, int z)
{
	if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE)
		return;
	chunk->data[x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE] = val;
}

