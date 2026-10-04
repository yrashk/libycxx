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

/* ---- waiting on an address ---------------------------------------------------------------- */
typedef __UINT32_TYPE__ ycxx_pal_u32;
/* Blocks while *addr == expected (may also return spuriously). */
void ycxx_pal_wait(const ycxx_pal_u32* addr, ycxx_pal_u32 expected) YCXX_PAL_NOEXCEPT;
/* Wakes every thread blocked in ycxx_pal_wait on addr. */
void ycxx_pal_wake_all(const ycxx_pal_u32* addr) YCXX_PAL_NOEXCEPT;
/* Wakes at least one thread blocked in ycxx_pal_wait on addr, if any (may wake more). */
void ycxx_pal_wake_one(const ycxx_pal_u32* addr) YCXX_PAL_NOEXCEPT;
/* As ycxx_pal_wait, but returns no later than (about) the absolute time sec:nsec of `clock`
   (ycxx_pal_clock_realtime or ycxx_pal_clock_monotonic). Returns 0 when woken, when *addr !=
   expected, or spuriously, and a nonzero value when it returned because the time passed.
   Callers re-check both their condition and the clock. */
int ycxx_pal_wait_until(const ycxx_pal_u32* addr, ycxx_pal_u32 expected, int clock, ycxx_pal_i64 sec,
                        ycxx_pal_i64 nsec) YCXX_PAL_NOEXCEPT;

/* ---- threads ------------------------------------------------------------------------------- */
/* Starts a thread running start(arg); stack_size 0 means the default. Stores its handle in
   *thread. The handle is also the thread's identity (ycxx_pal_thread_self in that thread
   returns it) until the thread is joined or, if detached, ends. */
int ycxx_pal_thread_create(ycxx_pal_handle* thread, void* (*start)(void*), void* arg,
                           ycxx_pal_size stack_size) YCXX_PAL_NOEXCEPT;
/* Waits for the thread to end and releases its handle. */
int ycxx_pal_thread_join(ycxx_pal_handle thread) YCXX_PAL_NOEXCEPT;
/* Releases the handle; the thread's resources are freed when it ends. */
int ycxx_pal_thread_detach(ycxx_pal_handle thread) YCXX_PAL_NOEXCEPT;
/* The calling thread's handle. */
ycxx_pal_handle ycxx_pal_thread_self(void) YCXX_PAL_NOEXCEPT;
/* Names the calling thread (best effort: may be truncated or ignored). */
void ycxx_pal_thread_set_name(const char* name) YCXX_PAL_NOEXCEPT;
/* Offers the rest of the calling thread's time slice to other threads. */
void ycxx_pal_thread_yield(void) YCXX_PAL_NOEXCEPT;
/* The number of hardware threads available, or 0 if unknown. */
unsigned ycxx_pal_hardware_concurrency(void) YCXX_PAL_NOEXCEPT;
/* Blocks the calling thread until the absolute time sec:nsec of `clock` has passed. */
void ycxx_pal_sleep_until(int clock, ycxx_pal_i64 sec, ycxx_pal_i64 nsec) YCXX_PAL_NOEXCEPT;

/* ---- thread exit --------------------------------------------------------------------------- */
/* Registers f(obj) to run when the calling thread exits (thread_local destructors); dso is the
   registering object's __dso_handle. Returns 0 on success. */
int ycxx_pal_thread_atexit(void (*f)(void*), void* obj, void* dso) YCXX_PAL_NOEXCEPT;
/* Registers f(arg) to run when the calling thread ends, after its thread_local objects are
   destroyed (std::notify_all_at_thread_exit, promise::set_value_at_thread_exit). Not run for the
   thread that ends the process. Returns 0 on success. */
int ycxx_pal_at_thread_end(void (*f)(void*), void* arg) YCXX_PAL_NOEXCEPT;

/* ---- error messages ----------------------------------------------------------------------- */
/* Writes the C library's description of error number `ev` (as strerror, but thread-safe) to buf
   as a null-terminated string, truncated to n bytes. Leaves errno unchanged. Used by
   std::generic_category() and std::system_category(). */
int ycxx_pal_error_message(int ev, char* buf, ycxx_pal_size n) YCXX_PAL_NOEXCEPT;

/* ---- debugging ---------------------------------------------------------------------------- */
/* Nonzero if the process is being traced, presumably by a debugger (std::is_debugger_present).
   An immediate query: the answer is not cached. */
int ycxx_pal_debugger_present(void) YCXX_PAL_NOEXCEPT;

/* ---- stack traces ------------------------------------------------------------------------- */
/* The loaded object (the executable or a shared library) containing the address pc: its file
   name, written to path as a null-terminated string truncated to n bytes (a name the object
   file can be opened by), and its load bias (run-time address minus link-time address).
   Returns 0 if found. */
int ycxx_pal_object_of(ycxx_pal_handle pc, char* path, ycxx_pal_size n, ycxx_pal_handle* bias) YCXX_PAL_NOEXCEPT;
/* The dynamic symbol containing pc (the name the dynamic linker knows, possibly mangled) and its
   start address. Returns 0 if found; *name stays valid while the object is loaded. */
int ycxx_pal_dynamic_symbol(ycxx_pal_handle pc, const char** name, ycxx_pal_handle* start) YCXX_PAL_NOEXCEPT;
/* Maps the file at path read-only into memory. Returns 0 on success. */
int ycxx_pal_map_file(const char* path, const void** data, ycxx_pal_size* size) YCXX_PAL_NOEXCEPT;
/* Unmaps a file mapped by ycxx_pal_map_file. */
void ycxx_pal_unmap_file(const void* data, ycxx_pal_size size) YCXX_PAL_NOEXCEPT;

/* ---- character encoding ------------------------------------------------------------------- */
/* Writes the name of the environment's character encoding (POSIX: the codeset of the locale "")
   to buf as a null-terminated string, truncated to n bytes (std::text_encoding::environment). */
int ycxx_pal_environment_encoding(char* buf, ycxx_pal_size n) YCXX_PAL_NOEXCEPT;

/* Filesystem and time zone hooks are added with phase 5. */

#ifdef __cplusplus
}
#endif

#endif /* YCXX_PAL_H */
