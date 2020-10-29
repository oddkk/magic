#include "render.h"
#include "hexGrid.h"
#include "math.h"
#include "color.h"
#include "intdef.h"

#include <stdio.h>
#include <stdlib.h>

GLuint compileShader(char *shaderCode, size_t codeLength, GLenum type) {
	GLuint shader;
	shader = glCreateShader(type);

	GLint shaderCodeLength = codeLength;
	glShaderSource(shader, 1, (const GLchar **)&shaderCode, &shaderCodeLength);

	glCompileShader(shader);

	GLint mStatus;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &mStatus);

	if (mStatus == GL_FALSE) {
		GLint len;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

		if (len > 0) {
			GLchar *str = calloc(len+1, 1);
			glGetShaderInfoLog(shader, len, NULL, str);
			fprintf(stderr, "Failed to compile shader: %.*s\n", len, str);
			free(str);
		}
		else {
			fprintf(stderr, "Failed to compile shader\n");
		}
	}

	return shader;
}

static GLuint
compileShaderFromFile(const char *file, GLenum type)
{
	FILE *fp = fopen(file, "rb");
	if (!fp) {
		printf("Could not find shader file '%s'.\n", file);
	}
	fseek(fp, 0, SEEK_END);
	long length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *buffer = malloc(length);
	size_t err;
	
	err = fread(buffer, sizeof(char), length, fp);
	if ((long)err < length) {
		perror("read shader");
	}

	GLuint shader = compileShader(buffer, length, type);
	free(buffer);

	return shader;
}

bool linkShaderProgram(GLuint program) {
	GLint link_status;

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, (int *)&link_status);
	if (link_status == GL_FALSE) {
		GLint log_length;

		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

		char *log = (char*)calloc(log_length+1, 1);
		glGetProgramInfoLog(program, log_length, &log_length, log);

		fprintf(stderr, "shader: %.*s\n", (int)log_length, log);
	}

	if (link_status == GL_FALSE) {
		glDeleteProgram(program);
		program = 0;
	}

	return !!program;
}

int
renderInitialize(RenderContext *ctx)
{
	glGenVertexArrays(1, &ctx->hex.vao);
	glBindVertexArray(ctx->hex.vao);

	glGenBuffers(1, &ctx->hex.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, ctx->hex.vbo);

	glGenBuffers(1, &ctx->hex.elementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx->hex.elementBuffer);

	const float hexData[] = {
		0.0f, hexR,
		 cos(30.0 * M_PI / 180.0) * hexR,  sin(30.0 * M_PI / 180.0) * hexR,
		 cos(30.0 * M_PI / 180.0) * hexR, -sin(30.0 * M_PI / 180.0) * hexR,
		0.0f, -hexR,
		-cos(30.0 * M_PI / 180.0) * hexR, -sin(30.0 * M_PI / 180.0) * hexR,
		-cos(30.0 * M_PI / 180.0) * hexR,  sin(30.0 * M_PI / 180.0) * hexR,
	};

	unsigned int hexDataElements[] = {
		0, 1, 2,
		0, 2, 3,
		0, 3, 4,
		0, 4, 5,
	};
	ctx->hex.elementBufferLength = sizeof(hexDataElements) / sizeof(unsigned int);

	glBufferData(GL_ARRAY_BUFFER, sizeof(hexData), hexData, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(hexDataElements), hexDataElements, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


	glGenVertexArrays(1, &ctx->line.vao);
	glBindVertexArray(ctx->line.vao);

	glGenBuffers(1, &ctx->line.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, ctx->line.vbo);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	glGenVertexArrays(1, &ctx->quad.vao);
	glBindVertexArray(ctx->quad.vao);

	glGenBuffers(1, &ctx->quad.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, ctx->quad.vbo);

	const v2 quadData[] = {
		V2(-0.5, -0.5),
		V2( 0.5, -0.5),
		V2( 0.5,  0.5),

		V2(-0.5, -0.5),
		V2( 0.5,  0.5),
		V2(-0.5,  0.5),
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	ctx->shader.id = glCreateProgram();
	GLuint vShader, fShader;

	vShader = compileShaderFromFile("assets/shaders/2dmap.vsh", GL_VERTEX_SHADER);
	fShader = compileShaderFromFile("assets/shaders/2dmap.fsh", GL_FRAGMENT_SHADER);

	glAttachShader(ctx->shader.id, vShader);
	glAttachShader(ctx->shader.id, fShader);
	if (!linkShaderProgram(ctx->shader.id)) {
		return -1;
	}

	ctx->shader.inColor    = glGetUniformLocation(ctx->shader.id, "inColor");
	ctx->shader.inPosition = glGetUniformLocation(ctx->shader.id, "inPosition");
	ctx->shader.inScale    = glGetUniformLocation(ctx->shader.id, "inScale");
	ctx->shader.inSize     = glGetUniformLocation(ctx->shader.id, "inSize");
	ctx->shader.inZOffset  = glGetUniformLocation(ctx->shader.id, "inZOffset");

	glDisable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);

	return 0;
}

void
renderTeardown(RenderContext *ctx)
{
}

Color
tileColor(MaterialTable *mats, Tile *tile)
{
	Material *mat = getMaterial(mats, tile->material);
	return mat->color;
}

void
renderChunk(RenderContext *ctx, Camera cam, Chunk *chunk)
{
	const float hexSize = 50.0f;

	float offsetX = -(
			cam.location.x * hexStrideX +
			cam.location.y * hexStaggerX);
	float offsetY = -(
			cam.location.y * hexStrideY);

	glUseProgram(ctx->shader.id);
	glBindVertexArray(ctx->hex.vao);

	glUniform2f(
		ctx->shader.inScale,
		cam.zoom * hexSize / ((float)ctx->screenSize.x / 2.0f),
		cam.zoom * hexSize / ((float)ctx->screenSize.y / 2.0f)
	);

	// Draw grid
	glLineWidth(1.0f);
	glUniform3f(ctx->shader.inColor, 0.0f, 0.0f, 0.0f);
	for (unsigned int y = 0; y < CHUNK_WIDTH; y++) {
		for (unsigned int x = 0; x < CHUNK_WIDTH; x++) {
			Tile *tile = &chunk->tiles[y*CHUNK_WIDTH+x];
			{
				v2 p = gridDrawCoord(V2i(
						(chunk->location.x * CHUNK_WIDTH) + (int)x,
						(chunk->location.y * CHUNK_WIDTH) + (int)y));

				glUniform2f(ctx->shader.inPosition,
						offsetX + p.x,
						offsetY + p.y);
				glUniform2f(ctx->shader.inSize, 1.0f, 1.0f);
				glUniform1f(ctx->shader.inZOffset, 0.0f);

				Color color = tileColor(ctx->materials, tile);
				glUniform3f(
						ctx->shader.inColor,
						(float)color.r / 255.0f,
						(float)color.g / 255.0f,
						(float)color.b / 255.0f
						);

				glDrawElements(
						GL_TRIANGLES, ctx->hex.elementBufferLength,
						GL_UNSIGNED_INT, NULL);

				glUniform3f(ctx->shader.inColor, 0.8f, 0.8f, 0.8f);

				glDrawArrays(GL_LINE_LOOP, 0, 6);
			}

			/*
			if (tile->occupant != nullptr) {
				Entity *entity = tile->occupant;

				glUniform2f(ctx->shader.inSize, 0.5f, 0.5f);
				glUniform1f(ctx->shader.inZOffset, 0.1f);
				glUniform3f(
						ctx->shader.inColor,
						(float)entity->r / 255.0f,
						(float)entity->g / 255.0f,
						(float)entity->b / 255.0f
						);

				glBindVertexArray(quadVAO);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glBindVertexArray(hexVao);
			}
			*/

		}
	}
}
