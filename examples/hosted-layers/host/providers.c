/* The hosted layers this host program provides itself (examples/hosted-layers/README.md): the
 * primitives of ycxx/pal.h for the layers abort, memory, console and clock. libycxx is built with
 * YCXX_PAL=none, so none of its POSIX platform layer is in the program: memory comes from a
 * static arena (../common/heap.c), never from malloc; the console is the write system call; the
 * clock is clock_gettime. Plain C, compiled with the host's C library. */
#include <ycxx/pal.h>

#include "../common/heap.h"

#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* ---- memory: an 8 MiB static arena ---- */
static _Alignas(64) unsigned char arena[8u << 20];
static int heap_ready;

void* ycxx_pal_allocate(ycxx_pal_size size, ycxx_pal_size align) {
  if (!heap_ready) {
    heap_init(arena, sizeof arena);
    heap_ready = 1;
  }
  return heap_allocate(size, align);
}

void ycxx_pal_deallocate(void* p, ycxx_pal_size size, ycxx_pal_size align) {
  (void)size;
  (void)align;
  heap_free(p);
}

/* ---- console (standard output and input) and abort's diagnostic stream ---- */
int ycxx_pal_write(ycxx_pal_handle fd, const void* data, ycxx_pal_size n, ycxx_pal_size* written) {
  *written = 0;
  if (fd != ycxx_pal_stdout && fd != ycxx_pal_stderr)
    return EBADF;
  for (;;) {
    const ssize_t r = write((int)fd, data, n);
    if (r >= 0) {
      *written = (ycxx_pal_size)r;
      return 0;
    }
    if (errno != EINTR)
      return errno;
  }
}

int ycxx_pal_read(ycxx_pal_handle fd, void* data, ycxx_pal_size n, ycxx_pal_size* got) {
  *got = 0;
  if (fd != ycxx_pal_stdin)
    return EBADF;
  for (;;) {
    const ssize_t r = read(0, data, n);
    if (r >= 0) {
      *got = (ycxx_pal_size)r;
      return 0;
    }
    if (errno != EINTR)
      return errno;
  }
}

int ycxx_pal_is_terminal(ycxx_pal_handle fd) { return isatty((int)fd); }

/* ---- abort ---- */
_Noreturn void ycxx_pal_abort(const char* msg) {
  static const char prefix[] = "hosted-layers (host): abort: ";
  ycxx_pal_size w;
  ycxx_pal_write(ycxx_pal_stderr, prefix, sizeof prefix - 1, &w);
  if (msg != 0) {
    ycxx_pal_size n = 0;
    while (msg[n] != '\0')
      ++n;
    ycxx_pal_write(ycxx_pal_stderr, msg, n, &w);
  }
  ycxx_pal_write(ycxx_pal_stderr, "\n", 1, &w);
  abort();
}

/* ---- clock ---- */
int ycxx_pal_clock_now(int clock, ycxx_pal_i64* sec, ycxx_pal_i64* nsec) {
  struct timespec ts;
  if (clock_gettime(clock == ycxx_pal_clock_monotonic ? CLOCK_MONOTONIC : CLOCK_REALTIME, &ts) != 0)
    return errno;
  *sec = ts.tv_sec;
  *nsec = ts.tv_nsec;
  return 0;
}
