// [mdspan.extents]: extents<IndexType, Extents...>: index_type, size_type
// (make_unsigned_t<index_type>), rank_type (size_t); rank(), rank_dynamic(), static_extent(r),
// extent(r); construction from all extents or only the dynamic ones, from span/array (explicit
// unless N == rank_dynamic()), from another extents (explicit if a static extent is made
// from a dynamic one or the index type narrows); operator== compares ranks and extents across
// index types; the deduction guide gives extents<size_t, maybe-static-ext<Integrals>...>;
// dextents<I, R> and dims<R, I = size_t> are all-dynamic. Each specialization is regular and
// trivially copyable.
// COUNTERPART: libcxx:containers/views/mdspan/extents/ctor_from_integral.pass.cpp
#include <mdspan>
#include <array>
#include <concepts>
#include <span>
#include <type_traits>
#include <utility>
#include "check.hpp"

using std::dynamic_extent;
using E = std::extents<int, 3, dynamic_extent, 5, dynamic_extent>;
static_assert(std::is_same_v<E::index_type, int>);
static_assert(std::is_same_v<E::size_type, unsigned>);
static_assert(std::is_same_v<E::rank_type, std::size_t>);
static_assert(E::rank() == 4 && E::rank_dynamic() == 2);
static_assert(E::static_extent(0) == 3 && E::static_extent(1) == dynamic_extent && E::static_extent(2) == 5);
static_assert(noexcept(E::rank()) && noexcept(E::static_extent(0)) && noexcept(std::declval<E>().extent(0)));
static_assert(std::regular<E> && std::is_trivially_copyable_v<E>);
static_assert(std::extents<short>::rank() == 0 && std::extents<short>::rank_dynamic() == 0);

static_assert(std::is_same_v<std::dextents<long, 3>, std::extents<long, dynamic_extent, dynamic_extent, dynamic_extent>>);
static_assert(std::is_same_v<std::dims<2>, std::extents<std::size_t, dynamic_extent, dynamic_extent>>);
static_assert(std::is_same_v<std::dims<1, unsigned char>, std::extents<unsigned char, dynamic_extent>>);

// Deduction guide.
static_assert(std::is_same_v<decltype(std::extents(2, 3u)), std::extents<std::size_t, dynamic_extent, dynamic_extent>>);
static_assert(std::is_same_v<decltype(std::extents(std::integral_constant<int, 4>{}, 2)),
                             std::extents<std::size_t, 4, dynamic_extent>>);
static_assert(std::is_same_v<decltype(std::extents(std::cw<7>, std::cw<2>)), std::extents<std::size_t, 7, 2>>);

// Constructor explicitness and constraints.
static_assert(std::is_nothrow_constructible_v<E, int, int>);
static_assert(std::is_nothrow_constructible_v<E, int, int, int, int>);
static_assert(!std::is_constructible_v<E, int, int, int>);
static_assert(!std::is_convertible_v<int, std::extents<int, dynamic_extent>>);  // explicit
static_assert(std::is_convertible_v<std::array<int, 2>, E>);                      // N == rank_dynamic()
static_assert(!std::is_convertible_v<std::array<int, 4>, E> && std::is_constructible_v<E, std::array<int, 4>>);
static_assert(std::is_convertible_v<std::span<int, 2>, E>);
static_assert(!std::is_constructible_v<E, std::span<int, 3>>);
static_assert(!std::is_constructible_v<E, std::span<int>>);
static_assert(std::is_convertible_v<std::extents<int, 3, 4, 5, 6>, E>);            // static -> dynamic
static_assert(!std::is_convertible_v<std::dextents<int, 4>, E> && std::is_constructible_v<E, std::dextents<int, 4>>);
static_assert(!std::is_constructible_v<E, std::extents<int, 4, 4, 5, 6>>);          // 3 vs 4
static_assert(!std::is_constructible_v<E, std::extents<int, 3, 4, 5>>);             // rank
static_assert(std::is_convertible_v<std::extents<short, 3, 1, 5, 1>, E>);
static_assert(!std::is_convertible_v<std::extents<long long, 3, 1, 5, 1>, E>);     // narrowing index type
static_assert(std::is_constructible_v<E, std::extents<long long, 3, 1, 5, 1>>);
static_assert(std::is_nothrow_default_constructible_v<E>);

constexpr bool run() {
  E a(7, 9);
  if (a.extent(0) != 3 || a.extent(1) != 7 || a.extent(2) != 5 || a.extent(3) != 9) return false;
  E b(3, 7, 5, 9);
  if (!(a == b)) return false;
  E d;  // dynamic extents value-initialized
  if (d.extent(1) != 0 || d.extent(3) != 0 || d.extent(0) != 3) return false;
  std::array<int, 2> dyn{1, 2};
  E c(dyn);
  if (c.extent(1) != 1 || c.extent(3) != 2) return false;
  std::array<long, 4> all{3, 4, 5, 6};
  E f(all);
  if (f.extent(1) != 4 || f.extent(3) != 6) return false;
  int raw[2] = {8, 9};
  E g{std::span<int, 2>(raw)};
  if (g.extent(1) != 8 || g.extent(3) != 9) return false;
  std::extents<long, 3, 7, 5, 9> s;
  if (!(a == s) || a != s) return false;
  if (a == std::extents<int, 3, 7, 5>()) return false;  // different ranks compare unequal
  std::dextents<unsigned, 4> conv(a);
  if (conv.extent(0) != 3 || conv.extent(3) != 9 || !(conv == a)) return false;
  E back(conv);
  if (!(back == a)) return false;
  if (!(std::extents<int>() == std::extents<long>())) return false;
  return true;
}
static_assert(run());

int main() {
  CHECK(run());
  return 0;
}
