#include "vox_render.h"

#include <GL/gl.h>
#include "thirdparty/glext.h"
#include "thirdparty/wglext.h"
#include "thirdparty/j_threedee.h"

#include "vox_noise.h"

//#include <stdlib.h>

// TODO: Move this into a separate file
#define GL_LIST \
/* Begin gl funcs*/ \
GLDEF(void, UseProgram, GLuint program) \
GLDEF(GLint, GetUniformLocation, GLuint program, const GLchar *name) \
GLDEF(void, GenBuffers, GLsizei n, GLuint *buffers) \
GLDEF(void, BindBuffer, GLenum target, GLuint buffer) \
GLDEF(void, GenVertexArrays, GLsizei n, GLuint *arrays) \
GLDEF(void, BufferData, GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) \
GLDEF(void, BindVertexArray, GLuint array) \
GLDEF(void, EnableVertexAttribArray, GLuint index) \
GLDEF(void, VertexAttribPointer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer) \
GLDEF(void, UniformMatrix4fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
GLDEF(void*, MapBuffer, GLenum target, GLenum access) \
GLDEF(GLboolean, UnmapBuffer, GLenum target) \
GLDEF(GLuint, CreateProgram, void) \
GLDEF(void, AttachShader, GLuint program, GLuint shader) \
GLDEF(void, LinkProgram, GLuint program) \
GLDEF(void, GetProgramiv, GLuint program, GLenum pname, GLint *params) \
GLDEF(void, GetProgramInfoLog, GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog) \
GLDEF(GLuint, CreateShader, GLenum shaderType) \
GLDEF(void, ShaderSource, GLuint shader, GLsizei count, const GLchar **string, const GLint *length) \
GLDEF(void, CompileShader, GLuint shader) \
GLDEF(void, GetShaderiv, GLuint shader, GLenum pname, GLint *params) \
GLDEF(void, GetShaderInfoLog, GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog) \
GLDEF(void, DeleteShader, GLuint shader) \
GLDEF(void, DetachShader, GLuint program, GLuint shader) \
/* End gl funcs */

#ifdef _WIN32
#define GLDECL APIENTRY
#else
#define GLDECL
#endif

#define GLDEF(retrn, name, ...) typedef retrn GLDECL name##proc(__VA_ARGS__); \
static name##proc * gl##name;
GL_LIST
#undef GLDEF

typedef struct
{
	uint programId;
	uint *quadIndices;
	JMat4 projMatrix;
	JMat4 viewMatrix;
	int viewLoc, projLoc;
} RenderState;

RenderState ___rs = {0};
RenderState *render;

uint createGlProgram(char *vertex, char *fragment);
uint loadGlShader(char *filedata, ShaderType shaderType);

void initRender()
{
	// TODO: This is EXTREMELY platform specific
	#define GLDEF(ret, name, ...) gl##name = \
		(name##proc *) wglGetProcAddress("gl" #name);
	GL_LIST
	#undef GLDEF

	// Ensure functions have successfully loaded
	// TODO: something more robust for release mode
	#define GLDEF(retrn, name, ...) assert(gl##name);
	GL_LIST
	#undef GLDEF

	render = &___rs;
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);

	// TODO: Don't use stdio for file io, make this part of platform
	FILE *shaderfile = fopen("vertex.glsl", "r");
	if (!shaderfile)
		printf("Couldn't load vertex file!\n");
	fseek(shaderfile, 0, SEEK_END);
	int filesize = ftell(shaderfile);
	rewind(shaderfile);
	char *vertexFile = calloc(1, filesize * sizeof(char) + 1);
	fread(vertexFile, 1, filesize, shaderfile);
	fclose(shaderfile);

	shaderfile = fopen("fragment.glsl", "r");
	if (!shaderfile)
		printf("Couldn't load fragment file!\n");
	fseek(shaderfile, 0, SEEK_END);
	filesize = ftell(shaderfile);
	rewind(shaderfile);
	char *fragmentFile = calloc(1, filesize * sizeof(char) + 1);
	fread(fragmentFile, 1, filesize, shaderfile);
	fclose(shaderfile);

	render->programId = createGlProgram(vertexFile, fragmentFile);
	free(vertexFile);
	free(fragmentFile);
	glUseProgram(render->programId);

	render->viewLoc = glGetUniformLocation(render->programId, "viewMatrix");
	render->projLoc = glGetUniformLocation(render->programId, "projMatrix");

	if (render->viewLoc == -1 || render->projLoc == -1)
		printf("failed to grabu niform locations!\n");
	
	render->viewMatrix = JMat4_Identity();
	render->projMatrix = JMat4_PerspectiveFOV((70.0f * 3.14159f) / 180.0f,
		1280/720.0f, 0.001f, 10000.f);

	int NUM_IND = 50331648;
	render->quadIndices = malloc(sizeof(uint) * NUM_IND);
	for (int i = 0; i < NUM_IND / 6; i++)
	{
		render->quadIndices[i * 6 + 0] = i * 4 + 0;
		render->quadIndices[i * 6 + 1] = i * 4 + 2;
		render->quadIndices[i * 6 + 2] = i * 4 + 1;
		render->quadIndices[i * 6 + 3] = i * 4 + 0;
		render->quadIndices[i * 6 + 4] = i * 4 + 3;
		render->quadIndices[i * 6 + 5] = i * 4 + 2;
	}
}

#if 0

void setPos(VertexColorNormal10 *v, int *count, short x, short y, short z)
{
	v[*count].x = x;
	v[*count].y = y;
	v[*count].z = z;
	*count = *count + 1;
}

uint8 cubeTable[] =
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

// TODO: Inline function to create int_2_10_10_10_rev
uint normTable[] = {
	0x200, // -x
	0x1ff, // +x
	0x80000, // -y
	0x7fc00, // +y
	0x20000000, // -z
	0x1FF00000  // +z
};


void addCube(ChunkMesh *mesh, int *count, int x, int y, int z, uint8 r, uint8 g, uint8 b)
{
	VertexColorNormal10 *v = mesh->vertices;
	for (int i = 0; i < 24; i++)
	{
		v[*count + i].color.r=r;
		v[*count + i].color.g=g;
		v[*count + i].color.b=b;
		v[*count + i].color.a=255;
		v[*count + i].normal = normTable[i / 4];
	}

	int offs = 0;
	for (int i = 0; i < 24; i++)
	{
		setPos(v, count, x + cubeTable[offs], y + cubeTable[offs + 1], z + cubeTable[offs + 2]);
		offs += 3;
	}
}

ChunkMesh *createSampleMesh()
{
	int CHUNK_SIZE = 64;
	srand(0);
	uint8* buf = calloc(1, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * sizeof(uint8));
	int num = 0;

	seedPerlin3(482924);
	for (int x = 0; x < CHUNK_SIZE; x++)
	{
		for (int y = 0; y < CHUNK_SIZE; y++)
		{
			for (int z = 0; z < CHUNK_SIZE; z++)
			{
				float p = perlin3(x / 30.0, y / 30.0, z / 30.0);
				if (p > 0)
				{
					num++;
					buf[x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE] = 255;
				}
			}
		}
	}

	ChunkMesh *mesh = createChunkMesh(num * 24);
	mesh->numVertices = num * 24;
	mesh->numIndices = num * 36;

	int count = 0;
	for (int y = 0; y < CHUNK_SIZE; y++)
		for (int x = 0; x < CHUNK_SIZE; x++)
			for (int z = 0; z < CHUNK_SIZE; z++)
				if (buf[x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE] == 255)
					addCube(mesh, &count, x, y, z, 40, 128, 128);
	
	uploadChunkMesh(mesh);
	
	free(buf);
	
	return mesh;
}

#endif

ChunkMesh* createChunkMesh(int allocVertices)
{
	ChunkMesh *mesh = calloc(1, sizeof(ChunkMesh) + allocVertices * sizeof(VertexColorNormal10));
	mesh->vertices = (VertexColorNormal10*)(mesh + 1);
	mesh->allocatedVertices = allocVertices;
	glGenVertexArrays(1, &mesh->vaoId);
	glGenBuffers(2, mesh->ids);

	glBindVertexArray(mesh->vaoId);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vboId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->iboId);

	glEnableVertexAttribArray(0); // position
	glEnableVertexAttribArray(1); // color
	glEnableVertexAttribArray(2); // normal

	int stride = sizeof(VertexColorNormal10);
	glVertexAttribPointer(0, 3, GL_SHORT, GL_FALSE, stride, 0);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)(8));
	glVertexAttribPointer(2, 4, GL_INT_2_10_10_10_REV, GL_TRUE, stride, (void*)(8+4));

	// Important that this happens first - keep buffers bound to VAO!
	glBindVertexArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return mesh;
}

void deleteChunkMesh(ChunkMesh *mesh)
{
}

void uploadChunkMesh(ChunkMesh *mesh)
{
	// TODO: Make this more intelligent, use bufferSubData conditionally
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vboId);
	glBufferData(GL_ARRAY_BUFFER,
		mesh->usedVertices * sizeof(VertexColorNormal10),
		mesh->vertices,
		GL_STATIC_DRAW); // TODO: Profile usage if we add mutability
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	if (mesh->indexMode != INDEX_TRIS)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->iboId);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			mesh->numIndices * sizeof(uint),
			mesh->indexMode == INDEX_QUADS ? render->quadIndices : mesh->indices,
			GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}

// TODO: Add local transform uniform update
void renderChunkMesh(ChunkMesh *mesh)
{
	glBindVertexArray(mesh->vaoId);
	switch(mesh->indexMode)
	{
		case INDEX_TRIS:
		glDrawArrays(GL_TRIANGLES, 0, mesh->usedVertices);
		break;

		case INDEX_CUSTOM:
		case INDEX_QUADS:
		glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
		break;
	}
	glBindVertexArray(0);
}

void setCam(Movement mov)
{
	glUseProgram(render->programId);
	glClearColor(.01f, .01f, .01f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	render->viewMatrix = JMat4_FPSCam(mov.pos, mov.yaw, mov.pitch);
	glUniformMatrix4fv(render->projLoc, 1, false, render->projMatrix.flat);
	glUniformMatrix4fv(render->viewLoc, 1, false, render->viewMatrix.flat);
}

uint createGlProgram(char *vertex, char *fragment)
{
	uint vId = loadGlShader(vertex, SHADER_VERT);
	uint fId = loadGlShader(fragment, SHADER_FRAG);

	if (!vId || !fId)
	{
		//TODO: Log
		return 0;
	}

	uint pId = glCreateProgram();
	glAttachShader(pId, vId);
	glAttachShader(pId, fId);

	glLinkProgram(pId);

	glDetachShader(pId, vId);
	glDetachShader(pId, fId);

	glDeleteShader(vId);
	glDeleteShader(fId);

	int status = 0;
	glGetProgramiv(pId, GL_LINK_STATUS, &status);

	if (status == GL_FALSE)
	{
		int len = 0;
		glGetProgramiv(pId, GL_INFO_LOG_LENGTH, &len);
		char *infolog = (char*)malloc(len);
		glGetProgramInfoLog(pId, len, 0, infolog);
		printf("Failed to link program:\n\n%s\n", infolog);
		free(infolog);
		return 0;
	}

	return pId;
}

uint loadGlShader(char *filedata, ShaderType shaderType)
{
	uint shaderId;
	GLenum glShaderType = GL_VERTEX_SHADER;
	if (shaderType == SHADER_FRAG)
		glShaderType = GL_FRAGMENT_SHADER;
	
	shaderId = glCreateShader(glShaderType);
	if (!shaderId)
		return shaderId;

	glShaderSource(shaderId, 1, &filedata, 0);

	glCompileShader(shaderId);
	int status = 0;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &status);

		char *buf = calloc(1, status * sizeof(char));

		glGetShaderInfoLog(shaderId, status, 0, buf);
		printf("GL shader error!!! \n\n%s\n", buf);
	}

	return shaderId;
}

