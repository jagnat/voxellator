#include "vox_world.h"

#include "vox_noise.h"

#if 0
void initWorld(World *wld, uint64 seed)
{
	wld->gen.seed = seed;
	wld->gen.mode = GEN_PERL2D;
	seedPerlin3(&wld->gen.perlin, wld->gen.seed);

	int realSize = CHUNK_SIZE + 2;
	int chunkDataStride = sizeof(uint8) * realSize * realSize * realSize;
	wld->dataBlocks = (uint8*)calloc(NUM_ALLOCATED_CHUNKS, chunkDataStride);

	// Init chunk free list
	//wld->freeChunks = wld->chunkList;
	//wld->chunkList[NUM_ALLOCATED_CHUNKS - 1].next = 0; // Might be unnecessary, because of calloc
	for (int i = 0; i < NUM_ALLOCATED_CHUNKS - 1; i++)
	{
		//wld->chunkList[i].next = wld->chunkList + i + 1;
		wld->chunkList[i].data = &wld->dataBlocks[i * chunkDataStride];
	}
	wld->chunkList[NUM_ALLOCATED_CHUNKS - 1].data =
		&wld->dataBlocks[(NUM_ALLOCATED_CHUNKS - 1) * chunkDataStride];
}
#endif

bool Chunk::coordsEqual(int x, int y, int z)
{
	return this->x == x && this->y == y && this->z == z;
}
void Chunk::setCoords(int x, int y, int z)
{
	this->x = x; this->y = y; this->z = z;
}

static int chunkCoordHash(int x, int y, int z)
{
	return (x * 8081 + y * 16703) ^ (z * 28057);
}

void World::init(uint64 seed)
{
	gen.seed = seed;
	gen.mode = GEN_PERL3D;
	seedPerlin3(&gen.perlin, gen.seed);

	int realSize = CHUNK_SIZE + 2;
	int chunkStride = sizeof(uint8) * realSize * realSize * realSize;
#if 0
	for (int i = 0; i < CHUNK_TABLE_LEN; i++)
	{
		//chunkList[i].data = &dataBlocks[i * chunkStride];
	}
#endif
}

void World::update()
{
	for (int i = 0; i < CHUNK_TABLE_LEN; i++)
	{
		Chunk *c = &chunkTable[i].chunk;
		if (chunkTable[i].used && c->generated && !c->hasMesh)
		{
			c->hasMesh = true;
			addGreedyJob(c);
		}
	}
}

int World::linearProbe(int x, int y, int z, int* firstEmpty)
{
	int raw = chunkCoordHash(x, y, z) % CHUNK_TABLE_LEN;
	// Check if chunk is at first index
	if (chunkTable[raw].used && chunkTable[raw].chunk.coordsEqual(x, y, z))
	{
		if (firstEmpty)
			*firstEmpty = -1;
		return raw;
	}
	int index = (raw + 1) % CHUNK_TABLE_LEN;
	int firstEmptyIndex = -1;
	bool found = false;
	while (chunkTable[index].dirty && index != raw)
	{
		if (firstEmptyIndex == -1 && !chunkTable[index].used)
			firstEmptyIndex = index;

		if (chunkTable[index].used && chunkTable[index].chunk.coordsEqual(x, y, z))
		{
			found = true;
			break;
		}
		index = (index + 1) % CHUNK_TABLE_LEN;
	}
	// Exiting from here cases:
	// 1. chunk found
	// 2. no chunk found
	//   a. empty index found
	//   b. no empty index found
	//     i. reached starting point
	//     ii. current index is empty

	int res = -1;
	if (found)
		res = index;
	else
		if (firstEmptyIndex == -1)
			if (index != raw)
				firstEmptyIndex = index;

	if (firstEmpty)
		*firstEmpty = firstEmptyIndex;

	return res;
}

Chunk* World::getOrCreateChunk(int x, int y, int z)
{
	int firstEmptyIndex = -1;
	int index = linearProbe(x, y, z, &firstEmptyIndex);

	if (index != -1)
		return &chunkTable[index].chunk;

	if (firstEmptyIndex == -1)
	{
		printf("Error: Chunk table out of space!\n");
		return 0;
	}

	memset(&chunkTable[firstEmptyIndex].chunk, 0, sizeof(Chunk));
	chunkTable[firstEmptyIndex].chunk.setCoords(x, y, z);

	addPerlinChunkJob(&chunkTable[firstEmptyIndex].chunk);

	chunkTable[firstEmptyIndex].dirty = true;
	chunkTable[firstEmptyIndex].used = true;

	return &chunkTable[firstEmptyIndex].chunk;
}

void World::unloadChunkAt(int x, int y, int z)
{

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

#if 0
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
#endif

inline int chunk__3Dto1D(int x, int y, int z)
{ return (x + 1) + (CHUNK_SIZE + 2) * ((z + 1) + (y + 1) * (CHUNK_SIZE + 2)); }

inline bool chunk__inChunk(int x, int y, int z)
{ return x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE; }

void fillPerlinChunk(Chunk *c);

void createPerlinChunkJobProc(void *args)
{
	fillPerlinChunk((Chunk*)args);
}

void addPerlinChunkJob(Chunk *c)
{
	Color col = {80, 50, 100, 255};
	c->color = col;
	allocateChunkData(c);
	Job job = {};
	job.jobProc = createPerlinChunkJobProc;
	//job.completionProc = createPerlinChunkJobCompletion;
	job.priority = 100;
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

	c->generated = 1;
}

#if 0
Chunk* createPerlinChunk(int xc, int yc, int zc)
{
	Chunk *c = createEmptyChunk();
	allocateChunkData(c);
	setChunkCoords(c, xc, yc, zc);

	fillPerlinChunk(c);
	
	return c;
}
#endif


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
