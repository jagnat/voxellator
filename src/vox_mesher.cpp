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
	uint norm = faceNormTable[face];
	Color c = context->color;
	int faceOffs = face * 12;
	for (int i = 0; i < 12; i += 3)
	{
		VertexColorNormal10 *v = context->current;
		v->color = c;
		v->normal = norm;
		v->x = context->x + facePosTable[faceOffs + i + 0];
		v->y = context->y + facePosTable[faceOffs + i + 1];
		v->z = context->z + facePosTable[faceOffs + i + 2];
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
	mesh->vertices = (VertexColorNormal10*)malloc(chunk->filledVoxels * 24 * sizeof(VertexColorNormal10));
	setModelMatrix(chunk, mesh);

	mesh->indexMode = INDEX_QUADS;
	mesh->usedVertices = chunk->filledVoxels * 24;
	mesh->numIndices = chunk->filledVoxels * 36;

	MeshBuildContext context = {0};
	context.current = mesh->vertices;

	context.color = chunk->color;

	for (int y = 0; y < CHUNK_SIZE; y++)
		for (int z = 0; z < CHUNK_SIZE; z++)
			for (int x = 0; x < CHUNK_SIZE; x++)
			{
				context.x = x; context.y = y; context.z = z;
				if (chunk_getBlockUnchecked(chunk, x, y, z))
					addCube(&context);
			}
}

void meshVanillaCull(Chunk *chunk, ChunkMesh *mesh)
{
	double startTime = getElapsedMs();

	VertexColorNormal10 *vertices = (VertexColorNormal10*)malloc(32 * sizeof(VertexColorNormal10));
	int allocatedVerts = 32;

	MeshBuildContext context = {0};
	context.current = vertices;
	context.color = chunk->color;

	for (int y = 0; y < CHUNK_SIZE; y++)
		for (int z = 0; z < CHUNK_SIZE; z++)
			for (int x = 0; x < CHUNK_SIZE; x++)
			{
				if (context.numFaces * 4 + 24 >= allocatedVerts)
				{
					if (allocatedVerts * 2 >= 67108864)
					{
						printf("Exceeding bounds! Exiting...\n");
						exit(400);
					}
					allocatedVerts *= 2;
					int currentIndex = context.current - vertices;
					vertices = (VertexColorNormal10*)realloc(vertices, allocatedVerts * sizeof(VertexColorNormal10));
					context.current = vertices + currentIndex;
				}

				// TODO: Unroll, use unchecked lookups for extra speed
				context.x = x; context.y = y; context.z = z;
				if (chunk_getBlockUnchecked(chunk, x, y, z))
				{
					if (!chunk_getBlockUnchecked(chunk, x - 1, y, z))
						addFace(0, &context);
					if (!chunk_getBlockUnchecked(chunk, x + 1, y, z))
						addFace(1, &context);
					if (!chunk_getBlockUnchecked(chunk, x, y - 1, z))
						addFace(2, &context);
					if (!chunk_getBlockUnchecked(chunk, x, y + 1, z))
						addFace(3, &context);
					if (!chunk_getBlockUnchecked(chunk, x, y, z - 1))
						addFace(4, &context);
					if (!chunk_getBlockUnchecked(chunk, x, y, z + 1))
						addFace(5, &context);
				}
			}
	
	setModelMatrix(chunk, mesh);

	mesh->vertices = vertices;

	mesh->indexMode = INDEX_QUADS;

	mesh->usedVertices = context.numFaces * 4;
	mesh->allocatedVertices = allocatedVerts;
	mesh->numIndices = context.numFaces * 6;

	double elapsedTime = getElapsedMs() - startTime;
	printf("Culled: Chunk %d,%d,%d took %0.4f ms\n", chunk->x, chunk->y, chunk->z, elapsedTime);
}

struct MeshJobArgs
{
	Chunk *chunk;
	ChunkMesh *mesh;
};

void meshVanillaGreedyJobCompletion(void *args)
{
	MeshJobArgs *real = (MeshJobArgs*)args;
	uploadChunkMesh(real->mesh);
	finishedMeshes[numFinishedMeshes++] = real->mesh;
	free(args);
}

void meshVanillaGreedyJobProc(void *args)
{
	MeshJobArgs *casted = (MeshJobArgs*)args;
	meshVanillaGreedy(casted->chunk, casted->mesh);
	casted->mesh->doneMeshing = 1;
}

void addGreedyJob(Chunk *chunk)
{
	if (chunk->empty)
		return;
	MeshJobArgs *args = (MeshJobArgs*)malloc(sizeof(MeshJobArgs));
	args->chunk = chunk;
	args->mesh = createChunkMesh();
	finishedMeshes[numFinishedMeshes++] = args->mesh;
	Job job = {0};
	job.jobProc = meshVanillaGreedyJobProc;
	//job.completionProc = meshVanillaGreedyJobCompletion;
	job.args = args;
	job.priority = 200;
	addJob(job);
}

bool greedy_getMaskJK(uint8 *mask, int j, int k) { return mask[j * CHUNK_SIZE + k]; }
void greedy_setMaskJK(uint8 *mask, int j, int k) { mask[j * CHUNK_SIZE + k] = 1; }

uint8 greedy_getU(Chunk *chunk, int dim1, int dim2, int dim3, int i, int j, int k)
{
	int p[3];
	p[dim1] = i;
	p[dim2] = j;
	p[dim3] = k;
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
#define GREEDY_PRINT_TIME
#ifdef GREEDY_PRINT_TIME
	double startTime = getElapsedMs();
#endif

	// Dynamic buffer for vertices
	int vertCap = 16, vertLen = 0;

	VertexColorNormal10 *vertices = (VertexColorNormal10*)malloc(vertCap * sizeof(VertexColorNormal10));

	// Mask - used to store whether or not a face is in a greedy rect yet
	uint8 *mask = (uint8*)malloc(sizeof(uint8) * CHUNK_SIZE * CHUNK_SIZE);

	for (int dim = 0; dim < 3; dim++) // Iterate through three dimensions
	{
		int dim2 = (dim + 1) % 3, dim3 = (dim + 2) % 3;

		for (int face = -1; face < 2; face += 2) // Iterate through each dimension twice, for each face
		{
			for (int i = 0; i < CHUNK_SIZE; i++) // Dimension we are currently building faces for
			{
				memset(mask, 0, sizeof(uint8) * CHUNK_SIZE * CHUNK_SIZE); // Clear mask
				for (int j = 0; j < CHUNK_SIZE; j++) // Iterate through the rect of the current face
				for (int k = 0; k < CHUNK_SIZE; k++)
				{
					if (greedy_getU(chunk, dim, dim2, dim3, i, j, k) &&
						!greedy_getU(chunk, dim, dim2, dim3, i + face, j, k) &&
						!greedy_getMaskJK(mask, j, k))
					{
						int a = 1, b = 1; // Height/width of greedy rect
						greedy_setMaskJK(mask, j, k);

						// Expand horizontally
						while (j + a < CHUNK_SIZE &&
							greedy_getU(chunk, dim, dim2, dim3, i, j + a, k) &&
							!greedy_getU(chunk, dim, dim2, dim3, i + face, j + a, k) &&
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
								if (!greedy_getU(chunk, dim, dim2, dim3, i, j + p, k + b) ||
									greedy_getU(chunk, dim, dim2, dim3, i + face, j + p, k + b) ||
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
							vertices = (VertexColorNormal10*)realloc(vertices, vertCap * sizeof(VertexColorNormal10));
						}
	
						VertexColorNormal10 *cV = vertices + vertLen;
	
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

	setModelMatrix(chunk, mesh);
	mesh->vertices = vertices;
	mesh->indexMode = INDEX_QUADS;
	
	//memcpy(mesh->vertices, vert, vertLen * sizeof(VertexColorNormal10));
	//free(vert);
	mesh->allocatedVertices = vertCap;
	mesh->usedVertices = vertLen;
	mesh->numIndices = (vertLen / 4) * 6;

#ifdef GREEDY_PRINT_TIME
	double elapsedTime = getElapsedMs() - startTime;
	printf("Greedy: Chunk %d,%d,%d took %0.4f ms to mesh\n", chunk->x, chunk->y, chunk->z, elapsedTime);
#endif
}

