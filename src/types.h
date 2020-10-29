#ifndef MAGIC_TYPES_H
#define MAGIC_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef float f32;
typedef double f64;

typedef uint8  u8;
typedef uint16 u16;
typedef uint32 u32;
typedef uint64 u64;
typedef int8  i8;
typedef int16 i16;
typedef int32 i32;
typedef int64 i64;

#define VEC_MAT_DEF(T, suffix) \
	typedef union { \
		struct { \
			T x, y; \
		}; \
		T m[2]; \
	} v2##suffix; \
	typedef union { \
		struct { \
			T x, y, z; \
		}; \
		T m[3]; \
	} v3##suffix; \
	typedef union { \
		struct { \
			T x, y, z, w; \
		}; \
		T m[4]; \
	} v4##suffix; \
	typedef union { \
		struct { \
			T c00, c01, \
			  c10, c11; \
		}; \
		T m[2*2]; \
	} m2##suffix; \
	typedef union { \
		struct { \
			T c00, c01, c02, \
			  c10, c11, c12, \
			  c20, c21, c22; \
		}; \
		T m[3*3]; \
	} m3##suffix; \
	typedef union { \
		struct { \
			T c00, c01, c02, c03, \
			  c10, c11, c12, c13, \
			  c20, c21, c22, c23, \
			  c30, c31, c32, c33; \
		}; \
		T m[4*4]; \
	} m4##suffix; \

VEC_MAT_DEF(f32, f)
VEC_MAT_DEF(i32, i)

#undef VEC_MAT_DEF

typedef v2f v2;
typedef v3f v3;
typedef v4f v4;
typedef m2f m2;
typedef m3f m3;
typedef m4f m4;

static inline v2f
V2(f32 x, f32 y) { return (v2f) {.x = x, .y = y}; }

static inline v3f
V3(f32 x, f32 y, f32 z) { return (v3f) {.x = x, .y = y, .z = z}; }

static inline v4f
V4(f32 x, f32 y, f32 z, f32 w) { return (v4f) {.x = x, .y = y, .z = z, .w = w}; }

static inline v2i
V2i(i32 x, i32 y) { return (v2i) {.x = x, .y = y}; }

static inline v3i
V3i(i32 x, i32 y, i32 z) { return (v3i) {.x = x, .y = y, .z = z}; }

static inline v4i
V4i(i32 x, i32 y, i32 z, i32 w) { return (v4i) {.x = x, .y = y, .z = z, .w = w}; }

#endif
