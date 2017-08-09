#version 330 core

in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragPos;

out vec4 finalColor;

uniform vec3 lightPos = vec3(32, 70, 32);

void main(void)
{
	float ambientStr = 0.5;
	vec3 ambient = ambientStr * vec3(1, 1, 1);

	vec3 lightDir = normalize(lightPos - fragPos);

	float diff = max(dot(fragNormal, lightDir), 0);

	vec3 diffuse = diff * vec3(1, 1, 1);
	// Color
	finalColor = vec4((ambient + diffuse) * fragColor.xyz, 1);
}
