#ifndef MAGIC_SIM_THREAD_H
#define MAGIC_SIM_THREAD_H

struct mgc_chunk_cache;
struct mgc_sim_buffer;
struct mgc_registry;
struct mgc_sim_thread_handle;

struct mgc_sim_thread {
	struct mgc_chunk_cache *chunk_cache;
	struct mgc_registry *registry;
	struct mgc_sim_buffer *sim_buffer;

	size_t num_threads;
	struct mgc_sim_thread_handle *thread_handles;

	volatile int should_quit;
};

int
mgc_sim_thread_start(
		struct mgc_sim_thread *thread,
		struct arena *arena,
		struct mgc_chunk_cache *chunk_cache,
		struct mgc_registry *registry);

int
mgc_sim_thread_stop(struct mgc_sim_thread *thread);

#endif
