#version 330 core

in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragPos;

out vec4 finalColor;

uniform vec3 lightPos = vec3(32, 70, 32);

void main(void)
{
	float ambientStr = 0.4;

	vec3 lightCol = vec3(1.0);

	vec3 ambient = ambientStr * lightCol;

	// Point lighting
	//vec3 lightDir = normalize(lightPos - fragPos);
	//float diff = max(dot(fragNormal, lightDir), 0);

	// Directional lighting
	vec3 lightDir1 = normalize(vec3(0.8, 1, 0.6));
	vec3 lightDir2 = normalize(vec3(-0.8, -1, -0.6));
	float diff = max(dot(fragNormal, lightDir1), 0) + 0.4 * max(dot(fragNormal, lightDir2), 0);

	vec3 diffuse = diff * lightCol;

	// Color
	finalColor = vec4((ambient + diffuse) * fragColor.xyz, 1);
}
