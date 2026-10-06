/* A first-fit free-list heap: see heap.h. */
#include "heap.h"

#include <stdint.h>

enum { granule = 16 };

/* A free block: its size (bytes, a multiple of granule) and the next free block, by address. */
struct free_block {
  size_t size;
  struct free_block* next;
};

/* Just before an allocated payload. */
struct header {
  size_t size;   /* the whole block, from its start */
  size_t offset; /* payload address - block start */
};

static struct free_block* free_list;
static struct heap_stats stats;

static uintptr_t align_up(uintptr_t v, size_t a) { return (v + (a - 1)) & ~(uintptr_t)(a - 1); }

void heap_init(void* base, size_t size) {
  uintptr_t b = align_up((uintptr_t)base, granule);
  uintptr_t e = ((uintptr_t)base + size) & ~(uintptr_t)(granule - 1);
  free_list = 0;
  stats = (struct heap_stats){0};
  if (e <= b || e - b < 2 * granule)
    return;
  free_list = (struct free_block*)b;
  free_list->size = e - b;
  free_list->next = 0;
  stats.size = e - b;
}

void* heap_allocate(size_t size, size_t align) {
  if (align < granule)
    align = granule;
  if (size == 0)
    size = 1;
  for (struct free_block **link = &free_list, *f; (f = *link) != 0; link = &f->next) {
    const uintptr_t start = (uintptr_t)f;
    const uintptr_t payload = align_up(start + sizeof(struct header), align);
    if (payload < start || size > (uintptr_t)-1 - payload) /* overflow */
      continue;
    const uintptr_t end = align_up(payload + size, granule);
    if (end > start + f->size)
      continue;
    size_t taken = end - start;
    const size_t rest = f->size - taken;
    if (rest >= 2 * granule) { /* the tail stays free */
      struct free_block* tail = (struct free_block*)end;
      tail->size = rest;
      tail->next = f->next;
      *link = tail;
    } else { /* too small to keep: part of the allocation */
      taken = f->size;
      *link = f->next;
    }
    struct header* h = (struct header*)payload - 1;
    h->size = taken;
    h->offset = payload - start;
    stats.in_use += taken;
    if (stats.in_use > stats.peak)
      stats.peak = stats.in_use;
    ++stats.blocks;
    ++stats.allocations;
    return (void*)payload;
  }
  return 0;
}

void heap_free(void* p) {
  if (p == 0)
    return;
  const struct header* h = (const struct header*)p - 1;
  struct free_block* b = (struct free_block*)((uintptr_t)p - h->offset);
  b->size = h->size;
  stats.in_use -= b->size;
  --stats.blocks;
  /* Insert in address order, then merge with the next and the previous block. */
  struct free_block* prev = 0;
  struct free_block* next = free_list;
  while (next != 0 && next < b) {
    prev = next;
    next = next->next;
  }
  b->next = next;
  if (next != 0 && (uintptr_t)b + b->size == (uintptr_t)next) {
    b->size += next->size;
    b->next = next->next;
  }
  if (prev != 0) {
    prev->next = b;
    if ((uintptr_t)prev + prev->size == (uintptr_t)b) {
      prev->size += b->size;
      prev->next = b->next;
    }
  } else {
    free_list = b;
  }
}

struct heap_stats heap_get_stats(void) { return stats; }
