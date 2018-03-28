#include "vox_world.h"

#include "vox_noise.h"

void initWorld(World *wld, uint64 seed)
{
	wld->gen.seed = seed;
	wld->gen.mode = GEN_PERL2D;
	seedPerlin3(&wld->gen.perlin, wld->gen.seed);

	int realSize = CHUNK_SIZE + 2;
	int chunkDataStride = sizeof(uint8) * realSize * realSize * realSize;
	wld->dataBlocks = (uint8*)calloc(NUM_ALLOCATED_CHUNKS, chunkDataStride);

	// Init chunk free list
	wld->freeChunks = wld->chunkList;
	wld->chunkList[NUM_ALLOCATED_CHUNKS - 1].next = 0; // Might be unnecessary, because of calloc
	for (int i = 0; i < NUM_ALLOCATED_CHUNKS - 1; i++)
	{
		wld->chunkList[i].next = wld->chunkList + i + 1;
		wld->chunkList[i].data = &wld->dataBlocks[i * chunkDataStride];
	}
	wld->chunkList[NUM_ALLOCATED_CHUNKS - 1].data =
		&wld->dataBlocks[(NUM_ALLOCATED_CHUNKS - 1) * chunkDataStride];
}

Chunk* createEmptyChunk()
{
	Chunk *c = (Chunk*)calloc(1, sizeof(Chunk));
	c->empty = true;
	return c;
}

void allocateChunkData(Chunk *chunk)
{
	if (chunk->data)
		return;
	// TODO: error checking
	int realSize = CHUNK_SIZE + 2;
	chunk->data = (uint8*)calloc(1, sizeof(uint8) * realSize * realSize * realSize);
	chunk->empty = false;
}

void freeChunk(Chunk *chunk)
{
	if (!chunk)
		return;
	if (!chunk->empty)
	{
		free(chunk->data);
		chunk->data = (uint8*)0;
	}
	free(chunk);
}

inline int chunk__3Dto1D(int x, int y, int z)
{ return (x + 1) + (CHUNK_SIZE + 2) * ((z + 1) + (y + 1) * (CHUNK_SIZE + 2)); }

inline bool chunk__inChunk(int x, int y, int z)
{ return x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE; }

void setChunkCoords(Chunk *chunk, int x, int y, int z)
{ chunk->x = x; chunk->y = y; chunk->z = z; }

void createPerlinChunkJobProc(void *args)
{
	fillPerlinChunk((Chunk*)args);
}

void createPerlinChunkJobCompletion(void *args)
{
	if (!((Chunk*)args)->empty)
		addGreedyJob((Chunk*)args);
}

void addPerlinChunkJob(int xc, int yc, int zc)
{
	// TODO: Pull chunk from free list instead
	Chunk *c = createEmptyChunk();
	Color col = {80, 50, 100, 255};
	c->color = col;
	allocateChunkData(c);
	setChunkCoords(c, xc, yc, zc);
	Job job = {};
	job.jobProc = createPerlinChunkJobProc;
	//job.completionProc = createPerlinChunkJobCompletion;
	job.args = c;
	addJob(job);
}

//#define USE_3D_PERLIN

void fillPerlinChunk(Chunk *c)
{
	// TODO: DON'T DO THIS EVERY CHUNK
	Perlin3 perl;
	seedPerlin3(&perl, 420895928332);

	int xc = CHUNK_SIZE * c->x, yc = CHUNK_SIZE * c->y, zc = CHUNK_SIZE * c->z;
	for (int x = -1; x < CHUNK_SIZE + 1; x++)
		for (int z = -1; z < CHUNK_SIZE + 1; z++)
			for (int y = -1; y < CHUNK_SIZE + 1; y++)
			{
#ifdef USE_3D_PERLIN
				float p = perlin3(&perl, (xc + x) / 30.0, (yc + y) / 30.0, (zc + z) / 30.0);
				if (p > 0)
				{
					chunk_setBlockUnchecked(c, 255, x, y, z);
					if (chunk__inChunk(x, y, z))
						c->filledVoxels++;
				}
#else
				float p = perlin3(&perl, (xc + x) / 30.0, 298.3219f, (zc + z) / 30.0);
				p = (p + 1) / 2;
				if ((double)((yc + y) / 120.0f) < p)
				{
					chunk_setBlockUnchecked(c, 255, x, y, z);
					if (chunk__inChunk(x, y, z))
						c->filledVoxels++;
				}
#endif
			}
}

Chunk* createPerlinChunk(int xc, int yc, int zc)
{
	Chunk *c = createEmptyChunk();
	allocateChunkData(c);
	setChunkCoords(c, xc, yc, zc);

	fillPerlinChunk(c);
	
	return c;
}


uint8 chunk_getBlockUnchecked(Chunk *chunk, int x, int y, int z)
{ return chunk->data[chunk__3Dto1D(x, y, z)]; }


uint8 chunk_getBlockChecked(Chunk *chunk, int x, int y, int z)
{
	if (x < -1 || x > CHUNK_SIZE + 1 || y < -1 || y > CHUNK_SIZE + 1 || z < -1 || z > CHUNK_SIZE + 1)
		return 0;
	return chunk->data[chunk__3Dto1D(x, y, z)];
}

void chunk_setBlockUnchecked(Chunk *chunk, uint8 val, int x, int y, int z)
{ chunk->data[chunk__3Dto1D(x, y, z)] = val; }

void chunk_setBlockChecked(Chunk *chunk, uint8 val, int x, int y, int z)
{
	if (x < -1 || x > CHUNK_SIZE + 1 || y < -1 || y > CHUNK_SIZE + 1 || z < -1 || z > CHUNK_SIZE + 1)
		return;
	chunk->data[chunk__3Dto1D(x, y, z)] = val;
}

