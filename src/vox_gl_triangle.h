#ifndef _VOX_GL_TRIANGLE_H_
#define _VOX_GL_TRIANGLE_H_

#include "vox_main.h"

typedef enum
{
	SHADER_VERT,
	SHADER_FRAG
} ShaderType;

void initGlTriangle();
void drawGlTriangle(Movement mov);

#endif // _VOX_GL_TRIANGLE_H_

