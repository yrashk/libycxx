// libycxx runtime: std::breakpoint and std::breakpoint_if_debugging ([debugging.utility]), in
// libycxx.a and the freestanding runtime archive. std::is_debugger_present is replaceable, so
// it is in an archive member of its own (is_debugger_present.cpp); the call below binds to a
// program's replacement when there is one.
#include <debugging>

// The processor's breakpoint instruction: with a debugger attached, execution stops and can
// be resumed after it; otherwise the trap ends the program (SIGTRAP on POSIX systems).
void std::breakpoint() noexcept {
  using ycxx::detail::cfg::cpu_family;
  if constexpr (ycxx::detail::cfg::cpu == cpu_family::x86)
    asm volatile("int3");
  else if constexpr (ycxx::detail::cfg::cpu == cpu_family::aarch64)
    asm volatile("brk #0xf000");
  else if constexpr (ycxx::detail::cfg::cpu == cpu_family::arm)
    asm volatile("bkpt #0");
  else if constexpr (ycxx::detail::cfg::cpu == cpu_family::riscv)
    asm volatile("ebreak");
  else
    __builtin_trap();
}

void std::breakpoint_if_debugging() noexcept {
  if (std::is_debugger_present())
    std::breakpoint();
}
