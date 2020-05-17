#include "noise.h"
#include "thirdparty/j_threedee.h"

static uint64 xorshift_64(NoiseSapling *noise)
{
	noise->xor_x ^= noise->xor_x >> 12;
	noise->xor_x ^= noise->xor_x << 25;
	noise->xor_x ^= noise->xor_x >> 27;
	return noise->xor_x * 2685821657736338717ul;
}

void seed_noise(NoiseSapling *noise, uint64 seed)
{
	noise->original_seed = seed;
	noise->xor_x = (seed + 1) * 12907;
	for (int i = 0; i < 256; i++)
	{
		noise->indices[i] = (uint8)i;
		noise->indices[i + 256] = (uint8)i;
	}

	for (int i = 0; i < 512; i++)
	{
		int r = xorshift_64(noise) % 512;
		uint8 swap = noise->indices[i];
		noise->indices[i] = noise->indices[r];
		noise->indices[r] = swap;
	}
}

const JVec3 grad3[] = {{1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},
						{1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},
						{0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1} };

float lerpf(float a, float b, float x)
{
	return a + (b - a) * x;
}

float pgrad3(int hash, float x, float y, float z)
{
	int h = hash & 15;
	float u = h < 8? x : y;
	float v;
	if (h < 4)
		v = y;
	else if (h == 12 || h == 14)
		v = x;
	else
		v = z;
	
	return ((h & 1) == 0? u: -u) + ((h & 2) == 0? v : -v);
}

float perlin3(NoiseSapling *noise, float x, float y, float z)
{
	int xi = (int)x & 255;
	int yi = (int)y & 255;
	int zi = (int)z & 255;

	float xf = x - (x < (int)x? (int)x - 1 : (int)x);
	float yf = y - (y < (int)y? (int)y - 1 : (int)y);
	float zf = z - (z < (int)z? (int)z - 1 : (int)z);

#define PERL_FADE(f) ((f) * (f) * (f) * ((f) * ((f) * 6 - 15) + 10))

	double u = PERL_FADE(xf);
	double v = PERL_FADE(yf);
	double w = PERL_FADE(zf);

	uint8 *p = noise->indices;
	int a = p[xi] + yi;
	int aa = p[a] + zi;
	int ab = p[a + 1] + zi;
	int b = p[xi + 1] + yi;
	int ba = p[b] + zi;
	int bb = p[b + 1] + zi;

	float x1, x2, y1, y2;
	x1 = lerpf(pgrad3(p[aa], xf, yf, zf), pgrad3(p[ba], xf - 1, yf, zf), u);
	x2 = lerpf(pgrad3(p[ab], xf, yf - 1, zf), pgrad3(p[bb], xf - 1, yf - 1, zf), u);
	y1 = lerpf(x1, x2, v);

	x1 = lerpf(pgrad3(p[aa + 1], xf, yf, zf - 1), pgrad3(p[ba + 1], xf - 1, yf, zf - 1), u);
	x2 = lerpf(pgrad3(p[ab + 1], xf, yf - 1, zf - 1), pgrad3(p[bb + 1], xf - 1, yf - 1, zf - 1), u);	
	y2 = lerpf(x1, x2, v);

	float r = lerpf(y1, y2, w);

	return r / 0.866025403784f;
}

const float F2 = 0.36602540378443f;
const float G2 = 0.21132486540518f;

static int fastfloor(float x)
{
	int xi = (int)x;
	return x<xi? xi-1:xi;
}

float simplex2(NoiseSapling *noise, float x, float y)
{
	float n0, n1, n2; // Noise contributions from the three corners
    // Skew the input space to determine which simplex cell we're in
    float s = (x+y)*F2; // Hairy factor for 2D
    int i = fastfloor(x+s);
    int j = fastfloor(y+s);
    float t = (i+j)*G2;
    float X0 = i-t; // Unskew the cell origin back to (x,y) space
    float Y0 = j-t;
    float x0 = x-X0; // The x,y distances from the cell origin
    float y0 = y-Y0;
    // For the 2D case, the simplex shape is an equilateral triangle.
    // Determine which simplex we are in.
    int i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
    if(x0>y0) {i1=1; j1=0;} // lower triangle, XY order: (0,0)->(1,0)->(1,1)
    else {i1=0; j1=1;}      // upper triangle, YX order: (0,0)->(0,1)->(1,1)
    // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
    // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
    // c = (3-sqrt(3))/6
    float x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
    float y1 = y0 - j1 + G2;
    float x2 = x0 - 1.0 + 2.0 * G2; // Offsets for last corner in (x,y) unskewed coords
    float y2 = y0 - 1.0 + 2.0 * G2;
    // Work out the hashed gradient indices of the three simplex corners
	uint8 *p = noise->indices;
    int ii = i & 255;
    int jj = j & 255;
    int gi0 = p[ii+p[jj]] % 12;
    int gi1 = p[ii+i1+p[jj+j1]] % 12;
    int gi2 = p[ii+1+p[jj+1]] % 12;
    // Calculate the contribution from the three corners
    float t0 = 0.5 - x0*x0-y0*y0;
    if(t0<0) n0 = 0.0;
    else {
      t0 *= t0;
      n0 = t0 * t0 * JVec2_Dot(JVec2_Create(grad3[gi0].x, grad3[gi0].y), JVec2_Create(x0, y0));  // (x,y) of grad3 used for 2D gradient
    }
    float t1 = 0.5 - x1*x1-y1*y1;
    if(t1<0) n1 = 0.0;
    else {
      t1 *= t1;
      n1 = t1 * t1 * JVec2_Dot(JVec2_Create(grad3[gi1].x, grad3[gi1].y), JVec2_Create(x1, y1));
    }
    float t2 = 0.5 - x2*x2-y2*y2;
    if(t2<0) n2 = 0.0;
    else {
      t2 *= t2;
      n2 = t2 * t2 * JVec2_Dot(JVec2_Create(grad3[gi2].x, grad3[gi2].y), JVec2_Create(x2, y2));
    }
    // Add contributions from each corner to get the final noise value.
    // The result is scaled to return values in the interval [-1,1].
    return 70.0 * (n0 + n1 + n2);
}