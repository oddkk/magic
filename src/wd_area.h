#ifndef MAGIC_WORLD_DEF_AREA_H
#define MAGIC_WORLD_DEF_AREA_H

#include "wd_world_def.h"
#include "area.h"

enum mgcd_area_op_kind {
	MGCD_AREA_ADD,
	MGCD_AREA_AREA,
	MGCD_AREA_REMOVE,
};

struct mgcd_area_op {
	struct mgcd_area_op *next;
	enum mgcd_area_op_kind op;
	struct mgc_location loc;

	union {
		struct {
			MGCD_TYPE(mgcd_resource_id) material;
			MGCD_TYPE(mgcd_resource_id) shape;
			MGCD_TYPE(v3i) translation;
			MGCD_TYPE(int) rotation;
		} add;

		struct {
			MGCD_TYPE(mgcd_resource_id) area;
			MGCD_TYPE(v3i) translation;
			MGCD_TYPE(int) rotation;
		} area;

		struct {
			MGCD_TYPE(mgcd_resource_id) shape;
			MGCD_TYPE(v3i) translation;
			MGCD_TYPE(int) rotation;
		} remove;
	};
};

struct mgcd_area {
	struct mgcd_area_op *ops;
	struct mgcd_var_def *vars;
};

struct mgcd_parser;

struct mgcd_area_op *
mgcd_parse_area_op(struct mgcd_parser *);

bool
mgcd_parse_area_block(struct mgcd_parser *, struct mgcd_area *, struct mgcd_area_op **out_ops);

int
mgcd_complete_area(struct mgcd_context *ctx,
		struct mgcd_area *area,
		struct arena *mem,
		struct mgc_area *out_area);

#endif
