#include "jobs.h"
#include "jobs_internal.h"
#include "../utils.h"
#include "../dlist.h"
#include "../term_color.h"

#include <stdlib.h>
#include <string.h>

struct string mgcd_job_names[] = {
	STR("FREE"),

#define JOB(name, ...) STR(#name),
	MGCD_JOBS
#undef JOB
};

void
mgcd_context_jobs_init(struct mgcd_context *ctx, struct mgc_memory *mem)
{
	paged_list_init(&ctx->jobs, mem,
			sizeof(struct mgcd_job));

	ctx->free_list = -1;
	ctx->terminal_jobs = -1;
	ctx->unvisited_job_deps = 0;
}

#if MGCD_DEBUG_JOBS
static int
mgcd_job_name_max_length()
{
	static int result = -1;
	if (result < 0) {
		int max = 0;
		for (size_t i = 0; i < MGCD_JOB_LENGTH; i++) {
			max = mgcd_job_names[i].length > max
				? mgcd_job_names[i].length : max;
		}
		result = max;
	}
	return result;
}
#endif

struct mgcd_job *
get_job(struct mgcd_context *ctx, mgcd_job_id id)
{
	return paged_list_get(&ctx->jobs, id);
}

static inline void
mgcd_free_job(struct mgcd_context *ctx, mgcd_job_id id)
{
	struct mgcd_job *job;
	job = get_job(ctx, id);
	assert(job->kind != MGCD_JOB_FREE);

	free(job->outgoing_deps);

	memset(job, 0, sizeof(struct mgcd_job));
	job->kind = MGCD_JOB_FREE;

	job->data.free_list = ctx->free_list;
	ctx->free_list = id;
}

static mgcd_job_id
mgcd_alloc_job(struct mgcd_context *ctx)
{
	mgcd_job_id res = -1;
	if (ctx->free_list != -1) {
		res = ctx->free_list;

		struct mgcd_job *job;
		job = get_job(ctx, res);

		ctx->free_list = job->data.free_list;
		job->data.free_list = -1;
	} else {
		res = paged_list_push(&ctx->jobs);
	}

	assert(res >= 0);

	struct mgcd_job *job;
	job = get_job(ctx, res);

	memset(job, 0, sizeof(struct mgcd_job));

	job->data.free_list = -1;

	job->terminal_jobs = ctx->terminal_jobs;
	ctx->terminal_jobs = res;

	return res;
}

#define JOB(name, type)								\
	mgcd_job_id									\
	mgcd_job_##name(struct mgcd_context *ctx,	\
			type value)								\
{													\
	mgcd_job_id job_id;							\
	job_id = mgcd_alloc_job(ctx);					\
													\
	struct mgcd_job *job;							\
	job = get_job(ctx, job_id);						\
	job->kind = MGCD_JOB_##name;					\
	job->data.name = value;							\
													\
	return job_id;									\
}
MGCD_JOBS
#undef JOB

#if MGCD_DEBUG_JOBS
mgcd_job_id
mgcd_job_nopf(struct mgcd_context *ctx, mgcd_job_id *id_ref, char *fmt, ...) {
	struct mgcd_job_nop_data data = {0};
	data.id_ref = id_ref;

	va_list ap;

	va_start(ap, fmt);
	data.name = arena_vsprintf(ctx->tmp_mem, fmt, ap);
	va_end(ap);

	return mgcd_job_nop(ctx, data);
}
#endif

struct mgcd_job_info
mgcd_job_get_info_nop(
		struct mgcd_context *ctx, mgcd_job_id job_id,
		struct mgcd_job_nop_data data)
{
	struct mgcd_job_info info = {0};

#if AST_DT_DEBUG_JOBS
	info.description = data.name;
#endif
	info.target = data.id_ref;

	return info;
}

int
mgcd_job_dispatch_nop(struct mgcd_context *ctx,
		mgcd_job_id job_id, struct mgcd_job_nop_data data)
{
	return 0;
}

static struct mgcd_job_info
mgcd_job_get_info(struct mgcd_context *ctx, mgcd_job_id job_id)
{
	struct mgcd_job *job;
	job = get_job(ctx, job_id);

	struct mgcd_job_info res = {0};

	switch (job->kind) {
		case MGCD_JOB_FREE:
			panic("Attempted to fetch info for a freed job.");
			break;

		case MGCD_JOB_LENGTH:
			panic("Invalid job kind");
			break;

#define JOB(name, type) 													\
		case MGCD_JOB_##name:												\
			res = mgcd_job_get_info_##name(ctx, job_id, job->data.name);	\
			break;
	MGCD_JOBS
#undef JOB
	}

	return res;
}

struct mgc_location
mgcd_job_location(struct mgcd_context *ctx, mgcd_job_id job_id)
{
	struct mgcd_job_info info;
	info = mgcd_job_get_info(ctx, job_id);
	return info.loc;
}

#if MGCD_DEBUG_JOBS
static struct string
mgcd_job_kind_name(enum mgcd_job_kind kind) {
	assert(kind < MGCD_JOB_LENGTH);
	return mgcd_job_names[kind];
}
#endif

#if MGCD_DEBUG_JOBS
void
mgcd_print_job_desc(struct mgcd_context *ctx,
		mgcd_job_id job_id)
{
	struct mgcd_job *job;
	job = get_job(ctx, job_id);
	printf(" 0x%03x %s %-*.*s|", job_id,
			job->suspended ? TC(TC_BRIGHT_MAGENTA, "s") : " ",
			mgcd_job_name_max_length(),
			LIT(mgcd_job_kind_name(job->kind)));

	struct mgcd_job_info info;
	info = mgcd_job_get_info(ctx, job_id);

	if (info.description.length > 0 && info.description.text) {
		printf("%.*s", LIT(info.description));
	}
}
#endif

static void
mgcd_remove_job_from_target(struct mgcd_context *ctx, mgcd_job_id job_id)
{
	struct mgcd_job_info info;
	info = mgcd_job_get_info(ctx, job_id);

	if (info.num_targets > 1) {
		for (size_t i = 0; i < info.num_targets; i++) {
			if (info.targets[i]) {
				assert(*info.targets[i] == job_id);
				*info.targets[i] = -1;
			}
		}
	} else if (info.target) {
		assert(*info.target == job_id);
		*info.target = -1;
	}
}

static void
mgcd_job_remove_from_terminal_jobs(struct mgcd_context *ctx,
		mgcd_job_id target_id)
{
	struct mgcd_job *target;
	target = get_job(ctx, target_id);

	for (mgcd_job_id *job_id = &ctx->terminal_jobs;
			(*job_id) >= 0;
			job_id = &get_job(ctx, *job_id)->terminal_jobs) {
		if ((*job_id) == target_id) {
			(*job_id) = target->terminal_jobs;
			target->terminal_jobs = -1;
			return;
		}
	}
}

void
mgcd_job_suspend(struct mgcd_context *ctx, mgcd_job_id target_id)
{
	assert(target_id > -1);

	struct mgcd_job *target;
	target = get_job(ctx, target_id);

	if (target->suspended) {
		return;
	}

#if MGCD_DEBUG_JOBS
		printf("%03zx " TC(TC_BRIGHT_MAGENTA, "sus") " ", ctx->run_i);
		mgcd_print_job_desc(ctx, target_id);
		printf("\n");
#endif

	target->suspended = true;

	if (target->num_incoming_deps == 0) {
		mgcd_job_remove_from_terminal_jobs(ctx, target_id);
	}
}

void
mgcd_job_resume(struct mgcd_context *ctx, mgcd_job_id target_id)
{
	assert(target_id > -1);

	struct mgcd_job *target;
	target = get_job(ctx, target_id);

	if (!target->suspended) {
		return;
	}

#if MGCD_DEBUG_JOBS
		printf("%03zx " TC(TC_BRIGHT_GREEN, "res") " ", ctx->run_i);
		mgcd_print_job_desc(ctx, target_id);
		printf("\n");
#endif

	target->suspended = false;

	if (target->num_incoming_deps == 0) {
		assert(target->terminal_jobs == -1);

		target->terminal_jobs = ctx->terminal_jobs;
		ctx->terminal_jobs = target_id;
	}
}

// Requests that from must be evaluated before to.
void
mgcd_job_dependency(struct mgcd_context *ctx,
		mgcd_job_id from_id, mgcd_job_id to_id)
{
	if (from_id < 0) {
		// The dependecy was already completed.
#if MGCD_DEBUG_JOBS
		printf("%03zx " TC(TC_BRIGHT_BLUE, "dep") " "
				TC(TC_BRIGHT_GREEN, "(completed)"), ctx->run_i);
		// mgcd_print_job_desc(ctx, from_id);
		// Move the cursor to column 80 to align the dependent jobs.
		printf("\033[80G " TC(TC_BRIGHT_BLUE, "->") " ");
		mgcd_print_job_desc(ctx, to_id);
		printf("\n");
#endif
		return;
	}

	assert(from_id != to_id);

	struct mgcd_job *from, *to;
	from = get_job(ctx, from_id);
	to = get_job(ctx, to_id);

	assert(from->kind != MGCD_JOB_FREE);

	for (size_t i = 0; i < from->num_outgoing_deps; i++) {
		if (from->outgoing_deps[i].to == to_id) {
			return;
		}
	}

#if MGCD_DEBUG_JOBS
	printf("%03zx " TC(TC_BRIGHT_BLUE, "dep") " ", ctx->run_i);
	mgcd_print_job_desc(ctx, from_id);
	// Move the cursor to column 80 to align the dependent jobs.
	printf("\033[80G " TC(TC_BRIGHT_BLUE, "->") " ");
	mgcd_print_job_desc(ctx, to_id);
	printf("\n");
#endif

	struct mgcd_job_dep dep = {0};
	dep.visited = false;
	dep.to = to_id;

	dlist_append(
			from->outgoing_deps,
			from->num_outgoing_deps,
			&dep);

	if (to->num_incoming_deps == 0) {
		mgcd_job_remove_from_terminal_jobs(ctx, to_id);
	}

	assert(to->terminal_jobs < 0);

	to->num_incoming_deps += 1;
	ctx->unvisited_job_deps += 1;
}


static inline int
mgcd_dispatch_job(struct mgcd_context *ctx, mgcd_job_id job_id)
{
	struct mgcd_job *job;
	job = get_job(ctx, job_id);

#if MGCD_DEBUG_JOBS
	printf("%03zx "TC(TC_BRIGHT_YELLOW, "==>") " ", ctx->run_i);
	mgcd_print_job_desc(ctx, job_id);
	printf("\n");
#endif

	int err = -1;

	switch (job->kind) {
		case MGCD_JOB_FREE:
			panic("Attempted to dispatch a freed job.");
			break;

		case MGCD_JOB_LENGTH:
			panic("Invalid job kind");
			break;

#define JOB(name, type) 													\
		case MGCD_JOB_##name:												\
			err = mgcd_job_dispatch_##name(ctx, job_id, job->data.name);	\
			break;
	MGCD_JOBS
#undef JOB
	}

	return err;
}

int
mgcd_jobs_dispatch(struct mgcd_context *ctx)
{
	int failed_jobs = 0;

	// Dispatch the jobs in topological order. (Kahn's algorithm.)
	while (ctx->terminal_jobs >= 0) {
		mgcd_job_id job_id;
		struct mgcd_job *job;

		job_id = ctx->terminal_jobs;
		job = get_job(ctx, job_id);
		ctx->terminal_jobs = job->terminal_jobs;
		job->terminal_jobs = -1;

		if (job->failed) {
			mgcd_remove_job_from_target(ctx, job_id);
			mgcd_free_job(ctx, job_id);
			continue;
		}

		int err;
		err = mgcd_dispatch_job(ctx, job_id);
		if (err < 0) {
#if MGCD_DEBUG_JOBS
			printf(TC(TC_BRIGHT_RED, "job failed!") "\n");
#endif

			failed_jobs += 1;

			if (job->num_incoming_deps > 0) {
				// The job prepared itself to yield, but failed instead.
				// Because other jobs now have outgoing dependencies on this
				// node we have to allow it to pass through again and
				// immediatly fail on the next dispatch.

				job->failed = true;
				continue;
			}

			mgcd_remove_job_from_target(ctx, job_id);
			mgcd_free_job(ctx, job_id);
			continue;
		}

		if (job->num_incoming_deps > 0) {
#if MGCD_DEBUG_JOBS
			printf(TC(TC_BRIGHT_YELLOW, "yield") "\n");
#endif
			// If the node gave itself new dependencies we don't mark it as
			// visited to allow it to pass through again.
			continue;
		}

		assert(!err);

		for (size_t i = 0; i < job->num_outgoing_deps; i++) {
			struct mgcd_job_dep *dep;
			dep = &job->outgoing_deps[i];

			if (!dep->visited) {
				dep->visited = true;

				struct mgcd_job *to;
				to = get_job(ctx, dep->to);

				assert(to->kind != MGCD_JOB_FREE);
				assert(to->num_incoming_deps > 0);
				to->num_incoming_deps -= 1;

				assert(ctx->unvisited_job_deps > 0);
				ctx->unvisited_job_deps -= 1;

				if (to->num_incoming_deps == 0 && !to->suspended) {
					to->terminal_jobs = ctx->terminal_jobs;
					ctx->terminal_jobs = dep->to;
				}
			}
		}

		mgcd_remove_job_from_target(ctx, job_id);
		mgcd_free_job(ctx, job_id);
	}

	if (failed_jobs) {
		return -1;
	}

	if (ctx->unvisited_job_deps > 0) {
		mgcd_report_cyclic_dependencies(ctx);

		printf("Failed to evalutate datatype because we found one or more cycles.\n");
#if MGCD_DEBUG_JOBS
		printf("Problematic jobs: \n");
		for (mgcd_job_id job_i = 0; job_i < ctx->jobs.length; job_i++) {
			struct mgcd_job *job;
			job = get_job(ctx, job_i);
			if (job->kind != MGCD_JOB_FREE) {
				printf(" - 0x%03x (%zu):", job_i, job->num_incoming_deps);

				// Find all refs to this job.
				for (mgcd_job_id job_j = 0; job_j < ctx->jobs.length; job_j++) {
					struct mgcd_job *other;
					other = get_job(ctx, job_j);
					if (other->kind == MGCD_JOB_FREE) {
						continue;
					}
					for (size_t i = 0; i < other->num_outgoing_deps; i++) {
						if (other->outgoing_deps[i].to == job_i) {
							printf(" 0x%03x", job_j);
						}
					}
				}

				printf("\n");
			}
		}

		printf("\n");
		mgcd_debug_print_cyclic_dependencies(ctx);
#endif
		return -1;
	}

	return 0;
}
