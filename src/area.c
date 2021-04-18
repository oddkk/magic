#include "area.h"

void
mgc_area_apply(struct mgc_area *area, struct mgc_chunk *chunk)
{
	for (size_t i = 0; i < area->num_ops; i++) {
		struct mgc_area_op *op = &area->ops[i];

		switch (op->op) {
			case MGC_AREA_ADD:
				{
					struct mgc_chunk_mask mask = {0};
					struct mgc_world_transform transform = op->add.transform;
					v3i chunk_origin = mgc_chunk_coord_to_world(chunk->location);
					chunk_origin.x = -chunk_origin.x;
					chunk_origin.y = -chunk_origin.y;
					chunk_origin.z = -chunk_origin.z;
					
					mgc_world_transform_translate(&transform,
							chunk_origin);

					mgc_shape_fill_chunk(
						&mask,
						transform,
						op->add.shape
					);

					for (size_t i = 0; i < CHUNK_NUM_TILES; i++) {
						if (mgc_chunk_mask_geti(&mask, i)) {
							chunk->tiles[i].material = op->add.material;
						}
					}
				}
				break;

			case MGC_AREA_REMOVE:
				{
					struct mgc_chunk_mask mask = {0};
					mgc_shape_fill_chunk(
						&mask,
						op->remove.transform,
						op->remove.shape
					);

					// TODO: Unset
				}
				break;
		}
	}
}
