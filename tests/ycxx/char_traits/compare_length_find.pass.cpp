// [tab:char.traits.req]:
//   compare(p,q,n): "0 if for each i in [0, n), X::eq(p[i],q[i]) is true; else, a negative
//     value if, for some j in [0, n), X::lt(p[j],q[j]) is true and for each i in [0, j)
//     X::eq(p[i],q[i]) is true; else a positive value."
//   length(p): "the smallest i such that X::eq(p[i],charT()) is true."
//   find(p,n,c): "the smallest q in [p, p+n) such that X::eq(*q,c) is true, nullptr otherwise."
// constexpr for all five specializations ([char.traits.specializations]); evaluated both
// during constant evaluation and at run time.
#include <string_view>
#include <cstddef>
#include "check.hpp"

template <class C>
constexpr bool test() {
  using T = std::char_traits<C>;
  const C abc[] = {C('a'), C('b'), C('c'), C(0)};
  const C abd[] = {C('a'), C('b'), C('d'), C(0)};
  const C ab0x[] = {C('a'), C('b'), C(0), C('x'), C(0)};
  const C empty[] = {C(0)};
  if (T::compare(abc, abc, 3) != 0 || T::compare(abc, abd, 2) != 0) return false;
  if (!(T::compare(abc, abd, 3) < 0) || !(T::compare(abd, abc, 3) > 0)) return false;
  if (T::compare(abc, abd, 0) != 0) return false;
  // embedded nulls are compared like any other character
  const C a0x[] = {C('a'), C(0), C('x')};
  const C a0y[] = {C('a'), C(0), C('y')};
  if (!(T::compare(a0x, a0y, 3) < 0)) return false;
  // the first mismatch decides, later characters do not
  const C bz[] = {C('b'), C('a')};
  const C ca[] = {C('c'), C('z')};
  if (!(T::compare(bz, ca, 2) < 0)) return false;

  if (T::length(abc) != 3 || T::length(empty) != 0 || T::length(ab0x) != 2) return false;

  if (T::find(abc, 3, C('b')) != abc + 1) return false;
  if (T::find(abc, 3, C('z')) != nullptr) return false;
  if (T::find(abc, 1, C('b')) != nullptr) return false;   // only [p, p+n) is searched
  if (T::find(abc, 0, C('a')) != nullptr) return false;
  if (T::find(abc, 4, C(0)) != abc + 3) return false;     // the null is an ordinary character
  const C aa[] = {C('a'), C('a')};
  if (T::find(aa, 2, C('a')) != aa) return false;          // the smallest such q
  return true;
}

static_assert(test<char>());
static_assert(test<wchar_t>());
static_assert(test<char8_t>());
static_assert(test<char16_t>());
static_assert(test<char32_t>());

int main() {
  CHECK(test<char>());
  CHECK(test<wchar_t>());
  CHECK(test<char8_t>());
  CHECK(test<char16_t>());
  CHECK(test<char32_t>());
  // run-time length of a long string
  static char big[1000];
  for (int i = 0; i < 999; ++i) big[i] = 'q';
  CHECK(std::char_traits<char>::length(big) == 999);
  return 0;
}
