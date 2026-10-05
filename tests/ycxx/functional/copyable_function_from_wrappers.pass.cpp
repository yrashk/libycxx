// [func.wrap.copy.ctor]/10: "Postconditions: *this has no target object if ... remove_cvref_t<F>
// is a specialization of the copyable_function class template, and f has no target object.
// Otherwise, *this has a target object of type VT direct-non-list-initialized with
// std::forward<F>(f)." So an empty std::function becomes a target (calling it throws
// bad_function_call); a non-empty copyable_function of another signature keeps its state, and
// copies of the result copy that state. /3: the copy constructor copies the target.
// REQUIRES: exceptions
// COUNTERPART: libstdcxx:20_util/copyable_function/conv.cc
#include <functional>
#include <utility>
#include "check.hpp"

struct Counter {
  mutable int n = 0;
  int operator()() const { return ++n; }
};

int main() {
  std::copyable_function<int()> e = std::function<int()>();
  CHECK(static_cast<bool>(e));
  bool threw = false;
  try {
    e();
  } catch (const std::bad_function_call&) {
    threw = true;
  }
  CHECK(threw);
  std::copyable_function<int()> ecopy = e;  // still a target after copying
  CHECK(ecopy != nullptr);

  // another signature, rvalue source: the state moves along
  std::copyable_function<int() const> src = Counter{};
  src();
  std::copyable_function<long()> dst = std::move(src);
  CHECK(dst() == 2L);
  // copies of the converted wrapper are independent
  std::copyable_function<long()> dcopy = dst;
  CHECK(dst() == 3L && dst() == 4L);
  CHECK(dcopy() == 3L);
  // lvalue source: the source keeps its own state
  std::copyable_function<int() const> s2 = Counter{};
  std::copyable_function<int()> d2 = s2;
  CHECK(d2() == 1 && d2() == 2 && s2() == 1);

  // a reference_wrapper target: all copies refer to the same object
  Counter c;
  std::copyable_function<int()> r = std::cref(c);
  std::copyable_function<int()> rcopy = r;
  r();
  rcopy();
  CHECK(c.n == 2);
  std::copyable_function<int() const> ri(std::in_place_type<std::reference_wrapper<Counter>>, c);
  CHECK(ri() == 3);
  return 0;
}
