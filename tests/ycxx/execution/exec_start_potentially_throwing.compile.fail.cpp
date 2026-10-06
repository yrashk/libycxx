// [exec.opstate.start]/1: start(op) is expression-equivalent to MANDATE-NOTHROW(op.start()), and
// [exec.general]/5 (MANDATE-NOTHROW(expr): Mandates: noexcept(expr) is true): an operation state
// whose start member is potentially-throwing makes the call ill-formed.
// EXPECT-ERROR: noexcept|nothrow|not potentially-throwing
#include <execution>

namespace ex = std::execution;

struct op {
  using operation_state_concept = ex::operation_state_tag;
  void start() & {} // not noexcept
};

void f() {
  op o;
  ex::start(o);
}

int main() { return 0; }
