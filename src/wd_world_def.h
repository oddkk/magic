#ifndef MAGIC_WORLD_DEF_H
#define MAGIC_WORLD_DEF_H

#include "types.h"
#include "atom.h"
#include "str.h"
#include "errors.h"
#include "wd_vars.h"
#include "wd_path.h"
#include <stdio.h>

typedef int mgcd_job_id;
typedef int mgcd_file_id;
typedef int mgcd_resource_id;
typedef int mgcd_shape_id;

#define MGCD_RESOURCE_NONE ((mgcd_resource_id)-1)
#define MGCD_FILE_NONE ((mgcd_file_id)-1)
#define MGCD_JOB_NONE ((mgcd_job_id)-1)

struct mgcd_version {
	int major, minor;
};

#define MGCD_ATOMS \
	ATOM(shape) \
	ATOM(cell) \
	ATOM(heightmap) \
	ATOM(hexagon) \
	ATOM(between) \
	ATOM(add) \
	ATOM(remove) \
	ATOM(coord) \
	ATOM(file) \
	ATOM(center) \
	ATOM(radius) \
	ATOM(height) \
	ATOM(shp) \
	ATOM(area) \
	ATOM(mat) \
	ATOM(mgc) \
	ATOM(matref) \

struct mgcd_atoms {
#define ATOM(name) struct atom *name;
	MGCD_ATOMS
#undef ATOM
};

enum mgcd_entry_type {
	MGCD_ENTRY_UNKNOWN = 0,
	MGCD_ENTRY_SCOPE,
	MGCD_ENTRY_SHAPE,
	MGCD_ENTRY_AREA,
	MGCD_ENTRY_MATERIAL,
};

enum mgcd_resource_state {
	MGCD_RES_UNLOADED = 0,
	MGCD_RES_REQUESTED,
	MGCD_RES_LOADED,
	MGCD_RES_FAILED,
};

struct mgcd_shape;
struct mgc_shape;

struct mgcd_area;
struct mgc_area;

typedef uint16_t mgc_material_id;

// TODO: Handle other file types like images and audio?
struct mgcd_resource {
	mgcd_resource_id id;
	struct atom *name;
	// struct string full_path;
	mgcd_file_id file;
	bool requested;
	bool dirty;
	bool failed;

	mgcd_job_id names_resolved;
	// Replace finalization with validation and let compilation of objects
	// be performed or requested directly by the requesting site.
	mgcd_job_id finalized;

	enum mgcd_entry_type type;

	union {
		struct {
			mgcd_resource_id children;
		} scope;

		struct {
			struct mgcd_shape *def;
			struct mgc_shape *ready;
		} shape;

		struct {
			struct mgcd_area *def;
			struct mgc_area *ready;
		} area;

		struct {
			struct mgcd_material *def;
			mgc_material_id real_id;
		} material;
	};

	mgcd_resource_id next, parent;
	struct mgc_location loc;
};

struct mgcd_resource_dependency {
	mgcd_resource_id from, to;
	struct mgcd_resource_dependency *next_from, *next_to;
};

struct mgcd_file {
	struct atom *file_name;
	enum mgcd_entry_type disposition;

	mgcd_resource_id resource;

	mgcd_job_id parsed;
	mgcd_job_id finalized;

	file_id_t error_fid;
};

struct mgcd_context {
	struct mgcd_atoms atoms;

	struct atom_table *atom_table;
	struct arena *mem;
	struct arena *tmp_mem;

	struct paged_list jobs;
	mgcd_job_id free_list;
	mgcd_job_id terminal_jobs;
	size_t unvisited_job_deps;

	struct mgc_registry *registry;

	struct paged_list files;
	struct paged_list resources;
	struct paged_list resource_dependencies;

	mgcd_resource_id root_scope;

	// This string is 0-terminated.
	struct string asset_root;

	struct mgc_error_context *err;

	size_t run_i;
};

void
mgcd_context_init(struct mgcd_context *,
		struct atom_table *atom_table,
		struct mgc_memory *memory,
		struct arena *mem,
		struct arena *tmp_mem,
		struct mgc_registry *registry,
		struct mgc_error_context *err);

mgcd_resource_id
mgcd_request_resource(struct mgcd_context *, mgcd_job_id req_job, struct mgc_location,
		mgcd_resource_id root_scope, struct mgcd_path);

mgcd_resource_id
mgcd_request_resource_str(struct mgcd_context *, mgcd_job_id req_job, struct mgc_location,
		mgcd_resource_id root_scope, struct string);

mgcd_resource_id
mgcd_alloc_anonymous_resource(struct mgcd_context *, enum mgcd_entry_type, struct mgc_location);

void
mgcd_init_job_resolve(struct mgcd_context *, enum mgcd_entry_type, mgcd_resource_id);

struct mgcd_resource *
mgcd_resource_get(struct mgcd_context *, mgcd_resource_id);

struct mgcd_file *
mgcd_file_get(struct mgcd_context *, mgcd_file_id);

mgcd_file_id 
mgcd_file_find(struct mgcd_context *, struct atom *file_name);

struct mgc_shape *
mgcd_expect_shape(struct mgcd_context *, mgcd_resource_id);

mgc_material_id
mgcd_expect_material(struct mgcd_context *, mgcd_resource_id);

enum mgcd_entry_type
mgcd_entry_type_from_name(struct mgcd_context *ctx, struct atom *name);

struct string
mgcd_entry_type_name(enum mgcd_entry_type type);

#endif
