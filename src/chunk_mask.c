#include "chunk_mask.h"
#include "utils.h"
#include <stdio.h>

bool
mgc_chunk_mask_get(struct mgc_chunk_mask *mask, v3i c)
{
	assert(c.x >= 0 && c.x < CHUNK_WIDTH &&
	       c.y >= 0 && c.y < CHUNK_WIDTH &&
		   c.z >= 0 && c.z < CHUNK_HEIGHT);
	size_t i = c.z*CHUNK_WIDTH*CHUNK_WIDTH + c.y*CHUNK_WIDTH + c.x;
	const size_t bitsPerUnit = MGC_CHUNK_MASK_BIT_PER_UNIT;
	return !!(mask->mask[i/bitsPerUnit] & (1ULL << (i%bitsPerUnit)));
}

bool
mgc_chunk_mask_geti(struct mgc_chunk_mask *mask, size_t i)
{
	assert(i < CHUNK_NUM_TILES);
	const size_t bitsPerUnit = MGC_CHUNK_MASK_BIT_PER_UNIT;
	return !!(mask->mask[i/bitsPerUnit] & (1ULL << (i%bitsPerUnit)));
}

void
mgc_chunk_mask_set_to(struct mgc_chunk_mask *chunk, v3i c, bool is_set)
{
	// assert(c.x >= 0 && c.x < CHUNK_WIDTH &&
	//        c.y >= 0 && c.y < CHUNK_WIDTH &&
	// 	   c.z >= 0 && c.z < CHUNK_HEIGHT);
	if (c.x >= 0 && c.x < CHUNK_WIDTH &&
			c.y >= 0 && c.y < CHUNK_WIDTH &&
			c.z >= 0 && c.z < CHUNK_HEIGHT) {
		size_t i = c.z*CHUNK_WIDTH*CHUNK_WIDTH + c.y*CHUNK_WIDTH + c.x;
		mgc_chunk_mask_seti_to(chunk, i, is_set);
	}
}

void
mgc_chunk_mask_seti_to(struct mgc_chunk_mask *chunk, size_t i, bool is_set)
{
	assert(i < CHUNK_NUM_TILES);
	const size_t bitsPerUnit = MGC_CHUNK_MASK_BIT_PER_UNIT;
	u64 mask = 1ULL << (i%bitsPerUnit);
	if (is_set) {
		chunk->mask[i/bitsPerUnit] |= mask;
	} else {
		chunk->mask[i/bitsPerUnit] &= ~mask;
	}
}

void
mgc_chunk_mask_set(struct mgc_chunk_mask *chunk, v3i c)
{
	mgc_chunk_mask_set_to(chunk, c, true);
}

bool
mgc_chunk_layer_mask_get(struct mgc_chunk_layer_mask *mask, v2i c)
{
	assert(c.x >= 0 && c.x < CHUNK_WIDTH &&
	       c.y >= 0 && c.y < CHUNK_WIDTH);
	size_t i = c.y*CHUNK_WIDTH + c.x;
	return mgc_chunk_layer_mask_geti(mask, i);
}

bool
mgc_chunk_layer_mask_geti(struct mgc_chunk_layer_mask *mask, size_t i)
{
	assert(i < CHUNK_WIDTH*CHUNK_WIDTH*CHUNK_HEIGHT);
	const size_t bitsPerUnit = MGC_CHUNK_MASK_BIT_PER_UNIT;
	return !!(mask->mask[i/bitsPerUnit] & (1ULL << (i%bitsPerUnit)));
}

void
mgc_chunk_layer_mask_seti_to(struct mgc_chunk_layer_mask *layer, size_t i, bool is_set)
{
	assert(i < CHUNK_WIDTH * CHUNK_WIDTH);
	const size_t bitsPerUnit = MGC_CHUNK_MASK_BIT_PER_UNIT;
	u64 mask = 1ULL << (i%bitsPerUnit);
	if (is_set) {
		layer->mask[i/bitsPerUnit] |= mask;
	} else {
		layer->mask[i/bitsPerUnit] &= ~mask;
	}
}

void
mgc_chunk_layer_mask_set_to(struct mgc_chunk_layer_mask *layer, v2i c, bool is_set)
{
	assert(c.x >= 0 && c.x < CHUNK_WIDTH &&
	       c.y >= 0 && c.y < CHUNK_WIDTH);
	size_t i = c.y*CHUNK_WIDTH + c.x;
	mgc_chunk_layer_mask_seti_to(layer, i, is_set);
}

void
mgc_chunk_layer_mask_set(struct mgc_chunk_layer_mask *layer, v2i c)
{
	mgc_chunk_layer_mask_set_to(layer, c, true);
}

void
mgc_chunk_set_layer(
		struct mgc_chunk_mask *chunk,
		struct mgc_chunk_layer_mask *layer,
		unsigned int z)
{
	assert(z < CHUNK_HEIGHT);
	const size_t bitsPerUnit = MGC_CHUNK_MASK_BIT_PER_UNIT;
	const size_t layerSize = CHUNK_LAYER_NUM_TILES / bitsPerUnit;
	for (size_t i = 0; i < layerSize; i++) {
		chunk->mask[z*layerSize+i] |= layer->mask[i];
	}
}

void
mgc_chunk_layer_print(struct mgc_chunk_layer_mask *layer)
{
	for (size_t y = 0; y < CHUNK_WIDTH; y++) {
		for (size_t i = 0; i < y; i++) {
			printf(" ");
		}
		for (size_t x = 0; x < CHUNK_WIDTH; x++) {
			bool is_set = mgc_chunk_layer_mask_get(layer, V2i(x, y));
			printf("%c ", is_set ? '#' : '.');
		}
		printf("\n");
	}
}
