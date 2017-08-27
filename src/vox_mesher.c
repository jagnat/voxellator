#include "vox_mesher.h"

typedef union
{
	struct
	{
		int16 x0, y0, z0;
		int16 x1, y1, z1;
		int16 x2, y2, z2;
		int16 x3, y3, z3;
	};
	int16 verts[12];
} FaceRect;

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

void addRect(MeshBuildContext *context, FaceRect rect)
{
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

void meshVanillaGreedy(Chunk *chunk, ChunkMesh *mesh)
{
	setModelMatrix(chunk, mesh);
	mesh->indexMode = INDEX_QUADS;

	MeshBuildContext context = {0};
	context.current = mesh->vertices;
	context.color.g=context.color.b=context.color.a=255;

	uint8 *mask = malloc(sizeof(uint8) * CHUNK_SIZE * CHUNK_SIZE);

	int vertCap = 512, vertLen = 0;
	VertexColorNormal10 *vert;

	vert = malloc(sizeof(VertexColorNormal10) * vertCap);
	
	// TODO: Make this dimension-agnostic
	for (int x = 0; x < CHUNK_SIZE; x++)
	{
		memset(mask, 0, sizeof(uint8) * CHUNK_SIZE * CHUNK_SIZE);

		for (int i = 0; i < CHUNK_SIZE; i++)
		for (int j = 0; j < CHUNK_SIZE; j++)
		{
			if (!chunk_getBlockUnchecked(chunk, x, i, j) ||
				(chunk_getBlockChecked(chunk, x - 1, i, j) && chunk_getBlockChecked(chunk, x + 1, i, j))
				|| (mask[i * CHUNK_SIZE + j] & 0x01))
				continue;
			else
			{
				int a = 1, b = 1; // Rectangle width/height
				if (!(mask[i * CHUNK_SIZE + j] & 0x01))
				{
					while (i + a < CHUNK_SIZE && chunk_getBlockUnchecked(chunk, x, i + a, j) &&
						!chunk_getBlockChecked(chunk, x - 1, i + a, j) &&
						!(mask[(i + a) * CHUNK_SIZE + j] & 0x01))
					{
						mask[(i + a) * CHUNK_SIZE + j] |= 0x01;
						a++;
					}

					while (j + b < CHUNK_SIZE)
					{
						bool done = false;
						for (int p = 0; p < a; p++)
							if (!chunk_getBlockChecked(chunk, x, i + p, j + b) ||
								chunk_getBlockChecked(chunk, x - 1, i + p, j + b) ||
								mask[(i + p) * CHUNK_SIZE + j + b])
							{
								done = true;
								break;
							}
						if (done)
							break;

						for (int p = 0; p < a; p++)
							mask[(i + p) * CHUNK_SIZE + j + b] |= 0x01;
						b++;
					}

					if (vertLen + 4 >= vertCap) // Expand buffer
					{
						if (vertCap * 2 >= 67108864)
						{
							printf("Exeeding bounds! Exiting...\n");
							exit(300);
						}
						vertCap *= 2;
						vert = realloc(vert, vertCap * sizeof(VertexColorNormal10));
					}

					VertexColorNormal10 *cV = vert + vertLen;


					cV[0].normal = cV[1].normal = cV[2].normal = cV[3].normal = faceNormTable[1];
					//Color mag = {n * 255 / (CHUNK_SIZE * CHUNK_SIZE), 0, 0, 255};
					//Color mag = {i * 255 / CHUNK_SIZE, 0, j * 255 / CHUNK_SIZE, 255};
					Color mag = {rand() % 128, rand() % 50, rand() % 255, 255};
					//Color mag = {128, 0, x * 255 / CHUNK_SIZE, 255};
					cV[0].color = cV[1].color = cV[2].color = cV[3].color = mag;
					cV[0].x = cV[1].x = cV[2].x = cV[3].x = x;
					cV[0].w = cV[1].w = cV[2].w = cV[3].w = 0;

					cV[0].y = i; cV[0].z = j;
					cV[1].y = i + a; cV[1].z = j;
					cV[2].y = i + a; cV[2].z = j + b;
					cV[3].y = i; cV[3].z = j + b;
					vertLen += 4;
					
				}
			}
		}
	}

	memcpy(mesh->vertices, vert, vertLen * sizeof(VertexColorNormal10));
	free(vert);

	mesh->usedVertices = vertLen;
	mesh->numIndices = (vertLen / 4) * 6;
	
#if 0
	for (int y = 0; y < CHUNK_SIZE; y++)
	{
		memset(mask, 0, sizeof(uint8) * CHUNK_SIZE * CHUNK_SIZE);
		for (int x = 0; x < CHUNK_SIZE; x++)
			for (int z = 0; x < CHUNK_SIZE; z++)
			{
			}
	}

	for (int z = 0; z < CHUNK_SIZE; z++)
	{
		memset(mask, 0, sizeof(uint8) * CHUNK_SIZE * CHUNK_SIZE);
		for (int x = 0; x < CHUNK_SIZE; x++)
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
			}
	}
#endif
}

