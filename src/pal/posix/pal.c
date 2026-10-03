// libycxx POSIX platform abstraction layer (Linux, macOS), on top of the system C library.
// Written in C: the PAL is the boundary between libycxx and the OS and needs no C++.
#define _GNU_SOURCE
#include <ycxx/pal.h>

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#if __has_include(<sys/random.h>)
#  include <sys/random.h>
#endif


void* ycxx_pal_allocate(ycxx_pal_size size, ycxx_pal_size align) {
  if (align <= alignof(max_align_t))
    return malloc(size);
  void* p = NULL;
  if (align < sizeof(void*))
    align = sizeof(void*);
  return posix_memalign(&p, align, size) == 0 ? p : NULL;
}

void ycxx_pal_deallocate(void* p, ycxx_pal_size, ycxx_pal_size) { free(p); }

void ycxx_pal_abort(const char* msg) {
  if (msg) {
    (void)!write(2, msg, strlen(msg));
    (void)!write(2, "\n", 1);
  }
  abort();
}

void ycxx_pal_exit(int status) { exit(status); }

int ycxx_pal_write(ycxx_pal_handle fd, const void* data, ycxx_pal_size n, ycxx_pal_size* written) {
  ssize_t r;
  do
    r = write((int)(fd), data, n);
  while (r < 0 && errno == EINTR);
  if (r < 0)
    return errno;
  *written = (ycxx_pal_size)(r);
  return 0;
}

int ycxx_pal_read(ycxx_pal_handle fd, void* data, ycxx_pal_size n, ycxx_pal_size* got) {
  ssize_t r;
  do
    r = read((int)(fd), data, n);
  while (r < 0 && errno == EINTR);
  if (r < 0)
    return errno;
  *got = (ycxx_pal_size)(r);
  return 0;
}

int ycxx_pal_is_terminal(ycxx_pal_handle fd) { return isatty((int)(fd)); }

int ycxx_pal_clock_now(int clock, ycxx_pal_i64* sec, ycxx_pal_i64* nsec) {
  struct timespec ts;
  if (clock_gettime(clock == ycxx_pal_clock_monotonic ? CLOCK_MONOTONIC : CLOCK_REALTIME, &ts) != 0)
    return errno;
  *sec = ts.tv_sec;
  *nsec = ts.tv_nsec;
  return 0;
}

int ycxx_pal_random(void* data, ycxx_pal_size n) {
  unsigned char* p = (unsigned char*)data;
  while (n) {
    ssize_t r = getentropy(p, n > 256 ? 256 : n) == 0 ? (n > 256 ? 256 : (ssize_t)(n)) : -1;
    if (r < 0)
      return errno;
    p += r;
    n -= (ycxx_pal_size)(r);
  }
  return 0;
}

