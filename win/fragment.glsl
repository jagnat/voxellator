#version 330 core

in vec3 out_Normal;
in vec4 out_Color;
in vec2 out_Texel;
in vec4 out_Depth;
in vec4 out_Pos;

out vec4 final_Color;

uniform vec4 cam_pos;

vec4 get_ambient()
{
	return out_Color * vec4(vec3(0.4), 1.0);
}

vec4 get_diffuse1()
{
	vec4 light_direction = normalize(vec4(1, 2, 3, 1) - vec4(out_Normal, 1));

	float diffuse_intensity = max(0.0, dot(vec4(out_Normal, 1), light_direction));
	vec4 diffuse_color;
	diffuse_color.xyz = out_Color.xyz * diffuse_intensity;
	diffuse_color.w = 1.0;
	return diffuse_color;
}

vec4 get_diffuse2()
{
	vec4 light_direction = normalize(vec4(-1, -2, -3, 1) - vec4(out_Normal, 1));

	float diffuse_intensity = max(0.0, dot(vec4(out_Normal, 1), light_direction));
	vec4 diffuse_color;
	diffuse_color.xyz = out_Color.xyz * diffuse_intensity;
	diffuse_color.w = 1.0;
	return diffuse_color;
}

vec4 get_specular()
{
	vec4 light_direction = normalize(vec4(1, 2, 3, 1) - out_Pos);
	vec4 cam_direction = normalize(cam_pos - out_Pos);

	vec4 reflection = reflect(-light_direction, vec4(out_Normal, 1));

	float spec = pow(max(0.0, dot(cam_direction, reflection)), 32);

	return vec4(vec3(spec), 1);
}

void main(void)
{
	// Color
	vec4 ambient_color = get_ambient();
	final_Color = ambient_color + (get_diffuse1() + get_diffuse2()) / 2;// + get_specular();

	// Depth buffer
	const float C = 1.0;
	const float far = 100000.0;
	const float offset = 1.0;
	gl_FragDepth = (log(C * out_Depth.z + offset) / log(C * far + offset));
}
