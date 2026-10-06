// Example A: the hosted-layers demonstration as a program of the host OS, with libycxx built
// without its POSIX platform layer (YCXX_PAL=none): the program's own providers.c supplies the
// hosted layers abort, memory, console and clock, and nothing else exists (no threads, files,
// random device or C library layer). Compiled freestanding, as every program of such a build is.
#include "../common/demo.hpp"
#include "../common/heap.h"

#include <print>

// extern "C": compiled freestanding, Clang gives main no special linkage (it is an ordinary
// function there, [basic.start.main]/1), and the host's C runtime calls the C symbol `main`.
extern "C" int main() {
  const int failed = hosted_layers_demo("the host OS");
  const heap_stats s = heap_get_stats();
  std::println("heap (providers.c's arena): {} bytes, peak in use {}, {} allocations, {} blocks still allocated",
               s.size, s.peak, s.allocations, s.blocks);
  return failed == 0 ? 0 : 1;
}
