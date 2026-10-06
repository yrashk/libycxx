// [exec.set.value]/1: set_value(rcvr, vs...) is expression-equivalent to
// MANDATE-NOTHROW(rcvr.set_value(vs...)), and [exec.general]/5 (MANDATE-NOTHROW(expr): Mandates:
// noexcept(expr) is true): a receiver whose set_value member is potentially-throwing makes the
// call ill-formed.
// EXPECT-ERROR: noexcept|nothrow|not potentially-throwing
#include <execution>
#include <utility>

namespace ex = std::execution;

struct rcvr {
  using receiver_concept = ex::receiver_tag;
  void set_value(int) && {} // not noexcept
};

void f() {
  rcvr r;
  ex::set_value(std::move(r), 1);
}

int main() { return 0; }
