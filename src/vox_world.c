#include "vox_world.h"
#include "vox_render.h"

#include "vox_noise.h"

World *world;

// World function declarations
void removeChunkFromList(ChunkEntry **list, ChunkEntry *entry);
void addChunkToList(ChunkEntry **list, ChunkEntry *entry);

#if 0
void initWorld(World *wld, uint64 seed)
{
	world = wld;
	wld->gen.seed = seed;
	wld->gen.mode = GEN_PERL2D;
	seedPerlin3(&wld->gen.perlin, wld->gen.seed);

	int realSize = CHUNK_SIZE + 2;
	int chunkDataStride = sizeof(uint8) * realSize * realSize * realSize;
	wld->dataBlocks = (uint8*)calloc(NUM_ALLOCATED_CHUNKS, chunkDataStride);

	for (int i = 0; i < NUM_ALLOCATED_CHUNKS - 1; i++)
	{
		wld->chunkList[i].data = &wld->dataBlocks[i * chunkDataStride];
	}
	wld->chunkList[NUM_ALLOCATED_CHUNKS - 1].data =
		&wld->dataBlocks[(NUM_ALLOCATED_CHUNKS - 1) * chunkDataStride];
}
#endif

bool coordsEqual(Chunk *c, int x, int y, int z)
{
	return c->x == x && c->y == y && c->z == z;
}
void setCoords(Chunk *c, int x, int y, int z)
{
	c->x = x; c->y = y; c->z = z;
}

static int chunkCoordHash(int x, int y, int z)
{
	return (x * 8081 + y * 16703) ^ (z * 28057);
}

void initWorld(World *wld, uint64 seed)
{
	world = wld;
	world->gen.seed = seed;
	world->gen.mode = GEN_PERL3D;
	seedPerlin3(&world->gen.perlin, seed);

	int realSize = CHUNK_SIZE + 2;
	int chunkStride = sizeof(uint8) * realSize * realSize * realSize;
}

void updateWorld()
{
	ChunkEntry *loadingPtr = world->loadingChunks;
	// Iterate through currently loading chunks
	while (loadingPtr)
	{
		ChunkEntry *current = loadingPtr;
		loadingPtr = loadingPtr->next;
		if (current->chunk.generated)
		{
			addGreedyJob(&current->chunk);
			removeChunkFromList(&world->loadingChunks, current);
			addChunkToList(&world->loadedChunks, current);
		}
	}
}

int chunkInRange(Chunk *c)
{
	int x = c->x, y = c->y, z = c->z;
	if (x > world->cx + LOADED_RAD || x < world->cx - LOADED_RAD ||
		y > world->cy + LOADED_RAD || y < world->cy - LOADED_RAD ||
		z > world->cz + LOADED_RAD || z < world->cz - LOADED_RAD)
		return 0;
	return 1;
}

int linearProbe(int x, int y, int z, int* firstEmpty)
{
	int raw = chunkCoordHash(x, y, z) % CHUNK_TABLE_LEN;
	ChunkEntry *ct = world->chunkTable;
	// Check if chunk is at first index
	if (ct[raw].used && coordsEqual(&ct[raw].chunk, x, y, z))
	{
		if (firstEmpty)
			*firstEmpty = -1;
		return raw;
	}

	int index = (raw + 1) % CHUNK_TABLE_LEN;
	int firstEmptyIndex = -1;
	bool found = false;
	while (ct[index].dirty && index != raw)
	{
		if (firstEmptyIndex == -1 && !ct[index].used)
			firstEmptyIndex = index;

		if (ct[index].used && coordsEqual(&ct[index].chunk, x, y, z))
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

void loadChunk(int x, int y, int z)
{
	ChunkEntry *ct = world->chunkTable;
	int firstEmptyIndex = -1;
	int index = linearProbe(x, y, z, &firstEmptyIndex);

	if (index != -1)
		return;

	if (firstEmptyIndex == -1)
	{
		printf("Error: Chunk table out of space!\n");
		return;
	}

	memset(&ct[firstEmptyIndex].chunk, 0, sizeof(Chunk));
	setCoords(&ct[firstEmptyIndex].chunk, x, y, z);

	addChunkToList(&world->loadingChunks, &ct[firstEmptyIndex]);
	addPerlinChunkJob(&ct[firstEmptyIndex].chunk);

	ct[firstEmptyIndex].dirty = true;
	ct[firstEmptyIndex].used = true;
}

void unloadChunkAt(int x, int y, int z)
{
	ChunkEntry *ct = world->chunkTable;
	int index = linearProbe(x, y, z, NULL);
	if (index == -1)
		return;

	ct[index].used = false;
	// TODO: BAD BAD BAD - REUSE CHUNK DATA
	if (!ct[index].chunk.empty)
	{
		free(ct[index].chunk.data);
	}

	// TODO: BAD BAD BAD - REUSE MESH
	if (ct[index].chunk.hasMesh)
	{
		deleteChunkMesh(ct[index].chunk.mesh);
	}

	removeChunkFromList(&world->loadingChunks, ct + index);
	removeChunkFromList(&world->loadedChunks, ct + index);
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

void addChunkToList(ChunkEntry **list, ChunkEntry *entry)
{
	ChunkEntry *p = *list;
	if (!p)
	{
		*list = entry;
		entry->next = 0;
		entry->prev = 0;
		return;
	}

	p->prev = entry;
	entry->next = p;
	entry->prev = NULL;
	*list = entry;
}

void removeChunkFromList(ChunkEntry **list, ChunkEntry *entry)
{
	if (entry == *list)
	{
		*list = entry->next;
		return;
	}
	if (entry->prev)
		entry->prev->next = entry->next;
	if (entry->next)
		entry->next->prev = entry->prev;
}

int chunk_3Dto1D(int x, int y, int z)
{ return (x + 1) + (CHUNK_SIZE + 2) * ((z + 1) + (y + 1) * (CHUNK_SIZE + 2)); }

bool chunk_inChunk(int x, int y, int z)
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
					if (chunk_inChunk(x, y, z))
						c->filledVoxels++;
				}
#else
				float p = perlin3(&perl, (xc + x) / 30.0, 298.3219f, (zc + z) / 30.0);
				p = (p + 1) / 2;
				if ((double)((yc + y) / 120.0f) < p)
				// if (1)
				{
					chunk_setBlockUnchecked(c, 255, x, y, z);
					if (chunk_inChunk(x, y, z))
						c->filledVoxels++;
				}
#endif
			}

	c->generated = 1;
	if (c->filledVoxels == 0)
		c->empty = true;
}


uint8 chunk_getBlockUnchecked(Chunk *chunk, int x, int y, int z)
{ return chunk->data[chunk_3Dto1D(x, y, z)]; }

uint8 chunk_getBlockChecked(Chunk *chunk, int x, int y, int z)
{
	if (x < -1 || x > CHUNK_SIZE + 1 || y < -1 || y > CHUNK_SIZE + 1 || z < -1 || z > CHUNK_SIZE + 1)
		return 0;
	return chunk->data[chunk_3Dto1D(x, y, z)];
}

void chunk_setBlockUnchecked(Chunk *chunk, uint8 val, int x, int y, int z)
{ chunk->data[chunk_3Dto1D(x, y, z)] = val; }

void chunk_setBlockChecked(Chunk *chunk, uint8 val, int x, int y, int z)
{
	if (x < -1 || x > CHUNK_SIZE + 1 || y < -1 || y > CHUNK_SIZE + 1 || z < -1 || z > CHUNK_SIZE + 1)
		return;
	chunk->data[chunk_3Dto1D(x, y, z)] = val;
}
