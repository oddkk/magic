#include "world_def.h"
#include "vars.h"
#include "shape.h"
#include "jobs.h"
#include "lexer.h"
#include "parser.h"
#include "../utils.h"

#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <stdlib.h>
#include <fts.h>

const char *
mgcd_res_type_name(enum mgcd_entry_type type)
{
	switch (type) {
		case MGCD_ENTRY_UNKNOWN:
			return "unknown";

		case MGCD_ENTRY_SCOPE:
			return "scope";

		case MGCD_ENTRY_SHAPE:
			return "shape";
	}
}

void
mgcd_world_init(struct mgcd_world *world, struct mgc_memory *mem)
{
	paged_list_init(&world->shapes, mem,
			sizeof(struct mgcd_shape_op));
	paged_list_init(&world->shape_ops, mem,
			sizeof(struct mgcd_shape_op));
}

static mgcd_resource_id
mgcd_alloc_resource(struct mgcd_context *ctx)
{
	mgcd_resource_id id;
	id = paged_list_push(&ctx->resources);

	struct mgcd_resource *root_resource;
	root_resource = mgcd_resource_get(ctx, id);
	root_resource->id = id;
	root_resource->type = MGCD_ENTRY_UNKNOWN;
	root_resource->next = -1;
	root_resource->parent = -1;

	return id;
}

static mgcd_resource_id
mgcd_file_alloc(struct mgcd_context *ctx)
{
	return paged_list_push(&ctx->files);
}

static int
mgcd_path_get_asset_root(struct arena *mem, struct string *out)
{
	char exe_path[PATH_MAX];
	memset(exe_path, 0, PATH_MAX * sizeof(char));
	ssize_t err;

	err = readlink("/proc/self/exe", exe_path, PATH_MAX);
	if (err == -1) {
		perror("readlink");
		return -1;
	} else if (err == PATH_MAX) {
		printf("readlink: Executable path was too long.\n");
		return -1;
	} else if (err < 0) {
		printf("readlink: Failed.\n");
		return -1;
	}

	char *exe_dir = dirname(exe_path);
	size_t exe_dir_length = strlen(exe_dir);

	struct string asset_dir = STR("/assets/");

	struct string result = {0};
	result.length = exe_dir_length + asset_dir.length;
	result.text = arena_allocn(mem, result.length + 1, sizeof(char));
	memcpy(&result.text[0], exe_dir, exe_dir_length);
	memcpy(&result.text[exe_dir_length], asset_dir.text, asset_dir.length);
	result.text[result.length] = 0;

	*out = result;

	return 0;
}

void
mgcd_context_init(struct mgcd_context *ctx,
		struct mgcd_world *world,
		struct atom_table *atom_table,
		struct mgc_memory *memory,
		struct arena *mem,
		struct arena *tmp_mem,
		struct mgc_error_context *err)
{
	ctx->world = world;
	ctx->atom_table = atom_table;
	ctx->mem = mem;
	ctx->tmp_mem = tmp_mem;
	ctx->err = err;

	int e;
	e = mgcd_path_get_asset_root(mem, &ctx->asset_root);
	assert(!e);

	// Initialize common atoms.
#define ATOM(name) ctx->atoms.name = atom_create(ctx->atom_table, STR(#name));
	MGCD_ATOMS
#undef ATOM

	mgcd_context_jobs_init(ctx, memory);

	paged_list_init(&ctx->files, memory,
			sizeof(struct mgcd_file));
	paged_list_init(&ctx->resources, memory,
			sizeof(struct mgcd_resource));
	paged_list_init(&ctx->resource_dependencies, memory,
			sizeof(struct mgcd_resource_dependency));

	ctx->root_scope = mgcd_alloc_resource(ctx);
	struct mgcd_resource *root_resource;
	root_resource = mgcd_resource_get(ctx, ctx->root_scope);
	root_resource->next = MGCD_RESOURCE_NONE;
	root_resource->parent = MGCD_RESOURCE_NONE;
	root_resource->scope.children = MGCD_RESOURCE_NONE;
	root_resource->name = NULL;
	root_resource->type = MGCD_ENTRY_SCOPE;
}

// Returns
//  =0 if the resource already exsist.
//  >0 if the resource was created.
//  <0 if an error occurred.
int
mgcd_resource_get_or_create(struct mgcd_context *ctx,
		mgcd_resource_id res_id, struct atom *name, mgcd_resource_id *out)
{
	struct mgcd_resource *res;
	res = mgcd_resource_get(ctx, res_id);

	if (res->type != MGCD_ENTRY_SCOPE) {
		return -1;
	}

	mgcd_resource_id iter = res->scope.children;
	mgcd_resource_id *tail = &res->scope.children;
	while (iter != MGCD_RESOURCE_NONE) {
		struct mgcd_resource *child;
		child = mgcd_resource_get(ctx, iter);

		if (child->name == name) {
			*out = iter;
			return 0;
		}

		iter = child->next;
		tail = &child->next;
	}

	mgcd_resource_id new_id;
	new_id = mgcd_alloc_resource(ctx);

	struct mgcd_resource *child;
	child = mgcd_resource_get(ctx, new_id);

	child->name = name;
	child->parent = res_id;

	assert(*tail == MGCD_RESOURCE_NONE);
	*tail = new_id;

	*out = new_id;
	return 1;
}

struct mgcd_file *
mgcd_file_get(struct mgcd_context *ctx, mgcd_file_id file_id)
{
	return paged_list_get(&ctx->files, file_id);
}

mgcd_file_id 
mgcd_file_find(struct mgcd_context *ctx, struct atom *file_name)
{
	for (mgcd_file_id i = 0; i < ctx->files.length; i++) {
		struct mgcd_file *file;
		file = mgcd_file_get(ctx, i);
		if (file->file_name == file_name) {
			return i;
		}
	}

	return MGCD_FILE_NONE;
}

// Returns
//  =0 if the file already exsist.
//  >0 if the file was created.
//  <0 if an error occurred.
int
mgcd_file_get_or_create(struct mgcd_context *ctx,
		struct atom *file_name, mgcd_file_id *out)
{
	mgcd_file_id fid;
	fid = mgcd_file_find(ctx, file_name);

	if (fid == MGCD_FILE_NONE) {
		fid = mgcd_file_alloc(ctx);

		struct mgcd_file *file;
		file = mgcd_file_get(ctx, fid);

		file->file_name = file_name;

		file->parsed = mgcd_job_file_parse(ctx, fid);
		file->finalized = mgcd_job_nopf(
				ctx, &file->finalized,
				"file finalized");

		mgcd_job_dependency(ctx, file->parsed, file->finalized);

		*out = fid;
		return 1;
	}

	*out = fid;
	return 0;
}
int
mgcd_extract_extension(struct string filename,
		struct string *out_name, struct string *out_ext)
{
	out_name->text = filename.text;
	out_name->length = 0;

	out_ext->text = NULL;
	out_ext->length = 0;

	bool in_ext = false;

	char *iter = filename.text;
	int c;
	while ((c = read_character(filename, &iter)) != 0) {
		if (!in_ext && c == '.') {
			in_ext = true;
			out_ext->text = iter;
			out_ext->length = (filename.text + filename.length) - iter;
			return 0;
		} else {
			out_name->length = iter - filename.text;
		}
	}

	return 0;
}

enum mgcd_entry_type
mgcd_entry_type_from_ext(struct mgcd_context *ctx, struct atom *ext)
{
	if (ext == ctx->atoms.mgc) {
		return MGCD_ENTRY_SCOPE;
	} else if (ext == ctx->atoms.shp) {
		return MGCD_ENTRY_SHAPE;
	} else {
		return MGCD_ENTRY_UNKNOWN;
	}
}

mgcd_resource_id
mgcd_request_resource(struct mgcd_context *ctx, mgcd_resource_id origin_scope, struct mgcd_path path)
{
	int err;

	struct mgcd_path real_path;
	err = mgcd_path_make_abs(ctx, origin_scope, path, &real_path);
	if (err) {
		return MGCD_RESOURCE_NONE;
	}
	assert(real_path.origin == MGCD_PATH_ABS);

	mgcd_resource_id scope_id = ctx->root_scope;
	struct atom *parent_atom = atom_create(ctx->atom_table, STR(".."));

	for (size_t path_i = 0; path_i < real_path.num_segments; path_i++) {
		struct atom *ident = real_path.segments[path_i];

		struct mgcd_resource *scope;
		scope = mgcd_resource_get(ctx, scope_id);

		if (ident == parent_atom) {
			if (scope->parent == MGCD_RESOURCE_NONE) {
				printf("Attempted to read resource outside of assets.\n");
				return -1;
			}

			scope_id = scope->parent;
			continue;
		}

		mgcd_resource_id it = scope->scope.children;

		while (it > MGCD_RESOURCE_NONE) {
			struct mgcd_resource *res;
			res = mgcd_resource_get(ctx, it);

			if (res->name == ident) {
				scope_id = it;
				break;
			}

			it = res->next;
		}

		scope_id = MGCD_RESOURCE_NONE;
		break;
	}

	// The name was not found. Check file system.

	char *asset_directories[] = {
		ctx->asset_root.text,
		NULL
	};

	FTS *ftsp;
	ftsp = fts_open(asset_directories, FTS_PHYSICAL, NULL);
	if (!ftsp) {
		perror("fts_open");
		return -1;
	}

	scope_id = ctx->root_scope;

	int ret_err = 0;

	size_t path_i = 0;
	size_t remaining_path_i = 0;
	mgcd_resource_id file_res = MGCD_RESOURCE_NONE;
	mgcd_file_id fid_res = MGCD_FILE_NONE;
	bool last_skipped = false;

	FTSENT *f;
	while ((f = fts_read(ftsp)) != NULL) {
		assert(path_i < real_path.num_segments);
		struct atom *current_segment = real_path.segments[path_i];

		switch (f->fts_info) {
			case FTS_F:
			case FTS_D:
				{
					if (f->fts_info == FTS_D && f->fts_level == 0) {
						// The root of the asset dir should be the root scope.
						break;
					}

					struct string name_str = {0};
					struct atom *name = NULL, *ext = NULL;

					name_str.text = f->fts_name;
					name_str.length = f->fts_namelen;

					if (f->fts_info == FTS_F) {
						struct string ext_str = {0};
						mgcd_extract_extension(name_str,
								&name_str, &ext_str);
						ext = atom_create(ctx->atom_table, ext_str);
					}

					name = atom_create(ctx->atom_table, name_str);

					if (file_res != MGCD_RESOURCE_NONE && name == current_segment) {
						struct mgcd_resource *found_res;
						found_res = mgcd_resource_get(ctx, file_res);

						/*
						struct string dup_full_path;
						dup_full_path.text = f->fts_path;
						dup_full_path.length = f->fts_pathlen;
						mgc_error(ctx->err, MGC_NO_LOC,
								"Ambigous scope name '%.*s' and '%.*s'.",
								LIT(found_res->full_path), LIT(dup_full_path));
						*/
						// TODO: Include asset path in error message.
						mgc_error(ctx->err, MGC_NO_LOC,
								"Ambigous scope name.");
						ret_err = -1;
					}

					if (name == current_segment) {
						int err;
						mgcd_resource_id new_id;
						err = mgcd_resource_get_or_create(ctx,
								scope_id, name, &new_id);
						if (err < 0) {
							ret_err = -1;
							break;
						}

						scope_id = new_id;

						if (f->fts_info == FTS_F) {
							file_res = new_id;
							remaining_path_i = path_i + 1;
						} else {
							path_i += 1;

							if (path_i >= real_path.num_segments) {
								file_res = new_id;
								remaining_path_i = path_i + 1;
								fts_set(ftsp, f, FTS_SKIP);
								// Do not skip the decrement as we just
								// incremented path_i.
							}
						}

						if (err > 0) {
							// A new resource was created. Fill it out.
							struct mgcd_resource *new_res;
							new_res = mgcd_resource_get(ctx, new_id);

							if (f->fts_info == FTS_F) {
								new_res->type = mgcd_entry_type_from_ext(ctx, ext);

								char real_path_buffer[PATH_MAX+1];
								if (!realpath(f->fts_path, real_path_buffer)) {
									perror("realpath");
									ret_err = -1;
									break;
								}

								struct string real_path = {0};
								real_path.text = real_path_buffer;
								real_path.length = strlen(real_path_buffer);

								struct atom *file_atom;
								file_atom = atom_create(ctx->atom_table, real_path);

								mgcd_file_id fid;
								int err;
								err = mgcd_file_get_or_create(ctx, file_atom, &fid);

								if (err < 0) {
									ret_err = -1;
									break;
								}

								// The file should not already exsist as a
								// matching resource was not discovered.
								assert(err != 0);

								new_res->file = fid;
								fid_res = fid;

								struct mgcd_file *file;
								file = mgcd_file_get(ctx, fid);
								file->disposition = new_res->type;
								file->resource = new_id;

								new_res->names_resolved = mgcd_job_res_names(ctx, new_id);
								new_res->finalized = mgcd_job_res_finalize(ctx, new_id);
								mgcd_job_dependency(ctx,
										file->finalized,
										new_res->names_resolved);
								mgcd_job_dependency(ctx,
										new_res->names_resolved,
										new_res->finalized);
							} else {
								new_res->type = MGCD_ENTRY_SCOPE;
								new_res->scope.children = MGCD_RESOURCE_NONE;
							}
						}

					} else {
						fts_set(ftsp, f, FTS_SKIP);
						last_skipped = true;
					}
				}
				break;

			case FTS_DP:
				if (f->fts_level == 0) {
					// The root of the asset dir should be the root scope.
					break;
				}
				if (last_skipped) {
					last_skipped = false;
					break;
				}
				assert(path_i > 0);
				path_i -= 1;
				break;

			case FTS_ERR:
				perror("fts");
				ret_err = -1;
				break;
		}
	}

	fts_close(ftsp);

	if (file_res == MGCD_RESOURCE_NONE) {
		// TODO: Print resource path.
		mgc_error(ctx->err, MGC_NO_LOC,
				"Could not find a file for the resource ''");
		return MGCD_RESOURCE_NONE;
	}

	if (ret_err || file_res == MGCD_RESOURCE_NONE) {
		return MGCD_RESOURCE_NONE;
	}

	// Traverse the remaining path to find the requested resource in the file.
	mgcd_resource_id result_res = file_res;
	for (size_t i = remaining_path_i; i < path.num_segments; i++) {
		struct atom *ident = path.segments[i];

		int err;
		mgcd_resource_id new_id;
		err = mgcd_resource_get_or_create(ctx,
				result_res, ident, &new_id);
		if (err < 0) {
			ret_err = -1;
			break;
		}

		struct mgcd_resource *new_res;
		new_res = mgcd_resource_get(ctx, new_id);
		new_res->requested = true;
		new_res->dirty = true;
		new_res->file = fid_res;

		if (err > 0) {
			new_res->type = MGCD_ENTRY_UNKNOWN;
		} else {
			if (i != path.num_segments-1 &&
					new_res->type != MGCD_ENTRY_UNKNOWN &&
					new_res->type != MGCD_ENTRY_SCOPE) {
				mgc_error(ctx->err, MGC_NO_LOC,
						"Expected path to be a scope, but it is a %s.",
						mgcd_res_type_name(new_res->type));
				ret_err = -1;
				break;
			}
		}

		result_res = new_id;
	}

	if (ret_err) {
		return MGCD_RESOURCE_NONE;
	}

	struct mgcd_resource *res;
	res = mgcd_resource_get(ctx, result_res);
	res->requested = true;

	return result_res;
}

mgcd_resource_id
mgcd_request_resource_str(struct mgcd_context *ctx, mgcd_resource_id root_scope, struct string str)
{
	arena_mark cp = arena_checkpoint(ctx->tmp_mem);

	struct mgcd_path path = {0};
	int err = mgcd_path_parse_str(ctx->mem, ctx->atom_table, str, &path);
	if (err) {
		return -1;
	}

	mgcd_resource_id result;
	result = mgcd_request_resource(ctx, root_scope, path);

	arena_reset(ctx->tmp_mem, cp);

	return result;
}

struct mgcd_resource *
mgcd_resource_get(struct mgcd_context *ctx, mgcd_resource_id id)
{
	return paged_list_get(&ctx->resources, id);
}

struct mgcd_job_info
mgcd_job_get_info_file_parse(
		struct mgcd_context *ctx,
		mgcd_job_id job_id,
		mgcd_file_id fid)
{
	struct mgcd_job_info info = {0};

	struct mgcd_file *file;
	file = mgcd_file_get(ctx, fid);

	struct string file_name = file->file_name->name;

	if (file_name.length > ctx->asset_root.length) {
		struct string file_name_prefix;
		file_name_prefix.text = file_name.text;
		file_name_prefix.length = ctx->asset_root.length;

		if (string_equal(file_name_prefix, ctx->asset_root)) {
			file_name.text += ctx->asset_root.length;
			file_name.length -= ctx->asset_root.length;
		}
	}

	info.description = arena_sprintf(ctx->tmp_mem, "(0x%03x) %.*s",
			fid, LIT(file_name));
	info.loc = MGC_NO_LOC;

	return info;
}

int
mgcd_job_dispatch_file_parse(
		struct mgcd_context *ctx,
		mgcd_job_id job_id,
		mgcd_file_id fid)
{
	int err;

	struct mgcd_file *file;
	file = mgcd_file_get(ctx, fid);

	struct mgcd_resource *resource;
	resource = mgcd_resource_get(ctx, file->resource);

	arena_mark cp = arena_checkpoint(ctx->tmp_mem);

	char *path;
	path = arena_alloc(ctx->tmp_mem, file->file_name->name.length + 1);
	memcpy(path, file->file_name->name.text, file->file_name->name.length);
	path[file->file_name->name.length] = '\0';

	struct mgcd_lexer lexer = {0};
	err = mgcd_parse_open_file(&lexer, ctx, path);
	arena_reset(ctx->tmp_mem, cp);
	if (err) {
		return -1;
	}

	struct mgcd_parser parser = {0};
	mgcd_parse_init(&parser, ctx, &lexer);

	struct mgcd_version version;
	if (!mgcd_try_eat_version(&parser, &version)) {
		mgc_warning(ctx->err, MGC_NO_LOC,
				"Missing version number. Assuming newest version.\n");
	}

	int parse_err = 0;
	switch (file->disposition) {
		case MGCD_ENTRY_UNKNOWN:
			mgc_error(ctx->err, MGC_NO_LOC,
					"Unknown file type.");
			break;

		case MGCD_ENTRY_SCOPE:
			{
			}
			break;

		case MGCD_ENTRY_SHAPE:
			{
				struct mgcd_shape *shape;
				shape = arena_alloc(ctx->mem, sizeof(struct mgcd_shape));

				if (!mgcd_parse_shape_block(
						&parser, shape, &shape->ops)) {
					parse_err = -1;
					break;
				}

				resource->shape.def = shape;
			}
			break;
	}

	if (!mgcd_expect_tok(&parser, MGCD_TOK_EOF)) {
		mgc_error(ctx->err, MGC_NO_LOC,
				"Expected end of file.");
		parse_err = -1;
	}

	fclose(lexer.fp);

	return parse_err;
}

void
mgcd_resource_job_get_info(
		struct mgcd_context *ctx,
		struct mgcd_resource *res,
		struct mgcd_job_info *info)
{
	// TODO: Include full path.
	info->description = arena_sprintf(ctx->tmp_mem, "(0x%03x) %.*s",
			res->id, ALIT(res->name));
	info->loc = MGC_NO_LOC;
}

struct mgcd_job_info
mgcd_job_get_info_res_names(
		struct mgcd_context *ctx,
		mgcd_job_id job_id,
		mgcd_resource_id rid)
{
	struct mgcd_resource *res;
	res = mgcd_resource_get(ctx, rid);

	struct mgcd_job_info info = {0};
	mgcd_resource_job_get_info(ctx, res, &info);
	info.target = &res->names_resolved;
	return info;
}

int
mgcd_job_dispatch_res_names(
		struct mgcd_context *ctx,
		mgcd_job_id job_id,
		mgcd_resource_id rid)
{
	struct mgcd_resource *res;
	res = mgcd_resource_get(ctx, rid);

	// res->

	return 0;
}

struct mgcd_job_info
mgcd_job_get_info_res_finalize(
		struct mgcd_context *ctx,
		mgcd_job_id job_id,
		mgcd_resource_id rid)
{
	struct mgcd_resource *res;
	res = mgcd_resource_get(ctx, rid);

	struct mgcd_job_info info = {0};
	mgcd_resource_job_get_info(ctx, res, &info);
	info.target = &res->finalized;
	return info;
}

int
mgcd_job_dispatch_res_finalize(
		struct mgcd_context *ctx,
		mgcd_job_id job_id,
		mgcd_resource_id rid)
{
	struct mgcd_resource *res;
	res = mgcd_resource_get(ctx, rid);

	struct arena *mem = ctx->mem;

	switch (res->type) {
		case MGCD_ENTRY_UNKNOWN:
			panic("Attempted to finalize an unknown resource.");
			break;

		case MGCD_ENTRY_SCOPE:
			break;

		case MGCD_ENTRY_SHAPE:
			res->shape.ready = arena_alloc(mem, sizeof(struct mgc_shape));
			mgcd_complete_shape(ctx, res->shape.def, mem, res->shape.ready);
			break;

	}

	return 0;
}

static struct mgcd_resource *
mgcd_resource_get_expecting(
		struct mgcd_context *ctx,
		mgcd_resource_id id,
		enum mgcd_entry_type type)
{
	struct mgcd_resource *res;
	res = mgcd_resource_get(ctx, id);

	if (res->type != type) {
		panic("Expected resource to be %i, got %i.", type, res->type);
	}

	return res;
}

struct mgc_shape *
mgcd_expect_shape(struct mgcd_context *ctx, mgcd_resource_id id)
{
	struct mgcd_resource *res;
	res = mgcd_resource_get_expecting(ctx, id, MGCD_ENTRY_SHAPE);

	assert(res->shape.ready);

	return res->shape.ready;
}
