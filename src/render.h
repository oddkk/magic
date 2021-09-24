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

#endif
