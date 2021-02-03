#ifndef MAGIC_WORLD_DEF_VARS_H
#define MAGIC_WORLD_DEF_VARS_H

#include "../types.h"

typedef int mgcd_var_id;
typedef int mgcd_resource_id;

#define MGCD_DEF_VAR_TYPES \
	TYPE(int, int) \
	TYPE(mgcd_resource_id, mgcd_resource_id) \
	TYPE(v3i, v3i)

#define TYPE(name, type) \
	struct mgcd_var_##name { \
		mgcd_var_id var; \
		type litteral; \
	};
MGCD_DEF_VAR_TYPES
#undef TYPE

#define MGCD_TYPE(type) struct mgcd_var_##type

enum mgcd_type {
#define TYPE(name, type) MGCD_TYPE_##name,
MGCD_DEF_VAR_TYPES
#undef TYPE

	MGCD_NUM_TYPES
};

struct mgcd_var_slot {
	enum mgcd_type type;
	union {
#define TYPE(name, type) type val_##name;
MGCD_DEF_VAR_TYPES
#undef TYPE
	};
};

struct mgcd_scope;

#define TYPE(name, type) \
	type mgcd_var_value_##name(struct mgcd_scope *, struct mgcd_var_##type); \
	struct mgcd_var_##type mgcd_var_lit_##name(type); \
	struct mgcd_var_##type mgcd_var_var_##name(mgcd_var_id); \
	void mgcd_var_print_##name(MGCD_TYPE(name));
MGCD_DEF_VAR_TYPES
#undef TYPE

struct string
mgcd_type_name(enum mgcd_type);

#define MGCD_VAR_LIT ((mgcd_var_id)-1)

#endif
