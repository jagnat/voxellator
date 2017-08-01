#include "vox_render.h"

#include <GL/gl.h>
#include "thirdparty/glext.h"
#include "thirdparty/wglext.h"
#include "thirdparty/j_threedee.h"

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

	int NUM_IND = 12582912;
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

void initChunkMesh(ChunkMesh *mesh)
{
	glGenVertexArrays(1, &mesh->vaoId);
	glGenBuffers(2, mesh->ids);

	glBindBuffer(GL_ARRAY_BUFFER, mesh->vboId);
	glBufferData(GL_ARRAY_BUFFER,
		mesh->numVertices * sizeof(VertexColorNormal10),
		mesh->vertices,
		GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->iboId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		mesh->numIndices * sizeof(uint),
		render->quadIndices,
		GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindVertexArray(mesh->vaoId);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vboId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->iboId);

	glEnableVertexAttribArray(0); // position
	glEnableVertexAttribArray(1); // w
	glEnableVertexAttribArray(2); // color
	glEnableVertexAttribArray(3); // normal

	const int stride = sizeof(VertexColorNormal10);
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

