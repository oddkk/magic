#ifndef MAGIC_AREA_H
#define MAGIC_AREA_H

#include "shape.h"
#include "material.h"

enum mgc_area_op_kind {
	MGC_AREA_ADD,
	MGC_AREA_REMOVE,
};

struct mgc_area_op {
	enum mgc_area_op_kind op;

	union {
		struct {
			struct mgc_shape *shape;
			mgc_material_id material;
			struct mgc_world_transform transform;
		} add;

		struct {
			struct mgc_shape *shape;
			struct mgc_world_transform transform;
		} remove;
	};
};

struct mgc_area {
	struct mgc_area_op *ops;
	size_t num_ops;
};

void
mgc_area_apply(struct mgc_area *, struct mgc_chunk *);

#endif
