#ifndef MAGIC_CHUNK_MESHER_H
#define MAGIC_CHUNK_MESHER_H

#include "mesh.h"
#include "types.h"
#include "config.h"

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

struct chunk_gen_mesh {
	struct chunk_gen_mesh *next;
	void *data;
	size_t cap;
	size_t num_verts;
};

// Keep the buffers for chunk generating to avoid reallocation.
struct chunk_gen_mesh_buffer {
	struct render_chunk_gen_mesh_buffer render_chunks[RENDER_CHUNKS_PER_CHUNK];
	u8 cullMask[sizeof(u8) * 8*RENDER_CHUNK_NUM_TILES];
	struct layer_neighbours neighbourMasks[3];
};

struct chunk_gen_mesh_out_buffer {
	// TODO: Thread safe

	void *data;
	size_t cap;

	void *head;

	// The next chunk_gen_mesh is the first that should be popped, and is also
	// the tail of the cyclic buffer.
	struct chunk_gen_mesh *next;

	// A reference to the next pointer to the newest chunk_gen_mesh, or
	// chunk_gen_mesh_out_buffer.next if the queue is empty.
	struct chunk_gen_mesh **tail;
};

#undef LAYER_MASK_UNITS
#undef BITS_PER_UNIT

struct mgc_chunk_gen_mesh_result {
	struct mgc_mesh mesh[RENDER_CHUNKS_PER_CHUNK];
	bool set[RENDER_CHUNKS_PER_CHUNK];
};

struct mgc_material_table;
struct mgc_chunk;

int
chunk_gen_mesh_buffer_init(struct chunk_gen_mesh_out_buffer *buffer);

struct mgc_chunk_gen_mesh_result
chunk_gen_mesh(struct chunk_gen_mesh_buffer *buffer, struct chunk_gen_mesh_out_buffer *out, struct mgc_material_table *materials, struct mgc_chunk *cnk, u64 dirty_mask);

#endif
