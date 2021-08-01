#include "arena.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef linux
// For sysconf
#include <unistd.h>

// For mmap
#include <sys/mman.h>
#endif

#ifdef MGC_ENABLE_VALGRIND
#include <valgrind/memcheck.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

static struct mgc_memory_page
mgc_memory_alloc_page(struct mgc_memory *mem, size_t hint_size)
{
	size_t num_pages;
	// Round the number of pages up to fit at least the hint size.
	num_pages = (hint_size + mem->sys_page_size - 1) / mem->sys_page_size;
	if (num_pages == 0) {
		num_pages = 1;
	}

	size_t real_size = num_pages * mem->sys_page_size;
	struct mgc_memory_page page = {0};

	page.size = real_size;

#ifdef linux
	page.data = mmap(
			NULL, real_size,
			PROT_READ|PROT_WRITE,
			MAP_PRIVATE|MAP_ANONYMOUS,
			-1, 0);

	if (page.data == MAP_FAILED) {
		perror("mmap");
		page.size = 0;
		page.data = NULL;
		return page;
	}

	memset(page.data, 0, page.size);
#endif

#ifdef _WIN32
	page.data = VirtualAlloc(
		NULL, real_size,
		MEM_COMMIT|MEM_RESERVE,
		PAGE_READWRITE
	);
	if (!page.data) {
		printf("Failed to allocate memory %i.\n",
				GetLastError());
		page.size = 0;
		page.data = NULL;
		return page;
	}

	memset(page.data, 0, page.size);
#endif

#if MGC_ENABLE_VALGRIND
	VALGRIND_MAKE_MEM_NOACCESS(page.data, page.size);
#endif

	return page;
}

static struct mgc_memory_page *
mgc_memory_try_alloc_from_free_list(struct mgc_memory *mem, size_t hint_size)
{
	struct mgc_memory_page **page;
	page = &mem->free_list;

	for (size_t n = 0; n < 100 && *page; n++) {
		if ((*page)->size > hint_size) {
			struct mgc_memory_page *res;
			res = *page;
			*page = res->next;
			res->next = NULL;
			assert(!res->in_use);
			res->in_use = true;

			// VALGRIND_MAKE_MEM_UNDEFINED(res->data, res->size);
			return res;
		}
		page = &(*page)->next;
	}

	return NULL;
}

// Note that this routine only allocates the mgc_memory_page structure. It does
// not allocate the memory pointed to by that structure.
static struct mgc_memory_page *
mgc_memory_request_page_slot(struct mgc_memory *mem)
{
	if (!mem->head_index_page ||
			mem->head_num_pages_alloced >= mem->head_index_page->num_pages) {
		struct mgc_memory_page index_page;
		index_page = mgc_memory_alloc_page(mem, 0);
		index_page.next = NULL;
		index_page.in_use = true;

#if MGC_ENABLE_VALGRIND
		VALGRIND_MAKE_MEM_UNDEFINED(index_page.data, index_page.size);
#endif


		struct mgc_memory_index_page *index;
		index = index_page.data;

		index->next = mem->head_index_page;
		index->num_pages =
			(index_page.size - sizeof(struct mgc_memory_index_page)) /
			sizeof(struct mgc_memory_page);

		index->pages[0] = index_page;

		mem->head_index_page = index;
		mem->head_num_pages_alloced = 1;
	}

	struct mgc_memory_page *slot = NULL;
	slot = &mem->head_index_page->pages[mem->head_num_pages_alloced];
	mem->head_num_pages_alloced += 1;

	return slot;
}

struct mgc_memory_page *
mgc_memory_request_page(struct mgc_memory *mem, size_t hint_size)
{
	struct mgc_memory_page *page;

	page = mgc_memory_try_alloc_from_free_list(mem, hint_size);

	if (!page) {
		page = mgc_memory_request_page_slot(mem);

		*page = mgc_memory_alloc_page(mem, hint_size);
		if (!page->data) {
			return NULL;
		}
		assert(!page->in_use);
		page->in_use = true;
		page->next = NULL;
	}

	return page;
}

void
mgc_memory_release_page(struct mgc_memory *mem, struct mgc_memory_page *page)
{
#if MGC_ENABLE_VALGRIND
	VALGRIND_MAKE_MEM_UNDEFINED(page->data, page->size);
#endif
	memset(page->data, 0, page->size);
	page->next = mem->free_list;
	assert(page->in_use);
	page->in_use = false;
	mem->free_list = page;

#if MGC_ENABLE_VALGRIND
	VALGRIND_MAKE_MEM_NOACCESS(page->data, page->size);
#endif
}

int
mgc_memory_init(struct mgc_memory *mem)
{
	memset(mem, 0, sizeof(struct mgc_memory));

#ifdef linux
	mem->sys_page_size = sysconf(_SC_PAGESIZE);
#elif _WIN32
	mem->sys_page_size = GetLargePageMinimum();
#endif
	mem->free_list = NULL;

	return 0;
}

void
mgc_memory_destroy(struct mgc_memory *mem)
{
	struct mgc_memory_index_page *index;
	index = mem->head_index_page;
	while (index) {
		struct mgc_memory_index_page *next_index;
		// Cache a reference to the next index page because index might be
		// unmapped as a page of itself.
		next_index = index->next;

		// We free the pages in reverse allocation order to make sure we free
		// the index pages after all their pages have been freed.
		size_t index_num_pages = index->num_pages;
		if (index == mem->head_index_page) {
			index_num_pages = mem->head_num_pages_alloced;
		}
		for (isize i = index_num_pages-1; i >= 0; i--) {
			struct mgc_memory_page *page;
			page = &index->pages[i];
			if (page->data) {
#ifdef linux
				int err;
				err = munmap(page->data, page->size);

				if (err) {
					perror("munmap");
				}
#endif

#ifdef _WIN32
				int err;
				err = VirtualFree(page->data, 0, MEM_RELEASE);
				if (err == 0) {
					printf("Failed to deallocate page: %i.\n",
							GetLastError());
				}
#endif
			}
		}

		index = next_index;
	}

	memset(mem, 0, sizeof(struct mgc_memory));
}

void
paged_list_init(struct paged_list *list,
		struct mgc_memory *mem, size_t element_size)
{
	memset(list, 0, sizeof(struct paged_list));
	list->mem = mem;
	list->element_size = element_size;
	list->elements_per_page = mem->sys_page_size / element_size;
	if (list->elements_per_page == 0) {
		list->elements_per_page = 1;
	}
}

void
paged_list_destroy(struct paged_list *list)
{
	for (size_t i = 0; i < list->num_pages; i++) {
		mgc_memory_release_page(list->mem, list->pages[i]);
	}

	free(list->pages);
	memset(list, 0, sizeof(struct paged_list));
}

size_t
paged_list_push(struct paged_list *list)
{
	size_t num_pages = list->num_pages;
	size_t num_alloced = list->length;
	size_t per_page = list->elements_per_page;

	if ((num_alloced + per_page) / per_page > num_pages) {
		size_t new_size;
		struct mgc_memory_page **new_pages;

		new_size = (num_pages + 1) * sizeof(struct mgc_memory_page *);
		new_pages = realloc(list->pages, new_size);
		if (!new_pages) {
			perror("realloc");
			panic("realloc");
			return 0;
		}

		list->pages = new_pages;

		list->pages[num_pages] =
			mgc_memory_request_page(
					list->mem, per_page * list->element_size);

		list->num_pages += 1;
	}

	size_t id;
	id = list->length;
	list->length += 1;

	void *data = paged_list_get(list, id);

#if MGC_ENABLE_VALGRIND
	VALGRIND_MAKE_MEM_UNDEFINED(data, list->element_size);
#endif
	memset(data, 0, list->element_size);

	return id;
}

void *
paged_list_get(struct paged_list *list, size_t id)
{
	assert(id < list->length);
	struct mgc_memory_page *page;
	page = list->pages[id / list->elements_per_page];
	return (uint8_t *)page->data + (id % list->elements_per_page) * list->element_size;
}

int arena_init(struct arena *arena, struct mgc_memory *mem)
{
	memset(arena, 0, sizeof(struct arena));
	arena->mem = mem;

	return 0;
}

void arena_destroy(struct arena *arena)
{
	struct mgc_memory_page *page;
	page = arena->head_page;
	while (page) {
		struct mgc_memory_page *next_page;
		next_page = page->next;

		mgc_memory_release_page(arena->mem, page);

		page = next_page;
	}

	memset(arena, 0, sizeof(struct arena));
}

void *arena_alloc(struct arena *arena, size_t length)
{
	void *result;
	result = arena_alloc_no_zero(arena, length);
	memset(result, 0, length);

	return result;
}

void *arena_allocn(struct arena *arena, size_t nmemb, size_t size)
{
	size_t res;
#ifdef __GNUC__
	// TODO: Make this cross platform and cross compiler compliant.
	if (__builtin_mul_overflow(nmemb, size, &res)) {
		panic("Attempted to allocate memory with a size that exeedes 64-bit integers.");
		return NULL;
	}
#elif _WIN32
	// TODO: Check for overflow
	res = nmemb * size;
#else
#error "TODO: Implement overflow check"
#endif

	return arena_alloc(arena, res);
}

void *arena_alloc_no_zero(struct arena *arena, size_t length)
{
	assert((arena->flags & ARENA_RESERVED) == 0);

	if (!arena->head_page || arena->head_page_head + length > arena->head_page->size) {
		struct mgc_memory_page *new_page;
		new_page = mgc_memory_request_page(arena->mem, length);

		if (arena->head_page) {
			arena->head_page_i += 1;
		}

		new_page->next = arena->head_page;
		arena->head_page = new_page;
		arena->head_page_head = 0;
	}

	void *result;
	result = (uint8_t *)arena->head_page->data + arena->head_page_head;
#if MGC_ENABLE_VALGRIND
	VALGRIND_MAKE_MEM_UNDEFINED(result, length);
#endif
	arena->head_page_head += length;

	return result;
}

arena_mark
arena_checkpoint(struct arena *arena)
{
	struct _arena_mark mark = {0};

	assert((arena->flags & (ARENA_NO_CHECKPOINT|ARENA_RESERVED)) == 0);

	mark.page_i = arena->head_page_i;
	mark.page_head = arena->head_page_head;

	return mark;
}

void
arena_reset(struct arena *arena, arena_mark mark)
{
	assert(arena->head_page_i > mark.page_i ||
			(arena->head_page_i == mark.page_i &&
			 arena->head_page_head >= mark.page_head));

	while (arena->head_page_i > mark.page_i) {
		struct mgc_memory_page *page;
		page = arena->head_page;
		arena->head_page = page->next;
		arena->head_page_i -= 1;

		mgc_memory_release_page(arena->mem, page);
	}

	arena->head_page_head = mark.page_head;

	if (arena->head_page) {
		void *head_ptr = (uint8_t *)arena->head_page->data + arena->head_page_head;
		size_t unused_size = arena->head_page->size - arena->head_page_head;

#if MGC_ENABLE_VALGRIND
		VALGRIND_MAKE_MEM_UNDEFINED(head_ptr, unused_size);
#endif
		memset(head_ptr, 0, unused_size);
#if MGC_ENABLE_VALGRIND
		VALGRIND_MAKE_MEM_NOACCESS(head_ptr, unused_size);
#endif
	}
}

void *arena_reset_and_keep(struct arena *arena, arena_mark mark,
		void *keep, size_t keep_size)
{
	assert(arena->head_page_i > mark.page_i ||
			(arena->head_page_i == mark.page_i &&
			 arena->head_page_head >= mark.page_head));

	struct mgc_memory_page *page;
	page = arena->head_page;
	for (size_t page_i = arena->head_page_i;
			page_i > mark.page_i; page_i--) {
		assert(page);
		page = page->next;
	}

	void *result = NULL;
	struct mgc_memory_page *new_page = NULL;
	size_t new_page_head = 0;

	if (keep_size <= (page->size - mark.page_head)) {
		result = (uint8_t *)page->data + mark.page_head;
		memmove(result, keep, keep_size);

		new_page_head = mark.page_head + keep_size;
	} else {
		new_page = mgc_memory_request_page(arena->mem, keep_size);
		new_page->next = page;

		assert(keep_size <= new_page->size);
		result = new_page->data;
#if MGC_ENABLE_VALGRIND
		VALGRIND_MAKE_MEM_UNDEFINED(new_page->data, keep_size);
#endif
		memcpy(new_page->data, keep, keep_size);
		new_page_head = keep_size;
	}

	while (arena->head_page_i > mark.page_i) {
		struct mgc_memory_page *page;
		page = arena->head_page;
		arena->head_page = page->next;
		arena->head_page_i -= 1;

		mgc_memory_release_page(arena->mem, page);
	}

	assert(arena->head_page == page);

	if (new_page) {
		if (arena->head_page) {
			arena->head_page_i += 1;
		}

		arena->head_page = new_page;
	}

	arena->head_page_head = new_page_head;

	return result;
}

arena_mark
arena_reserve_page(struct arena *arena, void **out_memory, size_t *out_size)
{
	arena_mark cp = arena_checkpoint(arena);
	assert((arena->flags & ARENA_RESERVED) == 0);
	arena->flags |= ARENA_RESERVED;

	if (!arena->head_page) {
		*out_memory = NULL;
		*out_size = 0;

		arena_mark res = {0};
		return res;
	}

	size_t size;
	size = arena->head_page->size - arena->head_page_head;

	void *result;
	result = (uint8_t *)arena->head_page->data + arena->head_page_head;
#if MGC_ENABLE_VALGRIND
	VALGRIND_MAKE_MEM_UNDEFINED(result, size);
#endif

	arena->head_page_head = arena->head_page->size;

	if (out_size) {
		*out_size = size;
	}

	if (out_memory) {
		memset(result, 0, size);
		*out_memory = result;
	}

	return cp;
}

void
arena_take_reserved(struct arena *arena, arena_mark cp, size_t length)
{
	assert((arena->flags & ARENA_RESERVED) != 0);
	arena->flags &= ~ARENA_RESERVED;

	assert(cp.page_head + length <= (arena->head_page ? arena->head_page->size : 0));
	assert(cp.page_i == arena->head_page_i);

	cp.page_head += length;

	arena_reset(arena, cp);
}
