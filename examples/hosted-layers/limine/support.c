/* What the environment of a C++ program provides besides libycxx's hosted layers, written for
 * this kernel (examples/hosted-layers/README.md, "What the environment provides"):
 *
 *   - the functions the compilers call: memcpy, memmove, memset, memcmp (every freestanding
 *     program), and strlen, strcmp, strncmp, which libycxx's ABI runtime calls;
 *   - __cxa_atexit and __dso_handle (static destructors; libycxx's single-thread fallback also
 *     registers thread_local destructors here, DECISIONS §18), run by run_atexit_functions;
 *   - what GCC's unwinder, libgcc_eh.a, needs. It is built for GNU/Linux: it finds a function's
 *     unwind table through glibc's _dl_find_object, which returns the .eh_frame_hdr of the object
 *     holding an address (here: the kernel image, whose .eh_frame_hdr the linker makes with
 *     --eh-frame-hdr), and it refers to abort, malloc, free and a few pthread functions, which
 *     only its paths for frames registered at run time (none here) or for several threads use.
 *     pthread_once is called (once): this kernel has one thread, so it just runs the function.
 *
 * Freestanding C. The memory functions use the string instructions, so that no compiler turns a
 * loop in them back into a call to themselves. */
#include <stddef.h>
#include <stdint.h>

#include <ycxx/pal.h>

#include "../common/heap.h"

/* ---- memory and string functions ---- */
void* memcpy(void* restrict dst, const void* restrict src, size_t n) {
  void* d = dst;
  __asm__ volatile("rep movsb" : "+D"(d), "+S"(src), "+c"(n) : : "memory");
  return dst;
}

void* memmove(void* dst, const void* src, size_t n) {
  if ((uintptr_t)dst - (uintptr_t)src >= n) { /* no overlap, or dst below src: forward */
    void* d = dst;
    __asm__ volatile("rep movsb" : "+D"(d), "+S"(src), "+c"(n) : : "memory");
  } else if (n != 0) { /* dst above src, overlapping: backward */
    unsigned char* d = (unsigned char*)dst + n - 1;
    const unsigned char* s = (const unsigned char*)src + n - 1;
    __asm__ volatile("std; rep movsb; cld" : "+D"(d), "+S"(s), "+c"(n) : : "memory");
  }
  return dst;
}

void* memset(void* dst, int c, size_t n) {
  void* d = dst;
  __asm__ volatile("rep stosb" : "+D"(d), "+c"(n) : "a"(c) : "memory");
  return dst;
}

int memcmp(const void* a, const void* b, size_t n) {
  const unsigned char* x = a;
  const unsigned char* y = b;
  for (size_t i = 0; i < n; ++i)
    if (x[i] != y[i])
      return x[i] < y[i] ? -1 : 1;
  return 0;
}

size_t strlen(const char* s) {
  size_t n = 0;
  while (s[n] != '\0')
    ++n;
  return n;
}

int strncmp(const char* a, const char* b, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    const unsigned char x = (unsigned char)a[i], y = (unsigned char)b[i];
    if (x != y)
      return x < y ? -1 : 1;
    if (x == '\0')
      return 0;
  }
  return 0;
}

int strcmp(const char* a, const char* b) { return strncmp(a, b, SIZE_MAX); }

/* ---- static destructors ---- */
struct atexit_entry {
  void (*f)(void*);
  void* obj;
};
static struct atexit_entry atexit_entries[128];
static size_t atexit_count;

/* The Itanium C++ ABI's handle of this "shared object" (the kernel). */
void* __dso_handle = &__dso_handle;

int __cxa_atexit(void (*f)(void*), void* obj, void* dso) {
  (void)dso;
  if (atexit_count == sizeof atexit_entries / sizeof atexit_entries[0])
    return -1;
  atexit_entries[atexit_count].f = f;
  atexit_entries[atexit_count].obj = obj;
  ++atexit_count;
  return 0;
}

/* In reverse order of registration ([basic.start.term]). */
void run_atexit_functions(void) {
  while (atexit_count != 0) {
    --atexit_count;
    atexit_entries[atexit_count].f(atexit_entries[atexit_count].obj);
  }
}

/* ---- for libgcc_eh ---- */
/* glibc's struct dl_find_object on x86_64 (<dlfcn.h>): the fields the unwinder reads first. */
struct dl_find_object {
  unsigned long long dlfo_flags;
  void* dlfo_map_start;
  void* dlfo_map_end;
  void* dlfo_link_map;
  void* dlfo_eh_frame;
  unsigned long long dlfo_reserved[7];
};

extern char __kernel_start[], __kernel_end[], __eh_frame_hdr_start[]; /* linker.ld */

int _dl_find_object(void* pc, struct dl_find_object* result) {
  if ((char*)pc < __kernel_start || (char*)pc >= __kernel_end)
    return -1;
  result->dlfo_flags = 0;
  result->dlfo_map_start = __kernel_start;
  result->dlfo_map_end = __kernel_end;
  result->dlfo_link_map = 0;
  result->dlfo_eh_frame = __eh_frame_hdr_start;
  return 0;
}

_Noreturn void abort(void) { ycxx_pal_abort("abort() called"); }

void* malloc(size_t n) { return heap_allocate(n, 16); }
void free(void* p) { heap_free(p); }

typedef int pthread_once_t; /* glibc's, as libgcc_eh was built against it */
int pthread_once(pthread_once_t* once, void (*f)(void)) {
  if (*once == 0) {
    *once = 1;
    f();
  }
  return 0;
}
int pthread_mutex_lock(void* m) {
  (void)m;
  return 0;
}
int pthread_mutex_unlock(void* m) {
  (void)m;
  return 0;
}
int pthread_cond_wait(void* c, void* m) {
  (void)c;
  (void)m;
  return 0;
}
int pthread_cond_broadcast(void* c) {
  (void)c;
  return 0;
}
