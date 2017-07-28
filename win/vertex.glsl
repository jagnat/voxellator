#version 330 core

uniform mat4 projMatrix = mat4(1);
uniform mat4 viewMatrix = mat4(1);


layout(location = 0) in vec3 in_Position;

out vec4 out_Color;

void main(void)
{
	gl_Position = (projMatrix * viewMatrix) * vec4(in_Position, 1);
	out_Color = vec4(1, 0, 0, 1);
}
