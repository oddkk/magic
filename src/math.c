#include "math.h"
#include <stdio.h>

void
v4i_print(v4i *v)
{
	printf("[%i, %i, %i]", v->x, v->y, v->z);
}

void
m4_print(m4 *m)
{
	printf(
		"|%f, %f, %f, %f|\n"
		"|%f, %f, %f, %f|\n"
		"|%f, %f, %f, %f|\n"
		"|%f, %f, %f, %f|\n\n",
		m->c00, m->c10, m->c20, m->c30,
		m->c01, m->c11, m->c21, m->c31,
		m->c02, m->c12, m->c22, m->c32,
		m->c03, m->c13, m->c23, m->c33);
}

void
m4i_print(m4i *m)
{
	printf(
		"|%i, %i, %i, %i|\n"
		"|%i, %i, %i, %i|\n"
		"|%i, %i, %i, %i|\n"
		"|%i, %i, %i, %i|\n\n",
		m->c00, m->c10, m->c20, m->c30,
		m->c01, m->c11, m->c21, m->c31,
		m->c02, m->c12, m->c22, m->c32,
		m->c03, m->c13, m->c23, m->c33);
}


int *
vec4i_multiply_mat4(int *result, int *v0, int *m0)
{
	int x = v0[0];
	int y = v0[1];
	int z = v0[2];
	int w = v0[3];
	result[0] = m0[0] * x + m0[4] * y + m0[8] * z + m0[12] * w;
	result[1] = m0[1] * x + m0[5] * y + m0[9] * z + m0[13] * w;
	result[2] = m0[2] * x + m0[6] * y + m0[10] * z + m0[14] * w;
	result[3] = m0[3] * x + m0[7] * y + m0[11] * z + m0[15] * w;
	return result;
}

// Rotate 60deg * rotation. The coordinates are expected to be in
// cube-cordinate space. (x, y, -x-y)
m4i *
mat4_hex_rotate(m4i *result, int rotation)
{
	// Matrix for rotating 60 degrees clockwise.
	//  0, -1,  0, 0,
	//  0,  0, -1, 0,
	// -1,  0,  0, 0,
	//  0,  0,  0, 1,

	rotation = ((rotation % 6) + 6) % 6;

	int sign = (rotation % 2) ? -1 : 1;

	int p0 = ((rotation % 3) == 0) * sign;
	int p1 = ((rotation % 3) == 1) * sign;
	int p2 = ((rotation % 3) == 2) * sign;

	result->c00 = p0;
	result->c10 = p1;
	result->c20 = p2;
	result->c30 = 0;

	result->c01 = p2;
	result->c11 = p0;
	result->c21 = p1;
	result->c31 = 0;

	result->c02 = p1;
	result->c12 = p2;
	result->c22 = p0;
	result->c32 = 0;

	result->c03 = 0;
	result->c13 = 0;
	result->c23 = 0;
	result->c33 = 1;

	return result;
}
