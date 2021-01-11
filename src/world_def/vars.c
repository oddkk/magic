#include "vars.h"
#include "../str.h"
#include "../utils.h"
#include <stdio.h>

static struct string mgcd_type_names[] = {
#define TYPE(name, type) STR(#name),
MGCD_DEF_VAR_TYPES
#undef TYPE
};

struct string
mgcd_type_name(enum mgcd_type type)
{
	assert(type < MGCD_NUM_TYPES);
	return mgcd_type_names[type];
}

#define TYPE(name, type) \
	type mgcd_var_value_##name(struct mgcd_scope *scope, struct mgcd_var_##type val) { \
		if (val.var == MGCD_VAR_LIT) { \
			return val.litteral; \
		} else { \
			panic("TODO: mgcd variables"); \
			struct mgcd_var_slot slot = {0}; \
			return slot.val_##name; \
		} \
	} \
	struct mgcd_var_##type mgcd_var_lit_##name(type value) { \
		struct mgcd_var_##type result = {0}; \
		result.var = MGCD_VAR_LIT; \
		result.litteral = value; \
		return result; \
	} \
	struct mgcd_var_##type mgcd_var_var_##name(mgcd_var_id id) { \
		struct mgcd_var_##type result = {0}; \
		result.var = id; \
		return result; \
	}
MGCD_DEF_VAR_TYPES
#undef TYPE

bool
mgcd_var_print_var(mgcd_var_id id)
{
	if (id == MGCD_VAR_LIT) {
		return false;
	}

	printf("<var %i>", id);
	return true;
}

#define TYPE(name, type) \
	static void mgcd_lit_print_##name(type); \
	void mgcd_var_print_##name(MGCD_TYPE(name) val) \
	{ \
		if (!mgcd_var_print_var(val.var)) { \
			mgcd_lit_print_##name(val.litteral); \
		} \
	}
MGCD_DEF_VAR_TYPES
#undef TYPE

void
mgcd_lit_print_int(int val)
{
	printf("%i", val);
}

void
mgcd_lit_print_v3i(v3i val)
{
	printf("(%i, %i, %i)", val.x, val.y, val.z);
}

static void
mgcd_lit_print_mgcd_shape_id(mgcd_shape_id val)
{
	printf("<shape %i>", val);
}
