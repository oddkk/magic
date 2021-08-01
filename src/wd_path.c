#include "wd_path.h"
#include "wd_world_def.h"
#include "str.h"
#include "arena.h"
#include "types.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

struct mgcd_path_tmp_segment {
	struct mgcd_path_tmp_segment *next;
	struct atom *ident;
};

bool is_valid_ident(int c)
{
	if ((c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '0')) {
		return true;
	}

	switch (c) {
		case '_':
			return true;

		default:
			return false;
	}
}

static size_t
mgcd_path_simplify(struct atom_table *atom_table,
		struct mgcd_path_tmp_segment **segments, size_t num_segments)
{
	struct mgcd_path_tmp_segment **it = segments;
	struct atom *parent_atom = atom_create(atom_table, STR(".."));
	// TODO: Optimize. This can probably be done in a single pass.
	while (*it) {
		if ((*it)->next && (*it)->next->ident == parent_atom) {
			*it = (*it)->next->next;
			num_segments -= 2;
			it = segments;
			continue;
		}

		it = &(*it)->next;
	}

	return num_segments;
}

int
mgcd_path_parse_str(struct arena *mem, struct atom_table *atom_table, struct string path,
		struct mgcd_path *out)
{
	if (path.length == 0) {
		return -1;
	}

	arena_mark cp = arena_checkpoint(mem);

	char *tok = path.text;
	char *cur = path.text;
	char *end = path.text + path.length;

	struct mgcd_path_tmp_segment *head = NULL;
	struct mgcd_path_tmp_segment **next_last = &head;
	size_t num_segments = 0;

	enum mgcd_path_origin origin;
	origin = (peek_character(path, cur) == '/') ? MGCD_PATH_ABS : MGCD_PATH_REL;

	while (cur < end) {
		char *cur_start = cur;
		int c = read_character(path, &cur);

		switch (c) {
			case '/':
				if (tok < cur_start) {
					struct string ident = {0};
					ident.text = tok;
					ident.length = cur_start - tok;
					struct mgcd_path_tmp_segment *segment;
					segment = arena_alloc(mem, sizeof(struct mgcd_path_tmp_segment));

					segment->ident = atom_create(atom_table, ident);

					*next_last = segment;
					next_last = &segment->next;
					num_segments += 1;
				}
				tok = cur;
				break;

			case '.':
				if (tok < cur_start) {
					// Disallow idents contianing '.'.
					arena_reset(mem, cp);
					return -1;
				}
				c = read_character(path, &cur);
				switch (c) {
					case '.':
						c = peek_character(path, cur);
						if (c != '/') {
							// Disallow idents contianing '..'.
							arena_reset(mem, cp);
							return -1;
						}
						break;

					case '/':
						// Ignore './' and '/./'
						tok = cur;
						break;

					default:
						// Disallow idents contianing '.'.
						arena_reset(mem, cp);
						return -1;
				}
				break;

			default:
				if (is_valid_ident(c)) {
					break;
				} else {
					arena_reset(mem, cp);
					return -1;
				}
		}
	}

	if (tok < cur) {
		struct string ident = {0};
		ident.text = tok;
		ident.length = cur - tok;
		struct mgcd_path_tmp_segment *segment;
		segment = arena_alloc(mem, sizeof(struct mgcd_path_tmp_segment));

		segment->ident = atom_create(atom_table, ident);

		*next_last = segment;
		next_last = &segment->next;
		num_segments += 1;
	}

	num_segments = mgcd_path_simplify(atom_table, &head, num_segments);

	struct atom *parent_atom = atom_create(atom_table, STR(".."));
	if (origin == MGCD_PATH_ABS && head && head->ident == parent_atom) {
		// If we encounter a '/..' as the first segment the user has attempted
		// to access a resource above the asset path.
		arena_reset(mem, cp);
		return -1;
	}

	struct atom **segments = arena_alloc(mem, sizeof(struct mgcd_path_tmp_segment));
	struct mgcd_path_tmp_segment *it = head;
	for (size_t i = 0; i < num_segments; i++, it = it->next) {
		assert(it);
		// Because of the simplify, we should never encounter any '..' as segments.
		assert(it->ident != parent_atom);
		segments[i] = it->ident;
	}

	segments = arena_reset_and_keep(mem, cp, segments, sizeof(struct atom *) * num_segments);

	memset(out, 0, sizeof(struct mgcd_path));
	out->segments = segments;
	out->num_segments = num_segments;

	return 0;
}

int
mgcd_path_make_abs(struct mgcd_context *ctx,
		mgcd_resource_id root_scope, struct mgcd_path path,
		struct mgcd_path *out)
{
	if (path.origin == MGCD_PATH_ABS) {
		*out = path;
		return 0;
	}

	struct atom *parent_atom = atom_create(ctx->atom_table, STR(".."));

	if (root_scope == MGCD_RESOURCE_NONE) {
		bool parent_segment_found = false;
		for (size_t i = 0; i < path.num_segments; i++) {
			if (path.segments[i] == parent_atom) {
				parent_segment_found = true;
				break;
			}
		}

		if (!parent_segment_found) {
			*out = path;
			return 0;
		}

		root_scope = ctx->root_scope;
	}

	struct arena *mem = ctx->tmp_mem;
	arena_mark cp = arena_checkpoint(mem);

	struct mgcd_path_tmp_segment *root = NULL;
	struct mgcd_path_tmp_segment **head_ptr = &root;
	size_t num_segments = 0;

	// Build path to the root scope.
	mgcd_resource_id scope_it = root_scope;
	while (scope_it != root_scope) {
		struct mgcd_resource *scope = mgcd_resource_get(ctx, root_scope);

		struct mgcd_path_tmp_segment *new_seg;
		new_seg = arena_alloc(mem, sizeof(struct mgcd_path_tmp_segment));

		assert(scope->name);
		new_seg->ident = scope->name;

		new_seg->next = root;
		root = new_seg;
		num_segments += 1;

		if (head_ptr == NULL) {
			head_ptr = &new_seg->next;
		}

		scope_it = scope->parent;
	}

	// Append the given path.
	for (size_t i = 0; i < path.num_segments; i++) {
		struct mgcd_path_tmp_segment *new_seg;
		new_seg = arena_alloc(mem, sizeof(struct mgcd_path_tmp_segment));

		*head_ptr = new_seg;
		head_ptr = &new_seg->next;
		num_segments += 1;
	}

	num_segments = mgcd_path_simplify(ctx->atom_table, &root, num_segments);

	struct atom **segments = arena_alloc(mem, sizeof(struct mgcd_path_tmp_segment));
	struct mgcd_path_tmp_segment *it = root;
	for (size_t i = 0; i < num_segments; i++, it = it->next) {
		assert(it);
		// Because of the simplify, we should never encounter any '..' as segments.
		assert(it->ident != parent_atom);
		segments[i] = it->ident;
	}

	segments = arena_reset_and_keep(mem, cp, segments, sizeof(struct atom *) * num_segments);

	memset(out, 0, sizeof(struct mgcd_path));
	out->segments = segments;
	out->num_segments = num_segments;

	return 0;
}

void
mgcd_path_print(struct mgcd_path *path)
{
	if (path->origin == MGCD_PATH_REL) {
		printf(".");
	}

	for (size_t i = 0; i < path->num_segments; i++) {
		printf("/%.*s", ALIT(path->segments[i]));
	}
}

struct string
mgcd_path_to_string(struct arena *mem, struct mgcd_path *path)
{
	size_t len = 0;
	if (path->origin == MGCD_PATH_REL) {
		len += 1;
	}

	for (size_t i = 0; i < path->num_segments; i++) {
		len += 1 + path->segments[i]->name.length;
	}

	struct string result = {0};
	result.length = len;
	result.text = arena_alloc(mem, len + 1);

	char *it = result.text;
	if (path->origin == MGCD_PATH_REL) {
		*it = '.';
		it += 1;
	}

	for (size_t i = 0; i < path->num_segments; i++) {
		*it = '/';
		it += 1;
		memcpy(it,
				path->segments[i]->name.text,
				path->segments[i]->name.length);
		it += path->segments[i]->name.length;
	}

	return result;
}
