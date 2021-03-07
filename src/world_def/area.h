#ifndef MAGIC_WORLD_DEF_AREA_H
#define MAGIC_WORLD_DEF_AREA_H

#include "world_def.h"

enum mgcd_area_op_kind {
	MGCD_AREA_ADD,
	MGCD_AREA_REMOVE,
};

struct mgcd_area_op {
	struct mgcd_area_op *next;
	enum mgcd_area_op_kind op;

	union {
		struct {
			MGCD_TYPE(mgcd_resource_id) material;
			MGCD_TYPE(mgcd_resource_id) shape;
			MGCD_TYPE(v3i) translation;
			MGCD_TYPE(int) rotation;
		} add;

		struct {
			MGCD_TYPE(mgcd_resource_id) shape;
			MGCD_TYPE(v3i) translation;
			MGCD_TYPE(int) rotation;
		} remove;
	};
};

struct mgcd_parser;

struct mgcd_area_op *
mgcd_parse_area_op(struct mgcd_parser *);

#endif
