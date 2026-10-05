// [propagation]: exception_ptr meets Cpp17NullablePointer; the default constructor produces
// the null value; "Two non-null values of type exception_ptr are equivalent and compare equal
// if and only if they refer to the same exception."; "shall not be implicitly convertible to
// any arithmetic, enumeration, or pointer type". current_exception() returns null when no
// exception is handled; make_exception_ptr(e) refers to a copy of e; rethrow_exception throws
// the referenced exception (or a copy).
// REQUIRES: exceptions
#include <exception>
#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "check.hpp"

using EP = std::exception_ptr;
static_assert(std::is_nothrow_default_constructible_v<EP>);
static_assert(std::is_nothrow_copy_constructible_v<EP>);
static_assert(std::is_nothrow_copy_assignable_v<EP>);
static_assert(std::is_nothrow_move_constructible_v<EP>);
static_assert(std::is_constructible_v<EP, std::nullptr_t>);
static_assert(std::is_convertible_v<std::nullptr_t, EP>);
static_assert(std::is_assignable_v<EP&, std::nullptr_t>);
static_assert(std::is_constructible_v<bool, EP>);  // contextual conversion
static_assert(!std::is_convertible_v<EP, bool>);
static_assert(!std::is_convertible_v<EP, int>);
static_assert(!std::is_convertible_v<EP, void*>);
static_assert(std::is_same_v<decltype(EP() == EP()), bool>);
static_assert(std::is_same_v<decltype(EP() == nullptr), bool>);
static_assert(noexcept(std::current_exception()));
static_assert(noexcept(std::make_exception_ptr(1)));
static_assert(std::is_same_v<decltype(std::make_exception_ptr(1)), EP>);

struct Counted {
  static inline int copies = 0;
  int v;
  explicit Counted(int x) : v(x) {}
  Counted(const Counted& o) : v(o.v) { ++copies; }
};

int main() {
  EP null;
  CHECK(!null);
  CHECK(null == nullptr && nullptr == null);
  CHECK(null == EP());
  CHECK(std::current_exception() == nullptr);

  EP p = std::make_exception_ptr(std::runtime_error("boom"));
  CHECK(p != nullptr);
  CHECK(static_cast<bool>(p));
  EP q = p;
  CHECK(q == p);  // same exception
  EP r = std::make_exception_ptr(std::runtime_error("boom"));
  CHECK(r != p);  // a different exception object

  bool caught = false;
  try {
    std::rethrow_exception(p);
  } catch (const std::runtime_error& e) {
    caught = e.what()[0] == 'b';
  }
  CHECK(caught);

  EP captured;
  try {
    throw Counted(5);
  } catch (...) {
    captured = std::current_exception();
    CHECK(captured != nullptr);
  }
  CHECK(std::current_exception() == nullptr);
  int value = 0;
  try {
    std::rethrow_exception(captured);
  } catch (const Counted& c) {
    value = c.v;
  }
  CHECK(value == 5);

  q = nullptr;
  CHECK(!q);
  EP m = std::move(p);
  CHECK(m != nullptr);
  return 0;
}
