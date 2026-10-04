// [utility.syn]: [[noreturn]] void unreachable(); [utility.undefined]/1: "Preconditions: false
// is true." -- calling it is undefined, so it may only appear on paths that are never taken.
// /2 Example 1: int f(int x) { switch (x) { case 0: case 1: return x; default:
// std::unreachable(); } }  int a = f(1); // OK, a has value 1
#include <utility>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::unreachable()), void>);

int f(int x) {
  switch (x) {
    case 0:
    case 1:
      return x;
    default:
      std::unreachable();
  }
}

// usable in a constexpr function on a path not taken during constant evaluation
constexpr int g(int x) {
  if (x >= 0) return x;
  std::unreachable();
}
static_assert(g(3) == 3);

[[noreturn]] void forward_noreturn() { std::unreachable(); }  // no warning: unreachable is noreturn
int h(bool b) {
  if (b) return 1;
  forward_noreturn();
}

int main() {
  volatile int one = 1;
  CHECK(f(one) == 1);
  CHECK(f(0) == 0);
  CHECK(g(one) == 1);
  CHECK(h(one == 1) == 1);
  return 0;
}
