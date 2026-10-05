// libycxx POSIX platform abstraction layer (Linux, macOS), on top of the system C library.
// Written in C: the PAL is the boundary between libycxx and the OS and needs no C++.
#define _GNU_SOURCE
#include <ycxx/pal.h>

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
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
#if defined(__APPLE__)
#  include <sys/sysctl.h>
#endif
#if __has_include(<sys/single_threaded.h>)
#  include <sys/single_threaded.h>
#endif
#if __has_include(<sys/random.h>)
#  include <sys/random.h>
#endif


#if __has_include(<sys/single_threaded.h>)
const char* const ycxx_pal_single_threaded = &__libc_single_threaded;
#else
static const char ycxx_pal_never_single_threaded = 0;
const char* const ycxx_pal_single_threaded = &ycxx_pal_never_single_threaded;
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

void ycxx_pal_wake_one(const ycxx_pal_u32* addr) {
#if defined(__linux__)
  syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
#else
  (void)addr;
#endif
}

static clockid_t pal_clockid(int clock) { return clock == ycxx_pal_clock_monotonic ? CLOCK_MONOTONIC : CLOCK_REALTIME; }

/* Whether the absolute time sec:nsec of `clock` has passed. */
__attribute__((unused)) static int pal_passed(int clock, ycxx_pal_i64 sec, ycxx_pal_i64 nsec) {
  struct timespec now;
  clock_gettime(pal_clockid(clock), &now);
  return now.tv_sec > sec || (now.tv_sec == sec && now.tv_nsec >= nsec);
}

int ycxx_pal_wait_until(const ycxx_pal_u32* addr, ycxx_pal_u32 expected, int clock, ycxx_pal_i64 sec,
                        ycxx_pal_i64 nsec) {
  if (sec < 0)
    return ETIMEDOUT;
#if defined(__linux__)
  /* FUTEX_WAIT_BITSET takes an absolute time, on CLOCK_MONOTONIC unless FUTEX_CLOCK_REALTIME. */
  struct timespec ts = {(time_t)sec, (long)nsec};
  int op = FUTEX_WAIT_BITSET_PRIVATE | (clock == ycxx_pal_clock_realtime ? FUTEX_CLOCK_REALTIME : 0);
  long r = syscall(SYS_futex, addr, op, expected, &ts, NULL, FUTEX_BITSET_MATCH_ANY);
  if (r != 0 && errno == ETIMEDOUT)
    return ETIMEDOUT;
  return 0;
#else
  if (pal_passed(clock, sec, nsec))
    return ETIMEDOUT;
  ycxx_pal_wait(addr, expected);
  return 0;
#endif
}

/* ---- threads ---- */
int ycxx_pal_thread_create(ycxx_pal_handle* thread, void* (*start)(void*), void* arg, ycxx_pal_size stack_size) {
  pthread_attr_t attr;
  int r = pthread_attr_init(&attr);
  if (r != 0)
    return r;
  if (stack_size != 0) {
    /* A size the system cannot use is a hint to ignore, not an error. */
    if (stack_size < (ycxx_pal_size)PTHREAD_STACK_MIN)
      stack_size = (ycxx_pal_size)PTHREAD_STACK_MIN;
    (void)pthread_attr_setstacksize(&attr, stack_size);
  }
  pthread_t t;
  r = pthread_create(&t, &attr, start, arg);
  pthread_attr_destroy(&attr);
  if (r == 0)
    *thread = (ycxx_pal_handle)t;
  return r;
}

int ycxx_pal_thread_join(ycxx_pal_handle thread) { return pthread_join((pthread_t)thread, NULL); }

int ycxx_pal_thread_detach(ycxx_pal_handle thread) { return pthread_detach((pthread_t)thread); }

ycxx_pal_handle ycxx_pal_thread_self(void) { return (ycxx_pal_handle)pthread_self(); }

void ycxx_pal_thread_set_name(const char* name) {
#if defined(__APPLE__)
  pthread_setname_np(name);
#elif defined(__linux__)
  /* Linux limits names to 15 bytes plus the terminator. */
  char buf[16];
  size_t n = strlen(name);
  if (n > 15)
    n = 15;
  memcpy(buf, name, n);
  buf[n] = '\0';
  pthread_setname_np(pthread_self(), buf);
#else
  (void)name;
#endif
}

void ycxx_pal_thread_yield(void) { sched_yield(); }

unsigned ycxx_pal_hardware_concurrency(void) {
#if defined(__linux__)
  cpu_set_t set;
  if (sched_getaffinity(0, sizeof set, &set) == 0) {
    int n = CPU_COUNT(&set);
    if (n > 0)
      return (unsigned)n;
  }
#endif
  long n = sysconf(_SC_NPROCESSORS_ONLN);
  return n > 0 ? (unsigned)n : 0;
}

void ycxx_pal_sleep_until(int clock, ycxx_pal_i64 sec, ycxx_pal_i64 nsec) {
  if (sec < 0)
    return;
#if defined(__linux__)
  struct timespec ts = {(time_t)sec, (long)nsec};
  while (clock_nanosleep(pal_clockid(clock), TIMER_ABSTIME, &ts, NULL) == EINTR) {
  }
#else
  while (!pal_passed(clock, sec, nsec)) {
    struct timespec now, rel;
    clock_gettime(pal_clockid(clock), &now);
    rel.tv_sec = (time_t)(sec - now.tv_sec);
    rel.tv_nsec = (long)(nsec - now.tv_nsec);
    if (rel.tv_nsec < 0) {
      rel.tv_nsec += 1000000000;
      --rel.tv_sec;
    }
    if (rel.tv_sec < 0)
      return;
    nanosleep(&rel, NULL);
  }
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

/* The thread-end list: a pthread key whose destructor runs the calling thread's entries. POSIX
   key destructors run after the C++ thread_local destructors (glibc: __call_tls_dtors comes
   first; macOS: the TLV destructors run from the first key destructor round). */
struct pal_end_entry {
  void (*f)(void*);
  void* arg;
  struct pal_end_entry* next;
};
static pthread_key_t pal_end_key;
static pthread_once_t pal_end_once = PTHREAD_ONCE_INIT;
static int pal_end_key_ok;

static void pal_run_end_list(void* p) {
  struct pal_end_entry* e = (struct pal_end_entry*)p;
  while (e) {
    struct pal_end_entry* next = e->next;
    e->f(e->arg);
    free(e);
    e = next;
  }
}

static void pal_make_end_key(void) { pal_end_key_ok = pthread_key_create(&pal_end_key, pal_run_end_list) == 0; }

int ycxx_pal_at_thread_end(void (*f)(void*), void* arg) {
  pthread_once(&pal_end_once, pal_make_end_key);
  if (!pal_end_key_ok)
    return EAGAIN;
  struct pal_end_entry* e = (struct pal_end_entry*)malloc(sizeof *e);
  if (!e)
    return ENOMEM;
  e->f = f;
  e->arg = arg;
  e->next = (struct pal_end_entry*)pthread_getspecific(pal_end_key);
  int r = pthread_setspecific(pal_end_key, e);
  if (r != 0)
    free(e);
  return r;
}

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
