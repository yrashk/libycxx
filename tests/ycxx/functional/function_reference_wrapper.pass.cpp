// [func.wrap.func.con]/13-14: the target has type FD; "Throws: Nothing if FD is a
// specialization of reference_wrapper or a function pointer type." /29: operator=(
// reference_wrapper<F> f) noexcept "Effects: As if by: function(f).swap(*this);".
// [func.wrap.func.inv]/1: INVOKE<R>(f, args...) where f is a reference_wrapper calls the
// referenced object.
#include <functional>
#include <utility>
#include "check.hpp"

struct Acc {
  int total = 0;
  int operator()(int x) { return total += x; }
};

int main() {
  Acc acc;
  std::function<int(int)> f = std::ref(acc);
  CHECK(f(2) == 2);
  CHECK(f(3) == 5);
  CHECK(acc.total == 5);  // the referenced object is used, not a copy
  std::function<int(int)> g = f;  // copies the reference_wrapper
  CHECK(g(1) == 6 && acc.total == 6);

  Acc other;
  g = std::ref(other);
  CHECK(g(4) == 4 && other.total == 4 && acc.total == 6);
  static_assert(noexcept(g = std::ref(other)));

  // a reference_wrapper to a const object calls the const overload
  struct Both {
    int operator()() { return 1; }
    int operator()() const { return 2; }
  };
  Both b;
  std::function<int()> h = std::cref(b);
  CHECK(h() == 2);
  h = std::ref(b);
  CHECK(h() == 1);
  return 0;
}
