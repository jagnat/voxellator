#include "world.h"
#include "render.h"
#include "jobs.h"

#include "noise.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define CHUNK_POOL_SIZE 1024

typedef struct
{
	GenContext gen;
	Chunk chunk_pool[CHUNK_POOL_SIZE];
	Chunk *free_list;
	Chunk *used_list;
	int cx, cy, cz; // Chunk coords to be centered around
} World;

World *world;

const uint8 face_pos_table[] = 
{
0, 0, 0, // -x
0, 1, 0,
0, 1, 1,
0, 0, 1,

1, 0, 1, // +x
1, 1, 1,
1, 1, 0,
1, 0, 0,

0, 0, 0, // -y
0, 0, 1,
1, 0, 1,
1, 0, 0,

0, 1, 0, // +y
1, 1, 0,
1, 1, 1,
0, 1, 1,

1, 0, 0, // -z
1, 1, 0,
0, 1, 0,
0, 0, 0,

0, 0, 1, // + z
0, 1, 1,
1, 1, 1,
1, 0, 1
};

const uint face_norm_table[] = {
	0x200, // -x
	0x1ff, // +x
	0x80000, // -y
	0x7fc00, // +y
	0x20000000, // -z
	0x1FF00000  // +z
};

const int neighbor_coord_table[6][3] = {{-1, 0, 0}, {1, 0, 0},
							{0, -1, 0}, {0, 1, 0},
							{0, 0, -1}, {0, 0, 1}};

void add_chunk_to_list(Chunk **list, Chunk *chunk);
void remove_chunk_from_list(Chunk **list, Chunk *chunk);

Chunk* allocate_chunk();
void free_chunk(Chunk *chunk);
Chunk* find_chunk(int cx, int cy, int cz);

// Returns the # of chunk neighbors that exist
int populate_chunk_neighbors(Chunk *chunk);

void gen_perlin_chunk(Chunk *c);
int mesh_chunk_culled(Chunk *chunk);

#define CHUNKS_EDGE_LEN 10

void init_world(uint64 seed)
{
	world = calloc(1, sizeof(World));
	assert(world);
	world->gen.mode = GEN_PERL3D;
	seed_noise(&world->gen.noise, seed);

	// Set up chunk free list
	for (int i = 0; i < CHUNK_POOL_SIZE; i++)
	{
		add_chunk_to_list(&world->free_list, world->chunk_pool + i);
	}

	for (int i = 0; i < CHUNKS_EDGE_LEN; i++)
	for (int j = 0; j < CHUNKS_EDGE_LEN; j++)
	for (int k = 0; k < CHUNKS_EDGE_LEN; k++)
	{
		Chunk *c = allocate_chunk();
		c->x = i;
		c->y = k - 1;
		c->z = j;
	}
	printf("Chunk size is %zu\n", sizeof(Chunk));
}

void update_world()
{
	// Check and queue if bounds requires any new chunk loading/unloading

	// Walk through chunk list and:
	// If chunk is not loaded, load it
	// If chunk is not meshed, mesh it
	// 

	Chunk *it = world->used_list;
	int performed_ops = 0;
	while (it)
	{
		if (it->generated == 0 && performed_ops < 1)
		{
			gen_perlin_chunk(it);
			break;
		}
		else
		{
			if (it->meshed == 0 && performed_ops < 1)
			{
				if (mesh_chunk_culled(it))
					break;
			}
			else
			{
				if (!it->empty)
					renderChunkMesh(it->mesh);
			}
		}
		
		it = it->next;
	}
}

void add_chunk_to_list(Chunk **list, Chunk *chunk)
{
	chunk->next = *list;
	*list = chunk;
}

void remove_chunk_from_list(Chunk **list, Chunk *chunk)
{
	if (!list || !*list) return;

	if (chunk == *list)
	{
		*list = chunk->next;
		chunk->next = NULL;
		return;
	}

	Chunk *prev_ptr = NULL, *it = *list;
	while (it)
	{
		if (it == chunk)
		{
			prev_ptr->next = chunk->next;
			return;
		}
		
		prev_ptr = it;
		it = it->next;
	}
}

Chunk* allocate_chunk()
{
	Chunk *alloc = world->free_list;
	if (alloc)
	{
		world->free_list = alloc->next;
		add_chunk_to_list(&world->used_list, alloc);
		alloc->in_use = true;
	}
	return alloc;
}

void free_chunk(Chunk *chunk)
{
	remove_chunk_from_list(&world->used_list, chunk);
	if (chunk->mesh)
		deleteChunkMesh(chunk->mesh);
	chunk->mesh = NULL;
	chunk->in_use = 0;
	chunk->generated = 0;
	chunk->meshed = 0;
	for (int i = 0; i < 6; i++)
	{
		chunk->neighbors[i] = 0;
	}
	add_chunk_to_list(&world->free_list, chunk);
}

Chunk* find_chunk(int cx, int cy, int cz)
{
	Chunk *it = world->used_list;

	while (true)
	{
		if (!it) return NULL;

		if (it->x == cx && it->y == cy && it->z == cz)
			return it;
		
		it = it->next;
	}
}

int populate_chunk_neighbors(Chunk *chunk)
{
	int pop_count = 0;
	for (int i = 0; i < 6; i++)
	{
		if (chunk->neighbors[i])
			pop_count++;
		else
		{
			chunk->neighbors[i] = find_chunk(
				chunk->x + neighbor_coord_table[i][0],
				chunk->y + neighbor_coord_table[i][1],
				chunk->z + neighbor_coord_table[i][2]);
			if (chunk->neighbors[i]) pop_count++;
		}
	}
	return pop_count;
}

int chunk_has_generated_neighbors(Chunk *chunk)
{
	for (int i = 0; i < 6; i++)
	{
		if (!chunk->neighbors[i] || chunk->neighbors[i]->generated == 0)
			return false;
	}
	return true;
}

void gen_perlin_chunk(Chunk *c)
{
	int xc = CHUNK_SIZE * c->x, yc = CHUNK_SIZE * c->y, zc = CHUNK_SIZE * c->z;
	double start_time = get_elapsed_ms();

	for (int z = 0; z < CHUNK_SIZE; z++)
	for (int x = 0; x < CHUNK_SIZE; x++)
	{
		const float scale = 60.0f;
		float p = perlin3(&world->gen.noise, (xc + x) / scale, 298.3219f, (zc + z) / scale);
		p = (p + 1) / 2; // 0 to 1
		int yh = 2 * CHUNK_SIZE * p;

		for (int y = 0; y < CHUNK_SIZE; y++)
		{
			if (y + yc < yh)
			{
				chunk_set_voxel(c, VOXEL_DIRT, x, y, z);
				c->set_voxel_count++;
			}
		}
	}

	c->generated = 1;
	if (c->set_voxel_count == 0)
		c->empty = true;
	
	printf("Chunkgen elapsed time: %0.4f ms\n", get_elapsed_ms() - start_time);
}

bool coord_in_chunk_bounds(Chunk *chunk, int x, int y, int z)
{ return x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE; }

VoxelType chunk_get_voxel_at(Chunk *chunk, int x, int y, int z)
{ return (VoxelType)chunk->voxels[CHUNK_COORD_TO_INDEX(x, y, z)]; }

void chunk_set_voxel(Chunk *chunk, VoxelType val, int x, int y, int z)
{ chunk->voxels[CHUNK_COORD_TO_INDEX(x, y, z)] = (uint8)val; }

VertexColorNormal10* add_face(int face, VertexColorNormal10 *at, int x, int y, int z, Color c)
{
	uint norm = face_norm_table[face];
	int faceOffs = face * 12;
	VertexColorNormal10 *v = at;
	for (int i = 0; i < 12; i += 3)
	{
		v->color = c;
		v->normal = face_norm_table[face];
		v->x = x + face_pos_table[faceOffs + i + 0];
		v->y = y + face_pos_table[faceOffs + i + 1];
		v->z = z + face_pos_table[faceOffs + i + 2];
		v->w = 1;
		v++;
	}
	return v;
}

int mesh_chunk_culled(Chunk *chunk)
{
	if (chunk->empty)
	{
		chunk->meshed = true;
		return 1;
	}
	if (!chunk->mesh)
	{
		chunk->mesh = createChunkMesh();
	}
	ChunkMesh *mesh = chunk->mesh;
	double startTime = get_elapsed_ms();

	// Check if all neighbors exist
	if (populate_chunk_neighbors(chunk) != 6 || !chunk_has_generated_neighbors(chunk))
		return 0;
	
	int allocatedVerts = 4096;
	VertexColorNormal10 *vertices = (VertexColorNormal10*)malloc(allocatedVerts * sizeof(VertexColorNormal10));
	VertexColorNormal10 *walk = vertices;

	Color voxel_color = {113, 97, 199, 255};

	const int cb = CHUNK_SIZE - 1;

	// // -x, +x planes
	// for (int z = 0; z < CHUNK_SIZE; z++)
	// for (int y = 0; y < CHUNK_SIZE; y++)
	// {
	// 	if (chunk_get_voxel_at(chunk, 0, y, z) != VOXEL_AIR &&
	// 		chunk_get_voxel_at(chunk->neighbors[0], cb, y, z) == VOXEL_AIR)
	// 		walk = add_face(0, walk, 0, y, z, voxel_color);
	// 	if (chunk_get_voxel_at(chunk, cb, y, z) != VOXEL_AIR &&
	// 		chunk_get_voxel_at(chunk->neighbors[1], 0, y, z) == VOXEL_AIR)
	// 		walk = add_face(1, walk, cb, y, z, voxel_color);
	// }

	// // -y, +y planes
	// for (int z = 0; z < CHUNK_SIZE; z++)
	// for (int x = 0; x < CHUNK_SIZE; x++)
	// {
	// 	if (chunk_get_voxel_at(chunk, x, 0, z) != VOXEL_AIR &&
	// 		chunk_get_voxel_at(chunk->neighbors[2], x, cb, z) == VOXEL_AIR)
	// 		walk = add_face(2, walk, x, 0, z, voxel_color);
	// 	if (chunk_get_voxel_at(chunk, x, cb, z) != VOXEL_AIR &&
	// 		chunk_get_voxel_at(chunk->neighbors[3], x, 0, z) == VOXEL_AIR)
	// 		walk = add_face(1, walk, x, cb, z, voxel_color);
	// }


	// Interior of chunk
	for (int z = 1; z < CHUNK_SIZE - 1; z++)
	for (int y = 1; y < CHUNK_SIZE - 1; y++)
	for (int x = 1; x < CHUNK_SIZE - 1; x++)
	{
		if ((walk - vertices) + 6 * 4 * sizeof(VertexColorNormal10) > allocatedVerts)
		{
			if (allocatedVerts * 2 >= 67108864)
			{
				printf("Vertex allocation exceeds bounds! Exiting...\n");
				exit(120);
			}
			allocatedVerts *= 2;
			int currentIndex = walk - vertices;
			vertices = (VertexColorNormal10*)realloc(vertices, allocatedVerts * sizeof(VertexColorNormal10));
			walk = vertices + currentIndex;
		}

		if (chunk_get_voxel_at(chunk, x, y, z) != VOXEL_AIR)
		{
			if (chunk_get_voxel_at(chunk, x - 1, y, z) == VOXEL_AIR)
				walk = add_face(0, walk, x, y, z, voxel_color);
			if (chunk_get_voxel_at(chunk, x + 1, y, z) == VOXEL_AIR)
				walk = add_face(1, walk, x, y, z, voxel_color);
			if (chunk_get_voxel_at(chunk, x, y - 1, z) == VOXEL_AIR)
				walk = add_face(2, walk, x, y, z, voxel_color);
			if (chunk_get_voxel_at(chunk, x, y + 1, z) == VOXEL_AIR)
				walk = add_face(3, walk, x, y, z, voxel_color);
			if (chunk_get_voxel_at(chunk, x, y, z - 1) == VOXEL_AIR)
				walk = add_face(4, walk, x, y, z, voxel_color);
			if (chunk_get_voxel_at(chunk, x, y, z + 1) == VOXEL_AIR)
				walk = add_face(5, walk, x, y, z, voxel_color);
		}
	}

	mesh->modelMatrix = JMat4_Translate(chunk->x * CHUNK_SIZE, chunk->y * CHUNK_SIZE, chunk->z * CHUNK_SIZE);

	mesh->vertices = vertices;

	mesh->indexMode = INDEX_QUADS;

	mesh->usedVertices = walk - vertices;
	mesh->allocatedVertices = allocatedVerts;
	mesh->numIndices = (mesh->usedVertices / 4) * 6;

	uploadChunkMesh(mesh);

	chunk->meshed = 1;

	double elapsedTime = get_elapsed_ms() - startTime;
	printf("Culled: Chunk %d,%d,%d took %0.4f ms\n", chunk->x, chunk->y, chunk->z, elapsedTime);
	return 1;
}
