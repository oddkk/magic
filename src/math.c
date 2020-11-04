#include "math.h"
#include <stdio.h>

void
m4Print(m4 *m)
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
