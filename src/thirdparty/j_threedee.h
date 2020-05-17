#ifndef THREEDEE_HEADER
#define THREEDEE_HEADER

typedef union _Color
{
	struct
	{
		unsigned char r, g, b, a;
	};
	struct
	{
		unsigned char red, green, blue, alpha;
	};
	unsigned char element[4];
} Color;

typedef union _JVec2
{
	struct { float x, y; };
	struct { float u, v; };
	float e[2];
} JVec2;

typedef union _JVec3
{
	struct { float x, y, z; };
	struct { float r, g, b; };
	struct { JVec2 xy; float unused_z; };
	float e[3];
} JVec3;

typedef union _JVec4
{
	struct { float x, y, z, w; };
	struct { JVec3 xyz; float unused_w; };
	float e[4];
} JVec4;

// NOTE: Column-major
typedef union _JMat4
{
	float flat[16];
	struct { JVec4 col0, col1, col2, col3; };
	struct
	{
		float m00;
		float m10;
		float m20;
		float m30;
		float m01;
		float m11;
		float m21;
		float m31;
		float m02;
		float m12;
		float m22;
		float m32;
		float m03;
		float m13;
		float m23;
		float m33;
	};
} JMat4;

Color Color_Lerp(Color c1, Color c2, float f);

JVec2 JVec2_Create(float x, float y);
JVec2 JVec2_Add(JVec2 v1, JVec2 v2);
JVec2 JVec2_Subtract(JVec2 v1, JVec2 v2);
JVec2 JVec2_Normalize(JVec2 v);
float JVec2_Dot(JVec2 v1, JVec2 v2);

JVec3 JVec3_Create(float x, float y, float z);
JVec3 JVec3_Add(JVec3 v1, JVec3 v2);
JVec3 JVec3_Subtract(JVec3 v1, JVec3 v2);
JVec3 JVec3_Normalize(JVec3 v);
float JVec3_Dot(JVec3 v1, JVec3 v2);
JVec3 JVec3_Cross(JVec3 v1, JVec3 v2);

JVec4 JVec4_Create(float x, float y, float z, float w);
JVec4 JVec4_Add(JVec4 v1, JVec4 v2);
JVec4 JVec4_Subtract(JVec4 v1, JVec4 v2);
JVec4 JVec4_Normalize(JVec4 v);
float JVec4_Dot(JVec4 v1, JVec4 v2);

JMat4 JMat4_Mult(JMat4 m1, JMat4 m2);
JMat4 JMat4_Ortho(float left, float right, float bottom, float top, float near, float far);
JMat4 JMat4_PerspectiveFOV(float fov, float aspect, float near, float far);
JMat4 JMat4_Identity();
JMat4 JMat4_Translate(float x, float y, float z);
JMat4 JMat4_Translatev(JVec3 direction);
JMat4 JMat4_Scale(float x, float y, float z);
JMat4 JMat4_Scalev(JVec3 factor);
int JMat4_Invert(JMat4 in, JMat4 *out); // Returns 1 if succeeded, 0 if failed
JMat4 JMat4_LookAt(JVec3 target, JVec3 eye, JVec3 up);
JMat4 JMat4_FPSCam(JVec3 eye, float rotationLR, float rotationUD);

#endif // THREEDEE_HEADER

#ifdef J_THREEDEE_IMPLEMENTATION

#include <math.h>

Color Color_Lerp(Color c1, Color c2, float f)
{
	Color c3;
	c3.r = c1.r + (unsigned char)(((float)c2.r - (float)c1.r) * f);
	c3.g = c1.g + (unsigned char)(((float)c2.g - (float)c1.g) * f);
	c3.b = c1.b + (unsigned char)(((float)c2.b - (float)c1.b) * f);
	c3.a = c1.a + (unsigned char)(((float)c2.a - (float)c1.a) * f);
	return c3;
}

JVec2 JVec2_Create(float x, float y) { JVec2 v = {x, y}; return v; }
JVec2 JVec2_Add(JVec2 v1, JVec2 v2) { return JVec2_Create(v1.x + v2.x, v1.y + v2.y); }
JVec2 JVec2_Subtract(JVec2 v1, JVec2 v2) { return JVec2_Create(v1.x - v2.x, v1.y - v2.y); }
JVec2 JVec2_Normalize(JVec2 v)
{
	float l = sqrt(v.x * v.x + v.y * v.y);
	if (l == 1 || l == 0)
		return v;
	return JVec2_Create(v.x / l, v.y / l);
}
float JVec2_Dot(JVec2 v1, JVec2 v2) { return v1.x * v2.x + v1.y * v2.y; }

JVec3 JVec3_Create(float x, float y, float z) { JVec3 v = {x, y, z}; return v; }
JVec3 JVec3_Add(JVec3 v1, JVec3 v2) { return JVec3_Create(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z); }
JVec3 JVec3_Subtract(JVec3 v1, JVec3 v2) { return JVec3_Create(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z); }
JVec3 JVec3_Normalize(JVec3 v)
{
	float l = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (l == 1 || l == 0)
		return v;
	return JVec3_Create(v.x / l, v.y / l, v.z / l);
}
float JVec3_Dot(JVec3 v1, JVec3 v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }
JVec3 JVec3_Cross(JVec3 v1, JVec3 v2)
{
	JVec3 r;
	r.x = v1.y * v2.z - v1.z * v2.y;
	r.y = v1.z * v2.x - v1.x * v2.z;
	r.z = v1.x * v2.y - v1.y * v2.x;
	return r;
}

JVec4 JVec4_Create(float x, float y, float z, float w) { JVec4 v = {x, y, z, w}; return v; }
JVec4 JVec4_Add(JVec4 v1, JVec4 v2) { return JVec4_Create(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w); }
JVec4 JVec4_Subtract(JVec4 v1, JVec4 v2) { return JVec4_Create(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w); }
JVec4 JVec4_Normalize(JVec4 v)
{
	float l = sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
	if (l == 1)
		return v;
	return JVec4_Create(v.x / l, v.y / l, v.z / l, v.w / l);
}
float JVec4_Dot(JVec4 v1, JVec4 v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w; }

JMat4 JMat4_Mult(JMat4 a, JMat4 b)
{
	JMat4 r = {0};
	JVec4 row0 = JVec4_Create(a.m00, a.m01, a.m02, a.m03);
	JVec4 row1 = JVec4_Create(a.m10, a.m11, a.m12, a.m13);
	JVec4 row2 = JVec4_Create(a.m20, a.m21, a.m22, a.m23);
	JVec4 row3 = JVec4_Create(a.m30, a.m31, a.m32, a.m33);
	r.m00 = JVec4_Dot(row0, b.col0);
	r.m01 = JVec4_Dot(row0, b.col1);
	r.m02 = JVec4_Dot(row0, b.col2);
	r.m03 = JVec4_Dot(row0, b.col3);

	r.m10 = JVec4_Dot(row1, b.col0);
	r.m11 = JVec4_Dot(row1, b.col1);
	r.m12 = JVec4_Dot(row1, b.col2);
	r.m13 = JVec4_Dot(row1, b.col3);

	r.m20 = JVec4_Dot(row2, b.col0);
	r.m21 = JVec4_Dot(row2, b.col1);
	r.m22 = JVec4_Dot(row2, b.col2);
	r.m23 = JVec4_Dot(row2, b.col3);

	r.m30 = JVec4_Dot(row3, b.col0);
	r.m31 = JVec4_Dot(row3, b.col1);
	r.m32 = JVec4_Dot(row3, b.col2);
	r.m33 = JVec4_Dot(row3, b.col3);
	return r;
}
JMat4 JMat4_Ortho(float l, float r, float b, float t, float n, float f)
{
	JMat4 m = {0};
	m.m00 = 2 / (r - l);
	m.m11 = 2 / (t - b);
	m.m22 = -2 / (f - n);

	m.m03 = -(r + l)/(r - l);
	m.m13 = -(t + b)/(t - b);
	m.m23 = -(f + n)/(f - n);
	m.m33 = 1;
	return m;
}
JMat4 JMat4_PerspectiveFOV(float fov, float aspect, float near, float far)
{
	float tanfovover2 = tan(fov / 2.0f);
	JMat4 m = {0};
	m.m00 = 1 / (aspect * tanfovover2);
	m.m11 = 1 / tanfovover2;
	m.m22 = -1 * (far + near)/(far - near);
	m.m23 = -1 * (2 * far * near)/(far - near);
	m.m32 = -1;
	return m;
}
JMat4 JMat4_Identity()
{
	JMat4 m = {0};
	m.m00 = 1;
	m.m11 = 1;
	m.m22 = 1;
	m.m33 = 1;
	return m;
}
JMat4 JMat4_Translate(float x, float y, float z)
{
	JMat4 r = JMat4_Identity();
	r.m03 = x;
	r.m13 = y;
	r.m23 = z;
	return r;
}
JMat4 JMat4_Translatev(JVec3 v) { return JMat4_Translate(v.x, v.y, v.z); }
JMat4 JMat4_Scale(float x, float y, float z)
{
	JMat4 r = JMat4_Identity();
	r.m00 = x;
	r.m11 = y;
	r.m22 = z;
	return r;
}
JMat4 JMat4_Scalev(JVec3 v) { return JMat4_Scale(v.x, v.y, v.z); }
int JMat4_Invert(JMat4 in, JMat4 *out) // Returns 1 if succeeded, 0 if failed
{
	float *m = in.flat;
	float inv[16], det;
	int i;
	
	inv[ 0] =  m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
	inv[ 4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
	inv[ 8] =  m[4] * m[ 9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[ 9];
	inv[12] = -m[4] * m[ 9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[ 9];
	inv[ 1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
	inv[ 5] =  m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
	inv[ 9] = -m[0] * m[ 9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[ 9];
	inv[13] =  m[0] * m[ 9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[ 9];
	inv[ 2] =  m[1] * m[ 6] * m[15] - m[1] * m[ 7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[ 7] - m[13] * m[3] * m[ 6];
	inv[ 6] = -m[0] * m[ 6] * m[15] + m[0] * m[ 7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[ 7] + m[12] * m[3] * m[ 6];
	inv[10] =  m[0] * m[ 5] * m[15] - m[0] * m[ 7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[ 7] - m[12] * m[3] * m[ 5];
	inv[14] = -m[0] * m[ 5] * m[14] + m[0] * m[ 6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[ 6] + m[12] * m[2] * m[ 5];
	inv[ 3] = -m[1] * m[ 6] * m[11] + m[1] * m[ 7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[ 9] * m[2] * m[ 7] + m[ 9] * m[3] * m[ 6];
	inv[ 7] =  m[0] * m[ 6] * m[11] - m[0] * m[ 7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[ 8] * m[2] * m[ 7] - m[ 8] * m[3] * m[ 6];
	inv[11] = -m[0] * m[ 5] * m[11] + m[0] * m[ 7] * m[ 9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[ 9] - m[ 8] * m[1] * m[ 7] + m[ 8] * m[3] * m[ 5];
	inv[15] =  m[0] * m[ 5] * m[10] - m[0] * m[ 6] * m[ 9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[ 9] + m[ 8] * m[1] * m[ 6] - m[ 8] * m[2] * m[ 5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
	if(det == 0)
		return 0;

	det = 1.f / det;

	for(i = 0; i < 16; i++)
		out->flat[i] = inv[i] * det;

	return 1;
}
static JMat4 jmat4ConstructXYZ(JVec3 x, JVec3 y, JVec3 z, JVec3 eye)
{
	JMat4 r = JMat4_Identity();
	r.m00 = x.x;
	r.m01 = x.y;
	r.m02 = x.z;

	r.m10 = y.x;
	r.m11 = y.y;
	r.m12 = y.z;

	r.m20 = z.x;
	r.m21 = z.y;
	r.m22 = z.z;

	r.m03 = -JVec3_Dot(eye, x);
	r.m13 = -JVec3_Dot(eye, y);
	r.m23 = -JVec3_Dot(eye, z);
	return r;
}
JMat4 JMat4_LookAt(JVec3 target, JVec3 eye, JVec3 up)
{
	JVec3 z = JVec3_Normalize(JVec3_Subtract(eye, target));
	JVec3 x = JVec3_Normalize(JVec3_Cross(up, z));
	JVec3 y = JVec3_Normalize(JVec3_Cross(z, x));
	
	return jmat4ConstructXYZ(x, y, z, eye);
}
JMat4 JMat4_FPSCam(JVec3 eye, float rotationLR, float rotationUD)
{
	float cosPitch = cos(rotationUD);
	float sinPitch = sin(rotationUD);
	float cosYaw = cos(rotationLR);
	float sinYaw = sin(rotationLR);

	JVec3 x = JVec3_Create(cosYaw, 0, -sinYaw);
	JVec3 y = JVec3_Create(sinYaw * sinPitch, cosPitch, cosYaw * sinPitch);
	JVec3 z = JVec3_Create(sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw);

	return jmat4ConstructXYZ(x, y, z, eye);
}

#endif // J_THREEDEE_IMPLEMENTATION
