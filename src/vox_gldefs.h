// NOTE: No include guards, since this needs to be included more than once

#define GL_LIST \
/* Begin gl funcs*/ \
GLDEF(void, UseProgram, GLuint program) \
GLDEF(GLint, GetUniformLocation, GLuint program, const GLchar *name) \
GLDEF(void, GenBuffers, GLsizei n, GLuint *buffers) \
GLDEF(void, DeleteBuffers, GLsizei n, GLuint *buffers) \
GLDEF(void, BindBuffer, GLenum target, GLuint buffer) \
GLDEF(void, GenVertexArrays, GLsizei n, GLuint *arrays) \
GLDEF(void, DeleteVertexArrays, GLsizei n, GLuint *arrays) \
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

