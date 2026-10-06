/* Example B's kernel: Limine's requests, the entry point and the start-up of a C++ program on
 * bare x86_64 (examples/hosted-layers/README.md). Limine enters kernel_entry in 64-bit mode with
 * paging on, interrupts off and a 64 KiB stack (the Limine boot protocol, "Machine State at
 * Entry"); kernel_boot then
 *   1. sets up COM1 and the thread-local storage (libycxx's ABI runtime keeps each thread's
 *      exceptions in thread_local objects: the thread pointer %fs must point at a TLS block);
 *   2. takes the largest usable region of the memory map as the heap (the 'memory' layer);
 *   3. calibrates the clocks (the 'clock' layer);
 *   4. runs the static constructors, then kernel_main (main.cpp), then the destructors
 *      registered with __cxa_atexit (support.c) and those of .fini_array;
 *   5. ends QEMU through isa-debug-exit with kernel_main's verdict. */
#include "../common/heap.h"
#include "hw.h"
#include "limine_protocol.h"

/* ---- Limine requests, between the delimiters (linker.ld keeps their sections first) ---- */
__attribute__((used, section(".limine_requests_start"))) static volatile uint64_t requests_start[4] =
    LIMINE_REQUESTS_START_MARKER;
/* Base revision 6, the current one; an older Limine leaves the third word alone (checked below). */
__attribute__((used, section(".limine_requests"))) static volatile uint64_t base_revision[3] =
    LIMINE_BASE_REVISION_TAG(6);
__attribute__((used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID, .revision = 0, .response = 0};
__attribute__((used, section(".limine_requests"))) static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID, .revision = 0, .response = 0};
__attribute__((used, section(".limine_requests_end"))) static volatile uint64_t requests_end[2] =
    LIMINE_REQUESTS_END_MARKER;

/* ---- the entry point ----
   A stack of our own (256 KiB, in .bss), and SSE: Limine leaves CR4.OSFXSR and CR4.OSXMMEXCPT
   clear (base revision 5 and later clear every CR0/CR4 bit it does not name), and the C and C++
   code is compiled for x86-64 with SSE2 (the System V ABI passes doubles in SSE registers). So:
   CR0.EM off, CR0.MP on, CR4.OSFXSR and CR4.OSXMMEXCPT on, and the x87 unit initialized. */
__asm__(".section .bss\n"
        ".balign 16\n"
        "boot_stack:\n"
        ".skip 262144\n"
        "boot_stack_top:\n"
        ".section .text.entry, \"ax\", @progbits\n"
        ".globl kernel_entry\n"
        ".type kernel_entry, @function\n"
        "kernel_entry:\n"
        "  cli\n"
        "  leaq boot_stack_top(%rip), %rsp\n"
        "  movq %cr0, %rax\n"
        "  andq $~(1 << 2), %rax\n"
        "  orq $(1 << 1), %rax\n"
        "  movq %rax, %cr0\n"
        "  movq %cr4, %rax\n"
        "  orq $(3 << 9), %rax\n"
        "  movq %rax, %cr4\n"
        "  fninit\n"
        "  xorl %ebp, %ebp\n"
        "  call kernel_boot\n"
        "1: cli\n"
        "  hlt\n"
        "  jmp 1b\n"
        ".size kernel_entry, . - kernel_entry\n"
        ".text\n");

/* ---- thread-local storage (x86_64 ELF TLS, variant II) ----
   The TLS block of the executable ends at the thread pointer, %fs's base, aligned to the TLS
   segment's alignment; the word at the thread pointer points to itself. The linker computes
   each thread_local's offset from it the same way (offset - aligned size of the segment). */
extern const uint64_t __tls_info[4]; /* start, initialized size, size, alignment (linker.ld) */
static _Alignas(64) unsigned char tls_area[16384];

static void tls_init(void) {
  const unsigned char* image = (const unsigned char*)__tls_info[0];
  const uint64_t init = __tls_info[1], size = __tls_info[2];
  const uint64_t align = __tls_info[3] < 8 ? 8 : __tls_info[3];
  const uint64_t aligned = (size + align - 1) & ~(align - 1);
  if (align > 64 || aligned + 8 > sizeof tls_area) {
    serial_puts("kernel: the TLS segment does not fit tls_area\n");
    qemu_exit(qemu_exit_failure);
  }
  unsigned char* block = tls_area;
  for (uint64_t i = 0; i < init; ++i)
    block[i] = image[i];
  for (uint64_t i = init; i < aligned; ++i)
    block[i] = 0;
  uint64_t* tp = (uint64_t*)(block + aligned);
  *tp = (uint64_t)tp;
  wrmsr(0xc0000100, (uint64_t)tp); /* IA32_FS_BASE */
}

/* ---- the heap: the largest usable region, through the higher-half direct map ---- */
static void heap_from_memory_map(void) {
  const struct limine_hhdm_response* hhdm = hhdm_request.response;
  const struct limine_memmap_response* map = memmap_request.response;
  if (hhdm == 0 || map == 0) {
    serial_puts("kernel: Limine gave no memory map or HHDM\n");
    qemu_exit(qemu_exit_failure);
  }
  uint64_t best_base = 0, best_length = 0;
  for (uint64_t i = 0; i < map->entry_count; ++i) {
    const struct limine_memmap_entry* e = map->entries[i];
    if (e->type == LIMINE_MEMMAP_USABLE && e->length > best_length) {
      best_base = e->base;
      best_length = e->length;
    }
  }
  if (best_length > (64u << 20)) /* plenty for the demonstration */
    best_length = 64u << 20;
  heap_init((void*)(best_base + hhdm->offset), best_length);
}

/* ---- start-up and shutdown ---- */
typedef void (*init_function)(void);
extern init_function __init_array_start[], __init_array_end[], __fini_array_start[], __fini_array_end[];
void run_atexit_functions(void); /* support.c */
int kernel_main(void);           /* main.cpp */

static void put_decimal(uint64_t v) {
  char buf[24];
  int n = 0;
  do {
    buf[n++] = (char)('0' + v % 10);
    v /= 10;
  } while (v != 0);
  while (n != 0)
    serial_write(&buf[--n], 1);
}

void kernel_boot(void) {
  serial_init();
  tls_init();
  serial_puts("kernel: booted by Limine, base revision ");
  if (base_revision[2] == 0) {
    put_decimal(6);
  } else {
    put_decimal(base_revision[1] != 0x6a7b384944536bdc ? base_revision[1] : 0);
    serial_puts(" (6 not supported by this Limine)");
  }
  serial_puts("\n");
  heap_from_memory_map();
  clocks_init();
  serial_puts("kernel: heap of ");
  put_decimal(heap_get_stats().size >> 20);
  serial_puts(" MiB from the memory map; TSC at ");
  put_decimal(clock_tsc_hz() / 1000000);
  serial_puts(" MHz\n");
  for (init_function* f = __init_array_start; f != __init_array_end; ++f)
    (*f)();
  const int failed = kernel_main();
  run_atexit_functions();
  for (init_function* f = __fini_array_end; f != __fini_array_start;)
    (*--f)();
  serial_puts(failed == 0 ? "kernel: done, exiting QEMU (success)\n" : "kernel: done, exiting QEMU (failure)\n");
  qemu_exit(failed == 0 ? qemu_exit_success : qemu_exit_failure);
}
