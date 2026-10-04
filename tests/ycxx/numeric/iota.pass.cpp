// [numeric.iota]: iota(first, last, value) assigns *i = value and then ++value for each i
// in order (exactly last - first increments and assignments). ranges::iota (both
// iterator/sentinel and range overloads) is equivalent to the loop "*first =
// as_const(value); ++first; ++value;" and returns iota_result<O, T> = {last, value} (an
// out_value_result); the range overload returns borrowed_iterator_t<R>.
#include <numeric>
#include <ranges>
#include <type_traits>
#include "test_iterators.hpp"
#include "check.hpp"

struct Counter {
  int v;
  int* incs;
  constexpr Counter& operator++() {
    ++v;
    ++*incs;
    return *this;
  }
  constexpr operator int() const { return v; }
};

constexpr bool test() {
  int a[5] = {};
  std::iota(a, a + 5, 3);
  if (a[0] != 3 || a[4] != 7) return false;
  std::iota(a, a, 100);  // empty: nothing assigned
  if (a[0] != 3) return false;
  double d[3];
  std::iota(d, d + 3, 0.5);
  if (d[2] != 2.5) return false;
  char c[3];
  std::iota(c, c + 3, 'x');
  if (c[0] != 'x' || c[2] != 'z') return false;
  int incs = 0;
  std::iota(a, a + 4, Counter{10, &incs});
  if (incs != 4 || a[3] != 13 || a[4] != 7) return false;
  std::iota(ForwardIter<int>(a), ForwardIter<int>(a + 2), -1);
  if (a[0] != -1 || a[1] != 0) return false;

  int b[4] = {};
  auto r = std::ranges::iota(b, b + 4, 1);
  static_assert(std::is_same_v<decltype(r), std::ranges::iota_result<int*, int>>);
  static_assert(std::is_same_v<std::ranges::iota_result<int*, int>, std::ranges::out_value_result<int*, int>>);
  if (r.out != b + 4 || r.value != 5 || b[0] != 1 || b[3] != 4) return false;
  auto r2 = std::ranges::iota(b, 10L);
  static_assert(std::is_same_v<decltype(r2), std::ranges::iota_result<int*, long>>);
  if (r2.out != b + 4 || r2.value != 14 || b[3] != 13) return false;
  ForwardRange<int> fr{b, b + 3};
  auto r3 = std::ranges::iota(fr, 0);
  if (r3.out.p != b + 3 || r3.value != 3 || b[2] != 2 || b[3] != 13) return false;
  int* p[3];
  int arr[3];
  std::ranges::iota(p, arr);  // pointers are weakly_incrementable
  if (p[0] != arr || p[2] != arr + 2) return false;
  return true;
}
static_assert(test());

struct Owning {
  int v[2] = {};
  constexpr int* begin() { return v; }
  constexpr int* end() { return v + 2; }
};
static_assert(std::is_same_v<decltype(std::ranges::iota(Owning{}, 0).out), std::ranges::dangling>);

int main() {
  CHECK(test());
  return 0;
}
