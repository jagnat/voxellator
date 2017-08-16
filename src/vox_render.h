#ifndef _VOX_RENDER_H_
#define _VOX_RENDER_H_

// Backend-agnostic rendering API for generic meshes
// Try not to expose any OpenGL/Vulkan specific stuff in here

#include "thirdparty/j_threedee.h"

typedef struct Mesh Mesh;

typedef union
{
	struct
	{
		uint8 r, g, b, a;
	};
	struct
	{
		uint8 red, green, blue, alpha;
	};
	uint8 element[4];
} Color;

typedef struct
{
	int16 x, y, z, w;
	Color color;
	uint normal;
} VertexColorNormal10;

typedef enum
{
	INDEX_QUADS,
	INDEX_TRIS,
	INDEX_CUSTOM
} IndexMode;

typedef struct
{
	int allocatedVertices;
	int usedVertices;
	VertexColorNormal10 *vertices;

	IndexMode indexMode;
	int numIndices;
	uint *indices;

	JMat4 modelMatrix;

	// TODO: Sub-struct this, decouple from OpenGL
	union
	{
		uint ids[3];
		struct
		{
			uint vboId, iboId, vaoId;
		};
	};
} ChunkMesh;

typedef enum
{
	SHADER_VERT,
	SHADER_FRAG
} ShaderType;

void initRender();

ChunkMesh* createChunkMesh(int allocVertices);
void uploadChunkMesh(ChunkMesh *mesh);
void renderChunkMesh(ChunkMesh *mesh);

void setCam(Movement mov);
ChunkMesh *createSampleMesh();

#endif // _VOX_RENDER_H_

