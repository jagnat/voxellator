#include "vox_mesher.h"

// TODO: Encode some information in w
typedef struct
{
	int16 x, y, z;
	Color color;
	uint normal;
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
	context->current->w = 1; 
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
		v->w = 1;
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
		addVertex(context);
	}
}

void setModelMatrix(Chunk *chunk, ChunkMesh *mesh)
{
	mesh->modelMatrix = JMat4_Translate(chunk->x * CHUNK_SIZE, chunk->y * CHUNK_SIZE, chunk->z * CHUNK_SIZE);
}

void meshVanillaNaive(Chunk *chunk, ChunkMesh *mesh)
{
	setModelMatrix(chunk, mesh);

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
	setModelMatrix(chunk, mesh);

	mesh->indexMode = INDEX_QUADS;

	MeshBuildContext context = {0};
	context.current = mesh->vertices;
	context.color.b=context.color.a=255;

	for (int x = 0; x < CHUNK_SIZE; x++)
		for (int y = 0; y < CHUNK_SIZE; y++)
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				// TODO: Unroll, use unchecked lookups for extra speed
				context.x = x; context.y = y; context.z = z;
				if (chunk_getBlockUnchecked(chunk, x, y, z))
				{
					if (!chunk_getBlockChecked(chunk, x - 1, y, z))
						addFace(0, &context);
					if (!chunk_getBlockChecked(chunk, x + 1, y, z))
						addFace(1, &context);
					if (!chunk_getBlockChecked(chunk, x, y - 1, z))
						addFace(2, &context);
					if (!chunk_getBlockChecked(chunk, x, y + 1, z))
						addFace(3, &context);
					if (!chunk_getBlockChecked(chunk, x, y, z - 1))
						addFace(4, &context);
					if (!chunk_getBlockChecked(chunk, x, y, z + 1))
						addFace(5, &context);
				}
			}

	mesh->usedVertices = context.numFaces * 4;
	mesh->numIndices = context.numFaces * 6;
}

bool greedy_getMaskJK(uint8 *mask, int j, int k) { return mask[j * CHUNK_SIZE + k]; }
void greedy_setMaskJK(uint8 *mask, int j, int k) { mask[j * CHUNK_SIZE + k] = 1; }

uint8 greedy_getC(Chunk *chunk, int dim, int i, int j, int k)
{
	int p[3];
	p[dim] = i;
	p[(dim + 1) % 3] = j;
	p[(dim + 2) % 3] = k;
	return chunk_getBlockChecked(chunk, p[0], p[1], p[2]);
}

uint8 greedy_getU(Chunk *chunk, int dim, int i, int j, int k)
{
	int p[3];
	p[dim] = i;
	p[(dim + 1) % 3] = j;
	p[(dim + 2) % 3] = k;
	return chunk_getBlockUnchecked(chunk, p[0], p[1], p[2]);
}

void greedy_setV(VertexColorNormal10 *vert, int dim, int16 i, int16 j, int16 k)
{
	vert->element[dim] = i;
	vert->element[(dim + 1) % 3] = j;
	vert->element[(dim + 2) % 3] = k;
	vert->w=0;
}

void meshVanillaGreedy(Chunk *chunk, ChunkMesh *mesh)
{
	setModelMatrix(chunk, mesh);
	mesh->indexMode = INDEX_QUADS;

	MeshBuildContext context = {0};
	context.current = mesh->vertices;
	context.color.g=context.color.b=context.color.a=255;

	// Mask - used to store whether or not a face is in a greedy rect yet
	uint8 *mask = (uint8*)malloc(sizeof(uint8) * CHUNK_SIZE * CHUNK_SIZE);

	// Dynamic buffer for vertices
	int vertCap = 512, vertLen = 0;
	VertexColorNormal10 *vert;
	vert = (VertexColorNormal10*)malloc(sizeof(VertexColorNormal10) * vertCap);

	for (int dim = 0; dim < 3; dim++) // Iterate through three dimensions
	{
		for (int face = -1; face < 2; face += 2) // Iterate through each dimension twice, for each face
		{
			for (int i = 0; i < CHUNK_SIZE; i++) // Dimension we are currently building faces for
			{
				memset(mask, 0, sizeof(uint8) * CHUNK_SIZE * CHUNK_SIZE); // Clear mask
				for (int j = 0; j < CHUNK_SIZE; j++) // Iterate through the rect of the current face
				for (int k = 0; k < CHUNK_SIZE; k++)
				{
					if (greedy_getU(chunk, dim, i, j, k) &&
						!greedy_getC(chunk, dim, i + face, j, k) &&
						!greedy_getMaskJK(mask, j, k))
					{
						int a = 1, b = 1; // Height/width of greedy rect
						greedy_setMaskJK(mask, j, k);

						// Expand horizontally
						while (j + a < CHUNK_SIZE &&
							greedy_getU(chunk, dim, i, j + a, k) &&
							!greedy_getC(chunk, dim, i + face, j + a, k) &&
							!greedy_getMaskJK(mask, j + a, k))
						{
							greedy_setMaskJK(mask, j + a, k);
							a++;
						}

						// Expand horizontal strip vertically
						while (k + b < CHUNK_SIZE)
						{
							bool rowInvalid = false;
							for (int p = 0; p < a; p++)
								if (!greedy_getU(chunk, dim, i, j + p, k + b) ||
									greedy_getC(chunk, dim, i + face, j + p, k + b) ||
									greedy_getMaskJK(mask, j + p, k + b))
								{
									rowInvalid = true;
									break;
								}

							if (rowInvalid)
								break;

							for (int p = 0; p < a; p++)
								greedy_setMaskJK(mask, j + p, k + b);

							b++;
						}

						if (vertLen + 4 >= vertCap) // Grow temp buffer
						{
							if (vertCap * 2 >= 67108864)
							{
								printf("Exeeding bounds! Exiting...\n");
								exit(300);
							}
							vertCap *= 2;
							vert = (VertexColorNormal10*)realloc(vert, vertCap * sizeof(VertexColorNormal10));
						}
	
						VertexColorNormal10 *cV = vert + vertLen;
	
						uint norm = faceNormTable[dim * 2 + (face == -1 ? 0 : 1)];

						cV[0].normal = cV[1].normal = cV[2].normal = cV[3].normal = norm;
						Color mag = {0, 0, 200, 255};
						cV[0].color = cV[1].color = cV[2].color = cV[3].color = chunk->color;

						// Handle CCW culling and side offset
						if (face == 1)
						{
							greedy_setV(&cV[0], dim, i + 1, j, k);
							greedy_setV(&cV[1], dim, i + 1, j, k+b);
							greedy_setV(&cV[2], dim, i + 1, j+a, k+b);
							greedy_setV(&cV[3], dim, i + 1, j+a, k);
						}
						else
						{
							greedy_setV(&cV[0], dim, i, j, k);
							greedy_setV(&cV[1], dim, i, j+a, k);
							greedy_setV(&cV[2], dim, i, j+a, k+b);
							greedy_setV(&cV[3], dim, i, j, k+b);
						}
						vertLen += 4;
					}
				}
			}
		}
	}
	
	memcpy(mesh->vertices, vert, vertLen * sizeof(VertexColorNormal10));
	free(vert);
	mesh->usedVertices = vertLen;
	mesh->numIndices = (vertLen / 4) * 6;
}

