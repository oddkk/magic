#include "chunk_mesher.h"
#include "hexGrid.h"
#include "color.h"
#include "utils.h"
#include "profile.h"
#include "material.h"
#include "config.h"
#include "chunk.h"

#include <glad/glad.h>

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

#define BITS_PER_UNIT (sizeof(u64)*8)

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
chunkLayerCullMask(u8 *cullMask, struct layer_neighbours *neighbours)
{
	for (size_t i = 0; i < RENDER_CHUNK_LAYER_NUM_TILES; i++) {
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

static inline u64
tilesMaskMove(u64 *mask, size_t i, i64 movement)
{
	u64 base = mask[i];

	if (movement >= 0) {
		const size_t unitsPerLayer = RENDER_CHUNK_LAYER_NUM_TILES / BITS_PER_UNIT;
		u64 high = (i < (unitsPerLayer-1)) ? mask[i+1] : 0;
		u64 result = ((base >> movement) | (high << (BITS_PER_UNIT - movement)));
		return result;
	} else {
		movement = -movement;
		u64 low = (i > 0) ? mask[i-1] : 0;
		u64 result = ((base << movement) | (low >> (BITS_PER_UNIT - movement)));
		return result;
	}
}

size_t
count_set_bits_u8(u8 v)
{
#if __GNUC__
	return __builtin_popcount(v);
#else
	size_t count = 0;
	while (v) {
		count += v & 1;
		v >>= 1;
	}
	return count;
#endif
}
struct mgc_chunk_gen_mesh_result
chunk_gen_mesh(struct chunk_gen_mesh_buffer *buffer, struct mgc_material_table *materials, struct mgc_chunk *cnk, u64 dirty_mask)
{
	TracyCZone(trace, true);

	memset(buffer, 0, sizeof(struct chunk_gen_mesh_buffer));

	struct layer_neighbours *neighbourMasks = buffer->neighbourMasks;

	for (size_t i = 0; i < CHUNK_NUM_TILES; i++) {
		// render chunk coord in chunk
		//
		// The full chunk is broken into strips (rchunk_idx) which contains a
		// full row from a specific chunk.
		size_t rchunk_idx = i / RENDER_CHUNK_WIDTH;
		size_t rchunk_x = rchunk_idx % RENDER_CHUNKS_PER_CHUNK_WIDTH;
		size_t rchunk_y = (rchunk_idx / (RENDER_CHUNKS_PER_CHUNK_WIDTH * RENDER_CHUNK_WIDTH)) % RENDER_CHUNKS_PER_CHUNK_WIDTH;
		size_t rchunk_z = (rchunk_idx / (RENDER_CHUNKS_PER_CHUNK_WIDTH * CHUNK_WIDTH * RENDER_CHUNK_WIDTH));
		size_t rchunk_i = rchunk_x +
			(rchunk_y * RENDER_CHUNKS_PER_CHUNK_WIDTH) +
			(rchunk_z * RENDER_CHUNKS_PER_CHUNK_LAYER);
		assert(rchunk_i < RENDER_CHUNKS_PER_CHUNK);

		struct render_chunk_gen_mesh_buffer *rbuffer;
		rbuffer = &buffer->render_chunks[rchunk_i];

		// render chunk local coordinates
		size_t r_x = i % RENDER_CHUNK_WIDTH;
		size_t r_y = (i / CHUNK_WIDTH) % RENDER_CHUNK_WIDTH;
		size_t r_z = (i / CHUNK_LAYER_NUM_TILES) % RENDER_CHUNK_HEIGHT;
		size_t r_i = r_x +
			(r_y * RENDER_CHUNK_WIDTH) +
			(r_z * RENDER_CHUNK_LAYER_NUM_TILES);
		assert(r_i < RENDER_CHUNK_NUM_TILES);

		u64 *solidMask = rbuffer->solidMask;
		u8 *tileColor = rbuffer->tileColor;

		struct mgc_tile *tile = &cnk->tiles[i];
		Material *mat = mgc_mat_get(materials, tile->material);
		solidMask[r_i/BITS_PER_UNIT] |= (mat->solid ? 1ULL : 0ULL) << (r_i % BITS_PER_UNIT);

		tileColor[r_i*3+0] = mat->color.r;
		tileColor[r_i*3+1] = mat->color.g;
		tileColor[r_i*3+2] = mat->color.b;
	}

	struct mgc_chunk_gen_mesh_result result = {0};

	for (size_t rchunk_i = 0; rchunk_i < RENDER_CHUNKS_PER_CHUNK; rchunk_i++) {
		if ((dirty_mask & (1 << rchunk_i)) == 0) {
			// This chunk is not dirty, just skip it.
			continue;
		}

		struct render_chunk_gen_mesh_buffer *rbuffer;
		rbuffer = &buffer->render_chunks[rchunk_i];

		// We assume the number of tiles is a multiple of 64.
		u64 *solidMask = rbuffer->solidMask;
		u8 *tileColor = rbuffer->tileColor;

		// [nw, ne, w, e, sw, se, above, below] for each tile.
		u8 *cullMask = buffer->cullMask;
		memset(cullMask, 0, sizeof(buffer->cullMask));

		struct layer_neighbours *prev, *curr, *next;
		prev = &neighbourMasks[0];
		curr = &neighbourMasks[1];
		next = &neighbourMasks[2];

		for (size_t y = 0; y < RENDER_CHUNK_HEIGHT; y++) {
			u64 *layerSolidMask = &solidMask[y*RENDER_CHUNK_LAYER_NUM_TILES/BITS_PER_UNIT];

			for (size_t i = 0; i < RENDER_CHUNK_LAYER_NUM_TILES / BITS_PER_UNIT; i++) {
				prev->above[i] |= layerSolidMask[i];
				next->below[i] |= layerSolidMask[i];

#if RENDER_CHUNK_WIDTH == 16
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
#elif RENDER_CHUNK_WIDTH == 32
				const u64 eastEdgeMask = ~(
					(1ULL << (32*0)) |
					(1ULL << (32*1))
				);
				const u64 westEdgeMask = ~(
					(1ULL << (32*1-1)) |
					(1ULL << (32*2-1))
				);
#else
#error "Edge masks are not implemented for the current chunk width."
#endif

				curr->w[i]  |= tilesMaskMove(layerSolidMask, i, -1) & eastEdgeMask;
				curr->e[i]  |= tilesMaskMove(layerSolidMask, i,  1) & westEdgeMask;
				curr->nw[i] |= tilesMaskMove(layerSolidMask, i,  RENDER_CHUNK_WIDTH-1) & eastEdgeMask;
				curr->ne[i] |= tilesMaskMove(layerSolidMask, i,  RENDER_CHUNK_WIDTH-0);
				curr->sw[i] |= tilesMaskMove(layerSolidMask, i, -RENDER_CHUNK_WIDTH+0);
				curr->se[i] |= tilesMaskMove(layerSolidMask, i, -RENDER_CHUNK_WIDTH+1) & westEdgeMask;
			}

			if (y > 0) {
				u8 *prevLayerCullMask = &cullMask[(y-1)*RENDER_CHUNK_LAYER_NUM_TILES];
				chunkLayerCullMask(prevLayerCullMask, prev);
			}

			struct layer_neighbours *newLayer = prev;
			memset(newLayer, 0, sizeof(struct layer_neighbours));
			newLayer->layerId = y + 1;

			prev = curr;
			curr = next;
			next = newLayer;
		}

		u8 *lastLayerCullMask = &cullMask[(RENDER_CHUNK_HEIGHT-1)*RENDER_CHUNK_LAYER_NUM_TILES];
		chunkLayerCullMask(lastLayerCullMask, prev);

		size_t numTriangles = 0;
		for (size_t i = 0; i < RENDER_CHUNK_NUM_TILES; i++) {
			bool solid = !!(solidMask[i / BITS_PER_UNIT] & (1ULL << (i % BITS_PER_UNIT)));
			if (!solid) {
				continue;
			}

			// The two top-most bits indicate if the top and bottom surfaces should
			// be drawn. These faces require 4 triangles instead of the 2 the sides
			// need.
			u8 visibleFaces = ~cullMask[i];
			u8 quadFaces = visibleFaces & 0xc0;
			u8 hexFaces  = visibleFaces & 0x3f;

			numTriangles += count_set_bits_u8(quadFaces >> 6) * 4;
			numTriangles += count_set_bits_u8(hexFaces)       * 2;
		}

		if (numTriangles > 0) {
			const f32 normalX = sin(30.0 * PI / 180.0);
			const f32 normalY = cos(30.0 * PI / 180.0);

			const f32 xOffset = cos(30.0 * PI / 180.0) * hexR;
			const f32 yOffset = sin(30.0 * PI / 180.0) * hexR;

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

			// position, normal, and color per vertex.
			const size_t vertexStride = sizeof(v3)*2 + sizeof(u8)*3;
			u8 *vertices = calloc(vertexStride, 3 * numTriangles);

			size_t vertI = 0;
			for (size_t i = 0; i < RENDER_CHUNK_NUM_TILES; i++) {
				bool solid = !!(solidMask[i / BITS_PER_UNIT] & (1ULL << (i % BITS_PER_UNIT)));
				if (!solid || cullMask[i] == 0xff) {
					continue;
				}

				v3i coord = V3i(
					i % RENDER_CHUNK_WIDTH,
					i / RENDER_CHUNK_LAYER_NUM_TILES,
					(i % RENDER_CHUNK_LAYER_NUM_TILES) / RENDER_CHUNK_WIDTH
				);
				v3 center = V3(
					coord.x * hexStrideX + coord.z * hexStaggerX,
					coord.y * hexH,
					coord.z * hexStrideY
				);

				u8 colorR = tileColor[i*3+0];
				u8 colorG = tileColor[i*3+1];
				u8 colorB = tileColor[i*3+2];

				u8 visibleMask = ~cullMask[i];

#define EMIT_HEX_VERTEX(n, i) \
					*(v3 *)(&vertices[vertI+vertexStride*n+ 0]) = v3_add(center, hexVerts[i]); \
					*(v3 *)(&vertices[vertI+vertexStride*n+12]) = normal; \
					*(u8 *)(&vertices[vertI+vertexStride*n+24]) = colorR; \
					*(u8 *)(&vertices[vertI+vertexStride*n+25]) = colorG; \
					*(u8 *)(&vertices[vertI+vertexStride*n+26]) = colorB;

#define EMIT_HEX_TRIANGLE(i0, i1, i2) \
				EMIT_HEX_VERTEX(0, i0) \
				EMIT_HEX_VERTEX(1, i1) \
				EMIT_HEX_VERTEX(2, i2) \
				vertI += 3*vertexStride;

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
			assert(vertI == vertexStride * 3 * numTriangles);

#undef EMIT_HEX_TRIANGLE

			struct mgc_mesh mesh = {0};

			mesh.numVertices = vertI;

			glGenVertexArrays(1, &mesh.vao);
			glGenBuffers(1, &mesh.vbo);

			glBindVertexArray(mesh.vao);
			glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);

			glBufferData(GL_ARRAY_BUFFER, numTriangles * 3*vertexStride, vertices, GL_STATIC_DRAW);

			// Position
			glVertexAttribPointer(0, 3, GL_FLOAT,         GL_FALSE, vertexStride, 0);
			// Normal
			glVertexAttribPointer(1, 3, GL_FLOAT,         GL_FALSE, vertexStride, (void*)(sizeof(v3)));
			// Colour
			glVertexAttribPointer(2, 3, GL_UNSIGNED_BYTE, GL_TRUE,  vertexStride, (void*)(sizeof(v3)*2));

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);

			glBindVertexArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			free(vertices);

			result.mesh[rchunk_i] = mesh;
			result.set[rchunk_i] = true;
		}
	}

	TracyCZoneEnd(trace);
	return result;
}
