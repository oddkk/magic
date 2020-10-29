#ifndef MAGIC_RENDER_H
#define MAGIC_RENDER_H

#include <glad/glad.h>
#include "chunk.h"
#include "math.h"

typedef struct {
	v2 location;
	float zoom;
} Camera;

typedef struct {
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
} RenderContext;

int
renderInitialize(RenderContext *);

void
renderTeardown(RenderContext *);

void
renderChunk(RenderContext *ctx, Camera cam, Chunk *chunk);

#endif
