#version 330 core

uniform mat4 projMatrix = mat4(1);
uniform mat4 viewMatrix = mat4(1);
uniform mat4 modelMatrix = mat4(1);

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inNormal;

out vec4 fragColor;
out vec3 fragNormal;
out vec3 fragPos;

void main(void)
{
	gl_Position = (projMatrix * viewMatrix * modelMatrix) * vec4(inPosition.xyz, 1);
	fragColor = inColor;
	fragNormal = inNormal.xyz;
	fragPos = vec3(modelMatrix * vec4(inPosition.xyz, 1));
}
