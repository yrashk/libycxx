// Example B: the hosted-layers demonstration as a bare-metal x86_64 kernel booted by Limine, with
// libycxx built with YCXX_PAL=none and the layers abort, memory, console and clock, which
// providers.c supplies over COM1, the Limine memory map, the TSC and the CMOS clock. boot.c calls
// kernel_main after the static constructors.
#include "../common/demo.hpp"
#include "../common/heap.h"

#include <print>

extern "C" int kernel_main() {
  const int failed = hosted_layers_demo("bare-metal x86_64, booted by Limine");
  const heap_stats s = heap_get_stats();
  std::println("heap (a region of the Limine memory map): {} bytes, peak in use {}, {} allocations, {} blocks "
               "still allocated",
               s.size, s.peak, s.allocations, s.blocks);
  return failed;
}
