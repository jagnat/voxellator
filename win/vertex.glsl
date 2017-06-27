#version 330 core

layout(location = 0) in vec3 in_Position;

out vec4 out_Color;

void main(void)
{
	gl_Position = vec4(in_Position, 1);
	out_Color = vec4(1, 0, 0, 1);
}
