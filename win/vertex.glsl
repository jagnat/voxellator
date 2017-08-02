#version 330 core

uniform mat4 projMatrix = mat4(1);
uniform mat4 viewMatrix = mat4(1);

layout(location = 0) in vec4 in_Position;
layout(location = 1) in vec4 in_Color;
layout(location = 2) in vec4 in_Normal;

out vec4 out_Color;

void main(void)
{
	gl_Position = (projMatrix * viewMatrix) * vec4(in_Position.xyz, 1);
	out_Color = in_Color;
}
