#ifndef MAGIC_TYPES_H
#define MAGIC_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#define PI (3.14159265359f)

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

static inline v2f
v2_add(v2 lhs, v2 rhs) { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }

static inline v3f
v3_add(v3 lhs, v3 rhs) { lhs.x += rhs.x; lhs.y += rhs.y; lhs.z += rhs.z; return lhs; }

static inline v4f
v4_add(v4 lhs, v4 rhs) { lhs.x += rhs.x; lhs.y += rhs.y; lhs.z += rhs.z; lhs.w += rhs.w; return lhs; }

static inline v2i
v2i_add(v2i lhs, v2i rhs) { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }

static inline v3i
v3i_add(v3i lhs, v3i rhs) { lhs.x += rhs.x; lhs.y += rhs.y; lhs.z += rhs.z; return lhs; }

static inline v4i
v4i_add(v4i lhs, v4i rhs) { lhs.x += rhs.x; lhs.y += rhs.y; lhs.z += rhs.z; lhs.w += rhs.w; return lhs; }


m4i *
m4i_identity(m4i *m);

static inline v2i
mgc_cube_to_axial(v3i p) { v2i r; r.x = p.x; r.y = p.y; return r; }

static inline v3i
mgc_axial_to_cube(v2i p) { v3i r; r.x = p.x; r.y = p.y; r.z = -p.x-p.y; return r; }

// Bounds includes min to max-1
struct mgc_aabbi {
	v3i min, max;
};

struct mgc_aabbi
mgc_aabbi_from_extents(v3i p0, v3i p1);

struct mgc_aabbi
mgc_aabbi_from_point(v3i p);

struct mgc_aabbi
mgc_aabbi_from_radius(v3i c, unsigned int r);

struct mgc_aabbi
mgc_aabbi_extend_bounds(struct mgc_aabbi lhs, struct mgc_aabbi rhs);

struct mgc_aabbi
mgc_aabbi_intersect_bounds(struct mgc_aabbi lhs, struct mgc_aabbi rhs);

struct mgc_hexbounds {
	v2i center;
	int radius;
	int height;
};

struct mgc_hexbounds
mgc_hexbounds_union(struct mgc_hexbounds lhs, struct mgc_hexbounds rhs);

struct mgc_hexbounds
mgc_hexbounds_intersect(struct mgc_hexbounds lhs, struct mgc_hexbounds rhs);

#if 0
#define max(a, b) \
	({ __typeof__ (a) _a = (a); \
	   __typeof__ (b) _b = (b); \
	   _a > _b ? _a : _b; })

#define min(a, b) \
	({ __typeof__ (a) _a = (a); \
	   __typeof__ (b) _b = (b); \
	   _a < _b ? _a : _b; })
#endif

#define max(a, b) \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wgnu-statement-expression\"") \
	_Pragma("GCC diagnostic ignored \"-Wgnu-auto-type\"") \
	({ __auto_type __a = (a); \
	   __auto_type __b = (b); \
	   __a > __b ? __a : __b; }) \
	_Pragma("GCC diagnostic pop")

#define min(a, b) \
	_Pragma("GCC diagnostic push") \
	_Pragma("GCC diagnostic ignored \"-Wgnu-statement-expression\"") \
	_Pragma("GCC diagnostic ignored \"-Wgnu-auto-type\"") \
	({ __typeof__ (a) _a = (a); \
	   __typeof__ (b) _b = (b); \
	   _a < _b ? _a : _b; }) \
	_Pragma("GCC diagnostic pop")

#endif
