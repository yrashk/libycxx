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
#include <stdint.h>
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

/* The alignment malloc guarantees: that of max_align_t, and 16 bytes on Darwin, whose malloc
   aligns every block to 16 although max_align_t (long double) is 8 on arm64; without this,
   every operator new there (__STDCPP_DEFAULT_NEW_ALIGNMENT__ 16) would take posix_memalign. */
#if defined(__APPLE__)
enum { pal_malloc_align = 16 };
#else
enum { pal_malloc_align = alignof(max_align_t) };
#endif

void* ycxx_pal_allocate(ycxx_pal_size size, ycxx_pal_size align) {
  if (align <= pal_malloc_align)
    return malloc(size);
  void* p = NULL;
  if (align < sizeof(void*))
    align = sizeof(void*);
  return posix_memalign(&p, align, size) == 0 ? p : NULL;
}

void ycxx_pal_deallocate(void* p, ycxx_pal_size, ycxx_pal_size) { free(p); }

void ycxx_pal_abort(const char* __msg) {
  if (__msg) {
    (void)!write(2, __msg, strlen(__msg));
    (void)!write(2, "\n", 1);
  }
  abort();
}

void ycxx_pal_exit(int status) { exit(status); }

int ycxx_pal_write(ycxx_pal_handle __fd, const void* data, ycxx_pal_size n, ycxx_pal_size* __written) {
  ssize_t r;
  do
    r = write((int)(__fd), data, n);
  while (r < 0 && errno == EINTR);
  if (r < 0)
    return errno;
  *__written = (ycxx_pal_size)(r);
  return 0;
}

int ycxx_pal_read(ycxx_pal_handle __fd, void* data, ycxx_pal_size n, ycxx_pal_size* __got) {
  ssize_t r;
  do
    r = read((int)(__fd), data, n);
  while (r < 0 && errno == EINTR);
  if (r < 0)
    return errno;
  *__got = (ycxx_pal_size)(r);
  return 0;
}

int ycxx_pal_is_terminal(ycxx_pal_handle __fd) { return isatty((int)(__fd)); }

int ycxx_pal_clock_now(int clock, ycxx_pal_i64* __sec, ycxx_pal_i64* __nsec) {
  struct timespec __ts;
  if (clock_gettime(clock == ycxx_pal_clock_monotonic ? CLOCK_MONOTONIC : CLOCK_REALTIME, &__ts) != 0)
    return errno;
  *__sec = __ts.tv_sec;
  *__nsec = __ts.tv_nsec;
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
int ycxx_pal_random_open(const char* token, ycxx_pal_size __len, ycxx_pal_handle* h) {
  static const char* const __files[] = {"/dev/urandom", "/dev/random"};
  if ((__len == 7 && memcmp(token, "default", 7) == 0) || (__len == 9 && memcmp(token, "getrandom", 9) == 0)) {
    *h = 0;
    return 0;
  }
  for (size_t i = 0; i < sizeof __files / sizeof __files[0]; ++i) {
    if (__len == strlen(__files[i]) && memcmp(token, __files[i], __len) == 0) {
      int __fd;
      do
        __fd = open(__files[i], O_RDONLY | O_CLOEXEC);
      while (__fd < 0 && errno == EINTR);
      if (__fd < 0)
        return errno;
      *h = (ycxx_pal_handle)__fd + 1;
      return 0;
    }
  }
  return EINVAL;
}

static int read_all(int __fd, unsigned char* p, ycxx_pal_size n) {
  while (n) {
    ssize_t r = read(__fd, p, n);
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
    int __fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (__fd < 0)
      return errno;
    e = read_all(__fd, (unsigned char*)data, n);
    close(__fd);
  }
  return e;
}

void ycxx_pal_random_close(ycxx_pal_handle h) {
  if (h != 0)
    close((int)(h - 1));
}

#if defined(__APPLE__)
/* Darwin's address wait: the kernel's ulock interface, on which libSystem's os_unfair_lock and
   Apple's own libc++ (std::atomic<T>::wait) are built, present since macOS 10.12. The SDK does
   not declare it (its public wrapper, os_sync_wait_on_address, needs macOS 14.4), so it is
   declared here. A 32-bit compare-and-wait, private to the process; the timeout is in
   microseconds, 0 meaning none. With ULF_NO_ERRNO a failure is returned as -errno. */
extern int __ulock_wait(uint32_t operation, void* __addr, uint64_t value, uint32_t timeout_us);
extern int __ulock_wake(uint32_t operation, void* __addr, uint64_t wake_value);
enum { pal_ul_compare_and_wait = 1, pal_ulf_wake_all = 0x100, pal_ulf_no_errno = 0x01000000 };
#endif

void ycxx_pal_wait(const ycxx_pal_u32* __addr, ycxx_pal_u32 expected) {
#if defined(__linux__)
  syscall(SYS_futex, __addr, FUTEX_WAIT_PRIVATE, expected, NULL, NULL, 0);
#elif defined(__APPLE__)
  __ulock_wait(pal_ul_compare_and_wait | pal_ulf_no_errno, (void*)__addr, expected, 0);
#else
  /* No address wait known for this system: poll briefly. */
  if (__atomic_load_n(__addr, __ATOMIC_ACQUIRE) == expected) {
    struct timespec __ts = {0, 50000};
    nanosleep(&__ts, NULL);
  }
#endif
}

void ycxx_pal_wake_all(const ycxx_pal_u32* __addr) {
#if defined(__linux__)
  syscall(SYS_futex, __addr, FUTEX_WAKE_PRIVATE, 0x7fffffff, NULL, NULL, 0);
#elif defined(__APPLE__)
  __ulock_wake(pal_ul_compare_and_wait | pal_ulf_wake_all | pal_ulf_no_errno, (void*)__addr, 0);
#else
  (void)__addr;
#endif
}

void ycxx_pal_wake_one(const ycxx_pal_u32* __addr) {
#if defined(__linux__)
  syscall(SYS_futex, __addr, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);
#elif defined(__APPLE__)
  __ulock_wake(pal_ul_compare_and_wait | pal_ulf_no_errno, (void*)__addr, 0);
#else
  (void)__addr;
#endif
}

static clockid_t pal_clockid(int clock) { return clock == ycxx_pal_clock_monotonic ? CLOCK_MONOTONIC : CLOCK_REALTIME; }

/* Whether the absolute time sec:nsec of `clock` has passed. */
__attribute__((__unused__)) static int pal_passed(int clock, ycxx_pal_i64 __sec, ycxx_pal_i64 __nsec) {
  struct timespec now;
  clock_gettime(pal_clockid(clock), &now);
  return now.tv_sec > __sec || (now.tv_sec == __sec && now.tv_nsec >= __nsec);
}

int ycxx_pal_wait_until(const ycxx_pal_u32* __addr, ycxx_pal_u32 expected, int clock, ycxx_pal_i64 __sec,
                        ycxx_pal_i64 __nsec) {
  if (__sec < 0)
    return ETIMEDOUT;
#if defined(__linux__)
  /* FUTEX_WAIT_BITSET takes an absolute time, on CLOCK_MONOTONIC unless FUTEX_CLOCK_REALTIME. */
  struct timespec __ts = {(time_t)__sec, (long)__nsec};
  int op = FUTEX_WAIT_BITSET_PRIVATE | (clock == ycxx_pal_clock_realtime ? FUTEX_CLOCK_REALTIME : 0);
  long r = syscall(SYS_futex, __addr, op, expected, &__ts, NULL, FUTEX_BITSET_MATCH_ANY);
  if (r != 0 && errno == ETIMEDOUT)
    return ETIMEDOUT;
  return 0;
#elif defined(__APPLE__)
  /* ulock takes a relative timeout: the time left on `clock`, in microseconds rounded up, at
     most UINT32_MAX - 1 (0 would mean none); a wait cut short by that limit is not a timeout. */
  struct timespec now;
  clock_gettime(pal_clockid(clock), &now);
  if (now.tv_sec > __sec || (now.tv_sec == __sec && now.tv_nsec >= __nsec))
    return ETIMEDOUT;
  const ycxx_pal_i64 left_ns = (__sec - (ycxx_pal_i64)now.tv_sec) * 1000000000 + (__nsec - (ycxx_pal_i64)now.tv_nsec);
  const ycxx_pal_i64 left_us = left_ns / 1000 + (left_ns % 1000 != 0);
  const uint32_t timeout = left_us >= (ycxx_pal_i64)UINT32_MAX ? UINT32_MAX - 1 : (uint32_t)left_us;
  const int r = __ulock_wait(pal_ul_compare_and_wait | pal_ulf_no_errno, (void*)__addr, expected, timeout);
  if (r == -ETIMEDOUT && pal_passed(clock, __sec, __nsec))
    return ETIMEDOUT;
  return 0;
#else
  if (pal_passed(clock, __sec, __nsec))
    return ETIMEDOUT;
  ycxx_pal_wait(__addr, expected);
  return 0;
#endif
}

/* ---- threads ---- */
int ycxx_pal_thread_create(ycxx_pal_handle* thread, void* (*start)(void*), void* arg, ycxx_pal_size __stack_size) {
  pthread_attr_t __attr;
  int r = pthread_attr_init(&__attr);
  if (r != 0)
    return r;
  if (__stack_size != 0) {
    /* A size the system cannot use is a hint to adjust or ignore, not an error. At least the
       minimum, asked of sysconf (PTHREAD_STACK_MIN is not a constant in newer glibc, and which
       header defines it differs between C libraries), in whole pages (Darwin's
       pthread_attr_setstacksize rejects anything else). */
    const long min = sysconf(_SC_THREAD_STACK_MIN), page = sysconf(_SC_PAGESIZE);
    if (min > 0 && __stack_size < (ycxx_pal_size)min)
      __stack_size = (ycxx_pal_size)min;
    if (page > 0 && __stack_size % (ycxx_pal_size)page != 0 && __stack_size <= (ycxx_pal_size)-1 - (ycxx_pal_size)page)
      __stack_size += (ycxx_pal_size)page - __stack_size % (ycxx_pal_size)page;
    (void)pthread_attr_setstacksize(&__attr, __stack_size);
  }
  pthread_t t;
  r = pthread_create(&t, &__attr, start, arg);
  pthread_attr_destroy(&__attr);
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
  char __buf[16];
  size_t n = strlen(name);
  if (n > 15)
    n = 15;
  memcpy(__buf, name, n);
  __buf[n] = '\0';
  pthread_setname_np(pthread_self(), __buf);
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

void ycxx_pal_sleep_until(int clock, ycxx_pal_i64 __sec, ycxx_pal_i64 __nsec) {
  if (__sec < 0)
    return;
#if defined(__linux__)
  struct timespec __ts = {(time_t)__sec, (long)__nsec};
  while (clock_nanosleep(pal_clockid(clock), TIMER_ABSTIME, &__ts, NULL) == EINTR) {
  }
#else
  while (!pal_passed(clock, __sec, __nsec)) {
    struct timespec now, __rel;
    clock_gettime(pal_clockid(clock), &now);
    __rel.tv_sec = (time_t)(__sec - now.tv_sec);
    __rel.tv_nsec = (long)(__nsec - now.tv_nsec);
    if (__rel.tv_nsec < 0) {
      __rel.tv_nsec += 1000000000;
      --__rel.tv_sec;
    }
    if (__rel.tv_sec < 0)
      return;
    nanosleep(&__rel, NULL);
  }
#endif
}

/* The C library's registration of a thread_local destructor: f(obj) runs when the calling thread
   ends, and when it calls exit() (before the static destructors), in reverse order of
   registration, a destructor registered while they run included. */
#if defined(__APPLE__)
/* libSystem's (run by the thread's end and by exit()'s _tlv_exit). */
extern void _tlv_atexit(void (*)(void*), void*);

static int pal_register_thread_dtor(void (*__f)(void*), void* __obj, void* __dso) {
  (void)__dso;
  _tlv_atexit(__f, __obj);
  return 0;
}
#else
/* glibc's (run by the thread's end and by exit()'s __call_tls_dtors); other C libraries may lack
   it. dso keeps the registering object loaded while the destructor is pending. */
extern int __cxa_thread_atexit_impl(void (*)(void*), void*, void*) __attribute__((__weak__));

static int pal_register_thread_dtor(void (*__f)(void*), void* __obj, void* __dso) {
  if (__cxa_thread_atexit_impl)
    return __cxa_thread_atexit_impl(__f, __obj, __dso);
  return -1;
}
#endif

/* The thread-end list (std::notify_all_at_thread_exit, the *_at_thread_exit results, RCU's
   reader records): run after every thread_local destructor of the thread, when it ends and when it
   calls exit() (DECISIONS §3). The list is a thread_local pointer, run by the "sentinel", a
   thread_local destructor of the PAL's own registered before the thread's first other one (by
   ycxx_pal_thread_atexit, through which libycxx registers every thread_local destructor, or by
   the first ycxx_pal_at_thread_end): destructors run in reverse order of registration, so the
   sentinel comes last. Where no destructor can be registered, a pthread key destructor runs the
   list instead (after the C++ thread_local destructors; never for a thread calling exit()). */
struct pal_end_entry {
  void (*__f)(void*);
  void* arg;
  struct pal_end_entry* next;
};
static _Thread_local struct pal_end_entry* pal_end_list; /* with the sentinel */
static _Thread_local int pal_end_armed;                  /* 0: no sentinel pending, 1: pending, 2: cannot */
static pthread_key_t pal_end_key;
static pthread_once_t pal_end_once = PTHREAD_ONCE_INIT;
static int pal_end_key_ok;

static void pal_run_end_list(void* p) {
  struct pal_end_entry* e = (struct pal_end_entry*)p;
  while (e) {
    struct pal_end_entry* next = e->next;
    e->__f(e->arg);
    free(e);
    e = next;
  }
}

/* The sentinel. An action may register another action or construct a thread_local: the list is
   drained, and such a registration arms a new sentinel (which then finds the list empty, or what
   a destructor run after this one added). */
static void pal_end_sentinel(void* __unused_arg) {
  (void)__unused_arg;
  pal_end_armed = 0;
  struct pal_end_entry* e;
  while ((e = pal_end_list) != NULL) {
    pal_end_list = e->next;
    e->__f(e->arg);
    free(e);
  }
}

/* Registers the sentinel unless it is pending. The dso is this object's (the sentinel's address):
   its code must stay loaded until the sentinel has run. */
static void pal_arm_end_sentinel(void) {
  if (pal_end_armed == 0)
    pal_end_armed = pal_register_thread_dtor(&pal_end_sentinel, NULL, (void*)&pal_end_sentinel) == 0 ? 1 : 2;
}

int ycxx_pal_thread_atexit(void (*__f)(void*), void* __obj, void* __dso) {
  pal_arm_end_sentinel();
  return pal_register_thread_dtor(__f, __obj, __dso);
}

static void pal_make_end_key(void) { pal_end_key_ok = pthread_key_create(&pal_end_key, pal_run_end_list) == 0; }

int ycxx_pal_at_thread_end(void (*__f)(void*), void* arg) {
  pal_arm_end_sentinel();
  struct pal_end_entry* e = (struct pal_end_entry*)malloc(sizeof *e);
  if (!e)
    return ENOMEM;
  e->__f = __f;
  e->arg = arg;
  if (pal_end_armed == 1) {
    e->next = pal_end_list;
    pal_end_list = e;
    return 0;
  }
  pthread_once(&pal_end_once, pal_make_end_key);
  if (!pal_end_key_ok) {
    free(e);
    return EAGAIN;
  }
  e->next = (struct pal_end_entry*)pthread_getspecific(pal_end_key);
  int r = pthread_setspecific(pal_end_key, e);
  if (r != 0)
    free(e);
  return r;
}

int ycxx_pal_error_message(int __ev, char* __buf, ycxx_pal_size n) {
  if (n == 0)
    return EINVAL;
  const int __saved = errno; /* [syserr.general]/2: errno stays unchanged */
#if defined(__GLIBC__)
  /* _GNU_SOURCE selects glibc's strerror_r, which returns the message, possibly in a static
     string other than buf. */
  const char* s = strerror_r(__ev, __buf, n);
  if (s != __buf) {
    size_t __len = strlen(s);
    if (__len >= n)
      __len = n - 1;
    memcpy(__buf, s, __len);
    __buf[__len] = '\0';
  }
#else
  /* The POSIX strerror_r (musl provides it even with _GNU_SOURCE; so does macOS). */
  if (strerror_r(__ev, __buf, n) != 0)
    snprintf(__buf, n, "Unknown error %d", __ev);
#endif
  errno = __saved;
  return 0;
}
