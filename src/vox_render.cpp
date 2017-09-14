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
	int viewLoc, projLoc, modelLoc;
} RenderState;

RenderState ___rs = {0};
RenderState *renderer;

uint createGlProgram(char *vertex, char *fragment);
uint loadGlShader(const char *filedata, ShaderType shaderType);

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

	renderer = &___rs;
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	// TODO: Add a setting for wireframe with this
	//glPolygonMode(GL_FRONT, GL_LINE);

	// TODO: Don't use stdio for file io, make this part of platform
	// TODO: Hotloading
	FILE *shaderfile = fopen("shaders/vertex.glsl", "r");
	if (!shaderfile)
		printf("Couldn't load vertex file!\n");
	fseek(shaderfile, 0, SEEK_END);
	int filesize = ftell(shaderfile);
	rewind(shaderfile);
	char *vertexFile = (char*)calloc(1, filesize * sizeof(char) + 1);
	fread(vertexFile, 1, filesize, shaderfile);
	fclose(shaderfile);

	shaderfile = fopen("shaders/fragment.glsl", "r");
	if (!shaderfile)
		printf("Couldn't load fragment file!\n");
	fseek(shaderfile, 0, SEEK_END);
	filesize = ftell(shaderfile);
	rewind(shaderfile);
	char *fragmentFile = (char*)calloc(1, filesize * sizeof(char) + 1);
	fread(fragmentFile, 1, filesize, shaderfile);
	fclose(shaderfile);

	renderer->programId = createGlProgram(vertexFile, fragmentFile);
	free(vertexFile);
	free(fragmentFile);
	glUseProgram(renderer->programId);

	renderer->viewLoc = glGetUniformLocation(renderer->programId, "viewMatrix");
	renderer->projLoc = glGetUniformLocation(renderer->programId, "projMatrix");
	renderer->modelLoc = glGetUniformLocation(renderer->programId, "modelMatrix");

	if (renderer->viewLoc == -1 || renderer->projLoc == -1 || renderer->modelLoc == -1)
		printf("failed to grabu niform locations!\n");
	
	renderer->viewMatrix = JMat4_Identity();
	renderer->projMatrix = JMat4_PerspectiveFOV((70.0f * 3.14159f) / 180.0f,
		1280/720.0f, 0.001f, 10000.f);

	int NUM_IND = 50331648;
	renderer->quadIndices = (uint*)malloc(sizeof(uint) * NUM_IND);
	for (int i = 0; i < NUM_IND / 6; i++)
	{
		renderer->quadIndices[i * 6 + 0] = i * 4 + 0;
		renderer->quadIndices[i * 6 + 1] = i * 4 + 2;
		renderer->quadIndices[i * 6 + 2] = i * 4 + 1;
		renderer->quadIndices[i * 6 + 3] = i * 4 + 0;
		renderer->quadIndices[i * 6 + 4] = i * 4 + 3;
		renderer->quadIndices[i * 6 + 5] = i * 4 + 2;
	}
}

ChunkMesh* createChunkMesh(int allocVertices)
{
	ChunkMesh *mesh = (ChunkMesh*)calloc(1, sizeof(ChunkMesh) + allocVertices * sizeof(VertexColorNormal10));
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
			mesh->indexMode == INDEX_QUADS ? renderer->quadIndices : mesh->indices,
			GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}

// TODO: Add local transform uniform update
void renderChunkMesh(ChunkMesh *mesh)
{
	glUniformMatrix4fv(renderer->modelLoc, 1, false, mesh->modelMatrix.flat);

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
	glUseProgram(renderer->programId);
	glClearColor(.01f, .01f, .01f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderer->viewMatrix = JMat4_FPSCam(mov.pos, mov.yaw, mov.pitch);
	glUniformMatrix4fv(renderer->projLoc, 1, false, renderer->projMatrix.flat);
	glUniformMatrix4fv(renderer->viewLoc, 1, false, renderer->viewMatrix.flat);
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

uint loadGlShader(const char *filedata, ShaderType shaderType)
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

		char *buf = (char*)calloc(1, status * sizeof(char));

		glGetShaderInfoLog(shaderId, status, 0, buf);
		printf("GL shader error!!! \n\n%s\n", buf);
	}

	return shaderId;
}

