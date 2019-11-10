#include <stdlib.h>
#include "memlib.h"

#define PAGES_IN_SUPERBLOCK 2

#define NUM_BLOCK_SIZES 10
const int BLOCK_SIZES[NUM_BLOCK_SIZES] = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 6144 };
#define MAX_BLOCK_SIZE (BLOCK_SIZES[NUM_BLOCK_SIZES - 1])

typedef ptrdiff_t vaddr_t;

struct superblock
{
	processor_heap* owner;
	unsigned int size_in_bytes;
	superblock* next;
};

struct free_block
{
	superblock* owner;
	free_block* next;
};

struct free_pages
{
	unsigned int num_pages;
	free_pages* prev;
	free_pages* next;
};

struct processor_heap
{
	pthread_mutex_t lock;

	superblock* subpage_allocations;
	large_allocation* large_allocations;
	
	free_pages* free_page_list;
	free_block* free_block_list[NUM_BLOCK_SIZES];
};

struct subpage_allocation
{
	unsigned int size_in_bytes;
	unsigned int padding;
	superblock* owner;
};

struct large_allocation
{
	unsigned int size_in_pages;
	large_allocation* next;
};

void* page_zero; // page dedicated for heap data

unsigned int num_processors;
unsigned int superblock_size;

pthread_mutex_t global_heap_lock = PTHREAD_MUTEX_INITIALIZER;

processor_heap *processor_heaps;

void initialize()
{
	num_processors = getNumProcessors();
	superblock_size = PAGES_IN_SUPERBLOCK * mem_pagesize();

	page_zero = mem_sbrk(mem_pagesize());
	processor_heaps = (processor_heap*) page_zero;

	for(unsigned int i = 0; i < num_processors; i++)
	{
		memset(&processor_heaps[i], 0, sizeof(processor_heap));
		processor_heaps[i].lock = PTHREAD_MUTEX_INITIALIZER;
	}
}

unsigned int calculate_size_class(size_t sz)
{
	unsigned int size_class = 0;
	for(unsigned int i = 0; i < NUM_BLOCK_SIZES; i++)
	{
		if(sz <= BLOCK_SIZES[i])
		{
			size_class = i;
			break;
		}
	}
	return size_class;
}

void* alloc_pages(processor_heap* heap, unsigned int num_pages)
{
	void* page = NULL;

	// try to find a page available for reuse
	for(free_pages* pages = heap->free_page_list; pages != NULL; pages = pages->next)
	{
		if(pages->num_pages > num_pages)
		{
			page = pages;
			pages->num_pages -= num_pages;

			unsigned int offset = mem_pagesize() * num_pages;
			if(pages->next != NULL) {
				pages->next->prev += offset;
			}
			memcpy(pages, (unsigned char*) pages + offset, sizeof(free_pages));
		}
		else if(pages->num_pages == num_pages)
		{
			page = pages;

			if(pages == heap->free_page_list)
			{
				free_pages* next = heap->free_page_list->next;
				if(next != NULL) {
					next->prev = NULL;
				}
				heap->free_page_list = next;
			}
			else
			{
				if(pages->prev != NULL) { pages->prev = pages->next; }
				if(pages->next != NULL) { pages->next = pages->prev; }
			}
		}
	}

	// no page could be recycled - allocate a new one
	if(page == NULL)
	{
		pthread_mutex_lock(&global_heap_lock);

		superblock* block = (superblock*) mem_sbrk(num_pages * mem_pagesize());
		if(block != NULL)
		{
			block->size = sizeof(superblock);
			block->next = NULL;

			page = block;
		}

		pthread_mutex_unlock(&global_heap_lock);
	}

	return page;
}

void* alloc_small_block(size_t sz)
{
	void* mem = NULL;
	processor_heap* heap = processor_heaps[getTID()];
	pthread_mutex_lock(&heap->lock);

	unsigned int page_size = mem_pagesize();

	unsigned int size_class = calculate_size_class(sz);
	unsigned int size = BLOCK_SIZES[size_class];

	// check if there are any blocks of the size class which are available for reuse
	if(heap->free_blocks[size_class] != NULL)
	{
		mem = heap->free_blocks[size_class];
		heap->free_blocks = heap->free_blocks->next;
	}

	if(mem == NULL)
	{
		// try to find a superblock to place the allocation in
		superblock *block = NULL, *tail = NULL;
		for(block = heap->allocations; block != NULL; block = block->next)
		{
			if(size + block->size <= block->num_pages * page_size)
			{
				break;
			}
			tail = block;
		}

		// allocate a new superblock and insert it into this heap
		if(block == NULL)
		{
			block = alloc_pages(heap, PAGES_IN_SUPERBLOCK);
			tail->next = block;
		}

		mem = (unsigned char*) block + block->size;
		block->size += size;
	}

	if(mem != NULL)
	{
		allocation_header* header = (allocation_header*) mem;
		header->size = size;
	}

	pthread_mutex_unlock(&heap->lock);
	return mem;
}

unsigned long long align(unsigned long long value, unsigned long long alignment)
{
	unsigned long long mask = alignment - 1;
	return (value + mask) & ~mask;
}

void* alloc_large_block(size_t sz)
{
	void* mem = NULL;
	processor_heap* heap = processor_heaps[getTID()];
	pthread_mutex_lock(&heap->lock);

	unsigned long long size_aligned_to_qwords = align(sz, 8);
	unsigned long long num_pages = align(size_aligned_to_qwords, page_size) / page_size;
	superblock* block = alloc_pages(heap, num_pages);
	mem = block;

	if(heap->allocations == NULL)
	{
		heap->allocations = block;
	}
	else
	{
		superblock* it = heap->allocations;
		while(1)
		{
			if(it->next == NULL)
			{
				it->next = block;
				break;
			}

			it = it->next;
		}
	}
	
	pthread_mutex_unlock(&heap->lock);
	return mem;
}

int free_small_block(subpage_allocation* ptr)
{
	processor_heap* heap = processor_heaps[getTID()];
	pthread_mutex_lock(&heap->lock);

	superblock* owner = ptr->owner;
	unsigned int size_class = calculate_size_class(ptr->size_in_bytes);

	free_block* block = (free_block* ptr);
	block->owner = owner;
	block->next = heap->free_blocks[size_class];
	heap->free_blocks[size_class] = block;
	
	pthread_mutex_unlock(&heap->lock);
	return 0;
}

int free_large_block(large_allocation* ptr)
{
	processor_heap* heap = processor_heaps[getTID()];
	pthread_mutex_lock(&heap->lock);

	superblock* owner = ptr->owner;
	unsigned int num_pages = ptr->size_in_pages;

	free_pages* pages = (free_pages*) ptr;
	pages->num_pages = num_pages;
	pages->next = heap->free_pages;
	heap->free_blocks = pages = next;

	pthread_mutex_unlock(&heap->lock);
	return 0;
}

void *mm_malloc(size_t sz)
{
	void* mem = NULL;
	size_t size = sz + sizeof(allocation_header);

	if(size > MAX_BLOCK_SIZE)
	{
		mem = alloc_small_block(size);
	}
	else
	{
		mem = alloc_large_block(size);
	}

	return (unsigned char*) mem + sizeof(allocation_header);
}

void mm_free(void *ptr)
{
	allocation_header* mem = (unsigned char*) ptr - sizeof(allocation_header);

	if(size > MAX_BLOCK_SIZE)
	{
		free_small_block(mem);
	}
	else
	{
		free_large_block(mem);
	}
}

int mm_init(void)
{
	if(dseg_lo == NULL && dseg_hi == NULL)
	{
		pthread_mutex_lock(&global_heap_lock);

		int result = mem_init();
		if(result == 0)
		{
			initialize();
		}
		
		pthread_mutex_unlock(&global_heap_lock);
		return result;
	}

	return 0;
}

