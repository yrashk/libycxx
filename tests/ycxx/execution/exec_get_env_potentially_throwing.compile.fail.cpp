// [exec.get.env]/1.1: get_env(o) is expression-equivalent to MANDATE-NOTHROW(AS-CONST(o).get_env())
// when that is well-formed, and [exec.general]/5 (MANDATE-NOTHROW(expr): Mandates: noexcept(expr)
// is true): a get_env member that is potentially-throwing makes the call ill-formed (rather than
// falling back to env<>{}, /1.2).
// EXPECT-ERROR: noexcept|nothrow|not potentially-throwing
#include <execution>

namespace ex = std::execution;

struct has_env {
  ex::env<> get_env() const { return {}; } // not noexcept
};

void f() {
  has_env h;
  (void)ex::get_env(h);
}

int main() { return 0; }
