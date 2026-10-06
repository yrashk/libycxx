/* The x86_64 hardware this kernel touches: port I/O, the time-stamp counter, model-specific
 * registers, and the functions of hw.c (COM1, the clocks, QEMU's isa-debug-exit device). */
#ifndef HOSTED_LAYERS_HW_H
#define HOSTED_LAYERS_HW_H

#include <stddef.h>
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t v) { __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(port)); }
static inline uint8_t inb(uint16_t port) {
  uint8_t v;
  __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
  return v;
}
static inline void outl(uint16_t port, uint32_t v) { __asm__ volatile("outl %0, %1" : : "a"(v), "Nd"(port)); }
static inline uint64_t rdtsc(void) {
  uint32_t lo, hi;
  __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)hi << 32) | lo;
}
static inline void wrmsr(uint32_t msr, uint64_t v) {
  __asm__ volatile("wrmsr" : : "c"(msr), "a"((uint32_t)v), "d"((uint32_t)(v >> 32)));
}

/* COM1 at 115200 baud, 8N1, polled (QEMU: -serial stdio). */
void serial_init(void);
void serial_write(const char* s, size_t n); /* "\n" goes out as "\r\n" */
void serial_puts(const char* s);
int serial_read(char* c);                    /* waits for one byte */

/* The clocks: the TSC, calibrated against the PIT, for the monotonic clock; the CMOS real-time
   clock, read once, for the realtime clock. */
void clocks_init(void);
uint64_t clock_tsc_hz(void);
int64_t clock_monotonic_ns(void);
int64_t clock_realtime_ns(void);

/* QEMU's isa-debug-exit device (-device isa-debug-exit,iobase=0xf4,iosize=0x04): QEMU exits with
   status (code << 1) | 1. Halts if the device is not there. */
_Noreturn void qemu_exit(uint32_t code);
enum { qemu_exit_success = 0x10, qemu_exit_failure = 0x11 }; /* QEMU's status: 33, 35 */

#endif
