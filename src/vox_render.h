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

/*
typedef struct
{
	int z : 10;
	int y : 10;
	int x : 10;
	int w : 2;
} Normal30;
*/

typedef struct
{
	short x, y, z, w;
	Color color;
	uint normal;
} VertexColorNormal10;

typedef enum
{
	POLYMODE_QUADS,
	POLYMODE_TRIS,
	POLYMODE_CUSTOM
} PolyMode;

typedef struct
{
	VertexColorNormal10 *vertices;
	int numVertices;
	// TODO: This is implicit from numVertices?
	// TODO: Add mode - quads, or tris, or user?
	int numIndices;
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

void initChunkMesh(ChunkMesh *mesh);
void uploadChunkMesh(ChunkMesh *mesh);
void renderChunkMesh(ChunkMesh *mesh);

void setCam(Movement mov);
ChunkMesh *createSampleMesh();

#endif // _VOX_RENDER_H_

