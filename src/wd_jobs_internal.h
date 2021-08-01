#ifndef MAGIC_WORLD_DEF_JOBS_INTERNAL_H
#define MAGIC_WORLD_DEF_JOBS_INTERNAL_H

#include "wd_jobs.h"

struct mgcd_job *
get_job(struct mgcd_context *ctx, mgcd_job_id id);

struct mgcd_job_dep {
	bool visited;
	mgcd_job_id to;
};

enum mgcd_job_kind {
	MGCD_JOB_FREE = 0,

#define JOB(name, ...) MGCD_JOB_##name,
	MGCD_JOBS
#undef JOB

	MGCD_JOB_LENGTH
};

struct mgcd_job {
	enum mgcd_job_kind kind;

	// Use to keep track of internal vertex id in cycle detection.
	int aux_id;
	bool failed;
	bool suspended;

	size_t num_incoming_deps;
	size_t num_outgoing_deps;
	struct mgcd_job_dep *outgoing_deps;

	// Used for the linked list terminal_jobs in mgcd_context.
	mgcd_job_id terminal_jobs;

	union {
#define JOB(name, type) type name;
		MGCD_JOBS
#undef JOB

		// Used when the job is not allocated.
		mgcd_job_id free_list;
	} data;
};

void
mgcd_report_cyclic_dependencies(struct mgcd_context *ctx);

struct mgc_location
mgcd_job_location(struct mgcd_context *ctx, mgcd_job_id job_id);

#if MGCD_DEBUG_JOBS
void
mgcd_debug_print_cyclic_dependencies(struct mgcd_context *ctx);
#endif

#if MGCD_DEBUG_JOBS
void
mgcd_print_job_desc(struct mgcd_context *ctx,
		mgcd_job_id job_id);
#endif

#endif
