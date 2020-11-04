#ifndef MAGIC_RENDER_H
#define MAGIC_RENDER_H

#include <glad/glad.h>
#include "chunk.h"
#include "math.h"

typedef struct {
	v3 location;
	float zoom;
} Camera;

typedef struct {
	GLuint vao, vbo; //, veo;
	// u32 veoLength;
	u32 numVertices;
} Mesh;

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

GLuint
compileShader(char *shaderCode, size_t codeLength, GLenum type);

GLuint
compileShaderFromFile(const char *file, GLenum type);

bool
linkShaderProgram(GLuint program);

void
renderChunk(RenderContext *ctx, Camera cam, Chunk *chunk);

Mesh
chunkGenMesh(MaterialTable *, Chunk *);

#endif
