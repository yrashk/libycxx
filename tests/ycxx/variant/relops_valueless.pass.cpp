// [variant.relops]: with a valueless_by_exception operand, == is "If v.index() != w.index(),
// false; otherwise if v.valueless_by_exception(), true"; != the converse; "<: If
// w.valueless_by_exception(), false; otherwise if v.valueless_by_exception(), true"; ">: If
// v.valueless_by_exception(), false; otherwise if w.valueless_by_exception(), true"; "<=: If
// v.valueless_by_exception(), true; otherwise if w.valueless_by_exception(), false"; ">=: If
// w.valueless_by_exception(), true; otherwise if v.valueless_by_exception(), false"; and <=>
// returns strong_ordering::equal for two valueless variants, less if only v is valueless,
// greater if only w is. The contained values' operators are not called in these cases.
// [variant.status]: a valueless variant's index() is variant_npos.
// REQUIRES: exceptions
#include <variant>
#include <compare>
#include "check.hpp"

static int calls = 0;
struct T {
  int v;
  T(int x) : v(x) {}
  T(int, bool) { throw 0; }
  T(const T& o) : v(o.v) {}  // not trivially copyable, and a potentially-throwing move:
  ~T() {}                     // emplace must destroy the old value before constructing
  friend bool operator==(const T& a, const T& b) { ++calls; return a.v == b.v; }
  friend bool operator!=(const T& a, const T& b) { ++calls; return a.v != b.v; }
  friend bool operator<(const T& a, const T& b) { ++calls; return a.v < b.v; }
  friend bool operator>(const T& a, const T& b) { ++calls; return a.v > b.v; }
  friend bool operator<=(const T& a, const T& b) { ++calls; return a.v <= b.v; }
  friend bool operator>=(const T& a, const T& b) { ++calls; return a.v >= b.v; }
  friend std::strong_ordering operator<=>(const T& a, const T& b) { ++calls; return a.v <=> b.v; }
};
using V = std::variant<T, int>;

V valueless() {
  V v(std::in_place_index<0>, 1);
  try {
    v.emplace<0>(1, true);
  } catch (int) {
  }
  return v;
}

int main() {
  V n1 = valueless();
  V n2 = valueless();
  CHECK(n1.valueless_by_exception() && n2.valueless_by_exception());
  CHECK(n1.index() == std::variant_npos);
  V a(std::in_place_index<0>, 5);  // index 0
  V b(std::in_place_index<1>, 5);  // index 1

  calls = 0;
  // both valueless
  CHECK(n1 == n2);
  CHECK(!(n1 != n2));
  CHECK(!(n1 < n2));
  CHECK(!(n1 > n2));
  CHECK(n1 <= n2);
  CHECK(n1 >= n2);
  CHECK((n1 <=> n2) == std::strong_ordering::equal);

  // one valueless: it orders before every non-valueless variant
  for (const V* x : {&a, &b}) {
    CHECK(!(n1 == *x) && !(*x == n1));
    CHECK(n1 != *x && *x != n1);
    CHECK(n1 < *x && !(*x < n1));
    CHECK(!(n1 > *x) && *x > n1);
    CHECK(n1 <= *x && !(*x <= n1));
    CHECK(!(n1 >= *x) && *x >= n1);
    CHECK((n1 <=> *x) == std::strong_ordering::less);
    CHECK((*x <=> n1) == std::strong_ordering::greater);
  }
  CHECK(calls == 0);  // no element comparison was needed

  // control: element comparisons do happen otherwise
  V a2(std::in_place_index<0>, 6);
  CHECK(a < a2 && calls == 1);
  return 0;
}
