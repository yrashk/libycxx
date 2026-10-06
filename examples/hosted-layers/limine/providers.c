/* The hosted layers this kernel provides (examples/hosted-layers/README.md): the primitives of
 * ycxx/pal.h for abort, memory, console and clock, on bare x86_64 hardware. There is no operating
 * system and no C library: memory is a region of the Limine memory map (heap_init in boot.c),
 * the console and the diagnostic stream are COM1, the clocks are the TSC and the CMOS clock
 * (hw.c), and abort ends QEMU through its isa-debug-exit device. Freestanding C. */
#include <ycxx/pal.h>

#include "../common/heap.h"
#include "hw.h"

/* Error numbers (ycxx/pal.h: errno-style codes; the Linux values, which are libycxx's on bare
   metal, DECISIONS §3). */
enum { error_bad_file = 9, error_invalid = 22 };

/* ---- memory ---- */
void* ycxx_pal_allocate(ycxx_pal_size size, ycxx_pal_size align) { return heap_allocate(size, align); }

void ycxx_pal_deallocate(void* p, ycxx_pal_size size, ycxx_pal_size align) {
  (void)size;
  (void)align;
  heap_free(p);
}

/* ---- console (standard output and input) and abort's diagnostic stream: both are COM1 ---- */
int ycxx_pal_write(ycxx_pal_handle fd, const void* data, ycxx_pal_size n, ycxx_pal_size* written) {
  *written = 0;
  if (fd != ycxx_pal_stdout && fd != ycxx_pal_stderr)
    return error_bad_file;
  serial_write((const char*)data, n);
  *written = n;
  return 0;
}

int ycxx_pal_read(ycxx_pal_handle fd, void* data, ycxx_pal_size n, ycxx_pal_size* got) {
  *got = 0;
  if (fd != ycxx_pal_stdin)
    return error_bad_file;
  if (n == 0)
    return 0;
  serial_read((char*)data);
  *got = 1;
  return 0;
}

int ycxx_pal_is_terminal(ycxx_pal_handle fd) { return fd <= ycxx_pal_stderr; }

/* ---- abort: report on COM1, then end QEMU with a failure status ---- */
_Noreturn void ycxx_pal_abort(const char* msg) {
  serial_puts("hosted-layers (bare metal): abort: ");
  serial_puts(msg != 0 ? msg : "(no message)");
  serial_puts("\n");
  qemu_exit(qemu_exit_failure);
}

/* ---- clock ---- */
int ycxx_pal_clock_now(int clock, ycxx_pal_i64* sec, ycxx_pal_i64* nsec) {
  int64_t ns;
  if (clock == ycxx_pal_clock_monotonic)
    ns = clock_monotonic_ns();
  else if (clock == ycxx_pal_clock_realtime)
    ns = clock_realtime_ns();
  else
    return error_invalid;
  *sec = ns / 1000000000;
  *nsec = ns % 1000000000;
  return 0;
}
