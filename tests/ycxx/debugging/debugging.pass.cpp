// [debugging.syn]: void breakpoint() noexcept; void breakpoint_if_debugging() noexcept;
// bool is_debugger_present() noexcept. [debugging.utility]/2: breakpoint_if_debugging() is
// equivalent to if (is_debugger_present()) breakpoint(); /3: is_debugger_present has no
// preconditions. The test is not run under a debugger, so neither call halts.
#include <debugging>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::is_debugger_present()), bool>);
static_assert(std::is_same_v<decltype(std::breakpoint()), void>);
static_assert(std::is_same_v<decltype(std::breakpoint_if_debugging()), void>);
static_assert(noexcept(std::is_debugger_present()) && noexcept(std::breakpoint()) && noexcept(std::breakpoint_if_debugging()));

int main() {
  bool present = std::is_debugger_present();
  if (!present) std::breakpoint_if_debugging();  // must return without breaking
  CHECK(std::is_debugger_present() == present);
  return 0;
}
