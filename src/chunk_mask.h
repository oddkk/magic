#ifndef MAGIC_CHUNK_MASK_H
#define MAGIC_CHUNK_MASK_H

#include "chunk.h"

#define MGC_CHUNK_MASK_BIT_PER_UNIT (sizeof(u64)*8)
struct mgc_chunk_mask {
	u64 mask[CHUNK_NUM_TILES / MGC_CHUNK_MASK_BIT_PER_UNIT];
};

// c is in chunk-local coordinates.
bool
mgc_chunk_mask_get(struct mgc_chunk_mask *mask, v3i c);

bool
mgc_chunk_mask_geti(struct mgc_chunk_mask *mask, size_t i);

// c is in chunk-local coordinates.
void
mgc_chunk_mask_set_to(struct mgc_chunk_mask *chunk, v3i c, bool is_set);

void
mgc_chunk_mask_seti_to(struct mgc_chunk_mask *chunk, size_t i, bool is_set);

void
mgc_chunk_mask_set(struct mgc_chunk_mask *chunk, v3i c);

struct mgc_chunk_layer_mask {
	u64 mask[CHUNK_LAYER_NUM_TILES / MGC_CHUNK_MASK_BIT_PER_UNIT];
};

// c is in chunk-local coordinates.
bool
mgc_chunk_layer_mask_get(struct mgc_chunk_layer_mask *mask, v2i c);

bool
mgc_chunk_layer_mask_geti(struct mgc_chunk_layer_mask *mask, size_t i);

// c is in chunk-local coordinates.
void
mgc_chunk_layer_mask_set_to(struct mgc_chunk_layer_mask *chunk, v2i c, bool is_set);

void
mgc_chunk_layer_mask_seti_to(struct mgc_chunk_layer_mask *chunk, size_t i, bool is_set);

void
mgc_chunk_layer_mask_set(struct mgc_chunk_layer_mask *chunk, v2i c);

void
mgc_chunk_set_layer(
		struct mgc_chunk_mask *chunk,
		struct mgc_chunk_layer_mask *layer,
		unsigned int z);

void
mgc_chunk_layer_print(struct mgc_chunk_layer_mask *layer);

#endif
