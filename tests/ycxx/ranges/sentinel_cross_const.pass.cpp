// Comparing a view's iterator with the sentinel of the other constness: the sentinels' friend
// operator== and operator- are templates over OtherConst, constrained on
// sentinel_for / sized_sentinel_for<sentinel_t<Base>, iterator_t<maybe-const<OtherConst, V>>>:
// [range.transform.sentinel], [range.elements.sentinel], [range.zip.sentinel],
// [range.zip.transform.sentinel], [range.adjacent.sentinel],
// [range.adjacent.transform.sentinel], [range.join.sentinel], [range.join.with.sentinel],
// [range.enumerate.sentinel]. Each also converts sentinel<false> to sentinel<true>.
// The base view here has distinct iterator types for const and non-const access (T* and
// const T*) and one sentinel type that compares with and subtracts from both. (zip and
// zip_transform of random access sized views are common, [range.zip.view]: their end() is an
// iterator, and iterator<false> converts to iterator<true> for the comparisons.)
#include <ranges>
#include <cstddef>
#include <utility>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

template <class T>
struct end_mark {
  const T* e = nullptr;
  friend constexpr bool operator==(const T* p, end_mark s) { return p == s.e; }
  friend constexpr std::ptrdiff_t operator-(const T* p, end_mark s) { return p - s.e; }
  friend constexpr std::ptrdiff_t operator-(end_mark s, const T* p) { return s.e - p; }
};

template <class T>
struct two_way : rg::view_base {
  T* b = nullptr;
  T* e = nullptr;
  constexpr T* begin() { return b; }
  constexpr const T* begin() const { return b; }
  constexpr end_mark<T> end() const { return {e}; }
};
static_assert(rg::random_access_range<two_way<int>> && rg::random_access_range<const two_way<int>>);
static_assert(!std::is_same_v<rg::iterator_t<two_way<int>>, rg::iterator_t<const two_way<int>>>);
static_assert(rg::sized_range<two_way<int>> && !rg::common_range<two_way<int>>);

template <class R>
constexpr bool cross(R& v, std::ptrdiff_t n) {
  const R& c = v;
  auto it = v.begin();
  auto cit = c.begin();
  static_assert(!std::is_same_v<decltype(it), decltype(cit)>);
  static_assert(!std::is_same_v<decltype(v.end()), decltype(c.end())>);
  for (std::ptrdiff_t i = 0; i < n; ++i) {
    if (it == c.end() || cit == v.end()) return false;
    ++it, ++cit;
  }
  if (!(it == c.end()) || !(cit == v.end()) || !(it == v.end()) || !(cit == c.end())) return false;
  // the converting constructor of the sentinel
  decltype(c.end()) converted = v.end();
  return v.begin() != converted;
}

template <class R>
constexpr bool cross_sized(R& v, std::ptrdiff_t n) {
  const R& c = v;
  if (c.end() - v.begin() != n || v.begin() - c.end() != -n) return false;
  if (v.end() - c.begin() != n || c.begin() - v.end() != -n) return false;
  return cross(v, n);
}

constexpr bool run() {
  int a[4] = {1, 2, 3, 4};
  two_way<int> b{{}, a, a + 4};
  auto f = [](int x) { return x * 2; };

  auto t = b | vw::transform(f);
  if (!cross_sized(t, 4)) return false;
  auto z = vw::zip(b, b);
  if (!cross_sized(z, 4)) return false;
  auto zt = vw::zip_transform([](int x, int y) { return x + y; }, b, b);
  if (!cross_sized(zt, 4)) return false;
  auto ad = b | vw::adjacent<2>;
  if (!cross_sized(ad, 3)) return false;
  auto at = b | vw::pairwise_transform([](int x, int y) { return x * y; });
  if (!cross_sized(at, 3)) return false;
  auto en = b | vw::enumerate;
  if (!cross_sized(en, 4)) return false;

  std::pair<int, long> p[3] = {{1, 10}, {2, 20}, {3, 30}};
  two_way<std::pair<int, long>> bp{{}, p, p + 3};
  auto el = bp | vw::values;
  if (!cross_sized(el, 3)) return false;

  std::vector<int> inner[2] = {{1, 2}, {3}};
  two_way<std::vector<int>> bv{{}, inner, inner + 2};
  auto j = bv | vw::join;
  if (!cross(j, 3)) return false;
  auto jw = bv | vw::join_with(0);
  if (!cross(jw, 4)) return false;
  return true;
}

int main() {
  CHECK(run());
  static_assert(run());
  return 0;
}
