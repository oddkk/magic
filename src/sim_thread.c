#include "sim_thread.h"
#include "sim.h"

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <processthreadapi.h>
#include <intrin.h>

struct mgc_sim_thread_handle {
	size_t id;
	uintptr_t handle;
	struct mgc_sim_thread *ctx;
};
#else

#include <pthread.h>
#include <signal.h>

struct mgc_sim_thread_handle {
	size_t id;
	pthread_t handle;
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
#else
static void *
mgc_sim_thread_entry(void *data)
{
	struct mgc_sim_thread_handle *handle = data;
	mgc_sim_thread_fn(handle->ctx, handle->id);
	return NULL;
}
#endif

static int
mgc_spawn_thread(struct mgc_sim_thread_handle *handle)
{
#ifdef _WIN32
	handle->handle = _beginthreadex(
		NULL,
		0,
		mgc_sim_thread_entry,
		handle,
		0,
		NULL
	);
	if (handle->handle == -1) {
		return -1;
	}
#else
	return pthread_create(
		&handle->handle,
		NULL,
		mgc_sim_thread_entry,
		handle
	);
#endif

	return 0;
}

static int
mgc_kill_thread(struct mgc_sim_thread_handle *handle)
{
#ifdef _WIN32
	BOOL ok;
	ok = TerminateThread(handle->handle, -1);
	if (!ok) {
		return -1;
	}
	return 0;
#else
	return pthread_kill(handle->handle, SIGKILL);
#endif
}

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
	for (size_t thread_i = 0; thread_i < thread->num_threads; thread_i++) {
		struct mgc_sim_thread_handle *handle = &thread->thread_handles[thread_i];
		handle->ctx = thread;

		int err;
		err = mgc_spawn_thread(handle);
		if (err) {
			for (size_t i = 0; i < thread_i; i++) {
				mgc_kill_thread(handle);
			}
			return -1;
		}
	}

	return -1;
}

int
mgc_sim_thread_stop(struct mgc_sim_thread *thread)
{
	thread->should_quit = true;

	for (size_t i = 0; i < thread->num_threads; i++) {
#ifdef _WIN32
		WaitForSingleObject((HANDLE)thread->thread_handles[i].handle, INFINITE);
#else
		int err;
		err = pthread_join(thread->thread_handles[i].handle, NULL);
		if (err) {
			printf("Join failed: %s\n", strerror(err));
			thread->thread_handles[i].handle = 0;
			continue;
		}
#endif
	}

#ifdef _WIN32
	for (size_t i = 0; i < thread->num_threads; i++) {
		CloseHandle((HANDLE)thread->thread_handles[i].handle);
	}
#endif

	thread->num_threads = 0;

	return 0;
}

