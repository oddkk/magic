#include <stdio.h>
#include <string.h>
#include "wd_lexer.h"
#include "wd_world_def.h"
#include "wd_parser.h"
#include "utils.h"

#define LEX_BUFFER_SIZE (4096)
#define LEX_BUFFER_MAX_FILL (4096)

static struct string mgcd_token_names[] = {
#define TOKEN(name) NOCAST_STR(#name),
MGCD_TOKEN_TYPES
#undef TOKEN
};

struct string
mgcd_token_type_name(enum mgcd_token_type type)
{
	assert(type < MGCD_NUM_TOKENS);
	return mgcd_token_names[type];
}

void
mgcd_print_token(struct mgcd_token *tok)
{
	printf("%.*s", LIT(mgcd_token_type_name(tok->type)));

	switch (tok->type) {
		case MGCD_TOK_IDENT:
			printf(" %.*s", ALIT(tok->ident));
			break;

		case MGCD_TOK_INTEGER_LIT:
			printf(" %lli", tok->integer_lit);
			break;

		case MGCD_TOK_STRING_LIT:
			printf(" %.*s", LIT(tok->string_lit));
			break;

		case MGCD_TOK_VERSION:
			printf(" %i.%i", tok->version.major, tok->version.minor);
			break;

		case MGCD_TOK_PATH_SEG:
			printf(" %.*s%s%s",
					ALIT(tok->path_seg.ident),
					(tok->path_seg.flags & MGCD_PATH_SEG_REL) ? " (rel)" : "",
					(tok->path_seg.flags & MGCD_PATH_SEG_END) ? " (end)" : "");
			break;

		default:
			break;
	}

	printf("\n");
}

static bool
mgcd_parse_fill(struct mgcd_lexer *ctx, size_t need)
{
	if (ctx->eof) {
		return false;
	}

	size_t free = ctx->tok - ctx->buf;

	if (free < need) {
		return false;
	}

	memmove(ctx->buf, ctx->tok, ctx->lim - ctx->tok);

	ctx->lim -= free;
	ctx->cur -= free;
	ctx->tok -= free;

	size_t bytes_read;
	bytes_read = fread(ctx->lim, 1, free, ctx->fp);

	ctx->lim += bytes_read;
	ctx->buf_begin = ctx->buf_end;
	ctx->buf_end += bytes_read;

	if (ctx->lim < ctx->buf + LEX_BUFFER_SIZE) {
		if (feof(ctx->fp)) {
			ctx->eof = true;
			memset(ctx->lim, 0, LEX_BUFFER_MAX_FILL);
			ctx->lim += LEX_BUFFER_MAX_FILL;
		} else {
			perror("world def parse feof");
		}
	}

	return true;
}

int
mgcd_parse_open_file(
		struct mgcd_lexer *lex,
		struct mgcd_context *ctx,
		char *file,
		file_id_t fid)
{
	memset(lex, 0, sizeof(struct mgcd_lexer));

	lex->fp = fopen(file, "rb");
	if (!lex->fp) {
		perror("world def fopen");
		return -1;
	}

	lex->ctx = ctx;
	lex->mem = ctx->mem;
	lex->atom_table = ctx->atom_table;
	lex->buf = arena_alloc(lex->mem, LEX_BUFFER_SIZE + LEX_BUFFER_MAX_FILL);

	lex->lim = lex->buf + LEX_BUFFER_SIZE;
	lex->cur = lex->lim;
	lex->tok = lex->lim;
	lex->eof = false;

	lex->buf_begin = 0;
	lex->buf_end = 0;

	lex->tok_loc.byte_from = 0;
	lex->tok_loc.byte_to = 0;
	lex->tok_loc.line_from = 0;
	lex->tok_loc.line_to = 0;
	lex->tok_loc.col_from = 0;
	lex->tok_loc.col_to = 0;
	lex->tok_loc.file_id = fid;

	mgcd_parse_fill(lex, LEX_BUFFER_SIZE);

	return 0;
}

static void
mgcd_reset_token(struct mgcd_lexer *ctx)
{
	ctx->tok = ctx->cur;

	ctx->tok_loc.byte_from = ctx->tok_loc.byte_to;
	ctx->tok_loc.line_from = ctx->tok_loc.line_to;
	ctx->tok_loc.col_from  = ctx->tok_loc.col_to;
}

int
mgcd_eat_n_char(struct mgcd_lexer *ctx, size_t n)
{
	if (ctx->lim <= (ctx->cur + n)) {
		if (!mgcd_parse_fill(ctx, n)) {
			return 0;
		}
	}

	for (size_t i = 0; i < n; i++) {
		ctx->tok_loc.byte_to += 1;
		// TODO: UTF-8
		ctx->tok_loc.col_to += 1;
		if (ctx->cur[i] == '\n') {
			ctx->tok_loc.line_to += 1;
			ctx->tok_loc.col_to = 1;
		}
	}

	ctx->cur += n;
	int c = *ctx->cur;

	// if (c == '\n') {
	// 	ctx->tok_loc.byte_to += 1;
	// }

	return c;
}

int
mgcd_peek_char(struct mgcd_lexer *ctx)
{
	return *ctx->cur;
}

int
mgcd_peek_char_offset(struct mgcd_lexer *ctx, size_t offset)
{
	if (ctx->lim <= (ctx->cur + offset)) {
		if (!mgcd_parse_fill(ctx, offset)) {
			return 0;
		}
	}

	return *(ctx->cur+offset);
}


static bool
mgcd_expect_sym(struct mgcd_lexer *ctx, struct string word)
{
	for (size_t i = 0; i < word.length; i++) {
		int c = mgcd_peek_char_offset(ctx, i);

		if (c != word.text[i]) {
			return false;
		}
	}

	mgcd_eat_n_char(ctx, word.length);
	return true;
}

bool
mgcd_expect_char(struct mgcd_lexer *ctx, int expected)
{
	int c = mgcd_peek_char(ctx);

	if (c == expected) {
		mgcd_eat_n_char(ctx, 1);
		return true;
	}

	return false;
}

/*
static struct string
mgcd_current_token(struct mgcd_lexer *ctx)
{
	struct string result = {0};
	result.text = ctx->tok;
	result.length = ctx->cur - ctx->tok;
	return result;
}
*/

static bool
char_is_alpha(int c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool
char_is_num(int c)
{
	return (c >= '0' && c <= '9');
}

static bool
char_is_alphanum(int c)
{
	return char_is_alpha(c) || char_is_num(c);
}

static bool
char_is_whitespace(int c)
{
	return c == ' '
		|| c == '\t';
}

static bool
char_is_newline(int c)
{
	return c == '\n'
		|| c == '\r';
}

static bool
char_is_any_whitespace(int c)
{
	return char_is_whitespace(c)
		|| char_is_newline(c);
}

struct string
mgcd_eat_word_alphanum(struct mgcd_lexer *ctx)
{
	int c = mgcd_peek_char(ctx);

	struct string result = {0};
	result.text = ctx->cur;

	while (char_is_alphanum(c)) {
		c = mgcd_eat_n_char(ctx, 1);
	}

	result.length = ctx->cur - result.text;

	return result;
}

struct string
mgcd_eat_word_num(struct mgcd_lexer *ctx)
{
	int c = mgcd_peek_char(ctx);

	struct string result = {0};
	result.text = ctx->cur;

	while (char_is_num(c)) {
		c = mgcd_eat_n_char(ctx, 1);
	}

	result.length = ctx->cur - result.text;

	return result;
}


void
mgcd_eat_whitespace(struct mgcd_lexer *ctx, bool keep_token)
{
	int c = mgcd_peek_char(ctx);

	while (char_is_whitespace(c)) {
		c = mgcd_eat_n_char(ctx, 1);

		if (!keep_token) {
			mgcd_reset_token(ctx);
		}
	}
}

void
mgcd_eat_any_whitespace(struct mgcd_lexer *ctx, bool keep_token)
{
	int c = mgcd_peek_char(ctx);

	while (char_is_any_whitespace(c)) {
		c = mgcd_eat_n_char(ctx, 1);

		if (!keep_token) {
			mgcd_reset_token(ctx);
		}
	}
}

void
mgcd_eat_rest_of_line(struct mgcd_lexer *ctx)
{
	int c = mgcd_peek_char(ctx);

	while (!char_is_newline(c) && c != 0) {
		c = mgcd_eat_n_char(ctx, 1);
		mgcd_reset_token(ctx);
	}
}

struct mgcd_token
mgcd_simple_token(struct mgcd_lexer *ctx, enum mgcd_token_type type)
{
	struct mgcd_token token = {0};
	token.type = type;
	token.loc = ctx->tok_loc;
	return token;
}

struct mgcd_token
mgcd_eat_single_char_token(struct mgcd_lexer *ctx, enum mgcd_token_type type)
{
	mgcd_eat_n_char(ctx, 1);
	return mgcd_simple_token(ctx, type);
}

bool
mgcd_expect_version_num(struct mgcd_lexer *ctx,
		struct mgcd_version *out_version)
{
	struct string major = {0}, minor = {0};
	char *cur_cp;

	cur_cp = ctx->cur;

	major = mgcd_eat_word_num(ctx);
	if (major.length == 0) {
		return false;
	}

	if (!mgcd_expect_char(ctx, '.')) {
		ctx->cur = cur_cp;
		return false;
	}

	minor = mgcd_eat_word_num(ctx);
	if (minor.length == 0) {
		ctx->cur = cur_cp;
		return false;
	}

	out_version->major = string_to_int64_base10(major);
	out_version->minor = string_to_int64_base10(minor);
	return true;
}

struct mgcd_token
mgcd_eat_path_seg(struct mgcd_lexer *ctx, enum mgcd_path_seg_flags flags)
{
	struct mgcd_token token = {0};

	token.path_seg.flags = flags;

	char sep = mgcd_peek_char(ctx);
	assert(sep == '/');
	mgcd_eat_n_char(ctx, 1);

	int c = mgcd_peek_char(ctx);
	if (char_is_alpha(c)) {
		struct string token_string;
		token_string = mgcd_eat_word_alphanum(ctx);

		token.type = MGCD_TOK_PATH_SEG;
		token.path_seg.ident = atom_create(ctx->atom_table, token_string);
	} else if (c == '.') {
		mgcd_eat_n_char(ctx, 1);

		c = mgcd_peek_char(ctx);
		if (c == '.') {
			mgcd_eat_n_char(ctx, 1);
			token.type = MGCD_TOK_PATH_SEG;
			token.path_seg.ident = atom_create(ctx->atom_table, STR(".."));
		} else if (c == '/') {
			token.type = MGCD_TOK_PATH_SEG;
		} else {
			// TODO: Should this case be allowed?
			// token.type = MGCD_TOK_PATH_SEG;
			mgc_error(ctx->ctx->err, ctx->tok_loc,
					"Cannot access path with trailing '/'.");
			struct mgcd_token error_token = {0};
			error_token.type = MGCD_TOK_ERROR;
			mgcd_eat_n_char(ctx, 1);
			return error_token;
		}
	} else if (c == '{') {
		token.type = MGCD_TOK_PATH_OPEN_EXPR;
		return token;
	}

	c = mgcd_peek_char(ctx);
	if (c == '/') {
		token.path_seg.flags &= ~MGCD_PATH_SEG_END;
	} else {
		token.path_seg.flags |= MGCD_PATH_SEG_END;
	}

	token.loc = ctx->tok_loc;

	return token;
}

struct mgcd_token
mgcd_read_token(struct mgcd_lexer *ctx)
{
	mgcd_reset_token(ctx);

	int c;

	bool should_continue = true;

	while (should_continue) {
		c = mgcd_peek_char(ctx);

		should_continue = false;

		switch (c) {
			case 0:
				{
					struct mgcd_token result = {0};
					result.type = MGCD_TOK_EOF;
					return result;
				}

			case '.':
				mgcd_eat_n_char(ctx, 1);
				switch (mgcd_peek_char(ctx)) {
					case '/':
						return mgcd_eat_path_seg(
								ctx, MGCD_PATH_SEG_REL);

					default:
						// The '.' has already been eaten.
						return mgcd_simple_token(
								ctx, MGCD_TOK_ACCESS);
				}

			case '/':
				return mgcd_eat_path_seg(ctx, 0);

			case '{':
				return mgcd_eat_single_char_token(
						ctx, MGCD_TOK_OPEN_BLOCK);

			case '}':
				{
					mgcd_eat_n_char(ctx, 1);
					c = mgcd_peek_char(ctx);
					if (c == '/') {
						return mgcd_simple_token(
								ctx, MGCD_TOK_PATH_CLOSE_EXPR);
					} else {
						return mgcd_simple_token(
								ctx, MGCD_TOK_CLOSE_BLOCK);
					}
				}

			case '(':
				return mgcd_eat_single_char_token(
						ctx, MGCD_TOK_OPEN_TUPLE);

			case ')':
				return mgcd_eat_single_char_token(
						ctx, MGCD_TOK_CLOSE_TUPLE);

			case ',':
				return mgcd_eat_single_char_token(
						ctx, MGCD_TOK_ARG_SEP);

			case '=':
				return mgcd_eat_single_char_token(
						ctx, MGCD_TOK_ASSIGN);

			case ';':
				return mgcd_eat_single_char_token(
						ctx, MGCD_TOK_END_STMT);

			case '#':
				{
					if (ctx->buf_begin == 0 && ctx->cur == ctx->buf) {
						struct string version_sym = STR("version");
						mgcd_eat_n_char(ctx, 1);
						mgcd_eat_whitespace(ctx, false);

						if (mgcd_expect_sym(ctx, version_sym)) {
							mgcd_eat_whitespace(ctx, false);
							mgcd_reset_token(ctx);

							struct mgcd_token version_token = {0};
							version_token.type = MGCD_TOK_VERSION;
							if (mgcd_expect_version_num(ctx, &version_token.version)) {
								mgcd_eat_rest_of_line(ctx);
								mgcd_eat_any_whitespace(ctx, false);
								return version_token;
							}
						}
					}

					mgcd_eat_rest_of_line(ctx);
					mgcd_eat_any_whitespace(ctx, false);
					should_continue = true;
					break;
				}

			default:
				if (char_is_alpha(c)) {
					struct string token_string;
					token_string = mgcd_eat_word_alphanum(ctx);

					struct mgcd_token token = {0};
					token.type = MGCD_TOK_IDENT;
					token.ident = atom_create(ctx->atom_table, token_string);
					return token;

				} else if (char_is_num(c) || c == '-') {
					int sign = 1;
					if (c == '-') {
						sign = -1;
						c = mgcd_eat_n_char(ctx, 1);
						if (!char_is_num(c)) {
							mgc_error(ctx->ctx->err, MGC_NO_LOC,
									"Expected number after '-'.");
							// TODO: Error handeling (error token)
							should_continue = true;
							continue;
						}
					}

					struct string int_string;
					int_string = mgcd_eat_word_num(ctx);

					struct mgcd_token token = {0};
					token.type = MGCD_TOK_INTEGER_LIT;
					token.integer_lit = string_to_int64_base10(int_string) * sign;
					return token;

				} else if (char_is_any_whitespace(c)) {
					mgcd_eat_any_whitespace(ctx, false);
					should_continue = true;
				} else {
					mgcd_eat_n_char(ctx, 1);
					mgc_error(ctx->ctx->err, ctx->tok_loc,
							"Unexpected character '%c' (%i)", c, c);
					mgcd_reset_token(ctx);
					should_continue = true;
				}
				break;
		}
	}

	panic("Unexpected end of token parser.");
	struct mgcd_token eof_token = {0};
	eof_token.type = MGCD_TOK_EOF;
	return eof_token;
}

