// [debugging.utility]/5: is_debugger_present "is replaceable ([dcl.fct.def.replace])", and /2:
// breakpoint_if_debugging() is "Equivalent to: if (is_debugger_present()) breakpoint();", so a
// program-provided replacement is called by the library (and, returning false, prevents the
// breakpoint).
#include <debugging>
#include "check.hpp"

static int calls = 0;
bool std::is_debugger_present() noexcept {
  ++calls;
  return false;
}

int main() {
  CHECK(!std::is_debugger_present() && calls == 1);
  std::breakpoint_if_debugging();
  CHECK(calls == 2);
  return 0;
}
