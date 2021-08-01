#ifndef MAGIC_WORLD_DEF_COMPILER_H
#define MAGIC_WORLD_DEF_COMPILER_H

#include "wd_world_def.h"
#include "config.h"

struct mgcd_job_nop_data {
	mgcd_job_id *id_ref;
#if MGCD_DEBUG_JOBS
	struct string name;
#endif
};

#define MGCD_JOBS                                       \
	JOB(nop, struct mgcd_job_nop_data)                  \
	                                                    \
	JOB(file_parse, mgcd_file_id)                       \
	                                                    \
	JOB(res_names, mgcd_resource_id)                    \
	JOB(res_finalize, mgcd_resource_id)                 \


#define JOB(name, type)							\
	mgcd_job_id									\
	mgcd_job_##name(struct mgcd_context *ctx,	\
			type value);
MGCD_JOBS
#undef JOB

#define JOB(name, type) \
	struct mgcd_job_info \
	mgcd_job_get_info_##name(struct mgcd_context *, mgcd_job_id, type);
MGCD_JOBS
#undef JOB

#define JOB(name, type) \
	int mgcd_job_dispatch_##name(struct mgcd_context *, mgcd_job_id, type);
MGCD_JOBS
#undef JOB

#if MGCD_DEBUG_JOBS
mgcd_job_id
mgcd_job_nopf(struct mgcd_context *ctx, mgcd_job_id *ptr, char *fmt, ...)
	// TODO: Make this cross-compiler compliant.
	__attribute__((__format__ (__printf__, 3, 4)));
#else
#define mgcd_job_nopf(ctx, ptr, ...) \
		mgcd_job_nop(ctx, (struct mgcd_job_nop_data){ .id_ref=ptr })
#endif
struct mgcd_job_info {
	struct string description;
	struct mgc_location loc;

	// If num_targets is 0 or 1 and target is not NULL, target points to the
	// field that contains the specified job. If num_targets is greater than 1,
	// targets points to an array of such targets.
	size_t num_targets;
	union {
		mgcd_job_id *target;
		mgcd_job_id **targets;
	};
};

// Marks the given job as suspended. Suspended jobs are not dispatched until
// after they have been resumed.
void
mgcd_job_suspend(struct mgcd_context *ctx, mgcd_job_id);

void
mgcd_job_resume(struct mgcd_context *ctx, mgcd_job_id);

// Requests that from must be evaluated before to.
void
mgcd_job_dependency(struct mgcd_context *ctx,
		mgcd_job_id from_id, mgcd_job_id to_id);

void
mgcd_context_jobs_init(struct mgcd_context *ctx, struct mgc_memory *mem);

int
mgcd_jobs_dispatch(struct mgcd_context *ctx);

#endif
