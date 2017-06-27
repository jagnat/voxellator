
#include "vox_gl_triangle.h"

// OpenGL test code

#include <stdio.h>

#include <GL/gl.h>
#include "thirdparty/glext.h"
#include "thirdparty/wglext.h"


// TODO: Move this into a platform-specific file
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

uint createGlProgram(char *vertex, char *fragment);
uint loadGlShader(char *filename, ShaderType shaderType);

typedef union
{
	struct
	{
		uint8 r, g, b, a;
	};
	uint8 element[4];
} Color;

typedef struct
{
	int w : 2;
	int z : 10;
	int y : 10;
	int x : 10;
} Normal;

typedef struct
{
	short x, y, z, w;
	Color color;
	Normal normal;
} Vertex;

typedef struct
{
	uint vboId;
	uint iboId;
	uint vaoId;
} Buffer;

Buffer triBuffer;

float vertices[] = {
	0, 0, 0,
	0.75, 0, 0,
	0, 0.75, 0
};

uint indices[] = {
	0, 1, 2
};

uint program = 0;
void initBuffer();

void initGlTriangle()
{
// TODO: This is also platform specific
#define GLDEF(ret, name, ...) gl##name = \
	(name##proc *) wglGetProcAddress("gl" #name);
GL_LIST
#undef GLDEF

// Ensure functions have successfully loaded
// TODO: something more robust for release mode
#define GLDEF(retrn, name, ...) assert(gl##name);
GL_LIST
#undef GLDEF

	program = createGlProgram("vertex.glsl", "fragment.glsl");

	initBuffer();
}

void drawGlTriangle()
{
	glUseProgram(program);
	glClearColor(.01f, .01f, .01f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(triBuffer.vaoId);
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void initBuffer()
{
	glGenVertexArrays(1, &triBuffer.vaoId);
	glGenBuffers(1, &triBuffer.vboId);
	glGenBuffers(1, &triBuffer.iboId);
	
	glBindBuffer(GL_ARRAY_BUFFER, triBuffer.vboId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triBuffer.iboId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


	glBindVertexArray(triBuffer.vaoId);
	glBindBuffer(GL_ARRAY_BUFFER, triBuffer.vboId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triBuffer.iboId);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
	glBindVertexArray(0);
	// IMPORTANT: unbind from the VAO before unbinding from these
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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

// TODO: Platform-specific file handling for hotloading
uint loadGlShader(char *filename, ShaderType shaderType)
{
	uint shaderId;
	FILE *shaderfile = fopen(filename, "r");
	if (!shaderfile)
	{
		printf("Couldn't load shader file: %s\n", filename);
		return 0;
	}
	
	fseek(shaderfile, 0, SEEK_END);
	int filesize = ftell(shaderfile);
	rewind(shaderfile);

	char *filebuf = calloc(1, filesize * sizeof(char) + 1);
	fread(filebuf, 1, filesize, shaderfile);
	fclose(shaderfile);
	
	GLenum glShaderType = GL_VERTEX_SHADER;
	if (shaderType == SHADER_FRAG)
		glShaderType = GL_FRAGMENT_SHADER;
	
	shaderId = glCreateShader(glShaderType);
	if (!shaderId)
		return shaderId;

	glShaderSource(shaderId, 1, &filebuf, 0);

	glCompileShader(shaderId);
	int status = 0;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &status);

		char *buf = calloc(1, status * sizeof(char));

		glGetShaderInfoLog(shaderId, status, 0, buf);
		printf("GL shader error in %s!!! \n\n%s\n", filename, buf);
	}

	return shaderId;

}

