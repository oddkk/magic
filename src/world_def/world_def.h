#ifndef MAGIC_WORLD_DEF_H
#define MAGIC_WORLD_DEF_H

#include "../types.h"
#include "../atom.h"
#include "../str.h"
#include "../errors.h"
#include "../shape.h"
#include "vars.h"
#include <stdio.h>

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

#endif
