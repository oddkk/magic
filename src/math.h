#ifndef MATH_H
#define MATH_H

#include "types.h"
#include <mathc.h>

void
v4i_print(v4i *);

void
m4i_print(m4i *m);

void
m4_print(m4 *);

int *
vec4i_multiply_mat4(int *result, int *v0, int *m);

m4i *
mat4_hex_rotate(m4i *result, int rotation);

#endif
