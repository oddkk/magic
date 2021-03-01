#ifndef MAGIC_RENDER_H
#define MAGIC_RENDER_H

#include <glad/glad.h>
#include "chunk.h"
#include "math.h"

#include "mesh.h"

/*
struct render_op {
	enum render_op_type type;
};
*/

typedef struct {
	v3 location;
	float zoom;
} Camera;

struct render_context {
	struct {
		GLuint vao, vbo, elementBuffer;
		unsigned int elementBufferLength;
	} hex;

	struct {
		GLuint vao, vbo;
	} line;

	struct {
		GLuint vao, vbo;
	} quad;

	struct {
		GLuint id;

		GLint inColor;
		GLint inPosition;
		GLint inScale;
		GLint inSize;
		GLint inZOffset;
	} shader;

	v2i screenSize;

	MaterialTable *materials;
};

int
render_initialize(struct render_context *);

void
render_teardown(struct render_context *);

GLuint
shader_compile(char *shaderCode, size_t codeLength, GLenum type);

GLuint
shader_compile_from_file(const char *file, GLenum type);

bool
shader_link(GLuint program);

#define BITS_PER_UNIT (sizeof(u64)*8)
#define LAYER_MASK_UNITS ((CHUNK_LAYER_NUM_TILES + BITS_PER_UNIT-1) / BITS_PER_UNIT)
struct layer_neighbours {
	u8 layerId;
	u64 nw[LAYER_MASK_UNITS];
	u64 ne[LAYER_MASK_UNITS];
	u64 w[LAYER_MASK_UNITS];
	u64 e[LAYER_MASK_UNITS];
	u64 sw[LAYER_MASK_UNITS];
	u64 se[LAYER_MASK_UNITS];

	u64 above[LAYER_MASK_UNITS];
	u64 below[LAYER_MASK_UNITS];
};

// Keep the buffers for chunk generating to avoid reallocation.
struct chunk_gen_mesh_buffer {
	u64 solidMask[sizeof(u64) * CHUNK_NUM_TILES / BITS_PER_UNIT];
	u8 cullMask[sizeof(u8) * 8*CHUNK_NUM_TILES];
	u8 tileColor[sizeof(u8) * CHUNK_NUM_TILES * 3];
	struct layer_neighbours neighbourMasks[3];
};

#undef LAYER_MASK_UNITS
#undef BITS_PER_UNIT


struct mgc_mesh
chunk_gen_mesh(struct chunk_gen_mesh_buffer *buffer, MaterialTable *materials, struct mgc_chunk *cnk);

#endif
