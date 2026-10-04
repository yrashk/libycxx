// [span.cons]/15-17: template<class R> constexpr explicit(extent != dynamic_extent)
// span(R&& r) is constrained on R modelling contiguous_range and sized_range, on
// either borrowed_range<R> or is_const_v<element_type>, and on the qualification conversion
// from remove_reference_t<ranges::range_reference_t<R>>(*)[] to element_type(*)[]; it views
// ranges::data(r) with ranges::size(r) elements. A vector<T> (a contiguous container,
// [container.reqmts]/68) therefore converts implicitly to span<T> from an lvalue, to
// span<const T> from an lvalue, const lvalue or rvalue, but not to span<T> from a const
// vector or an rvalue; vector<bool> is not contiguous and does not convert at all.
// [span.deduct]: span(R&&) deduces span<remove_reference_t<range_reference_t<R>>>.
#include <vector>
#include <span>
#include <type_traits>
#include "check.hpp"

using V = std::vector<int>;
static_assert(std::is_convertible_v<V&, std::span<int>>);
static_assert(std::is_convertible_v<V&, std::span<const int>>);
static_assert(std::is_convertible_v<const V&, std::span<const int>>);
static_assert(std::is_convertible_v<V&&, std::span<const int>>);
static_assert(!std::is_constructible_v<std::span<int>, const V&>);
static_assert(!std::is_constructible_v<std::span<int>, V&&>);
static_assert(!std::is_constructible_v<std::span<long>, V&>);
static_assert(!std::is_constructible_v<std::span<bool>, std::vector<bool>&>);
static_assert(!std::is_constructible_v<std::span<const bool>, const std::vector<bool>&>);
static_assert(!std::is_convertible_v<V&, std::span<int, 3>>);  // explicit for a static extent
static_assert(std::is_constructible_v<std::span<int, 3>, V&>);

constexpr int sum(std::span<const int> s) {
  int t = 0;
  for (int x : s) t += x;
  return t;
}

constexpr bool test() {
  V v{1, 2, 3, 4};
  std::span<int> s = v;
  if (s.data() != v.data() || s.size() != 4) return false;
  s[0] = 10;
  if (v[0] != 10) return false;
  if (sum(v) != 19 || sum(V{5, 5}) != 10) return false;
  const V& cv = v;
  std::span cs(cv);
  static_assert(std::is_same_v<decltype(cs), std::span<const int>>);
  std::span ms(v);
  static_assert(std::is_same_v<decltype(ms), std::span<int>>);
  if (cs.size() != 4 || ms.data() != v.data()) return false;
  std::span<int, 4> fixed(v);
  if (fixed.data() != v.data()) return false;
  V e;
  std::span<int> es = e;
  if (!es.empty()) return false;
  auto sub = std::span<int>(v).subspan(1, 2);
  sub[1] = 30;
  return v[2] == 30;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
