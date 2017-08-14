#include "vox_mesher.h"

typedef struct
{
	int16 x, y, z, w;
	Color color;
	uint normal;
	uint8 sideMask; // bit 0=x-, 1=x+, 2=y-, etc.
	int numFaces;
	VertexColorNormal10 *current;
} MeshBuildContext;

uint8 facePosTable[] = 
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

uint faceNormTable[] = {
	0x200, // -x
	0x1ff, // +x
	0x80000, // -y
	0x7fc00, // +y
	0x20000000, // -z
	0x1FF00000  // +z
};

void addVertex(MeshBuildContext *context)
{
	context->current->color = context->color;
	context->current->normal = context->normal;

	context->current->x = context->x;
	context->current->y = context->y;
	context->current->z = context->z;
	context->current->w = context->w;
	context->current++;
}

void addFace(int face, MeshBuildContext *context)
{
	for (int i = 0; i < 4; i++)
	{
		VertexColorNormal10 *v = context->current;
		v->color = context->color;
		v->normal = faceNormTable[face];
		v->x = context->x + facePosTable[face * 12 + i * 3 + 0];
		v->y = context->y + facePosTable[face * 12 + i * 3 + 1];
		v->z = context->z + facePosTable[face * 12 + i * 3 + 2];
		context->current++;
	}
	context->numFaces++;
}

void addCube(MeshBuildContext *context)
{
	int16 x = context->x, y = context->y, z = context->z;

	for (int i = 0; i < 24; i++)
	{
		context->normal = faceNormTable[i/4];
		context->x = x + facePosTable[i * 3 + 0];
		context->y = y + facePosTable[i * 3 + 1];
		context->z = z + facePosTable[i * 3 + 2];
		context->w = 0;
		addVertex(context);
	}
}

void meshVanillaNaive(Chunk *chunk, ChunkMesh *mesh)
{
	mesh->indexMode = INDEX_QUADS;
	mesh->usedVertices = chunk->filledVoxels * 24;
	mesh->numIndices = chunk->filledVoxels * 36;

	MeshBuildContext context = {0};
	context.current = mesh->vertices;

	context.color.g=context.color.a=255;

	for (int x = 0; x < CHUNK_SIZE; x++)
		for (int y = 0; y < CHUNK_SIZE; y++)
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				context.x = x; context.y = y; context.z = z;
				if (chunk_getBlockUnchecked(chunk, x, y, z))
					addCube(&context);
			}
}

void meshVanillaCull(Chunk *chunk, ChunkMesh *mesh)
{
	mesh->indexMode = INDEX_QUADS;

	MeshBuildContext context = {0};
	context.current = mesh->vertices;
	context.color.b=context.color.a=255;

	for (int x = 0; x < CHUNK_SIZE; x++)
		for (int y = 0; y < CHUNK_SIZE; y++)
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				context.x = x; context.y = y; context.z = z;
				if (chunk_getBlockUnchecked(chunk, x, y, z))
				{
					if (!chunk_GetBlockChecked(chunk, x - 1, y, z))
						addFace(0, &context);
					if (!chunk_GetBlockChecked(chunk, x + 1, y, z))
						addFace(1, &context);
					if (!chunk_GetBlockChecked(chunk, x, y - 1, z))
						addFace(2, &context);
					if (!chunk_GetBlockChecked(chunk, x, y + 1, z))
						addFace(3, &context);
					if (!chunk_GetBlockChecked(chunk, x, y, z - 1))
						addFace(4, &context);
					if (!chunk_GetBlockChecked(chunk, x, y, z + 1))
						addFace(5, &context);
				}
			}

	mesh->usedVertices = context.numFaces * 4;
	mesh->numIndices = context.numFaces * 6;
}

void meshVanillaGreedy(Chunk *chunk, ChunkMesh *mesh)
{
}

