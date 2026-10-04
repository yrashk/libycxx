// Empty sources when constructing one polymorphic wrapper from another:
// [func.wrap.move.ctor]/8: move_only_function(F&&) has no target if "remove_cvref_t<F> is a
// specialization of the move_only_function or copyable_function class template, and f has no
// target object" -- any specialization, whatever its signature or qualifiers. Otherwise the
// target is a VT direct-non-list-initialized with std::forward<F>(f).
// [func.wrap.copy.ctor]/10: copyable_function likewise, for specializations of
// copyable_function. [func.wrap.func.con]/12: function(F&&) is empty only if F is "a
// specialization of the function class template, and !f is true". So an empty std::function
// becomes a target of a move_only_function / copyable_function (calling it throws
// bad_function_call, [func.wrap.func.inv]/2), and an empty copyable_function becomes a target
// of a std::function (which then is not empty).
#include <functional>
#include "check.hpp"

template <class F>
bool throws_bad_call(F& f) {
  try {
    f(1);
  } catch (const std::bad_function_call&) {
    return true;
  }
  return false;
}

int main() {
  // move_only_function from empty move_only_function / copyable_function of other signatures
  {
    std::move_only_function<int(int)> a{std::copyable_function<int(int)>{}};
    CHECK(!a);
    std::move_only_function<int(int)> b{std::copyable_function<int(int) const>{}};
    CHECK(!b);
    std::move_only_function<long(int)> c{std::copyable_function<int(long)>{}};
    CHECK(!c);
    std::move_only_function<int(int)> d{std::move_only_function<int(int) const>{}};
    CHECK(!d);
    std::move_only_function<int(int)&&> e{std::move_only_function<int(int)>{}};
    CHECK(!e);
    std::move_only_function<int(int)> g{std::move_only_function<int(int) noexcept>{}};
    CHECK(!g);
  }
  // copyable_function from empty copyable_function of other signatures
  {
    std::copyable_function<int(int)> a{std::copyable_function<int(int) const>{}};
    CHECK(!a);
    std::copyable_function<long(int)> b{std::copyable_function<int(long)>{}};
    CHECK(!b);
    std::copyable_function<int(int)> c{std::copyable_function<int(int) noexcept>{}};
    CHECK(!c);
  }
  // function from empty function of another signature
  {
    std::function<long(int)> a{std::function<int(long)>{}};
    CHECK(!a);
  }
  // an empty std::function is not special for the other wrappers
  {
    std::move_only_function<int(int)> a{std::function<int(int)>{}};
    CHECK(static_cast<bool>(a));
    CHECK(throws_bad_call(a));
    std::copyable_function<int(int)> b{std::function<int(int)>{}};
    CHECK(static_cast<bool>(b));
    CHECK(throws_bad_call(b));
    std::copyable_function<int(int)> b2 = b;  // the copy holds a copy of the empty function
    CHECK(static_cast<bool>(b2));
    CHECK(throws_bad_call(b2));
  }
  // an empty copyable_function / move_only_function is not special for std::function
  {
    std::function<int(int)> a{std::copyable_function<int(int)>{}};
    CHECK(static_cast<bool>(a));  // has a target (calling it would violate its precondition)
  }
  // non-empty sources of other signatures are wrapped and called
  {
    std::move_only_function<long(int)> a{std::copyable_function<int(long)>{[](long x) { return int(x) + 1; }}};
    CHECK(a && a(4) == 5);
    std::copyable_function<long(int)> b{std::copyable_function<int(long) const>{[](long x) { return int(x) * 2; }}};
    CHECK(b && b(4) == 8);
  }
  return 0;
}
