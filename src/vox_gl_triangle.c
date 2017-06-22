
// OpenGL test code

#include <stdio.h>

uint createGlProgram(char *vertex, char *fragment);
uint loadGlShader(char *filename);

void initGlTriangle()
{
	uint program = createGlProgram("vertex.glsl", "fragment.glsl");
}

void drawGlTriangle()
{

}

void createGlProgram(char *vertex, char *fragment)
{

}

// TODO: Platform-specific file handling for hotloading
uint loadGlShader(char *filename, ShaderType shaderType)
{
	uint shaderId;
	FILE *shaderfile = fopen(filename, "r");
	if (!shaderfile)
		return 0;
	
	fseek(shaderfile, 0, SEEK_END);
	int filesize = ftell(shaderfile);
	rewind(shaderfile);

	char *filebuf = malloc(filesize * sizeof(char));
	if (fread(filebuf, 1, filesize, shaderfile) != filesize)
		return 0;
	
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
		printf("GL shader error!!! \n\n%s\n", buf);
	}

}

