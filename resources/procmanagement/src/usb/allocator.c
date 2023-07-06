#include "../thread.h"
#include "../util.h"
#include "allocator.h"
#include "debug.h"

/* Memory allocator */
#define CHUNK_SIZE 64
struct mem_chunk {
	uint8_t is_free;
	uint32_t total_size;
	uint32_t size;
};

#define CHUNK_NUM ((KERNEL_ALLOC_STOP - KERNEL_ALLOC_START) / CHUNK_SIZE)

static struct mem_chunk mem_chunk_list[CHUNK_NUM];

static spinlock_t memlock;

void report_usage(int *free_mem, int *alloc_mem)
{
	int i;

	for (i = 0; i < CHUNK_NUM; i += mem_chunk_list[i].total_size) {
		if (mem_chunk_list[i].is_free == 1)
			free_mem += mem_chunk_list[i].total_size;
		else
			alloc_mem += mem_chunk_list[i].total_size;
	}
}

static void update_allocated(uint32_t i, uint32_t total_size)
{
	while (i > 0) {
		if (mem_chunk_list[i - 1].is_free == 0) {
			/* Move to the start of the previous chunk */
			i -= mem_chunk_list[i - 1].size;
			/* Update total size */
			total_size += mem_chunk_list[i].size;
			mem_chunk_list[i].total_size = total_size;
		} else
			break;
	}
}

static void update_free(uint32_t i, uint32_t total_size)
{
	while (i > 0) {
		if (mem_chunk_list[i - 1].is_free == 1) {
			i--;
			total_size++;
			mem_chunk_list[i].total_size = total_size;
		} else
			break;
	}
}

void *kzalloc_align(int size, int alignment)
{
	uint32_t req_chunks;
	uint32_t chunk_align;
	uint32_t req_to_align = 0;
	uint32_t chunk;
	//  int free_mem = 0;
	//  int alloc_mem = 0;
	char *ptr;

	//  DEBUG("Allocating %dB with alignment %d", size, alignment);

	/* The number of required chunks */
	req_chunks = ((size - 1) / CHUNK_SIZE) + 1;

	chunk_align = alignment / CHUNK_SIZE;

	spinlock_acquire(&memlock);
	/* Find continuous free space in memory */
	for (chunk = 0; chunk < CHUNK_NUM;
	     chunk += mem_chunk_list[chunk].total_size) {
		/*    DEBUG("chunk %4d, is free %d, size %4d, total size %4d",
			chunk,
			mem_chunk_list[chunk].is_free,
			mem_chunk_list[chunk].size,
			mem_chunk_list[chunk].total_size);
		*/
		if (mem_chunk_list[chunk].is_free == 0)
			continue;

		if (chunk_align)
			req_to_align = (chunk_align - (chunk % chunk_align))
				       % chunk_align;
		else
			req_to_align = 0;

		if (mem_chunk_list[chunk].total_size
		    >= req_chunks + req_to_align)
			break;
	}

	if (chunk == CHUNK_NUM) {
		//    DEBUG("OUT OF FREE MEMORY");
		//    ms_delay(10000);
		/* Terminating chunk, return NULL */
		spinlock_release(&memlock);
		return NULL;
	}

	chunk += req_to_align;

	/*
	 * Allocate this chunk by marking
	 * the first and the last block
	 */
	mem_chunk_list[chunk].is_free = 0;
	mem_chunk_list[chunk].size = req_chunks;
	mem_chunk_list[chunk + req_chunks - 1].is_free = 0;
	mem_chunk_list[chunk + req_chunks - 1].size = req_chunks;


	/*
	 * If the next adjacent chunk is also allocated,
	 * compute the total size of allocated space
	 */
	if (mem_chunk_list[chunk + req_chunks].is_free == 0)
		mem_chunk_list[chunk].total_size =
			req_chunks
			+ mem_chunk_list[chunk + req_chunks].total_size;
	else
		mem_chunk_list[chunk].total_size = req_chunks;

	/*
	 * Update previous chunk total size
	 */
	if (chunk > 0) {
		if (mem_chunk_list[chunk - 1].is_free == 0)
			update_allocated(chunk,
					 mem_chunk_list[chunk].total_size);
		else
			update_free(chunk, 0);
	}

	spinlock_release(&memlock);

	ptr = (char *)(KERNEL_ALLOC_START + chunk * CHUNK_SIZE);

	bzero(ptr, req_chunks * CHUNK_SIZE);

	return (void *)ptr;
}

void *kzalloc(int size)
{
	return kzalloc_align(size, 0);
}

void kfree(void *ptr)
{
	uint32_t chunk;
	uint32_t size;
	uint32_t total_size;

	spinlock_acquire(&memlock);

	chunk = ((uint32_t)((char *)ptr - KERNEL_ALLOC_START)) / CHUNK_SIZE;
	ASSERT(chunk < CHUNK_NUM);

	size = mem_chunk_list[chunk].size;

	/* Clear chunk edges */
	mem_chunk_list[chunk].is_free = 1;
	mem_chunk_list[chunk + size - 1].is_free = 1;

	/* Update total size fields in this chunks */
	total_size = 0;
	if (chunk + size < CHUNK_NUM)
		if (mem_chunk_list[chunk + size].is_free == 1)
			total_size = mem_chunk_list[chunk + size].total_size;

	update_free(chunk + size, total_size);

	/* Update total size if previous chunk is allocated */
	if (chunk > 0)
		if (mem_chunk_list[chunk - 1].is_free == 0)
			update_allocated(chunk, 0);
	spinlock_release(&memlock);

	return;
}

void allocator_init()
{
	int i;

	for (i = 0; i < CHUNK_NUM; i++) {
		mem_chunk_list[i].is_free = 1;
		mem_chunk_list[i].total_size = CHUNK_NUM - i;
		mem_chunk_list[i].size = CHUNK_NUM - i;
	}
	spinlock_init(&memlock);
}
