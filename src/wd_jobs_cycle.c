#include "wd_jobs.h"
#include "wd_jobs_internal.h"
#include "utils.h"
#include "term_color.h"

struct mgcd_jobc_vertex {
	mgcd_job_id job_id;

	bool visited, assigned;
	struct mgcd_jobc_vertex *next_l, *next_in_component;

	struct mgcd_jobc_edge *outgoing_edges;
	size_t num_outgoing_edges;

	struct mgcd_jobc_edge *incoming_edges;
	size_t num_incoming_edges;
	size_t num_incoming_edges_filled;
};

struct mgcd_jobc_edge {
	struct mgcd_jobc_vertex *from, *to;
};

struct mgcd_jobc_component {
	struct mgcd_jobc_vertex *head;
};


struct mgcd_jobc_components {
	struct mgcd_jobc_component *components;
	size_t num_components;
};

struct mgcd_jobc_kosaraju {
	struct mgcd_jobc_vertex *head_l, *head_components;
};

static void
mgcd_jobc_kosaraju_visit(struct mgcd_jobc_kosaraju *ctx, struct mgcd_jobc_vertex *vert)
{
	if (vert->visited) {
		return;
	}

	vert->visited = true;

	for (size_t i = 0; i < vert->num_outgoing_edges; i++) {
		mgcd_jobc_kosaraju_visit(ctx, vert->outgoing_edges[i].to);
	}

	vert->next_l = ctx->head_l;
	ctx->head_l = vert;
}

static void
mgcd_jobc_kosaraju_assign(
		struct mgcd_jobc_kosaraju *ctx,
		struct mgcd_jobc_vertex *vert,
		struct mgcd_jobc_component *component)
{
	if (vert->assigned) {
		return;
	}

	vert->assigned = true;
	vert->next_in_component = component->head;
	component->head = vert;

	for (size_t i = 0; i < vert->num_incoming_edges; i++) {
		mgcd_jobc_kosaraju_assign(ctx,
				vert->incoming_edges[i].from, component);
	}
}

struct mgcd_jobc_components
mgcd_jobc_find_components(struct mgcd_context *ctx, struct arena *mem)
{
	// Recreate the job dependency graph.

	size_t num_unfinished_jobs = 0;
	size_t num_unvisited_edges = 0;

	for (mgcd_job_id job_i = 0; job_i < ctx->jobs.length; job_i++) {
		struct mgcd_job *job;
		job = get_job(ctx, job_i);

		if (job->kind == MGCD_JOB_FREE) {
			continue;
		}

		job->aux_id = num_unfinished_jobs;
		num_unfinished_jobs += 1;
		num_unvisited_edges += job->num_incoming_deps;
	}

	size_t num_vertices = num_unfinished_jobs;
	struct mgcd_jobc_vertex *vertices = arena_allocn(mem,
			num_vertices, sizeof(struct mgcd_jobc_vertex));

	struct mgcd_jobc_edge *incoming_edges = arena_allocn(mem,
			num_unvisited_edges, sizeof(struct mgcd_jobc_edge));
	struct mgcd_jobc_edge *outgoing_edges = arena_allocn(mem,
			num_unvisited_edges, sizeof(struct mgcd_jobc_edge));

	size_t unvisited_incoming_edge_i = 0;
	for (mgcd_job_id job_i = 0; job_i < ctx->jobs.length; job_i++) {
		struct mgcd_job *job;
		job = get_job(ctx, job_i);

		if (job->kind == MGCD_JOB_FREE) {
			continue;
		}

		struct mgcd_jobc_vertex *vert;
		vert = &vertices[job->aux_id];

		vert->job_id = job_i;
		vert->num_outgoing_edges = job->num_outgoing_deps;
		vert->num_incoming_edges = job->num_incoming_deps;
		vert->incoming_edges = &incoming_edges[unvisited_incoming_edge_i];
		unvisited_incoming_edge_i += job->num_incoming_deps;
	}

	size_t num_unvisited_outgoing_edges = 0;
	for (size_t vert_i = 0; vert_i < num_unfinished_jobs; vert_i++) {
		struct mgcd_jobc_vertex *vert = &vertices[vert_i];
		struct mgcd_job *job = get_job(ctx, vert->job_id);

		vert->outgoing_edges = &outgoing_edges[num_unvisited_outgoing_edges];

		for (size_t edge_i = 0; edge_i < job->num_outgoing_deps; edge_i++) {
			if (!job->outgoing_deps[edge_i].visited) {
				struct mgcd_job *to_job;
				to_job = get_job(ctx, job->outgoing_deps[edge_i].to);

				struct mgcd_jobc_vertex *to;
				to = &vertices[to_job->aux_id];
				assert(to->num_incoming_edges_filled < to->num_incoming_edges);

				struct mgcd_jobc_edge *forward_edge, *back_edge;
				forward_edge = &outgoing_edges[num_unvisited_outgoing_edges];
				back_edge = &to->incoming_edges[to->num_incoming_edges_filled];

				forward_edge->from = vert;
				forward_edge->to = to;

				*back_edge = *forward_edge;

				num_unvisited_outgoing_edges += 1;
				to->num_incoming_edges_filled += 1;
			}
		}
	}

	assert(num_unvisited_edges == num_unvisited_outgoing_edges);

	// Perform Kosaraju's algorithm to find the strongly connected components
	// of the graph.
	struct mgcd_jobc_kosaraju comp_ctx = {0};
	for (size_t vert_i = 0; vert_i < num_vertices; vert_i++) {
		struct mgcd_jobc_vertex *vert = &vertices[vert_i];
		mgcd_jobc_kosaraju_visit(&comp_ctx, vert);
	}

	struct mgcd_jobc_component *comps;
	comps = arena_allocn(mem, num_vertices, sizeof(struct mgcd_jobc_component));
	size_t comps_i = 0;

	for (struct mgcd_jobc_vertex *vert = comp_ctx.head_l; vert; vert = vert->next_l) {
		struct mgcd_jobc_component comp = {0};
		mgcd_jobc_kosaraju_assign(&comp_ctx, vert, &comp);

		// We only keep components that contains at least two vertices (a head
		// and its next) because single node components can not contain cycles.
		if (comp.head && comp.head->next_in_component) {
			comps[comps_i] = comp;
			comps_i += 1;
		}
	}

	struct mgcd_jobc_components res = {0};
	res.num_components = comps_i;
	res.components = comps;

	return res;
}

void
mgcd_report_cyclic_dependency_chain(struct mgcd_context *ctx,
		struct mgcd_jobc_vertex *component)
{
	struct mgcd_jobc_vertex *it = component;

	mgc_error(ctx->err,
			mgcd_job_location(ctx, it->job_id),
			"Found member dependency cycle. %03x", it->job_id);
	it = it->next_in_component;

	while (it) {
		mgc_appendage(ctx->err,
				mgcd_job_location(ctx, it->job_id),
				"Through. %03x", it->job_id);
		it = it->next_in_component;
	}
}

void
mgcd_report_cyclic_dependencies(struct mgcd_context *ctx)
{
	struct arena *mem = ctx->tmp_mem;
	arena_mark cp = arena_checkpoint(mem);

	struct mgcd_jobc_components comps;
	comps = mgcd_jobc_find_components(ctx, mem);

	for (size_t i = 0; i < comps.num_components; i++) {
		mgcd_report_cyclic_dependency_chain(
				ctx, comps.components[i].head);
	}

	arena_reset(mem, cp);
}

#if MGCD_DEBUG_JOBS
static bool
mgcd_jobc_component_contains(
		struct mgcd_jobc_component *comp,
		struct mgcd_jobc_vertex *vert)
{
	struct mgcd_jobc_vertex *it = comp->head;
	while (it) {
		if (it == vert) {
			return true;
		}

		it = it->next_in_component;
	}

	return false;
}

void
mgcd_debug_print_cyclic_dependencies(struct mgcd_context *ctx)
{
	struct arena *mem = ctx->tmp_mem;
	arena_mark cp = arena_checkpoint(mem);

	struct mgcd_jobc_components comps;
	comps = mgcd_jobc_find_components(ctx, mem);

	for (size_t i = 0; i < comps.num_components; i++) {
		struct mgcd_jobc_component *comp = &comps.components[i];
		struct mgcd_jobc_vertex *it = comp->head;

		printf("\nCycle %zu/%zu:\n", i+1, comps.num_components);
		while (it) {
			printf(TC(TC_BRIGHT_YELLOW, "->") " ");
			mgcd_print_job_desc(ctx, it->job_id);
			printf("\n");

			struct mgcd_job *job;
			job = get_job(ctx, it->job_id);

			bool first_print;

			printf(TC_BRIGHT_YELLOW "       (");
			first_print = true;
			for (size_t edge_i = 0; edge_i < it->num_incoming_edges; edge_i++) {
				struct mgcd_jobc_vertex *in = it->incoming_edges[edge_i].from;
				if (mgcd_jobc_component_contains(comp, in)) {
					printf("%s0x%03x", first_print ? "" : ", ", in->job_id);
					first_print = false;
				}
			}

			printf(") -> 0x%03x -> (", it->job_id);
			first_print = true;
			for (size_t edge_i = 0; edge_i < it->num_outgoing_edges; edge_i++) {
				struct mgcd_jobc_vertex *out = it->outgoing_edges[edge_i].to;
				if (mgcd_jobc_component_contains(comp, out)) {
					printf("%s0x%03x", first_print ? "" : ", ", out->job_id);
					first_print = false;
				}
			}
			printf(")" TC_CLEAR "\n");

			it = it->next_in_component;
		}
	}

	fflush(stdout);
	arena_reset(mem, cp);
}
#endif
