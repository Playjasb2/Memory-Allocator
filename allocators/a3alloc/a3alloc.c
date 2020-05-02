
#define _GNU_SOURCE

#include <assert.h>
#include <stdlib.h>

#include <sched.h>

#include "memlib.h"
#include "mm_thread.h"
#include "timer.h"

// #define debug_print(frmt, ...) { printf(frmt, ##__VA_ARGS__); }
#define debug_print(frmt, ...)

#define PAGES_IN_SUPERBLOCK 2

#define NUM_BLOCK_SIZES 8
const int BLOCK_SIZES[NUM_BLOCK_SIZES] = { 32, 64, 128, 256, 512, 1024, 2048, 4096 };
#define MAX_BLOCK_SIZE (BLOCK_SIZES[NUM_BLOCK_SIZES - 1])

typedef ptrdiff_t vaddr_t;

// forward declare the main structures since they reference each other
typedef struct superblock_t superblock;
typedef struct free_block_t free_block;
typedef struct free_pages_t free_pages;
typedef struct processor_heap_t processor_heap;
typedef struct subpage_allocation_t subpage_allocation;
typedef struct large_allocation_t large_allocation;

struct superblock_t // size will be rounded to nearest block size during initialization
{
	processor_heap* owner;
	unsigned int size_in_bytes;
	free_block* free_block_list[NUM_BLOCK_SIZES];
	superblock* next;
};

struct free_block_t // size = 16 bytes
{
	free_block* prev;
	free_block* next;
};

struct free_pages_t // size = 24 bytes
{
	unsigned long long num_pages;
	free_pages* prev;
	free_pages* next;
};

struct processor_heap_t
{
	pthread_mutex_t lock;

	superblock* subpage_allocations;
	// large_allocation* large_allocations;
	
	free_pages* free_page_list;
};

// structure at the beginning of every subpage allocation (size = 16 bytes)
struct subpage_allocation_t
{
	superblock* owner;
	unsigned long long size_in_bytes;
};

// structure at the beginning of every large allocation (size = 16 bytes)
struct large_allocation_t
{
	processor_heap* owner;
	unsigned long long size_in_bytes;
};

void* page_zero; // page dedicated for heap data

unsigned int num_processors;
unsigned int superblock_size;

pthread_mutex_t global_heap_lock = PTHREAD_MUTEX_INITIALIZER;

processor_heap *processor_heaps;

void initialize()
{
	num_processors = getNumProcessors();
	unsigned int page_size = mem_pagesize();

	superblock_size = PAGES_IN_SUPERBLOCK * page_size;

	page_zero = mem_sbrk(page_size);
	processor_heaps = (processor_heap*) page_zero;

	for(unsigned int i = 0; i < num_processors; i++)
	{
		memset(&processor_heaps[i], 0, sizeof(processor_heap));
		pthread_mutex_init(&processor_heaps[i].lock, NULL);
	}
}

processor_heap* get_processor_heap()
{
	return &processor_heaps[sched_getcpu() % num_processors];
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

void insert_free_entry(unsigned int size_class, free_block* block, superblock* super_block)
{
	if(super_block->free_block_list[size_class] == NULL)
	{
		super_block->free_block_list[size_class] = block;
	}
	else
	{
		free_block* it = super_block->free_block_list[size_class];
		while(1)
		{
			if((block < it) || (it->next == NULL))
			{
				if(block < it)
				{
					// link the new block to the previous node
					if(it->prev != NULL) { it->prev->next = block; }
					block->prev = it->prev;

					// link the new block to the current node
					it->prev = block;
					block->next = it;

					// if the iterator was the first entry, then we need to overwrite it with the new block
					if(it == super_block->free_block_list[size_class])
					{
						super_block->free_block_list[size_class] = block;
					}
				}
				else
				{
					it->next = block;
					block->prev = it;
				}

				// check if the block can be merged with an adjacent block (only blocks < 4096 bytes can be merged)
				if(size_class < NUM_BLOCK_SIZES - 1)
				{
					unsigned long long prev_diff = (block->prev == NULL) ? 0 : (unsigned char*) block - (unsigned char*) block->prev;
					unsigned long long next_diff = (block->next == NULL) ? 0 : (unsigned char*) block->next - (unsigned char*) block;

					if(prev_diff == BLOCK_SIZES[size_class])
					{
						// remove the blocks from the link
						if(block->prev->prev != NULL) { block->prev->prev->next = block->next; }
						if(block->next != NULL) { block->next->prev = block->prev->prev; }

						// if the root of the linked list points to the previous block, point it to the next block
						if(block->prev == super_block->free_block_list[size_class])
						{
							free_block* next = block->next;
							if(next != NULL) next->prev = NULL;
							super_block->free_block_list[size_class] = next;
						}
						
						block->prev->prev = NULL;
						block->prev->next = NULL;

						debug_print("Combining block of size %lli with block of size %i\n", prev_diff, BLOCK_SIZES[size_class]);
						insert_free_entry(size_class + 1, block->prev, super_block);
					}
					else if(next_diff == BLOCK_SIZES[size_class])
					{
						// remove the blocks from the link
						if(block->prev != NULL) { block->prev->next = block->next->next; }
						if(block->next->next != NULL) { block->next->next->prev = block->prev; }

						// if the root of the linked list points to the current block, point it to the next block
						if(block == super_block->free_block_list[size_class])
						{
							free_block* next = block->next->next;
							if(next != NULL) next->prev = NULL;
							super_block->free_block_list[size_class] = next;
						}

						block->prev = NULL;
						block->next = NULL;

						debug_print("Combining block of size %lli with block of size %i\n", next_diff, BLOCK_SIZES[size_class]);
						insert_free_entry(size_class + 1, block, super_block);
					}
				}

				break;
			}

			it = it->next;
		}
	}
}

// removes and returns the first entry from the size_class linked list in a given super_block
free_block* get_free_entry(superblock* super_block, unsigned int size_class)
{
	free_block* block = super_block->free_block_list[size_class];
	free_block* next = block->next;

	if(next != NULL) { next->prev = NULL; }
	super_block->free_block_list[size_class] = next;

	block->next = NULL;
	block->prev = NULL;
	return block;
}

unsigned long long align(unsigned long long value, unsigned long long alignment)
{
	unsigned long long mask = alignment - 1;
	return (value + mask) & ~mask;
}

void* alloc_pages(processor_heap* heap, unsigned int num_pages)
{
	void* page = NULL;

	// try to find a page available for reuse
	for(free_pages* pages = heap->free_page_list; pages != NULL; pages = pages->next)
	{
		if(pages->num_pages > num_pages) // take some of the pages in a free page list
		{
			// printf("take some pages!\n");
			page = pages;
			pages->num_pages -= num_pages;

			unsigned int offset = mem_pagesize() * num_pages;
			if((pages->next != NULL) && (pages->next->prev != NULL)) {
				pages->next->prev += offset; // adjust the next free page list pointer to point to the correct pages
			}
			memcpy(pages, (unsigned char*) pages + offset, sizeof(free_pages));
		}
		else if(pages->num_pages == num_pages) // take all of the pages in a free page list
		{
			// printf("take all pages!\n");
			page = pages;

			if(pages == heap->free_page_list) // remove the free page list from the head of the list
			{
				free_pages* next = heap->free_page_list->next;
				if(next != NULL) {
					next->prev = NULL;
				}
				heap->free_page_list = next;
			}
			else // remove the free page list from inside the list
			{
				if(pages->prev != NULL) { pages->prev->next = pages->next; }
				if(pages->next != NULL) { pages->next->prev = pages->prev; }
			}
		}

		if(page != NULL)
		{
			break;
		}
	}

	// no page could be recycled - allocate a new one
	if(page == NULL)
	{
		pthread_mutex_lock(&global_heap_lock);
		page = mem_sbrk(num_pages * mem_pagesize());
		pthread_mutex_unlock(&global_heap_lock);
	}

	return page;
}

void* alloc_small_block(size_t sz)
{
	void* mem = NULL;
	superblock* owner = NULL;
	
	processor_heap* heap = get_processor_heap();
	pthread_mutex_lock(&heap->lock);

	unsigned int size_class = calculate_size_class(sz);
	unsigned int size = BLOCK_SIZES[size_class];

	// check if there are any blocks of the size class which are available for reuse
	for(superblock* super_block = heap->subpage_allocations; super_block != NULL; super_block = super_block->next)
	{
		// check all the block sizes >= the required block size
		for(unsigned int i = size_class; i < NUM_BLOCK_SIZES; i++)
		{
			if(super_block->free_block_list[i] != NULL)
			{
				free_block* block = get_free_entry(super_block, i); // take the block

				if(i != size_class) // decompose the block if it is larger than the required size
				{
					debug_print("Reducing block at %p of size [%i] to [%i]\n", block, BLOCK_SIZES[i], BLOCK_SIZES[size_class]);
					// decompose the large block into smaller blocks
					for(unsigned int j = i; j > size_class; j--)
					{
						free_block* second = (free_block*) ((unsigned char*) block + (BLOCK_SIZES[j] / 2));
						second->next = NULL;
						second->prev = NULL;
						insert_free_entry(size_class, second, super_block);

						debug_print("\tCreate new entry at offset %li\n", (unsigned char*) second - (unsigned char*) block);
					}
				}

				mem = block;
				owner = super_block;
				goto DONE;
			}
		}
	}
	
	DONE:
	if(mem == NULL)
	{
		// try to find a superblock with space to place the allocation in
		superblock *block = NULL, *tail = NULL;
		for(block = heap->subpage_allocations; block != NULL; block = block->next)
		{
			if(size + block->size_in_bytes <= superblock_size)
			{
				break;
			}
			tail = block;
		}

		// allocate a new superblock and insert it into this heap
		if(block == NULL)
		{
			block = alloc_pages(heap, PAGES_IN_SUPERBLOCK);

			if(block)
			{
				memset(block, 0, sizeof(superblock));

				block->owner = heap;
				block->size_in_bytes = BLOCK_SIZES[calculate_size_class(sizeof(superblock))]; // offset the size of the header
			}

			if(tail == NULL) {
				heap->subpage_allocations = block;
			} else {
				tail->next = block;
			}
		}

		owner = block;
		mem = (unsigned char*) block + block->size_in_bytes;
		block->size_in_bytes += size;
	}

	// initialize the allocation header
	if(mem != NULL)
	{
		subpage_allocation* header = (subpage_allocation*) mem;
		header->owner = owner;
		header->size_in_bytes = size;
	}

	pthread_mutex_unlock(&heap->lock);
	return mem;
}

void* alloc_large_block(size_t sz)
{
	void* mem = NULL;
	processor_heap* heap = get_processor_heap();

	pthread_mutex_lock(&heap->lock);

	unsigned long long page_size = mem_pagesize();
	unsigned long long size_aligned_to_qwords = align(sz, 8);
	unsigned long long size_aligned_to_pages = align(size_aligned_to_qwords, page_size);
	unsigned long long num_pages = size_aligned_to_pages / page_size;
	
	large_allocation* allocation = alloc_pages(heap, num_pages);
	allocation->size_in_bytes = size_aligned_to_pages;
	allocation->owner = heap;

	mem = allocation;
	
	pthread_mutex_unlock(&heap->lock);
	return mem;
}

int free_small_block(subpage_allocation* ptr)
{
	processor_heap* heap = ptr->owner->owner;

	pthread_mutex_lock(&heap->lock);

	superblock* owner = ptr->owner;
	unsigned int size_class = calculate_size_class(ptr->size_in_bytes);

	free_block* block = (free_block*) ptr;
	block->next = NULL;
	block->prev = NULL;

	insert_free_entry(size_class, block, owner);
	
	pthread_mutex_unlock(&heap->lock);
	return 0;
}

int free_large_block(large_allocation* ptr)
{
	processor_heap* heap = ptr->owner;

	pthread_mutex_lock(&heap->lock);

	unsigned int num_pages = ptr->size_in_bytes / mem_pagesize();

	free_pages* pages = (free_pages*) ptr;
	pages->num_pages = num_pages;
	pages->next = heap->free_page_list;
	heap->free_page_list = pages;

	pthread_mutex_unlock(&heap->lock);
	return 0;
}

void *mm_malloc(size_t sz)
{
	void* mem = NULL;
	size_t size = sz + sizeof(subpage_allocation);

	if(size <= MAX_BLOCK_SIZE)
	{
		mem = alloc_small_block(size);
	}
	else
	{
		mem = alloc_large_block(size);
	}

	return (unsigned char*) mem + sizeof(subpage_allocation);
}

void mm_free(void *ptr)
{
	unsigned long long size = *((unsigned long long*) ptr - 1);

	if(size <= MAX_BLOCK_SIZE)
	{
		free_small_block((subpage_allocation*) ((unsigned char*) ptr - sizeof(subpage_allocation)));
	}
	else
	{
		free_large_block((large_allocation*) ((unsigned char*) ptr - sizeof(large_allocation)));
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
