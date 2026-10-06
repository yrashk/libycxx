/* A small first-fit free-list heap over one memory region: the 'memory' hosted layer of both
 * examples (examples/hosted-layers). Freestanding C: no C library needed. Not thread-safe (the
 * examples have one thread of execution: the 'threads' layer is absent).
 *
 * The free blocks form a list in address order; freeing a block merges it with its free
 * neighbours. An allocated block keeps, just before the address it returns, its size and its
 * distance from the block's start, so any power-of-two alignment works and deallocation needs no
 * size. */
#ifndef HOSTED_LAYERS_HEAP_H
#define HOSTED_LAYERS_HEAP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Makes [base, base + size) the heap. */
void heap_init(void* base, size_t size);
/* size bytes aligned to align (a power of two), or null when no free block fits. */
void* heap_allocate(size_t size, size_t align);
/* Returns a block from heap_allocate; null is ignored. */
void heap_free(void* p);

struct heap_stats {
  size_t size;       /* the region's size */
  size_t in_use;     /* bytes in allocated blocks, headers included */
  size_t peak;       /* the most in_use has been */
  size_t blocks;     /* allocated blocks */
  size_t allocations; /* heap_allocate calls that succeeded */
};
struct heap_stats heap_get_stats(void);

#ifdef __cplusplus
}
#endif

#endif
