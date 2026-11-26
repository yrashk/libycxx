// Round stack-size hints consistently in the runtime and portable worker threads.
#pragma once

#include <stddef.h>

static inline size_t __ycxx_thread_stack_size(size_t __stack_size, long __min, long __page) {
  /* The platform supplies its minimum at runtime. Darwin requires whole pages. */
  if (__min > 0 && __stack_size < (size_t)__min)
    __stack_size = (size_t)__min;
  if (__page > 0 && __stack_size % (size_t)__page != 0 && __stack_size <= (size_t)-1 - (size_t)__page)
    __stack_size += (size_t)__page - __stack_size % (size_t)__page;
  return __stack_size;
}
