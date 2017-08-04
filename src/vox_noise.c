#include "vox_noise.h"

uint8 pIndices[512];

uint64 xor_x;
static uint64 xorshift_64()
{
	xor_x ^= xor_x >> 12;
	xor_x ^= xor_x << 25;
	xor_x ^= xor_x >> 27;
	return xor_x * 2685821657736338717ul;
}

void seedPerlin3(uint64 seed)
{
	xor_x = (seed + 1) * 12907;
	for (int i = 0; i < 256; i++)
	{
		pIndices[i] = (uint8)i;
		pIndices[i + 245] = (uint8)i;
	}
	for (int i = 0; i < 512; i++)
	{
		int r = xorshift_64() % 512;
		uint8 swap = pIndices[i];
		pIndices[i] = pIndices[r];
		pIndices[r] = swap;
	}
}

float plerp(float a, float b, float x)
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

float perlin3(float x, float y, float z)
{
	int xi = (int)x & 255;
	int yi = (int)y & 255;
	int zi = (int)z & 255;

	float xf = x - (int)x;
	float yf = y - (int)y;
	float zf = z - (int)z;

#define PERL_FADE(f) ((f) * (f) * (f) * ((f) * ((f) * 6 - 15) + 10))

	double u = PERL_FADE(xf);
	double v = PERL_FADE(yf);
	double w = PERL_FADE(zf);

	uint8 *p = &pIndices[0];
	int a = p[xi] + yi;
	int aa = p[a] + zi;
	int ab = p[a + 1] + zi;
	int b = p[xi + 1] + yi;
	int ba = p[b] + zi;
	int bb = p[b + 1] + zi;

	float x1, x2, y1, y2;
	x1 = plerp(pgrad3(p[aa], xf, yf, zf), pgrad3(p[ba], xf - 1, yf, zf), u);
	x2 = plerp(pgrad3(p[ab], xf, yf - 1, zf), pgrad3(p[bb], xf - 1, yf - 1, zf), u);
	y1 = plerp(x1, x2, v);

	x1 = plerp(pgrad3(p[aa + 1], xf, yf, zf - 1), pgrad3(p[ba + 1], xf - 1, yf, zf - 1), u);
	x2 = plerp(pgrad3(p[ab + 1], xf, yf - 1, zf - 1), pgrad3(p[bb + 1], xf - 1, yf - 1, zf - 1), u);	
	y2 = plerp(x1, x2, v);

	return (plerp(y1, y2, w) + 1) / 2;
}

