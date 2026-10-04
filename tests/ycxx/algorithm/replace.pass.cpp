// [alg.replace]: replace / replace_if substitute new_value for every element equal to
// old_value / satisfying pred; replace_copy / replace_copy_if copy into result, writing
// new_value for those elements, and return result + N. ranges::replace / replace_if return
// last; ranges::replace_copy(_if) return {last, result + N}. Projections apply to the test
// only. C++26 (P2248): replace's value parameters default to the projected value type, so
// braced initializers work.
#include <algorithm>
#include <ranges>
#include <type_traits>
#include "check.hpp"

struct Pt {
  int x, y;
  constexpr bool operator==(const Pt&) const = default;
};

constexpr bool test() {
  int a[] = {1, 2, 1, 3, 1};
  std::replace(a, a + 5, 1, 9);
  if (a[0] != 9 || a[2] != 9 || a[4] != 9 || a[1] != 2) return false;

  std::replace_if(a, a + 5, [](int v) { return v > 2; }, 0);
  if (a[0] != 0 || a[1] != 2 || a[3] != 0) return false;

  int src[] = {5, 6, 5, 7};
  int out[4] = {};
  int* r = std::replace_copy(src, src + 4, out, 5, 50);
  if (r != out + 4 || out[0] != 50 || out[1] != 6 || out[2] != 50 || src[0] != 5) return false;
  r = std::replace_copy_if(src, src + 4, out, [](int v) { return v == 6; }, 60);
  if (r != out + 4 || out[1] != 60 || out[0] != 5) return false;

  // ranges forms
  int b[] = {1, 2, 3, 2};
  int* rl = std::ranges::replace(b, 2, 20);
  if (rl != b + 4 || b[1] != 20 || b[3] != 20) return false;
  rl = std::ranges::replace_if(b, b + 4, [](int v) { return v == 20; }, 7);
  if (rl != b + 4 || b[1] != 7) return false;

  Pt ps[] = {{1, 1}, {2, 2}, {1, 3}};
  std::ranges::replace(ps, 1, Pt{0, 0}, &Pt::x);
  if (!(ps[0] == Pt{0, 0}) || !(ps[2] == Pt{0, 0}) || !(ps[1] == Pt{2, 2})) return false;

  // braced value (C++26 default template argument for T1/T2)
  Pt qs[] = {{1, 1}, {2, 2}};
  std::ranges::replace(qs, {2, 2}, {9, 9});
  if (!(qs[1] == Pt{9, 9})) return false;
  std::replace(qs, qs + 2, {9, 9}, {3, 3});
  if (!(qs[1] == Pt{3, 3})) return false;

  int c[] = {1, 2, 3};
  int o2[3] = {};
  auto rc = std::ranges::replace_copy(c, o2, 2, 0);
  static_assert(std::is_same_v<decltype(rc), std::ranges::replace_copy_result<int*, int*>>);
  if (rc.in != c + 3 || rc.out != o2 + 3 || o2[1] != 0 || c[1] != 2) return false;
  auto rci = std::ranges::replace_copy_if(c, o2, [](int v) { return v != 2; }, -1);
  static_assert(std::is_same_v<decltype(rci), std::ranges::replace_copy_if_result<int*, int*>>);
  if (rci.out != o2 + 3 || o2[0] != -1 || o2[1] != 2 || o2[2] != -1) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
