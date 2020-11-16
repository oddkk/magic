#include "render.h"
#include "hexGrid.h"
#include "math.h"
#include "color.h"
#include "intdef.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

GLuint
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

typedef enum {
	NEIGHBOUR_NW    = 0,
	NEIGHBOUR_NE    = 1,
	NEIGHBOUR_W     = 2,
	NEIGHBOUR_E     = 3,
	NEIGHBOUR_SW    = 4,
	NEIGHBOUR_SE    = 5,
	NEIGHBOUR_ABOVE = 6,
	NEIGHBOUR_BELOW = 7,
} LayerNeighbourName;

typedef enum {
	NEIGHBOUR_MASK_NW    = (1<<NEIGHBOUR_NW),
	NEIGHBOUR_MASK_NE    = (1<<NEIGHBOUR_NE),
	NEIGHBOUR_MASK_W     = (1<<NEIGHBOUR_W),
	NEIGHBOUR_MASK_E     = (1<<NEIGHBOUR_E),
	NEIGHBOUR_MASK_SW    = (1<<NEIGHBOUR_SW),
	NEIGHBOUR_MASK_SE    = (1<<NEIGHBOUR_SE),
	NEIGHBOUR_MASK_ABOVE = (1<<NEIGHBOUR_ABOVE),
	NEIGHBOUR_MASK_BELOW = (1<<NEIGHBOUR_BELOW),
} LayerNeighbourNameMask;


#define LAYER_MASK_UNITS ((CHUNK_LAYER_NUM_TILES + (sizeof(u64)*8)-1) / (sizeof(u64)*8))
typedef struct {
	u8 layerId;
	u64 nw[LAYER_MASK_UNITS];
	u64 ne[LAYER_MASK_UNITS];
	u64 w[LAYER_MASK_UNITS];
	u64 e[LAYER_MASK_UNITS];
	u64 sw[LAYER_MASK_UNITS];
	u64 se[LAYER_MASK_UNITS];

	u64 above[LAYER_MASK_UNITS];
	u64 below[LAYER_MASK_UNITS];
} LayerNeighbours;

static inline bool
bitfieldIndexed(u64 *bitfield, size_t i)
{
	const size_t unitWidth = sizeof(u64)*8;
	return !!(bitfield[i/unitWidth] & (1ULL << (i % unitWidth)));
}

static inline u8
cullMaskFromNeighbour(size_t i, LayerNeighbourName neighbour, u64 *neighbourMask)
{
	return bitfieldIndexed(neighbourMask, i)
		? (1 << neighbour)
		: 0;
}

void
chunkLayerCullMask(u8 *cullMask, LayerNeighbours *neighbours)
{
	for (size_t i = 0; i < CHUNK_LAYER_NUM_TILES; i++) {
		cullMask[i] =
			  cullMaskFromNeighbour(i, NEIGHBOUR_NW,    neighbours->nw)
			| cullMaskFromNeighbour(i, NEIGHBOUR_NE,    neighbours->ne)
			| cullMaskFromNeighbour(i, NEIGHBOUR_W,     neighbours->w)
			| cullMaskFromNeighbour(i, NEIGHBOUR_E,     neighbours->e)
			| cullMaskFromNeighbour(i, NEIGHBOUR_SW,    neighbours->sw)
			| cullMaskFromNeighbour(i, NEIGHBOUR_SE,    neighbours->se)
			| cullMaskFromNeighbour(i, NEIGHBOUR_ABOVE, neighbours->above)
			| cullMaskFromNeighbour(i, NEIGHBOUR_BELOW, neighbours->below);
	}
}

#if 0
static void
printTileLayerMask(u64 *mask)
{
	const size_t bitsPerUnit = sizeof(u64)*8;

	for (size_t z = 0; z < CHUNK_WIDTH; z++) {
		for (size_t i = 0; i < CHUNK_WIDTH-(z+1); i++) {
			printf(" ");
		}
		for (size_t x = 0; x < CHUNK_WIDTH; x++) {
			size_t idx = x + (CHUNK_WIDTH-z-1) * CHUNK_WIDTH;
			u64 unit = mask[idx / bitsPerUnit];
			bool set = !!((1ULL << (idx % bitsPerUnit)) & unit);
			printf("%c ", set ? '#' : '.');
		}
		printf("\n");
	}
}

const char *boxDrawing[] = {
	"▖",
	"▗",
	"▘",
	"▝",

	" ",
	"▏",
	"▕",
	"▔",
	"▁",
};

static void
printTileLayerCullMask(u8 *mask)
{
	for (size_t z = 0; z < CHUNK_WIDTH; z++) {
		for (size_t i = 0; i < CHUNK_WIDTH-(z+1); i++) {
			printf("  ");
		}
		for (size_t x = 0; x < CHUNK_WIDTH; x++) {
			size_t idx = x + (CHUNK_WIDTH-z-1) * CHUNK_WIDTH;
			u8 unit = mask[idx];
			printf("%02x  ", unit);
		}
		printf("\n");
	}
}
#endif

static inline u64
tilesMaskMove(u64 *mask, size_t i, i64 movement)
{
	u64 base = mask[i];
	const size_t bitsPerUnit = (sizeof(u64)*8);
	if (movement >= 0) {
		const size_t unitsPerLayer = CHUNK_LAYER_NUM_TILES / bitsPerUnit;
		u64 high = (i < (unitsPerLayer-1)) ? mask[i+1] : 0;
		u64 result = ((base >> movement) | (high << (bitsPerUnit - movement)));
		return result;
	} else {
		movement = -movement;
		u64 low = (i > 0) ? mask[i-1] : 0;
		u64 result = ((base << movement) | (low >> (bitsPerUnit - movement)));
		return result;
	}
}

size_t
u8CountSetBits(u8 v)
{
	size_t count = 0;
	while (v) {
		count += v & 1;
		v >>= 1;
	}
	return count;
}

Mesh
chunkGenMesh(MaterialTable *materials, Chunk *cnk)
{
	const size_t bitsPerUnit = sizeof(u64)*8;

	// We assume the number of tiles is a multiple of 64.
	u64 *solidMask;
	solidMask = calloc(sizeof(u64), CHUNK_NUM_TILES / bitsPerUnit);

	// [nw, ne, w, e, sw, se, above, below] for each tile.
	u8 *cullMask = calloc(sizeof(u8), 8*CHUNK_NUM_TILES);

	LayerNeighbours *neighbourMasks = calloc(sizeof(LayerNeighbours), 3);

	for (size_t i = 0; i < CHUNK_NUM_TILES; i++) {
		Tile *tile = &cnk->tiles[i];
		Material *mat = getMaterial(materials, tile->material);
		solidMask[i/bitsPerUnit] |= (mat->solid ? 1ULL : 0ULL) << (i % bitsPerUnit);
	}

	LayerNeighbours *prev, *curr, *next;
	prev = &neighbourMasks[0];
	curr = &neighbourMasks[1];
	next = &neighbourMasks[2];

	for (size_t y = 0; y < CHUNK_HEIGHT; y++) {
		u64 *layerSolidMask = &solidMask[y*CHUNK_LAYER_NUM_TILES/bitsPerUnit];

		for (size_t i = 0; i < CHUNK_LAYER_NUM_TILES / bitsPerUnit; i++) {
			prev->above[i] |= layerSolidMask[i];
			next->below[i] |= layerSolidMask[i];

#if CHUNK_WIDTH == 16
			const u64 eastEdgeMask = ~(
				(1ULL << (16*0)) |
				(1ULL << (16*1)) |
				(1ULL << (16*2)) |
				(1ULL << (16*3))
			);
			const u64 westEdgeMask = ~(
				(1ULL << (16*1-1)) |
				(1ULL << (16*2-1)) |
				(1ULL << (16*3-1)) |
				(1ULL << (16*4-1))
			);
#endif

			curr->w[i]  |= tilesMaskMove(layerSolidMask, i, -1) & eastEdgeMask;
			curr->e[i]  |= tilesMaskMove(layerSolidMask, i,  1) & westEdgeMask;
			curr->nw[i] |= tilesMaskMove(layerSolidMask, i,  CHUNK_WIDTH-1) & eastEdgeMask;
			curr->ne[i] |= tilesMaskMove(layerSolidMask, i,  CHUNK_WIDTH-0);
			curr->sw[i] |= tilesMaskMove(layerSolidMask, i, -CHUNK_WIDTH+0);
			curr->se[i] |= tilesMaskMove(layerSolidMask, i, -CHUNK_WIDTH+1) & westEdgeMask;
		}

		if (y > 0) {
			u8 *prevLayerCullMask = &cullMask[(y-1)*CHUNK_LAYER_NUM_TILES];
			chunkLayerCullMask(prevLayerCullMask, prev);
		}
#if 0
		if (y == 0) {
			printf("solid\n");
			printTileLayerMask(layerSolidMask);
			printf("nw\n");
			printTileLayerMask(curr->nw);
			printf("ne\n");
			printTileLayerMask(curr->ne);
			printf("w\n");
			printTileLayerMask(curr->w);
			printf("e\n");
			printTileLayerMask(curr->e);
			printf("sw\n");
			printTileLayerMask(curr->sw);
			printf("se\n");
			printTileLayerMask(curr->se);
		}
#endif

		LayerNeighbours *newLayer = prev;
		memset(newLayer, 0, sizeof(LayerNeighbours));
		newLayer->layerId = y + 1;

		prev = curr;
		curr = next;
		next = newLayer;
	}

	u8 *lastLayerCullMask = &cullMask[(CHUNK_HEIGHT-1)*CHUNK_LAYER_NUM_TILES];
	chunkLayerCullMask(lastLayerCullMask, prev);

#if 0
	printTileLayerCullMask(cullMask);
#endif

	size_t numTriangles = 0;
	for (size_t i = 0; i < CHUNK_NUM_TILES; i++) {
		bool solid = !!(solidMask[i / bitsPerUnit] & (1ULL << (i % bitsPerUnit)));
		if (!solid) {
			continue;
		}

		// The two top-most bits indicate if the top and bottom surfaces should
		// be drawn. These faces require 4 triangles instead of the 2 the sides
		// need.
		u8 visibleFaces = ~cullMask[i];
		u8 quadFaces = visibleFaces & 0xc0;
		u8 hexFaces  = visibleFaces & 0x3f;

		numTriangles += u8CountSetBits(quadFaces >> 6) * 4;
		numTriangles += u8CountSetBits(hexFaces)       * 2;
	}

	const f32 normalX = sin(30.0 * M_PI / 180.0);
	const f32 normalY = cos(30.0 * M_PI / 180.0);

	const f32 xOffset = cos(30.0 * M_PI / 180.0) * hexR;
	const f32 yOffset = sin(30.0 * M_PI / 180.0) * hexR;

	const v3 hexVerts[] = {
		V3(0.0f,     hexH,  hexR),
		V3(xOffset,  hexH,  yOffset),
		V3(xOffset,  hexH, -yOffset),
		V3(0.0f,     hexH, -hexR),
		V3(-xOffset, hexH, -yOffset),
		V3(-xOffset, hexH,  yOffset),

		V3(0.0f,     0.0f,  hexR),
		V3(xOffset,  0.0f,  yOffset),
		V3(xOffset,  0.0f, -yOffset),
		V3(0.0f,     0.0f, -hexR),
		V3(-xOffset, 0.0f, -yOffset),
		V3(-xOffset, 0.0f,  yOffset),
	};

	v3 *vertices = calloc(sizeof(v3), 3*2 * numTriangles);
	size_t vertI = 0;
	for (size_t i = 0; i < CHUNK_NUM_TILES; i++) {
		bool solid = !!(solidMask[i / bitsPerUnit] & (1ULL << (i % bitsPerUnit)));
		if (!solid || cullMask[i] == 0xff) {
			continue;
		}

		v3i coord = V3i(
			i % CHUNK_WIDTH,
			i / CHUNK_LAYER_NUM_TILES,
			(i % CHUNK_LAYER_NUM_TILES) / CHUNK_WIDTH
		);
		v3 center = V3(
			coord.x * hexStrideX + coord.z * hexStaggerX,
			coord.y * hexH,
			coord.z * hexStrideY
		);

		u8 visibleMask = ~cullMask[i];

#define EMIT_HEX_TRIANGLE(i0, i1, i2) \
			vertices[vertI+0] = v3_add(center, hexVerts[i0]); \
			vertices[vertI+1] = normal; \
			vertices[vertI+2] = v3_add(center, hexVerts[i1]); \
			vertices[vertI+3] = normal; \
			vertices[vertI+4] = v3_add(center, hexVerts[i2]); \
			vertices[vertI+5] = normal; \
			vertI += 6;

#define FLAG_SET(v, flag) ((v & flag) != 0)
		if (FLAG_SET(visibleMask, NEIGHBOUR_MASK_NW)) {
			v3 normal = V3(-normalX,  0.0f, normalY);
			EMIT_HEX_TRIANGLE(5, 6, 11);
			EMIT_HEX_TRIANGLE(5, 0,  6);
		}

		if (FLAG_SET(visibleMask, NEIGHBOUR_MASK_NE)) {
			v3 normal = V3(normalX,  0.0f, normalY);
			EMIT_HEX_TRIANGLE(0, 7, 6);
			EMIT_HEX_TRIANGLE(0, 1, 7);
		}

		if (FLAG_SET(visibleMask, NEIGHBOUR_MASK_W)) {
			v3 normal = V3(-1.0f,  0.0f,  0.0f);
			EMIT_HEX_TRIANGLE(4, 11, 10);
			EMIT_HEX_TRIANGLE(4,  5, 11);
		}

		if (FLAG_SET(visibleMask, NEIGHBOUR_MASK_E)) {
			v3 normal = V3( 1.0f,  0.0f,  0.0f);
			EMIT_HEX_TRIANGLE(1, 8, 7);
			EMIT_HEX_TRIANGLE(1, 2, 8);
		}

		if (FLAG_SET(visibleMask, NEIGHBOUR_MASK_SW)) {
			v3 normal = V3(-normalX,  0.0f, -normalY);
			EMIT_HEX_TRIANGLE(3, 10,  9);
			EMIT_HEX_TRIANGLE(3,  4, 10);
		}

		if (FLAG_SET(visibleMask, NEIGHBOUR_MASK_SE)) {
			v3 normal = V3(normalX,  0.0f, -normalY);
			EMIT_HEX_TRIANGLE(2, 9, 8);
			EMIT_HEX_TRIANGLE(2, 3, 9);
		}

		if (FLAG_SET(visibleMask, NEIGHBOUR_MASK_ABOVE)) {
			v3 normal = V3( 0.0f,  1.0f,  0.0f);
			EMIT_HEX_TRIANGLE(0, 5, 1);
			EMIT_HEX_TRIANGLE(1, 4, 2);
			EMIT_HEX_TRIANGLE(1, 5, 4);
			EMIT_HEX_TRIANGLE(2, 4, 3);
		}

		if (FLAG_SET(visibleMask, NEIGHBOUR_MASK_BELOW)) {
			v3 normal = V3( 0.0f, -1.0f,  0.0f);
			EMIT_HEX_TRIANGLE(6,  7, 11);
			EMIT_HEX_TRIANGLE(7,  8, 10);
			EMIT_HEX_TRIANGLE(7, 10, 11);
			EMIT_HEX_TRIANGLE(8,  9, 10);
		}
#undef FLAG_SET
	}
	assert(vertI == (3+3) * numTriangles);

#undef EMIT_HEX_TRIANGLE

	free(solidMask);
	free(cullMask);
	free(neighbourMasks);

	Mesh mesh = {0};

	mesh.numVertices = vertI;

	glGenVertexArrays(1, &mesh.vao);
	glGenBuffers(1, &mesh.vbo);

	glBindVertexArray(mesh.vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);

	glBufferData(GL_ARRAY_BUFFER, numTriangles * 2*3*sizeof(v3), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2*sizeof(v3), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 2*sizeof(v3), (void*)(sizeof(v3)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	free(vertices);

	/*
	v3 vertecies[] = {
		V3(0.0f,     hexH, hexR),
		V3(xOffset,  hexH, yOffset),
		V3(xOffset,  hexH, -yOffset),
		V3(0.0f,     hexH, -hexR),
		V3(-xOffset, hexH, -yOffset),
		V3(-xOffset, hexH,  yOffset),

		V3(0.0f,     0.0f, hexR),
		V3(xOffset,  0.0f, yOffset),
		V3(xOffset,  0.0f, -yOffset),
		V3(0.0f,     0.0f, -hexR),
		V3(-xOffset, 0.0f, -yOffset),
		V3(-xOffset, 0.0f,  yOffset),
	};

	u32 elements[] = {
		// Top
		0,  2,  1,
		0,  3,  2,
		0,  4,  3,
		0,  5,  4,

		// Bottom
		6,  7,  8,
		6,  8,  9,
		6,  9, 10,
		6, 10, 11,

		// Sides
		0,  7,  6,
		0,  1,  7,
              
		1,  8,  7,
		1,  2,  8,
              
		2,  9,  8,
		2,  3,  9,
              
		3, 10,  9,
		3,  4, 10,
              
		4, 11, 10,
		4,  5, 11,
              
		5,  6, 11,
		5,  0,  6,
	};

	Mesh mesh = {0};

	mesh.veoLength = sizeof(elements) / sizeof(u32);

	glGenVertexArrays(1, &mesh.vao);
	glGenBuffers(1, &mesh.vbo);
	glGenBuffers(1, &mesh.veo);

	glBindVertexArray(mesh.vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.veo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertecies), vertecies, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	*/

	return mesh;
}
