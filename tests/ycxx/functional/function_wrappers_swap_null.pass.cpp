// [func.wrap.func.mod]/2, [func.wrap.move.util]/1, [func.wrap.copy.util]/1: swap "Exchanges the
// target objects of *this and other" (self-swap through the member or the friend leaves the target in place); the friend swap is
// "Equivalent to f1.swap(f2)". [func.wrap.func.nullptr], [func.wrap.move.util]/3,
// [func.wrap.copy.util]/3: operator==(f, nullptr) "Returns: true if f has no target object,
// otherwise false"; [over.match.oper] rewritten candidates give nullptr == f, f != nullptr and
// nullptr != f. Generic std::swap works through the (noexcept) move operations.
#include <functional>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

template <class W>
void exercise() {
  W a = []() noexcept { return 1; };
  W b = []() noexcept { return 2; };
  W e;
  a.swap(a);
  CHECK(a() == 1);
  swap(a, a);
  CHECK(a() == 1);
  std::swap(a, b);
  CHECK(a() == 2 && b() == 1);
  a.swap(e);
  CHECK(!a && e() == 2);
  CHECK(a == nullptr && nullptr == a && !(a != nullptr) && !(nullptr != a));
  CHECK(e != nullptr && nullptr != e && !(e == nullptr) && !(nullptr == e));
  swap(e, a);
  CHECK(a() == 2 && !e);
  e.swap(e);
  CHECK(!e);
  static_assert(noexcept(a == nullptr) && noexcept(nullptr != a));
  static_assert(std::is_same_v<decltype(nullptr != a), bool>);
  static_assert(std::is_nothrow_swappable_v<W>);
}

int main() {
  exercise<std::function<int()>>();
  exercise<std::move_only_function<int()>>();
  exercise<std::move_only_function<int() const noexcept>>();
  exercise<std::copyable_function<int()>>();
  exercise<std::copyable_function<int() const>>();
  return 0;
}
