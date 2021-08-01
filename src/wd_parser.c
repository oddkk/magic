#include <stdio.h>
#include <string.h>
#include "wd_parser.h"
#include "wd_lexer.h"
#include "utils.h"
#include "config.h"

#include "wd_shape.h"
#include "wd_area.h"
#include "wd_material.h"

void
mgcd_parse_init(struct mgcd_parser *parser,
		struct mgcd_context *ctx,
		struct mgcd_lexer *lex)
{
	parser->ctx = ctx;
	parser->lex = lex;
	parser->finalize_job = MGCD_JOB_NONE;
}

struct mgcd_token
mgcd_eat_token(struct mgcd_parser *parser)
{
	struct mgcd_token tok;
	if (parser->peek_buffer_count > 0) {
		tok = parser->peek_buffer[0];
		parser->peek_buffer_count = 0;
	} else {
		tok = mgcd_read_token(parser->lex);
#if MGCD_DEBUG_TOKENIZER
		mgcd_print_token(&tok);
#endif
	}

	return tok;
}

struct mgcd_token
mgcd_peek_token(struct mgcd_parser *parser)
{
	if (parser->peek_buffer_count == 0) {
		parser->peek_buffer[0] = mgcd_read_token(parser->lex);
#if MGCD_DEBUG_TOKENIZER
		mgcd_print_token(&parser->peek_buffer[0]);
#endif
		parser->peek_buffer_count = 1;
	}

	struct mgcd_token tok;
	tok = parser->peek_buffer[0];
	return tok;
}

bool
mgcd_try_get_ident(struct mgcd_token tok, struct atom **out_atom)
{
	if (tok.type != MGCD_TOK_IDENT) {
		return false;
	}

	if (out_atom) {
		*out_atom = tok.ident;
	}
	return true;
}

bool
mgcd_try_eat_ident(struct mgcd_parser *parser, struct atom **out_atom, struct mgc_location *out_loc)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);
	if (mgcd_try_get_ident(tok, out_atom)) {
		mgcd_eat_token(parser);
		if (out_loc) {
			*out_loc = tok.loc;
		}
		return true;
	}

	return false;
}

bool
mgcd_try_get_version(struct mgcd_token tok, struct mgcd_version *out_version)
{
	if (tok.type != MGCD_TOK_VERSION) {
		return false;
	}

	if (out_version) {
		*out_version = tok.version;
	}
	return true;
}

bool
mgcd_try_eat_version(struct mgcd_parser *parser,
		struct mgcd_version *out_version,
		struct mgc_location *out_loc)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);
	if (mgcd_try_get_version(tok, out_version)) {
		mgcd_eat_token(parser);
		if (out_loc) {
			*out_loc = tok.loc;
		}
		return true;
	}

	return false;
}

struct mgcd_tmp_path_segment {
	struct atom *ident;
	struct mgcd_tmp_path_segment *next;
};

bool
mgcd_expect_path(struct mgcd_parser *parser, struct mgc_location *out_loc, struct mgcd_path *out_path)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);

	struct mgc_location loc = tok.loc;

	bool path_relative = tok.path_seg.flags & MGCD_PATH_SEG_REL;

	size_t num_segments = 0;
	struct mgcd_tmp_path_segment *head = NULL, **end = &head;
	struct arena *mem = parser->ctx->tmp_mem;
	arena_mark cp = arena_checkpoint(mem);

	while ((tok.type == MGCD_TOK_PATH_SEG ||
				tok.type == MGCD_TOK_PATH_OPEN_EXPR)) {
		mgcd_eat_token(parser);

		if (tok.type == MGCD_TOK_PATH_OPEN_EXPR) {
			panic("TODO: Path expressions");
			arena_reset(mem, cp);
			return false;
		}

		if (tok.path_seg.ident) {
			struct mgcd_tmp_path_segment *seg;
			seg = arena_alloc(mem, sizeof(struct mgcd_tmp_path_segment));
			seg->ident = tok.path_seg.ident;
			*end = seg;
			end = &seg->next;
			num_segments += 1;
		}
		
		if (tok.path_seg.flags & MGCD_PATH_SEG_END) {
			break;
		}

		tok = mgcd_peek_token(parser);
	}

	if (tok.type != MGCD_TOK_PATH_SEG &&
			tok.type != MGCD_TOK_CLOSE_BLOCK) {
		// struct string token_name;
		// token_name = mgcd_token_type_name(tok.type);
		// mgc_error(parser->ctx->err, tok.loc,
		// 		"Expected path, got %.*s.",
		// 		LIT(token_name));
		arena_reset(mem, cp);
		return false;
	}

	struct mgcd_path result = {0};

	result.origin = path_relative ? MGCD_PATH_REL : MGCD_PATH_ABS;
	result.num_segments = num_segments;
	result.segments = arena_allocn(
		parser->ctx->mem,
		result.num_segments,
		sizeof(struct atom *)
	);

	size_t seg_i = 0;
	struct mgcd_tmp_path_segment *it = head;
	while (it) {
		assert(seg_i < result.num_segments);
		result.segments[seg_i] = it->ident;

		it = it->next;
		seg_i += 1;
	}
	assert(seg_i == result.num_segments);

	*out_path = result;

	loc = mgc_loc_combine(loc, tok.loc);

	if (out_loc) {
		*out_loc = loc;
	}

	arena_reset(mem, cp);
	return true;
}

bool
mgcd_expect_var_resource(
		struct mgcd_parser *parser,
		enum mgcd_entry_type disposition,
		MGCD_TYPE(mgcd_resource_id) *out)
{
	struct mgcd_path path = {0};
	struct mgc_location loc = {0};
	struct atom *ident = NULL;
	struct mgc_location ident_loc = {0};

	if (mgcd_expect_path(parser, &loc, &path)) {
		mgcd_resource_id result;
		result = mgcd_request_resource(
				parser->ctx, parser->finalize_job,
				loc, parser->root_scope, path);

		if (result >= 0) {
			*out = mgcd_var_lit_mgcd_resource_id(result);
			return true;
		}
	} else if (mgcd_try_eat_ident(parser, &ident, &ident_loc)) {
		enum mgcd_entry_type ident_disposition;
		ident_disposition = mgcd_entry_type_from_name(parser->ctx, ident);

		if (ident_disposition == MGCD_ENTRY_UNKNOWN) {
			mgc_error(parser->ctx->err, ident_loc,
					"Expected a resource path or a resource block.");
			return false;
		}

		if (disposition != MGCD_ENTRY_UNKNOWN &&
				ident_disposition != disposition) {
			struct string expected, got;
			expected = mgcd_entry_type_name(disposition);
			got = mgcd_entry_type_name(disposition);
			mgc_error(parser->ctx->err, ident_loc,
					"Expected a %.*s, got %.*s.",
					LIT(expected), LIT(got));
			return false;
		}
	}

	if (mgcd_expect_tok(parser, MGCD_TOK_OPEN_BLOCK)) {
		if (disposition == MGCD_ENTRY_UNKNOWN) {
			mgc_error(parser->ctx->err, ident_loc,
					"Unable to determine the type of this inline resource. "
					"Specify the intended resource type");
			return false;
		}
		mgcd_resource_id res_id;
		if (!mgcd_parse_bracketed_resource_block(parser, disposition, &res_id)) {
			mgc_error(parser->ctx->err, ident_loc,
					"Expected a path or resource block.");
			return false;
		}

		*out = mgcd_var_lit_mgcd_resource_id(res_id);
		return true;
	}

	return false;
}

bool
mgcd_expect_var_int(struct mgcd_parser *parser, MGCD_TYPE(int) *out)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);

	switch (tok.type) {

		case MGCD_TOK_IDENT:
			panic("TODO: Variables");
			return false;

		case MGCD_TOK_INTEGER_LIT:
			mgcd_eat_token(parser);
			*out = mgcd_var_lit_int(tok.integer_lit);
			return true;

		default:
			return false;
	}
}

bool
mgcd_expect_tok(struct mgcd_parser *parser, enum mgcd_token_type type)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);

	if (tok.type == type) {
		mgcd_eat_token(parser);
		return true;
	}

	return false;
}

void
mgcd_report_unexpect_tok(struct mgcd_parser *parser, enum mgcd_token_type type)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);

	struct string expected_token_name, token_name;
	expected_token_name = mgcd_token_type_name(type);
	token_name = mgcd_token_type_name(tok.type);
	mgc_error(parser->ctx->err, tok.loc,
			"Expected '%.*s', got '%.*s'.", LIT(expected_token_name), LIT(token_name));
}

#if 0
static bool
mgcd_try_get_int(struct mgcd_token tok, int *out_int)
{
	if (tok.type != MGCD_TOK_INTEGER_LIT) {
		return false;
	}

	if (out_int) {
		*out_int = tok.integer_lit;
	}

	return true;
}
#endif

bool
mgcd_expect_int(struct mgcd_parser *parser, int *out)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);

	switch (tok.type) {

		case MGCD_TOK_INTEGER_LIT:
			mgcd_eat_token(parser);
			*out = tok.integer_lit;
			return true;

		default:
			return false;
	}
}

bool
mgcd_expect_var_v3i(struct mgcd_parser *parser, MGCD_TYPE(v3i) *out)
{
	struct mgcd_token tok;
	tok = mgcd_peek_token(parser);

	switch (tok.type) {

		case MGCD_TOK_IDENT:
			panic("TODO: Variables");
			return false;

		case MGCD_TOK_OPEN_TUPLE:
			{
				v3i result = {0};

				mgcd_eat_token(parser);

				for (size_t i = 0; i < 3; i++) {
					if (!mgcd_expect_int(parser, &result.m[i])) {
						return false;
					}
					if (!mgcd_expect_tok(parser, MGCD_TOK_ARG_SEP) && i != 2) {
						return false;
					}
				}

				if (!mgcd_expect_tok(parser, MGCD_TOK_CLOSE_TUPLE)) {
					mgcd_report_unexpect_tok(parser, MGCD_TOK_CLOSE_TUPLE);
					return false;
				}

				*out = mgcd_var_lit_v3i(result);
				return true;
			}

		default:
			return false;
	}
}

bool
mgcd_stmt_error_recover(struct mgcd_parser *parser)
{
	struct mgcd_token current_token;
	current_token = mgcd_peek_token(parser);
	while (current_token.type != MGCD_TOK_END_STMT &&
			current_token.type != MGCD_TOK_CLOSE_BLOCK &&
			current_token.type != MGCD_TOK_EOF) {
		mgcd_eat_token(parser);
		current_token = mgcd_peek_token(parser);
	}

	return current_token.type != MGCD_TOK_EOF;
}

bool
mgcd_block_error_recover(struct mgcd_parser *parser)
{
	struct mgcd_token current_token;
	current_token = mgcd_peek_token(parser);
	while (current_token.type != MGCD_TOK_CLOSE_BLOCK &&
			current_token.type != MGCD_TOK_EOF) {
		mgcd_eat_token(parser);
		current_token = mgcd_peek_token(parser);
	}

	return current_token.type != MGCD_TOK_EOF;
}

bool
mgcd_block_continue(struct mgcd_token tok)
{
	switch (tok.type) {
		case MGCD_TOK_EOF:
		case MGCD_TOK_CLOSE_BLOCK:
			return false;

		default:
			return true;
	}
}

bool
mgcd_parse_bracketed_resource_block(
		struct mgcd_parser *parser,
		enum mgcd_entry_type type,
		mgcd_resource_id *out_id)
{
	// if (!mgcd_expect_tok(parser, MGCD_TOK_OPEN_BLOCK)) {
	// 	mgcd_report_unexpect_tok(parser, MGCD_TOK_OPEN_BLOCK);
	// 	return false;
	// }

	if (!mgcd_parse_anonymous_resource_block(parser, type, out_id)) {
		mgcd_block_error_recover(parser);
		return false;
	}

	if (!mgcd_expect_tok(parser, MGCD_TOK_CLOSE_BLOCK)) {
		mgcd_report_unexpect_tok(parser, MGCD_TOK_CLOSE_BLOCK);
		return false;
	}

	return true;
}

bool
mgcd_parse_anonymous_resource_block(
		struct mgcd_parser *parser,
		enum mgcd_entry_type type,
		mgcd_resource_id *out_id)
{
	mgcd_resource_id res_id;
	res_id = mgcd_alloc_anonymous_resource(parser->ctx, type, MGC_NO_LOC);

	if (mgcd_parse_resource_block(parser, type, res_id)) {
		*out_id = res_id;
		return true;
	}

	return false;
}

bool
mgcd_parse_resource_block(
		struct mgcd_parser *parser,
		enum mgcd_entry_type disposition,
		mgcd_resource_id res_id)
{
	struct mgcd_resource *resource;
	resource = mgcd_resource_get(parser->ctx, res_id);

	struct mgcd_context *ctx = parser->ctx;

	switch (disposition) {
		case MGCD_ENTRY_UNKNOWN:
			mgc_error(ctx->err, MGC_NO_LOC, // mgc_loc_file(resource->error_fid),
					"Unknown file type.");
			return false;

		case MGCD_ENTRY_SCOPE:
			{
			}
			break;

		case MGCD_ENTRY_SHAPE:
			{
				struct mgcd_shape *shape;
				shape = arena_alloc(ctx->mem, sizeof(struct mgcd_shape));

				if (!mgcd_parse_shape_block(
						parser, shape, &shape->ops)) {
					return false;
				}

				resource->shape.def = shape;

				return true;
			}

		case MGCD_ENTRY_AREA:
			{
				struct mgcd_area *area;
				area = arena_alloc(ctx->mem, sizeof(struct mgcd_area));

				if (!mgcd_parse_area_block(
							parser, area, &area->ops)) {
					return false;
				}

				resource->area.def = area;

				return true;
			}

		case MGCD_ENTRY_MATERIAL:
			{
				struct mgcd_material *material;
				material = arena_alloc(ctx->mem, sizeof(struct mgcd_material));

				if (!mgcd_parse_material_block(parser, material)) {
					return false;
				}

				if (!material->name) {
					mgc_error(ctx->err, MGC_NO_LOC, // mgc_loc_file(file->error_fid),
							"Material is missing name.");
					return false;
				}

				resource->material.def = material;

				return true;
			}
	}

	return false;
}
