#include "sim_thread.h"
#include "sim.h"

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <intrin.h>

struct mgc_sim_thread_handle {
	size_t id;
	uintptr_t handle;
	struct mgc_sim_thread *ctx;
};
#endif

static void
mgc_sim_thread_fn(struct mgc_sim_thread *thread, size_t id)
{
	size_t tick = 0;
	while (!thread->should_quit) {
		if (thread->chunk_cache->update_state == MGC_CHUNK_CACHE_UPDATE_SIM) {

			// mgc_sim_tick(sim_buffer, &chunk_cache, &reg, sim_tick);
			mgc_sim_tick(
				thread->sim_buffer,
				thread->chunk_cache,
				thread->registry,
				tick
			);
			tick += 1;

			thread->chunk_cache->update_state = MGC_CHUNK_CACHE_UPDATE_RENDER;
		}
	}
}

#ifdef _WIN32
static int __cdecl
mgc_sim_thread_entry(void *data)
{
	struct mgc_sim_thread_handle *handle = data;

	mgc_sim_thread_fn(handle->ctx, handle->id);

	return -1;
}
#endif


int
mgc_sim_thread_start(
		struct mgc_sim_thread *thread,
		struct arena *arena,
		struct mgc_chunk_cache *chunk_cache,
		struct mgc_registry *registry)
{
	thread->num_threads = 1;

	thread->chunk_cache = chunk_cache;
	thread->registry = registry;
	thread->sim_buffer = arena_alloc(arena, sizeof(struct mgc_sim_buffer));

	thread->thread_handles = arena_allocn(arena, sizeof(struct mgc_sim_thread_handle), thread->num_threads);
#ifdef _WIN32
	for (size_t i = 0; i < thread->num_threads; i++) {
		struct mgc_sim_thread_handle *handle = &thread->thread_handles[i];
		handle->ctx = thread;

		thread->thread_handles[i].handle = _beginthreadex(
			NULL,
			0,
			mgc_sim_thread_entry,
			handle,
			0,
			NULL
		);
	}
#endif

	return -1;
}

int
mgc_sim_thread_stop(struct mgc_sim_thread *thread)
{
	thread->should_quit = true;

	for (size_t i = 0; i < thread->num_threads; i++) {
		WaitForSingleObject((HANDLE)thread->thread_handles[i].handle, INFINITE);
	}

	for (size_t i = 0; i < thread->num_threads; i++) {
		CloseHandle((HANDLE)thread->thread_handles[i].handle);
	}

	thread->num_threads = 0;

	return 0;
}

