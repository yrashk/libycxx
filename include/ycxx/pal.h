/* libycxx Platform Abstraction Layer.
 *
 * The hosted layer of libycxx talks to the operating system ONLY through the C-linkage
 * functions declared here. Porting libycxx to a new OS or bare-metal target means providing
 * definitions for these functions (see src/pal/posix for the reference implementation).
 *
 * Conventions:
 *   - Functions returning int return 0 on success and a positive errno-style code on failure.
 *   - No function throws. All are callable from C.
 *   - Opaque handles are pointer-sized integers (__ycxx_pal_handle).
 *   - Every function has hidden visibility (the attribute on each declaration, which a definition
 *     inherits): an image that links libycxx binds to its own PAL and exports none of it.
 */
#ifndef _YCXX_PAL_H
#define _YCXX_PAL_H

#ifdef __cplusplus
extern "C" {
#  define _YCXX_PAL_NOEXCEPT noexcept
#  define _YCXX_PAL_NORETURN [[noreturn]]
#else
#  define _YCXX_PAL_NOEXCEPT
#  define _YCXX_PAL_NORETURN _Noreturn
#endif

typedef __SIZE_TYPE__ __ycxx_pal_size;
typedef __INT64_TYPE__ __ycxx_pal_i64;
typedef __UINTPTR_TYPE__ __ycxx_pal_handle;

/* ---- memory ------------------------------------------------------------------------------- */
/* Allocate `size` bytes aligned to `align` (a power of two). Returns null on failure. */
[[__gnu__::__visibility__("hidden")]] void* __ycxx_pal_allocate(__ycxx_pal_size size, __ycxx_pal_size align) _YCXX_PAL_NOEXCEPT;
/* Free memory from __ycxx_pal_allocate. `size`/`align` are the values passed to allocate
 * (size may be 0 when unknown). Null is ignored. */
[[__gnu__::__visibility__("hidden")]] void __ycxx_pal_deallocate(void* p, __ycxx_pal_size size, __ycxx_pal_size align) _YCXX_PAL_NOEXCEPT;

/* ---- process ------------------------------------------------------------------------------ */
/* Terminate abnormally after writing `__msg` (may be null) to the diagnostic stream. */
[[__gnu__::__visibility__("hidden")]] _YCXX_PAL_NORETURN void __ycxx_pal_abort(const char* __msg) _YCXX_PAL_NOEXCEPT;
/* Normal termination with status. */
[[__gnu__::__visibility__("hidden")]] _YCXX_PAL_NORETURN void __ycxx_pal_exit(int status) _YCXX_PAL_NOEXCEPT;

/* ---- I/O ---------------------------------------------------------------------------------- */
enum { __ycxx_pal_stdin = 0, __ycxx_pal_stdout = 1, __ycxx_pal_stderr = 2 };
/* Write up to n bytes to stream `__fd`. Stores bytes written in *written. */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_write(__ycxx_pal_handle __fd, const void* data, __ycxx_pal_size n, __ycxx_pal_size* __written) _YCXX_PAL_NOEXCEPT;
/* Read up to n bytes from stream `__fd`. *got == 0 with return 0 means end of file. */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_read(__ycxx_pal_handle __fd, void* data, __ycxx_pal_size n, __ycxx_pal_size* __got) _YCXX_PAL_NOEXCEPT;
/* Whether `__fd` refers to a terminal (used by std::print for unicode/vprint_unicode). */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_is_terminal(__ycxx_pal_handle __fd) _YCXX_PAL_NOEXCEPT;

/* ---- clocks ------------------------------------------------------------------------------- */
enum { __ycxx_pal_clock_realtime = 0, __ycxx_pal_clock_monotonic = 1 };
/* Current time as seconds + nanoseconds since the clock's epoch (realtime: Unix epoch). */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_clock_now(int clock, __ycxx_pal_i64* __sec, __ycxx_pal_i64* __nsec) _YCXX_PAL_NOEXCEPT;

/* ---- randomness ---------------------------------------------------------------------------- */
/* Fill buffer with non-deterministic random bytes (std::random_device). */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_random(void* data, __ycxx_pal_size n) _YCXX_PAL_NOEXCEPT;
/* A random source chosen by name (std::random_device's token, `__len` bytes, not null-terminated).
 * POSIX: "default" and "getrandom" (the system generator: getrandom/getentropy), "/dev/urandom",
 * "/dev/random". Stores a handle in *h; returns EINVAL for an unknown token. */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_random_open(const char* token, __ycxx_pal_size __len, __ycxx_pal_handle* h) _YCXX_PAL_NOEXCEPT;
/* Fills data with n bytes from the source. */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_random_read(__ycxx_pal_handle h, void* data, __ycxx_pal_size n) _YCXX_PAL_NOEXCEPT;
/* Releases the source. */
[[__gnu__::__visibility__("hidden")]] void __ycxx_pal_random_close(__ycxx_pal_handle h) _YCXX_PAL_NOEXCEPT;

/* ---- waiting on an address ---------------------------------------------------------------- */
typedef __UINT32_TYPE__ __ycxx_pal_u32;
/* Blocks while *addr == expected (may also return spuriously). */
[[__gnu__::__visibility__("hidden")]] void __ycxx_pal_wait(const __ycxx_pal_u32* __addr, __ycxx_pal_u32 expected) _YCXX_PAL_NOEXCEPT;
/* Wakes every thread blocked in __ycxx_pal_wait on addr. */
[[__gnu__::__visibility__("hidden")]] void __ycxx_pal_wake_all(const __ycxx_pal_u32* __addr) _YCXX_PAL_NOEXCEPT;
/* Wakes at least one thread blocked in __ycxx_pal_wait on addr, if any (may wake more). */
[[__gnu__::__visibility__("hidden")]] void __ycxx_pal_wake_one(const __ycxx_pal_u32* __addr) _YCXX_PAL_NOEXCEPT;
/* As __ycxx_pal_wait, but returns no later than (about) the absolute time sec:nsec of `clock`
   (__ycxx_pal_clock_realtime or __ycxx_pal_clock_monotonic). Returns 0 when woken, when *addr !=
   expected, or spuriously, and a nonzero value when it returned because the time passed.
   Callers re-check both their condition and the clock. */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_wait_until(const __ycxx_pal_u32* __addr, __ycxx_pal_u32 expected, int clock, __ycxx_pal_i64 __sec,
                        __ycxx_pal_i64 __nsec) _YCXX_PAL_NOEXCEPT;

/* ---- threads ------------------------------------------------------------------------------- */
/* Starts a thread running start(arg); stack_size 0 means the default. Stores its handle in
   *thread. The handle is also the thread's identity (__ycxx_pal_thread_self in that thread
   returns it) until the thread is joined or, if detached, ends. */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_thread_create(__ycxx_pal_handle* thread, void* (*start)(void*), void* arg,
                           __ycxx_pal_size __stack_size) _YCXX_PAL_NOEXCEPT;
/* Waits for the thread to end and releases its handle. */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_thread_join(__ycxx_pal_handle thread) _YCXX_PAL_NOEXCEPT;
/* Releases the handle; the thread's resources are freed when it ends. */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_thread_detach(__ycxx_pal_handle thread) _YCXX_PAL_NOEXCEPT;
/* The calling thread's handle. */
[[__gnu__::__visibility__("hidden")]] __ycxx_pal_handle __ycxx_pal_thread_self(void) _YCXX_PAL_NOEXCEPT;
/* Names the calling thread (best effort: may be truncated or ignored). */
[[__gnu__::__visibility__("hidden")]] void __ycxx_pal_thread_set_name(const char* name) _YCXX_PAL_NOEXCEPT;
/* Offers the rest of the calling thread's time slice to other threads. */
[[__gnu__::__visibility__("hidden")]] void __ycxx_pal_thread_yield(void) _YCXX_PAL_NOEXCEPT;
/* The number of hardware threads available, or 0 if unknown. */
[[__gnu__::__visibility__("hidden")]] unsigned __ycxx_pal_hardware_concurrency(void) _YCXX_PAL_NOEXCEPT;
/* Blocks the calling thread until the absolute time sec:nsec of `clock` has passed. */
[[__gnu__::__visibility__("hidden")]] void __ycxx_pal_sleep_until(int clock, __ycxx_pal_i64 __sec, __ycxx_pal_i64 __nsec) _YCXX_PAL_NOEXCEPT;

/* Points to a flag that is nonzero only while the process certainly has a single thread: it is
   cleared before a second thread starts, and set again (if ever) only once the process is back to
   one thread, in a way that synchronizes with the other threads' ends. While it is set, the
   library updates the reference counts and uncontended locks of process-private objects with
   plain instead of atomic read-modify-write instructions. A port that cannot tell points it to
   a constant zero (the freestanding default). POSIX/glibc: glibc's __libc_single_threaded. */
[[__gnu__::__visibility__("hidden")]] extern const char* const __ycxx_pal_single_threaded;

/* ---- thread exit --------------------------------------------------------------------------- */
/* Registers f(obj) to run when the calling thread exits (thread_local destructors); dso is the
   registering object's __dso_handle. Returns 0 on success. */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_thread_atexit(void (*__f)(void*), void* __obj, void* __dso) _YCXX_PAL_NOEXCEPT;
/* Registers f(arg) to run when the calling thread ends, after its thread_local objects are
   destroyed (std::notify_all_at_thread_exit, promise::set_value_at_thread_exit). Not run for the
   thread that ends the process. Returns 0 on success. */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_at_thread_end(void (*__f)(void*), void* arg) _YCXX_PAL_NOEXCEPT;

/* ---- error messages ----------------------------------------------------------------------- */
/* Writes the C library's description of error number `__ev` (as strerror, but thread-safe) to buf
   as a null-terminated string, truncated to n bytes. Leaves errno unchanged. Used by
   std::generic_category() and std::system_category(). */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_error_message(int __ev, char* __buf, __ycxx_pal_size n) _YCXX_PAL_NOEXCEPT;

/* ---- debugging ---------------------------------------------------------------------------- */
/* Nonzero if the process is being traced, presumably by a debugger (std::is_debugger_present).
   An immediate query: the answer is not cached. */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_debugger_present(void) _YCXX_PAL_NOEXCEPT;

/* ---- stack traces ------------------------------------------------------------------------- */
/* The loaded object (the executable or a shared library) containing the address pc: its file
   name, written to path as a null-terminated string truncated to n bytes (a name the object
   file can be opened by), and its load bias (run-time address minus link-time address).
   Returns 0 if found. */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_object_of(__ycxx_pal_handle __pc, char* path, __ycxx_pal_size n, __ycxx_pal_handle* __bias) _YCXX_PAL_NOEXCEPT;
/* The dynamic symbol containing pc (the name the dynamic linker knows, possibly mangled) and its
   start address. Returns 0 if found; *name stays valid while the object is loaded. */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_dynamic_symbol(__ycxx_pal_handle __pc, const char** name, __ycxx_pal_handle* start) _YCXX_PAL_NOEXCEPT;
/* Maps the file at path read-only into memory. Returns 0 on success. */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_map_file(const char* path, const void** data, __ycxx_pal_size* size) _YCXX_PAL_NOEXCEPT;
/* Unmaps a file mapped by __ycxx_pal_map_file. */
[[__gnu__::__visibility__("hidden")]] void __ycxx_pal_unmap_file(const void* data, __ycxx_pal_size size) _YCXX_PAL_NOEXCEPT;

/* ---- character encoding ------------------------------------------------------------------- */
/* Writes the name of the environment's character encoding (POSIX: the codeset of the locale "")
   to buf as a null-terminated string, truncated to n bytes (std::text_encoding::environment). */
[[__gnu__::__visibility__("hidden")]] int __ycxx_pal_environment_encoding(char* __buf, __ycxx_pal_size n) _YCXX_PAL_NOEXCEPT;

/* Filesystem and time zone hooks are added with phase 5. */

#ifdef __cplusplus
}
#endif

#endif /* _YCXX_PAL_H */
