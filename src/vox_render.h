#ifndef _VOX_RENDER_H_
#define _VOX_RENDER_H_

// Backend-agnostic rendering API for generic meshes
// Try not to expose any OpenGL/Vulkan specific stuff in here

#include "thirdparty/j_threedee.h"

struct Mesh;

union Color
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
};

Color COLOR_RED = {255, 0, 0, 255};
Color COLOR_GREEN = {0, 255, 0, 255};
Color COLOR_BLUE = {0, 0, 255, 255};

struct VertexColorNormal10
{
	union
	{
		struct
		{
			int16 x, y, z, w;
		};
		int16 element[4];
	};
	Color color;
	uint normal;
};

enum IndexMode
{
	INDEX_QUADS,
	INDEX_TRIS,
	INDEX_CUSTOM
};

struct ChunkMesh
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
};

enum ShaderType
{
	SHADER_VERT,
	SHADER_FRAG
};

void initRender();

ChunkMesh* createChunkMesh(int allocVertices);
void uploadChunkMesh(ChunkMesh *mesh);
void renderChunkMesh(ChunkMesh *mesh);

void setCam(Movement mov);
ChunkMesh *createSampleMesh();

#endif // _VOX_RENDER_H_

