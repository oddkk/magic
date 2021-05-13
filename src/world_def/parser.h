#ifndef MAGIC_WORLD_DEF_PARSER_H
#define MAGIC_WORLD_DEF_PARSER_H

#include "world_def.h"
#include "vars.h"
#include "../errors.h"

#define MGCD_TOKEN_TYPES \
	TOKEN(ERROR) \
	TOKEN(EOF) \
	TOKEN(VERSION) \
	TOKEN(IDENT) \
	TOKEN(INTEGER_LIT) \
	TOKEN(STRING_LIT) \
	TOKEN(OPEN_BLOCK) \
	TOKEN(CLOSE_BLOCK) \
	TOKEN(OPEN_TUPLE) \
	TOKEN(CLOSE_TUPLE) \
	TOKEN(ARG_SEP) \
	TOKEN(ASSIGN) \
	TOKEN(ACCESS) \
	TOKEN(PATH_SEG) \
	TOKEN(PATH_OPEN_EXPR) \
	TOKEN(PATH_CLOSE_EXPR) \
	TOKEN(END_STMT)

enum mgcd_token_type {
#define TOKEN(name) MGCD_TOK_##name,
MGCD_TOKEN_TYPES
#undef TOKEN

	MGCD_NUM_TOKENS
};

struct string
mgcd_token_type_name(enum mgcd_token_type);

enum mgcd_path_seg_flags {
	// If set, the segment is relative to the current directory
	// (eg. starts with './').
	MGCD_PATH_SEG_REL = (1<<0),
	MGCD_PATH_SEG_END = (1<<1),
};

struct mgcd_token {
	enum mgcd_token_type type;
	struct mgc_location loc;

	union {
		struct atom *ident;
		int64_t integer_lit;
		struct string string_lit;
		struct string file_path;
		struct mgcd_version version;
		struct {
			struct atom *ident;
			enum mgcd_path_seg_flags flags;
		} path_seg;
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

	mgcd_resource_id root_scope;
	mgcd_job_id finalize_job;
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
mgcd_try_eat_ident(struct mgcd_parser *parser,
		struct atom **out_atom,
		struct mgc_location *out_loc);

bool
mgcd_try_get_version(struct mgcd_token tok, struct mgcd_version *out_version);

bool
mgcd_try_eat_version(struct mgcd_parser *parser,
		struct mgcd_version *out_version,
		struct mgc_location *out_loc);

bool
mgcd_try_get_path(struct mgcd_token tok, struct string *out_path);

bool
mgcd_expect_path(struct mgcd_parser *parser,
		struct mgc_location *out_loc, struct mgcd_path *out_path);

bool
mgcd_expect_var_resource(
		struct mgcd_parser *parser,
		enum mgcd_entry_type disposition,
		MGCD_TYPE(mgcd_resource_id) *out);

bool
mgcd_expect_var_int(struct mgcd_parser *parser, MGCD_TYPE(int) *out);

bool
mgcd_expect_tok(struct mgcd_parser *parser, enum mgcd_token_type type);

void
mgcd_report_unexpect_tok(struct mgcd_parser *parser, enum mgcd_token_type type);

bool
mgcd_expect_int(struct mgcd_parser *parser, int *out);

bool
mgcd_expect_var_v3i(struct mgcd_parser *parser, MGCD_TYPE(v3i) *out);

bool
mgcd_stmt_error_recover(struct mgcd_parser *parser);

bool
mgcd_block_error_recover(struct mgcd_parser *parser);

bool
mgcd_block_continue(struct mgcd_token tok);

bool
mgcd_parse_bracketed_resource_block(struct mgcd_parser *parser, enum mgcd_entry_type type, mgcd_resource_id *out_id);

bool
mgcd_parse_anonymous_resource_block(struct mgcd_parser *parser, enum mgcd_entry_type type, mgcd_resource_id *out_id);

bool
mgcd_parse_resource_block(
		struct mgcd_parser *parser,
		enum mgcd_entry_type disposition,
		mgcd_resource_id);

#endif
