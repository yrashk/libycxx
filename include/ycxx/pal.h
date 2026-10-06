/* libycxx Platform Abstraction Layer.
 *
 * The hosted layer of libycxx talks to the operating system ONLY through the C-linkage
 * functions declared here. Porting libycxx to a new OS or bare-metal target means providing
 * definitions for these functions (see src/pal/posix for the reference implementation).
 *
 * The functions are grouped into hosted layers (DECISIONS §18): each section below names its
 * layer. A build with YCXX_PAL=none needs definitions only for the layers it selects
 * (YCXX_HOSTED_LAYERS), from the integrator's providers; using a feature of an absent layer fails
 * to compile or link, and the link error names the missing function. 'abort' is always needed.
 * Three absent layers have fallbacks in libycxx (threads: one thread of execution; environment:
 * plain messages; debug: no debugger). examples/hosted-layers has providers for a host program
 * and for a bare-metal kernel.
 *
 * Conventions:
 *   - Functions returning int return 0 on success and a positive errno-style code on failure.
 *   - No function throws. All are callable from C.
 *   - Opaque handles are pointer-sized integers (ycxx_pal_handle).
 *   - Every function has hidden visibility (the attribute on each declaration, which a definition
 *     inherits): an image that links libycxx binds to its own PAL and exports none of it.
 */
#ifndef _YCXX_PAL_H
#define _YCXX_PAL_H

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

/* ---- memory (layer 'memory') --------------------------------------------------------------- */
/* Allocate `size` bytes aligned to `align` (a power of two). Returns null on failure. */
[[__gnu__::__visibility__("hidden")]] void* ycxx_pal_allocate(ycxx_pal_size size, ycxx_pal_size align) YCXX_PAL_NOEXCEPT;
/* Free memory from ycxx_pal_allocate. `size`/`align` are the values passed to allocate
 * (size may be 0 when unknown). Null is ignored. */
[[__gnu__::__visibility__("hidden")]] void ycxx_pal_deallocate(void* p, ycxx_pal_size size, ycxx_pal_size align) YCXX_PAL_NOEXCEPT;

/* ---- process (layer 'abort'; ycxx_pal_exit is not used by the library) ---------------------- */
/* Terminate abnormally after writing `__msg` (may be null) to the diagnostic stream. Must not
   return; must not throw or unwind. */
[[__gnu__::__visibility__("hidden")]] YCXX_PAL_NORETURN void ycxx_pal_abort(const char* __msg) YCXX_PAL_NOEXCEPT;
/* Normal termination with status. */
[[__gnu__::__visibility__("hidden")]] YCXX_PAL_NORETURN void ycxx_pal_exit(int status) YCXX_PAL_NOEXCEPT;

/* ---- I/O (layers 'abort': ycxx_pal_stderr; 'console': ycxx_pal_stdout, ycxx_pal_stdin) ------ */
enum { ycxx_pal_stdin = 0, ycxx_pal_stdout = 1, ycxx_pal_stderr = 2 };
/* Write up to n bytes to stream `__fd`. Stores bytes written in *written. A provider of the 'abort'
   layer alone handles ycxx_pal_stderr (the diagnostic stream: terminate's report, contract
   violations; it may discard the bytes and report them written); 'console' adds
   ycxx_pal_stdout (std::print without a C library). */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_write(ycxx_pal_handle __fd, const void* data, ycxx_pal_size n, ycxx_pal_size* __written) YCXX_PAL_NOEXCEPT;
/* Read up to n bytes from stream `__fd`. *got == 0 with return 0 means end of file. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_read(ycxx_pal_handle __fd, void* data, ycxx_pal_size n, ycxx_pal_size* __got) YCXX_PAL_NOEXCEPT;
/* Whether `__fd` refers to a terminal (used by std::print for unicode/vprint_unicode). */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_is_terminal(ycxx_pal_handle __fd) YCXX_PAL_NOEXCEPT;

/* ---- clocks (layer 'clock') ---------------------------------------------------------------- */
enum { ycxx_pal_clock_realtime = 0, ycxx_pal_clock_monotonic = 1 };
/* Current time as seconds + nanoseconds since the clock's epoch (realtime: Unix epoch). */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_clock_now(int clock, ycxx_pal_i64* __sec, ycxx_pal_i64* __nsec) YCXX_PAL_NOEXCEPT;

/* ---- randomness (layer 'random') ----------------------------------------------------------- */
/* Fill buffer with non-deterministic random bytes (std::random_device). */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_random(void* data, ycxx_pal_size n) YCXX_PAL_NOEXCEPT;
/* A random source chosen by name (std::random_device's token, `__len` bytes, not null-terminated).
 * POSIX: "default" and "getrandom" (the system generator: getrandom/getentropy), "/dev/urandom",
 * "/dev/random". Stores a handle in *h; returns EINVAL for an unknown token. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_random_open(const char* token, ycxx_pal_size __len, ycxx_pal_handle* h) YCXX_PAL_NOEXCEPT;
/* Fills data with n bytes from the source. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_random_read(ycxx_pal_handle h, void* data, ycxx_pal_size n) YCXX_PAL_NOEXCEPT;
/* Releases the source. */
[[__gnu__::__visibility__("hidden")]] void ycxx_pal_random_close(ycxx_pal_handle h) YCXX_PAL_NOEXCEPT;

/* ---- waiting on an address (layer 'threads'; the fallback without it: wait returns at once,
        the wakes do nothing) ------------------------------------------------------------- */
typedef __UINT32_TYPE__ ycxx_pal_u32;
/* Blocks while *addr == expected (may also return spuriously). */
[[__gnu__::__visibility__("hidden")]] void ycxx_pal_wait(const ycxx_pal_u32* __addr, ycxx_pal_u32 expected) YCXX_PAL_NOEXCEPT;
/* Wakes every thread blocked in ycxx_pal_wait on addr. */
[[__gnu__::__visibility__("hidden")]] void ycxx_pal_wake_all(const ycxx_pal_u32* __addr) YCXX_PAL_NOEXCEPT;
/* Wakes at least one thread blocked in ycxx_pal_wait on addr, if any (may wake more). */
[[__gnu__::__visibility__("hidden")]] void ycxx_pal_wake_one(const ycxx_pal_u32* __addr) YCXX_PAL_NOEXCEPT;
/* As ycxx_pal_wait, but returns no later than (about) the absolute time sec:nsec of `clock`
   (ycxx_pal_clock_realtime or ycxx_pal_clock_monotonic). Returns 0 when woken, when *addr !=
   expected, or spuriously, and a nonzero value when it returned because the time passed.
   Callers re-check both their condition and the clock. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_wait_until(const ycxx_pal_u32* __addr, ycxx_pal_u32 expected, int clock, ycxx_pal_i64 __sec,
                        ycxx_pal_i64 __nsec) YCXX_PAL_NOEXCEPT;

/* ---- threads (layer 'threads'; the fallback without it: thread_self 1, thread_yield nothing,
        ycxx_pal_single_threaded points to 0) -------------------------------------------- */
/* Starts a thread running start(arg); stack_size 0 means the default. Stores its handle in
   *thread. The handle is also the thread's identity (ycxx_pal_thread_self in that thread
   returns it) until the thread is joined or, if detached, ends. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_thread_create(ycxx_pal_handle* thread, void* (*start)(void*), void* arg,
                           ycxx_pal_size __stack_size) YCXX_PAL_NOEXCEPT;
/* Waits for the thread to end and releases its handle. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_thread_join(ycxx_pal_handle thread) YCXX_PAL_NOEXCEPT;
/* Releases the handle; the thread's resources are freed when it ends. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_thread_detach(ycxx_pal_handle thread) YCXX_PAL_NOEXCEPT;
/* The calling thread's handle. */
[[__gnu__::__visibility__("hidden")]] ycxx_pal_handle ycxx_pal_thread_self(void) YCXX_PAL_NOEXCEPT;
/* Names the calling thread (best effort: may be truncated or ignored). */
[[__gnu__::__visibility__("hidden")]] void ycxx_pal_thread_set_name(const char* name) YCXX_PAL_NOEXCEPT;
/* Offers the rest of the calling thread's time slice to other threads. */
[[__gnu__::__visibility__("hidden")]] void ycxx_pal_thread_yield(void) YCXX_PAL_NOEXCEPT;
/* The number of hardware threads available, or 0 if unknown. */
[[__gnu__::__visibility__("hidden")]] unsigned ycxx_pal_hardware_concurrency(void) YCXX_PAL_NOEXCEPT;
/* Blocks the calling thread until the absolute time sec:nsec of `clock` has passed. */
[[__gnu__::__visibility__("hidden")]] void ycxx_pal_sleep_until(int clock, ycxx_pal_i64 __sec, ycxx_pal_i64 __nsec) YCXX_PAL_NOEXCEPT;

/* Points to a flag that is nonzero only while the process certainly has a single thread: it is
   cleared before a second thread starts, and set again (if ever) only once the process is back to
   one thread, in a way that synchronizes with the other threads' ends. While it is set, the
   library updates the reference counts and uncontended locks of process-private objects with
   plain instead of atomic read-modify-write instructions. A port that cannot tell points it to
   a constant zero (the freestanding default). POSIX/glibc: glibc's __libc_single_threaded. */
[[__gnu__::__visibility__("hidden")]] extern const char* const ycxx_pal_single_threaded;

/* ---- thread exit (layer 'threads'; the fallback for ycxx_pal_thread_atexit without it:
        __cxa_atexit) ------------------------------------------------------------------ */
/* Registers f(obj) to run when the calling thread exits (thread_local destructors); dso is the
   registering object's __dso_handle. Returns 0 on success. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_thread_atexit(void (*__f)(void*), void* __obj, void* __dso) YCXX_PAL_NOEXCEPT;
/* Registers f(arg) to run when the calling thread ends, after its thread_local objects are
   destroyed (std::notify_all_at_thread_exit, promise::set_value_at_thread_exit). Not run for the
   thread that ends the process. Returns 0 on success. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_at_thread_end(void (*__f)(void*), void* arg) YCXX_PAL_NOEXCEPT;

/* ---- error messages (layer 'environment'; the fallback without it: "error N") ------------ */
/* Writes the C library's description of error number `__ev` (as strerror, but thread-safe) to buf
   as a null-terminated string, truncated to n bytes. Leaves errno unchanged. Used by
   std::generic_category() and std::system_category(). */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_error_message(int __ev, char* __buf, ycxx_pal_size n) YCXX_PAL_NOEXCEPT;

/* ---- debugging (layer 'debug'; the fallback without it: 0) ------------------------------ */
/* Nonzero if the process is being traced, presumably by a debugger (std::is_debugger_present).
   An immediate query: the answer is not cached. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_debugger_present(void) YCXX_PAL_NOEXCEPT;

/* ---- stack traces (layer 'debug'; <stacktrace> is not built without it) ------------------ */
/* The loaded object (the executable or a shared library) containing the address pc: its file
   name, written to path as a null-terminated string truncated to n bytes (a name the object
   file can be opened by), and its load bias (run-time address minus link-time address).
   Returns 0 if found. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_object_of(ycxx_pal_handle __pc, char* path, ycxx_pal_size n, ycxx_pal_handle* __bias) YCXX_PAL_NOEXCEPT;
/* The dynamic symbol containing pc (the name the dynamic linker knows, possibly mangled) and its
   start address. Returns 0 if found; *name stays valid while the object is loaded. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_dynamic_symbol(ycxx_pal_handle __pc, const char** name, ycxx_pal_handle* start) YCXX_PAL_NOEXCEPT;
/* Maps the file at path read-only into memory. Returns 0 on success. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_map_file(const char* path, const void** data, ycxx_pal_size* size) YCXX_PAL_NOEXCEPT;
/* Unmaps a file mapped by ycxx_pal_map_file. */
[[__gnu__::__visibility__("hidden")]] void ycxx_pal_unmap_file(const void* data, ycxx_pal_size size) YCXX_PAL_NOEXCEPT;

/* ---- character encoding (layer 'environment'; the fallback without it: "", unknown) ------- */
/* Writes the name of the environment's character encoding (POSIX: the codeset of the locale "")
   to buf as a null-terminated string, truncated to n bytes (std::text_encoding::environment). */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_environment_encoding(char* __buf, ycxx_pal_size n) YCXX_PAL_NOEXCEPT;

/* ---- files (layer 'files') ----------------------------------------------------------------- */
/* The file operations of std::basic_filebuf (the file streams) over the provider's storage, for
   YCXX_PAL=none builds that select the layer (with YCXX_PAL=posix, and with the C library
   without this layer, files are C stdio FILEs). Text and binary files are alike (no newline
   translation). */
enum {
  ycxx_pal_file_read_access = 1,  /* open for reading */
  ycxx_pal_file_write_access = 2, /* open for writing */
  ycxx_pal_file_append = 4,       /* every write goes to the end of the file */
  ycxx_pal_file_create = 8,       /* create the file if it does not exist */
  ycxx_pal_file_truncate = 16,    /* make an existing file empty */
  ycxx_pal_file_exclusive = 32    /* fail with EEXIST if the file exists (with create) */
};
/* Opens the file `name` with a combination of the flags above ([filebuf.members]'s table of
   modes, as fopen's "r", "w", "a", "r+", "w+", "a+" and "x"); stores a nonzero handle in *h. The
   position starts at 0. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_file_open(const char* name, int flags, ycxx_pal_handle* h) YCXX_PAL_NOEXCEPT;
/* Closes the file; the handle is released even on failure. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_file_close(ycxx_pal_handle h) YCXX_PAL_NOEXCEPT;
/* Reads up to n bytes at the position, which advances; *got == 0 with return 0 means end of
   file. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_file_read(ycxx_pal_handle h, void* data, ycxx_pal_size n, ycxx_pal_size* __got) YCXX_PAL_NOEXCEPT;
/* Writes up to n bytes at the position (at the end with ycxx_pal_file_append), which advances. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_file_write(ycxx_pal_handle h, const void* data, ycxx_pal_size n, ycxx_pal_size* __written) YCXX_PAL_NOEXCEPT;
/* Sets the position to off from the start (whence 0), the position (1) or the end (2) and stores
   the new position, from the start, in *pos. */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_file_seek(ycxx_pal_handle h, ycxx_pal_i64 __off, int __whence, ycxx_pal_i64* __pos) YCXX_PAL_NOEXCEPT;
/* Makes what was written durable as far as the provider's storage needs (may do nothing). */
[[__gnu__::__visibility__("hidden")]] int ycxx_pal_file_flush(ycxx_pal_handle h) YCXX_PAL_NOEXCEPT;

/* <filesystem> and the time zone database call POSIX directly (src/hosted/filesystem.cpp,
   tzdb.cpp; DECISIONS §8): they are built with YCXX_PAL=posix only. */

#ifdef __cplusplus
}
#endif

#endif /* _YCXX_PAL_H */
