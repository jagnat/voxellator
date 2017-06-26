#version 330 core

layout(location = 0) in vec3 in_Position;
layout(location = 1) in int in_Normal;
layout(location = 2) in vec4 in_Color;
layout(location = 3) in vec2 in_Texel;

out vec3 out_Normal;
out vec4 out_Color;
out vec2 out_Texel;
out vec4 out_Depth;
out vec4 out_Pos;

uniform mat4 model_matrix = mat4(1.0);
uniform mat4 view_matrix = mat4(1.0);
uniform mat4 projection_matrix = mat4(1.0);

void main(void)
{
	vec4 pos = (projection_matrix * view_matrix * model_matrix) * vec4(in_Position, 1.0);
	gl_Position = pos;
	out_Pos = view_matrix * vec4(in_Position, 1.0);
	out_Color = in_Color;
	vec3 tempNormal;
	switch (in_Normal)
	{
		case 0: tempNormal = vec3(1.0, 0.0, 0.0); break;
		case 1: tempNormal = vec3(-1.0, 0.0, 0.0); break;
		case 2: tempNormal = vec3(0.0, 1.0, 0.0); break;
		case 3: tempNormal = vec3(0.0, -1.0, 0.0); break;
		case 4: tempNormal = vec3(0.0, 0.0, 1.0); break;
		case 5: tempNormal = vec3(0.0, 0.0, -1.0); break;
		default: tempNormal = vec3(0); break;
	}
	out_Normal = tempNormal * mat3(model_matrix);
	out_Texel = in_Texel;
	out_Depth = pos;
}
