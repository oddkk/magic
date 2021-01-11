#ifndef MAGIC_WORLD_DEF_PARSER_H
#define MAGIC_WORLD_DEF_PARSER_H

#include "world_def.h"
#include "vars.h"

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

struct mgcd_lexer;

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

struct mgcd_token
mgcd_eat_token(struct mgcd_parser *parser);

struct mgcd_token
mgcd_peek_token(struct mgcd_parser *parser);

bool
mgcd_try_get_ident(struct mgcd_token tok, struct atom **out_atom);

bool
mgcd_try_eat_ident(struct mgcd_parser *parser, struct atom **out_atom);

bool
mgcd_try_get_version(struct mgcd_token tok, struct mgcd_version *out_version);

bool
mgcd_try_eat_version(struct mgcd_parser *parser, struct mgcd_version *out_version);

bool
mgcd_try_get_path(struct mgcd_token tok, struct string *out_path);

bool
mgcd_expect_var_path(struct mgcd_parser *parser, struct string *out_path);

bool
mgcd_expect_var_int(struct mgcd_parser *parser, MGCD_TYPE(int) *out);

bool
mgcd_expect_tok(struct mgcd_parser *parser, enum mgcd_token_type type);

bool
mgcd_expect_int(struct mgcd_parser *parser, int *out);

bool
mgcd_expect_var_v3i(struct mgcd_parser *parser, MGCD_TYPE(v3i) *out);

bool
mgcd_stmt_error_recover(struct mgcd_parser *parser);

bool
mgcd_block_continue(struct mgcd_token tok);

#endif
