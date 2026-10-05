// [propagation]/2: "exception_ptr meets the requirements of Cpp17NullablePointer"; that adds
// Cpp17EqualityComparable, Cpp17DefaultConstructible, Cpp17CopyConstructible,
// Cpp17CopyAssignable, Cpp17Swappable and Cpp17Destructible, construction and assignment from
// nullptr, contextual conversion to bool, and == / != against nullptr in either order
// ([nullablepointer.requirements], Table 36). /3: "Two non-null values of type exception_ptr
// are equivalent and compare equal if and only if they refer to the same exception." /4: the
// default constructor produces the null value.
// REQUIRES: exceptions
#include <exception>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

using EP = std::exception_ptr;
static_assert(std::equality_comparable<EP>);
static_assert(std::equality_comparable_with<EP, std::nullptr_t>);
static_assert(std::swappable<EP>);
static_assert(std::is_swappable_v<EP>);
static_assert(std::copyable<EP>);
static_assert(std::default_initializable<EP>);
static_assert(std::is_nothrow_destructible_v<EP>);
static_assert(std::is_same_v<decltype(EP() != EP()), bool>);
static_assert(std::is_same_v<decltype(nullptr == EP()), bool>);
static_assert(std::is_same_v<decltype(nullptr != EP()), bool>);

int which(const EP& p) {
  try {
    std::rethrow_exception(p);
  } catch (int i) {
    return i;
  }
  return -1;
}

int main() {
  EP a = std::make_exception_ptr(1);
  EP b = std::make_exception_ptr(2);
  EP n1 = nullptr;
  EP n2{nullptr};
  EP n3{};
  CHECK(n1 == n2 && n2 == n3 && n1 == EP());
  CHECK(!(n1 != n3));
  CHECK(a != nullptr && nullptr != a && !(a == nullptr) && !(nullptr == a));
  CHECK(a != b && b != a);
  CHECK(a != n1 && n1 != a);
  CHECK(a == a);

  // contextual conversion
  CHECK(a ? true : false);
  CHECK(!n1 ? true : false);
  CHECK(a && !n1);
  if (n1) CHECK(false);

  // swap (std::swap and the member-less ADL path)
  EP a0 = a, b0 = b;
  std::swap(a, b);
  CHECK(a == b0 && b == a0);
  CHECK(which(a) == 2 && which(b) == 1);
  using std::swap;
  swap(a, n1);
  CHECK(a == nullptr && n1 == b0);
  swap(a, n1);
  CHECK(a == b0 && n1 == nullptr);

  // copy and move assignment
  EP c;
  c = a;
  CHECK(c == a && which(c) == 2);
  c = c;  // self-assignment keeps the value
  CHECK(c == a);
  EP d;
  d = std::move(c);
  CHECK(d == a && which(d) == 2);
  c = b;  // a moved-from exception_ptr is assignable
  CHECK(c == b);
  d = nullptr;
  CHECK(d == nullptr && !d);
  d = n3;
  CHECK(d == nullptr);

  // copies of copies keep referring to the same exception
  EP e1 = a;
  EP e2 = e1;
  EP e3(std::move(e2));
  CHECK(e3 == a && e1 == a);
  return 0;
}
