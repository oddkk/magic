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

	struct mgc_material_table *materials;
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
#define LAYER_MASK_UNITS ((RENDER_CHUNK_LAYER_NUM_TILES + BITS_PER_UNIT-1) / BITS_PER_UNIT)
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

struct render_chunk_gen_mesh_buffer {
	u64 solidMask[(RENDER_CHUNK_NUM_TILES + BITS_PER_UNIT-1) / BITS_PER_UNIT];
	u8 tileColor[RENDER_CHUNK_NUM_TILES * 3];
};

// Keep the buffers for chunk generating to avoid reallocation.
struct chunk_gen_mesh_buffer {
	struct render_chunk_gen_mesh_buffer render_chunks[RENDER_CHUNKS_PER_CHUNK];
	u8 cullMask[sizeof(u8) * 8*RENDER_CHUNK_NUM_TILES];
	struct layer_neighbours neighbourMasks[3];
};

#undef LAYER_MASK_UNITS
#undef BITS_PER_UNIT

struct mgc_chunk_gen_mesh_result {
	struct mgc_mesh mesh[RENDER_CHUNKS_PER_CHUNK];
	bool set[RENDER_CHUNKS_PER_CHUNK];
};

struct mgc_chunk_gen_mesh_result
chunk_gen_mesh(struct chunk_gen_mesh_buffer *buffer, struct mgc_material_table *materials, struct mgc_chunk *cnk, u64 dirty_mask);

#endif
