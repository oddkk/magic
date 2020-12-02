#ifndef MAGIC_WORLD_DEF_H
#define MAGIC_WORLD_DEF_H

#include "types.h"
#include "atom.h"
#include "str.h"
#include "errors.h"
#include <stdio.h>

typedef int mgcd_shape_id;
typedef int mgcd_heightmap_id;
typedef int mgcd_var_id;

#define MGCD_VAR_LIT ((mgcd_var_id)-1)

enum mgcd_shape_op_kind {
	MGCD_SHAPE_SHAPE,
	MGCD_SHAPE_CELL,
	MGCD_SHAPE_CUBE,
	MGCD_SHAPE_HEXAGON,
	MGCD_SHAPE_HEIGHTMAP,
	MGCD_SHAPE_BETWEEN,
};

#define MGCD_DEF_VAR_TYPES \
	TYPE(int, int) \
	TYPE(mgcd_shape_id, mgcd_shape_id) \
	TYPE(v3i, v3i)

#define TYPE(name, type) \
	struct mgcd_var_##name { \
		mgcd_var_id var; \
		type litteral; \
	};
MGCD_DEF_VAR_TYPES
#undef TYPE

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

#define TYPE(name, type) \
	type mgcd_var_value_##name(struct mgcd_var_##type); \
	struct mgcd_var_##type mgcd_var_lit_##name(type); \
	struct mgcd_var_##type mgcd_var_var_##name(mgcd_var_id);
MGCD_DEF_VAR_TYPES
#undef TYPE

struct string
mgcd_type_name(enum mgcd_type);

#define MGCD_TYPE(type) struct mgcd_var_##type

struct mgcd_shape_op {
	struct mgcd_shape_op *next;
	enum mgcd_shape_op_kind kind;
	bool inverted;

	union {
		struct {
			MGCD_TYPE(mgcd_shape_id) id;
			MGCD_TYPE(int) rotation;
			MGCD_TYPE(v3i) translation;
		} shape;

		struct {
			MGCD_TYPE(v3i) coord;
		} cell;

		struct {
			MGCD_TYPE(v3i) p0;
			MGCD_TYPE(v3i) p1;
		} cube;

		struct {
			MGCD_TYPE(v3i) center;
			MGCD_TYPE(int) radius;
			MGCD_TYPE(int) height;
		} hexagon;

		struct {
			struct string file;
			MGCD_TYPE(v3i) coord;
		} heightmap;

		struct {
			struct mgcd_shape_op *s0;
			struct mgcd_shape_op *s1;
		} between;
	};
};

size_t
mgcd_shape_op_num_children(struct mgcd_shape_op *);

void
mgcd_shape_op_print(struct mgcd_shape_op *);

struct mgcd_var_def {
	struct atom *name;
};

struct mgcd_shape {
	struct mgcd_shape_op *ops;
	size_t num_ops;

	struct mgcd_var_def *vars;
	size_t num_vars;
};

struct mgcd_world {
	struct paged_list shapes;
	struct paged_list shape_ops;
};

void
mgcd_world_init(struct mgcd_world *world, struct mgc_memory *mem);

#define MGCD_ATOMS \
	ATOM(shape) \
	ATOM(cell) \
	ATOM(heightmap) \
	ATOM(cube) \
	ATOM(hexagon) \
	ATOM(between) \
	ATOM(coord) \
	ATOM(file) \
	ATOM(center) \
	ATOM(radius) \
	ATOM(height) \

struct mgcd_atoms {
#define ATOM(name) struct atom *name;
	MGCD_ATOMS
#undef ATOM
};

struct mgcd_context {
	struct mgcd_world *world;
	struct mgcd_atoms atoms;

	struct atom_table *atom_table;
	struct arena *mem;
	struct arena *tmp_mem;

	struct mgc_error_context *err;
};

void
mgcd_context_init(struct mgcd_context *,
		struct mgcd_world *world,
		struct atom_table *atom_table,
		struct arena *mem,
		struct arena *tmp_mem,
		struct mgc_error_context *err);


#define MGCD_TOKEN_TYPES \
	TOKEN(EOF) \
	TOKEN(VERSION) \
	TOKEN(IDENT) \
	TOKEN(INTEGER_LIT) \
	TOKEN(FILE_PATH) \
	TOKEN(STRING_LIT) \
	TOKEN(OPEN_BLOCK) \
	TOKEN(CLOSE_BLOCK) \
	TOKEN(OPEN_TUPLE) \
	TOKEN(CLOSE_TUPLE) \
	TOKEN(ARG_SEP) \
	TOKEN(ASSIGN) \
	TOKEN(END_STMT)

enum mgcd_token_type {
#define TOKEN(name) MGCD_TOK_##name,
MGCD_TOKEN_TYPES
#undef TOKEN

	MGCD_NUM_TOKENS
};

struct string
mgcd_token_type_name(enum mgcd_token_type);

struct mgcd_version {
	int major, minor;
};

struct mgcd_token {
	enum mgcd_token_type type;

	union {
		struct atom *ident;
		int64_t integer_lit;
		struct string string_lit;
		struct string file_path;
		struct mgcd_version version;
	};
};

void
mgcd_print_token(struct mgcd_token *tok);

struct mgcd_lexer {
	FILE *fp;
	char *cur, *lim, *tok, *buf;
	bool eof;

	size_t buf_begin, buf_end;

	struct arena *mem;
	struct atom_table *atom_table;
	struct mgcd_context *ctx;
};

int
mcgd_parse_open_file(
		struct mgcd_lexer *lex,
		struct mgcd_context *ctx,
		char *file);

struct mgcd_token
mgcd_read_token(struct mgcd_lexer *ctx);

struct mgcd_parser {
	struct mgcd_context *ctx;
	struct mgcd_lexer *lex;

	struct mgcd_token peek_buffer[1];
	int peek_buffer_count;
};

void
mgcd_parse_init(struct mgcd_parser *,
		struct mgcd_context *,
		struct mgcd_lexer *);

struct mgcd_shape_op *
mgcd_parse_shape_op(struct mgcd_parser *);

struct mgcd_shape_file {
	struct mgcd_version version;
	struct mgcd_shape_op *ops;
};

bool
mgcd_parse_shape_file(
		struct mgcd_context *parse_ctx,
		struct mgcd_lexer *lexer,
		struct mgcd_shape_file *file);

void
mgcd_print_shape_file(struct mgcd_shape_file *file);

#endif
