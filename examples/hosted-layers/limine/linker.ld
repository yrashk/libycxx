/* Example B's kernel image (examples/hosted-layers/limine): an x86_64 ELF executable that Limine
 * loads in the higher half, at 0xffffffff80000000 (the Limine boot protocol, "Memory Layout at
 * Entry"). Linked as a static, non-relocatable executable; its code is position independent
 * (-fPIE/-fPIC, small code model), which works at any address within 2 GiB of itself.
 *
 * Besides the usual sections the kernel needs, for libycxx:
 *   - .init_array and .fini_array: the static constructors and the destructors that are not
 *     registered with __cxa_atexit (libgcc_eh has one), run by boot.c;
 *   - .eh_frame and .eh_frame_hdr (-Wl,--eh-frame-hdr): the unwind tables of C++ exceptions; the
 *     unwinder finds .eh_frame_hdr through _dl_find_object (support.c);
 *   - .tdata/.tbss: the initial image of the thread-local storage (libycxx's runtime keeps the
 *     current exceptions there), which boot.c copies below the thread pointer (%fs). */
OUTPUT_FORMAT(elf64-x86-64)
ENTRY(kernel_entry)

PHDRS {
  limine_requests PT_LOAD;
  text            PT_LOAD;
  rodata          PT_LOAD;
  data            PT_LOAD;
  tls             PT_TLS;
  eh_frame_hdr    PT_GNU_EH_FRAME;
}

SECTIONS {
  . = 0xffffffff80000000;
  __kernel_start = .;

  /* The Limine requests, between their start and end markers (boot.c). */
  .limine_requests : {
    KEEP(*(.limine_requests_start))
    KEEP(*(.limine_requests))
    KEEP(*(.limine_requests_end))
  } :limine_requests

  . = ALIGN(CONSTANT(MAXPAGESIZE));
  .text : {
    *(.text.entry)
    *(.text .text.*)
  } :text

  . = ALIGN(CONSTANT(MAXPAGESIZE));
  .rodata : {
    *(.rodata .rodata.*)
  } :rodata
  .eh_frame_hdr : {
    __eh_frame_hdr_start = .;
    *(.eh_frame_hdr)
  } :rodata :eh_frame_hdr
  .eh_frame : {
    KEEP(*(.eh_frame))
  } :rodata
  .gcc_except_table : {
    *(.gcc_except_table .gcc_except_table.*)
  } :rodata

  . = ALIGN(CONSTANT(MAXPAGESIZE));
  .init_array : {
    __init_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
    KEEP(*(.init_array))
    __init_array_end = .;
  } :data
  .fini_array : {
    __fini_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
    KEEP(*(.fini_array))
    __fini_array_end = .;
  } :data
  .data.rel.ro : {
    *(.data.rel.ro .data.rel.ro.*)
  } :data
  .got : {
    *(.got) *(.igot)
  } :data
  .got.plt : {
    *(.got.plt) *(.igot.plt)
  } :data
  .data : {
    *(.data .data.*)
  } :data
  .tdata : {
    *(.tdata .tdata.*)
  } :data :tls
  .tbss : {
    *(.tbss .tbss.*) *(.tcommon)
  } :data :tls
  /* The TLS segment's layout for boot.c, as data (absolute symbols are out of reach of the
     position-independent code): its start, initialized size, size and alignment. */
  .tls_info : {
    . = ALIGN(8);
    __tls_info = .;
    QUAD(ADDR(.tdata))
    QUAD(SIZEOF(.tdata))
    QUAD(ADDR(.tbss) + SIZEOF(.tbss) - ADDR(.tdata))
    QUAD(MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)))
  } :data
  .bss : {
    *(.bss .bss.*) *(COMMON)
  } :data
  __kernel_end = .;

  /DISCARD/ : {
    *(.note .note.*) *(.comment) *(.interp) *(.dynamic) *(.dynsym) *(.dynstr) *(.gnu.hash) *(.hash)
  }
}
