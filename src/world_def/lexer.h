#ifndef MAGIC_WORLD_DEF_LEXER_H
#define MAGIC_WORLD_DEF_LEXER_H

#include <stdio.h>
#include "../types.h"
#include "../string.h"

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

/*
int
mgcd_eat_n_char(struct mgcd_lexer *ctx, size_t n);

int
mgcd_peek_char(struct mgcd_lexer *ctx);

int
mgcd_peek_char_offset(struct mgcd_lexer *ctx, size_t offset);

bool
mgcd_expect_sym(struct mgcd_lexer *ctx, struct string word);

bool
mgcd_expect_char(struct mgcd_lexer *ctx, int expected);

struct string
mgcd_eat_word_alphanum(struct mgcd_lexer *ctx);

struct string
mgcd_eat_word_num(struct mgcd_lexer *ctx);

void
mgcd_eat_whitespace(struct mgcd_lexer *ctx, bool keep_token);

void
mgcd_eat_any_whitespace(struct mgcd_lexer *ctx, bool keep_token);

void
mgcd_eat_rest_of_line(struct mgcd_lexer *ctx);

struct mgcd_token
mgcd_eat_single_char_token(struct mgcd_lexer *ctx, enum mgcd_token_type type);

bool
mgcd_expect_version_num(struct mgcd_lexer *ctx,
		struct mgcd_version *out_version);
*/

#endif
