// [mdspan.mdspan]: member types; rank, rank_dynamic, static_extent, extent; constructors from
// a data handle and extents (all or only dynamic), span/array, extents, mapping, mapping and
// accessor; operator[] with indices, span or array (multidimensional subscript); size(),
// empty(), swap, extents(), data_handle(), mapping(), accessor(), stride(r) and the
// is_[always_]unique/exhaustive/strided queries forwarded to the mapping. Conversion from an
// mdspan with convertible element type, extents, layout and accessor.
#include <mdspan>
#include <array>
#include <span>
#include <type_traits>
#include <utility>
#include "check.hpp"

using std::dynamic_extent;
using E = std::extents<int, 2, dynamic_extent>;
using M = std::mdspan<double, E>;

static_assert(std::is_same_v<M::extents_type, E>);
static_assert(std::is_same_v<M::layout_type, std::layout_right>);
static_assert(std::is_same_v<M::accessor_type, std::default_accessor<double>>);
static_assert(std::is_same_v<M::mapping_type, std::layout_right::mapping<E>>);
static_assert(std::is_same_v<M::element_type, double> && std::is_same_v<M::value_type, double>);
static_assert(std::is_same_v<std::mdspan<const int, E>::value_type, int>);
static_assert(std::is_same_v<M::index_type, int> && std::is_same_v<M::size_type, unsigned>);
static_assert(std::is_same_v<M::rank_type, std::size_t>);
static_assert(std::is_same_v<M::data_handle_type, double*> && std::is_same_v<M::reference, double&>);
static_assert(M::rank() == 2 && M::rank_dynamic() == 1 && M::static_extent(0) == 2 &&
              M::static_extent(1) == dynamic_extent);
static_assert(std::is_trivially_copyable_v<M> && std::copyable<M>);
static_assert(std::is_nothrow_move_constructible_v<M> && std::is_nothrow_swappable_v<M>);
// Conversions between mdspans.
static_assert(std::is_convertible_v<M, std::mdspan<const double, E>>);
static_assert(!std::is_constructible_v<std::mdspan<double, E>, std::mdspan<const double, E>>);
static_assert(std::is_convertible_v<std::mdspan<double, std::extents<int, 2, 3>>, M>);
static_assert(!std::is_convertible_v<std::mdspan<double, std::dextents<int, 2>>, M>);
static_assert(std::is_constructible_v<M, std::mdspan<double, std::dextents<int, 2>>>);
static_assert(!std::is_constructible_v<M, std::mdspan<double, std::extents<int, 3, 3>>>);
// Explicitness of the handle + extents constructors.
static_assert(!std::is_convertible_v<double*, M>);
static_assert(std::is_constructible_v<M, double*, int>);
static_assert(std::is_constructible_v<M, double*, int, int>);
static_assert(!std::is_constructible_v<M, double*, int, int, int>);

constexpr bool run() {
  double data[6] = {0, 1, 2, 3, 4, 5};
  M a(data, 3);  // 2 x 3, row-major
  if (a.extent(0) != 2 || a.extent(1) != 3 || a.size() != 6 || a.empty()) return false;
  if (a[1, 2] != 5 || a[0, 1] != 1) return false;
  a[1, 0] = 30;
  if (data[3] != 30) return false;
  if (a[std::array<int, 2>{1, 1}] != 4) return false;
  int idx[2] = {0, 2};
  if (a[std::span<int, 2>(idx)] != 2) return false;
  if (a.data_handle() != data || a.stride(0) != 3 || a.stride(1) != 1) return false;
  if (!a.is_unique() || !a.is_exhaustive() || !a.is_strided()) return false;
  if (!M::is_always_unique() || !M::is_always_exhaustive() || !M::is_always_strided()) return false;
  if (!(a.extents() == E(3)) || !(a.mapping() == std::layout_right::mapping<E>(E(3)))) return false;

  M b(data, 2, 3);  // all extents
  M c(data, std::array<int, 1>{3});
  M d(data, E(3));
  M e(data, std::layout_right::mapping<E>(E(3)));
  M f(data, std::layout_right::mapping<E>(E(3)), std::default_accessor<double>());
  if (&b[1, 1] != &a[1, 1] || &c[1, 1] != &a[1, 1] || &d[0, 2] != &data[2] || &e[1, 2] != &data[5] || &f[0, 0] != data)
    return false;

  M g;  // default: null handle, dynamic extents 0
  if (g.data_handle() != nullptr || g.extent(1) != 0 || !g.empty() || g.size() != 0) return false;
  swap(g, a);
  if (g.data_handle() != data || g.extent(1) != 3 || a.data_handle() != nullptr) return false;

  std::mdspan<const double, E> cv = g;  // conversion to const elements
  if (cv[1, 2] != 5) return false;

  // layout_left element order.
  std::mdspan<double, E, std::layout_left> l(data, 3);
  if (&l[1, 0] != &data[1] || &l[0, 1] != &data[2]) return false;
  // rank 0
  double one = 7;
  std::mdspan<double, std::extents<int>> s(&one);
  if (s[] != 7 || s.size() != 1 || s.empty()) return false;
  return true;
}
static_assert(run());

int main() {
  CHECK(run());
  return 0;
}
