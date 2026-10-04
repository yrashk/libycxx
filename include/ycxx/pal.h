/* libycxx Platform Abstraction Layer.
 *
 * The hosted layer of libycxx talks to the operating system ONLY through the C-linkage
 * functions declared here. Porting libycxx to a new OS or bare-metal target means providing
 * definitions for these functions (see src/pal/posix for the reference implementation).
 *
 * Conventions:
 *   - Functions returning int return 0 on success and a positive errno-style code on failure.
 *   - No function throws. All are callable from C.
 *   - Opaque handles are pointer-sized integers (ycxx_pal_handle).
 */
#ifndef YCXX_PAL_H
#define YCXX_PAL_H

#ifdef __cplusplus
extern "C" {
#  define YCXX_PAL_NOEXCEPT noexcept
#  define YCXX_PAL_NORETURN [[noreturn]]
#else
#  define YCXX_PAL_NOEXCEPT
#  define YCXX_PAL_NORETURN _Noreturn
#endif

typedef __SIZE_TYPE__ ycxx_pal_size;
typedef __INT64_TYPE__ ycxx_pal_i64;
typedef __UINTPTR_TYPE__ ycxx_pal_handle;

/* ---- memory ------------------------------------------------------------------------------- */
/* Allocate `size` bytes aligned to `align` (a power of two). Returns null on failure. */
void* ycxx_pal_allocate(ycxx_pal_size size, ycxx_pal_size align) YCXX_PAL_NOEXCEPT;
/* Free memory from ycxx_pal_allocate. `size`/`align` are the values passed to allocate
 * (size may be 0 when unknown). Null is ignored. */
void ycxx_pal_deallocate(void* p, ycxx_pal_size size, ycxx_pal_size align) YCXX_PAL_NOEXCEPT;

/* ---- process ------------------------------------------------------------------------------ */
/* Terminate abnormally after writing `msg` (may be null) to the diagnostic stream. */
YCXX_PAL_NORETURN void ycxx_pal_abort(const char* msg) YCXX_PAL_NOEXCEPT;
/* Normal termination with status. */
YCXX_PAL_NORETURN void ycxx_pal_exit(int status) YCXX_PAL_NOEXCEPT;

/* ---- I/O ---------------------------------------------------------------------------------- */
enum { ycxx_pal_stdin = 0, ycxx_pal_stdout = 1, ycxx_pal_stderr = 2 };
/* Write up to n bytes to stream `fd`. Stores bytes written in *written. */
int ycxx_pal_write(ycxx_pal_handle fd, const void* data, ycxx_pal_size n, ycxx_pal_size* written) YCXX_PAL_NOEXCEPT;
/* Read up to n bytes from stream `fd`. *got == 0 with return 0 means end of file. */
int ycxx_pal_read(ycxx_pal_handle fd, void* data, ycxx_pal_size n, ycxx_pal_size* got) YCXX_PAL_NOEXCEPT;
/* Whether `fd` refers to a terminal (used by std::print for unicode/vprint_unicode). */
int ycxx_pal_is_terminal(ycxx_pal_handle fd) YCXX_PAL_NOEXCEPT;

/* ---- clocks ------------------------------------------------------------------------------- */
enum { ycxx_pal_clock_realtime = 0, ycxx_pal_clock_monotonic = 1 };
/* Current time as seconds + nanoseconds since the clock's epoch (realtime: Unix epoch). */
int ycxx_pal_clock_now(int clock, ycxx_pal_i64* sec, ycxx_pal_i64* nsec) YCXX_PAL_NOEXCEPT;

/* ---- randomness ---------------------------------------------------------------------------- */
/* Fill buffer with non-deterministic random bytes (std::random_device). */
int ycxx_pal_random(void* data, ycxx_pal_size n) YCXX_PAL_NOEXCEPT;
/* A random source chosen by name (std::random_device's token, `len` bytes, not null-terminated).
 * POSIX: "default" and "getrandom" (the system generator: getrandom/getentropy), "/dev/urandom",
 * "/dev/random". Stores a handle in *h; returns EINVAL for an unknown token. */
int ycxx_pal_random_open(const char* token, ycxx_pal_size len, ycxx_pal_handle* h) YCXX_PAL_NOEXCEPT;
/* Fills data with n bytes from the source. */
int ycxx_pal_random_read(ycxx_pal_handle h, void* data, ycxx_pal_size n) YCXX_PAL_NOEXCEPT;
/* Releases the source. */
void ycxx_pal_random_close(ycxx_pal_handle h) YCXX_PAL_NOEXCEPT;

/* ---- waiting on an address ---------------------------------------------------------------- */
typedef __UINT32_TYPE__ ycxx_pal_u32;
/* Blocks while *addr == expected (may also return spuriously). */
void ycxx_pal_wait(const ycxx_pal_u32* addr, ycxx_pal_u32 expected) YCXX_PAL_NOEXCEPT;
/* Wakes every thread blocked in ycxx_pal_wait on addr. */
void ycxx_pal_wake_all(const ycxx_pal_u32* addr) YCXX_PAL_NOEXCEPT;

/* ---- thread exit --------------------------------------------------------------------------- */
/* Registers f(obj) to run when the calling thread exits (thread_local destructors); dso is the
   registering object's __dso_handle. Returns 0 on success. */
int ycxx_pal_thread_atexit(void (*f)(void*), void* obj, void* dso) YCXX_PAL_NOEXCEPT;

/* ---- error messages ----------------------------------------------------------------------- */
/* Writes the C library's description of error number `ev` (as strerror, but thread-safe) to buf
   as a null-terminated string, truncated to n bytes. Leaves errno unchanged. Used by
   std::generic_category() and std::system_category(). */
int ycxx_pal_error_message(int ev, char* buf, ycxx_pal_size n) YCXX_PAL_NOEXCEPT;

/* Threads, filesystem and time zone hooks are added with phase 5. */

#ifdef __cplusplus
}
#endif

#endif /* YCXX_PAL_H */
