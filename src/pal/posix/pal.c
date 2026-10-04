// libycxx POSIX platform abstraction layer (Linux, macOS), on top of the system C library.
// Written in C: the PAL is the boundary between libycxx and the OS and needs no C++.
#define _GNU_SOURCE
#include <ycxx/pal.h>

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#if defined(__linux__)
#  include <linux/futex.h>
#  include <sys/syscall.h>
#endif
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

/* Random sources: handle 0 is the system generator, any other handle is a file descriptor + 1. */
int ycxx_pal_random_open(const char* token, ycxx_pal_size len, ycxx_pal_handle* h) {
  static const char* const files[] = {"/dev/urandom", "/dev/random"};
  if ((len == 7 && memcmp(token, "default", 7) == 0) || (len == 9 && memcmp(token, "getrandom", 9) == 0)) {
    *h = 0;
    return 0;
  }
  for (size_t i = 0; i < sizeof files / sizeof files[0]; ++i) {
    if (len == strlen(files[i]) && memcmp(token, files[i], len) == 0) {
      int fd;
      do
        fd = open(files[i], O_RDONLY | O_CLOEXEC);
      while (fd < 0 && errno == EINTR);
      if (fd < 0)
        return errno;
      *h = (ycxx_pal_handle)fd + 1;
      return 0;
    }
  }
  return EINVAL;
}

static int read_all(int fd, unsigned char* p, ycxx_pal_size n) {
  while (n) {
    ssize_t r = read(fd, p, n);
    if (r < 0) {
      if (errno == EINTR)
        continue;
      return errno;
    }
    if (r == 0)
      return EIO;
    p += r;
    n -= (ycxx_pal_size)r;
  }
  return 0;
}

int ycxx_pal_random_read(ycxx_pal_handle h, void* data, ycxx_pal_size n) {
  if (h != 0)
    return read_all((int)(h - 1), (unsigned char*)data, n);
  int e = ycxx_pal_random(data, n);
  if (e == ENOSYS) { /* no getrandom/getentropy: fall back to the device */
    int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd < 0)
      return errno;
    e = read_all(fd, (unsigned char*)data, n);
    close(fd);
  }
  return e;
}

void ycxx_pal_random_close(ycxx_pal_handle h) {
  if (h != 0)
    close((int)(h - 1));
}


void ycxx_pal_wait(const ycxx_pal_u32* addr, ycxx_pal_u32 expected) {
#if defined(__linux__)
  syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, expected, NULL, NULL, 0);
#else
  /* No portable futex (macOS's address wait is private API or macOS 14.4+): poll briefly. */
  if (__atomic_load_n(addr, __ATOMIC_ACQUIRE) == expected) {
    struct timespec ts = {0, 50000};
    nanosleep(&ts, NULL);
  }
#endif
}

void ycxx_pal_wake_all(const ycxx_pal_u32* addr) {
#if defined(__linux__)
  syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, 0x7fffffff, NULL, NULL, 0);
#else
  (void)addr;
#endif
}

#if defined(__APPLE__)
/* libSystem's registration function for thread_local destructors. */
extern void _tlv_atexit(void (*)(void*), void*);

int ycxx_pal_thread_atexit(void (*f)(void*), void* obj, void* dso) {
  (void)dso;
  _tlv_atexit(f, obj);
  return 0;
}
#else
/* glibc's registration function for thread_local destructors; other C libraries may lack it. */
extern int __cxa_thread_atexit_impl(void (*)(void*), void*, void*) __attribute__((weak));

int ycxx_pal_thread_atexit(void (*f)(void*), void* obj, void* dso) {
  if (__cxa_thread_atexit_impl)
    return __cxa_thread_atexit_impl(f, obj, dso);
  return -1;
}
#endif

int ycxx_pal_error_message(int ev, char* buf, ycxx_pal_size n) {
  if (n == 0)
    return EINVAL;
  const int saved = errno; /* [syserr.general]/2: errno stays unchanged */
#if defined(__GLIBC__)
  /* _GNU_SOURCE selects glibc's strerror_r, which returns the message, possibly in a static
     string other than buf. */
  const char* s = strerror_r(ev, buf, n);
  if (s != buf) {
    size_t len = strlen(s);
    if (len >= n)
      len = n - 1;
    memcpy(buf, s, len);
    buf[len] = '\0';
  }
#else
  /* The POSIX strerror_r (musl provides it even with _GNU_SOURCE; so does macOS). */
  if (strerror_r(ev, buf, n) != 0)
    snprintf(buf, n, "Unknown error %d", ev);
#endif
  errno = saved;
  return 0;
}
