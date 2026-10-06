// [range.as.input.overview]/1-2: views::as_input(E) is views::all(E) when decltype((E)) is an
// input range that is neither common nor forward, otherwise as_input_view(E): an input-only,
// non-common view. [range.as.input.view]/1-6: base(), begin() (iterator<false> / <true>), end()
// is the base's sentinel, size() and the deduction guide. [range.as.input.iterator]/1-11:
// iterator_concept is input_iterator_tag, difference_type and value_type are the base's, the
// iterator is move-only, operator++(int) returns void, base() & and &&, conversion from
// iterator<false> to iterator<true>, == with the sentinel, - with a sized sentinel (both
// orders), iter_move and iter_swap of the underlying iterator.
#include <ranges>
#include <concepts>
#include <iterator>
#include <sstream>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

using V = rg::ref_view<std::vector<int>>;
using AI = rg::as_input_view<V>;
using It = rg::iterator_t<AI>;
using CIt = rg::iterator_t<const AI>;

static_assert(rg::view<AI> && rg::input_range<AI> && !rg::forward_range<AI> && !rg::common_range<AI>);
static_assert(rg::sized_range<AI> && rg::sized_range<const AI>);
static_assert(std::is_same_v<std::iter_value_t<It>, int> && std::is_same_v<std::iter_difference_t<It>, std::ptrdiff_t>);
static_assert(std::is_same_v<typename It::iterator_concept, std::input_iterator_tag>);
static_assert(std::is_same_v<std::iter_reference_t<It>, int&> && std::is_same_v<std::iter_rvalue_reference_t<It>, int&&>);
static_assert(!std::is_copy_constructible_v<It> && std::is_move_constructible_v<It> && !std::is_copy_assignable_v<It>);
static_assert(std::is_same_v<decltype(std::declval<It&>()++), void>);
static_assert(std::is_same_v<rg::sentinel_t<AI>, std::vector<int>::iterator>);
// ref_view<vector> is a simple view: begin() of the non-const view is the const iterator.
static_assert(std::is_same_v<It, CIt>);
using OV = rg::as_input_view<rg::owning_view<std::vector<int>>>;
static_assert(!std::is_same_v<rg::iterator_t<OV>, rg::iterator_t<const OV>>);
static_assert(std::is_convertible_v<rg::iterator_t<OV>, rg::iterator_t<const OV>>);
static_assert(!std::is_convertible_v<rg::iterator_t<const OV>, rg::iterator_t<OV>>);
static_assert(std::sized_sentinel_for<rg::sentinel_t<AI>, It>);
static_assert(noexcept(std::declval<const It&>().base()));
static_assert(std::is_same_v<decltype(std::declval<It&&>().base()), std::vector<int>::iterator>);
static_assert(std::is_same_v<decltype(std::declval<const It&>().base()), const std::vector<int>::iterator&>);

// /2: what views::as_input produces.
static_assert(std::is_same_v<decltype(std::declval<std::vector<int>&>() | vw::as_input), AI>);
static_assert(std::is_same_v<decltype(vw::iota(0) | vw::as_input), rg::as_input_view<rg::iota_view<int>>>);
using ISV = rg::basic_istream_view<int, char>;
static_assert(rg::input_range<ISV> && !rg::common_range<ISV> && !rg::forward_range<ISV>);
// (basic_istream_view is copyable, so views::all(E) is a copy of it.)
static_assert(std::is_same_v<decltype(vw::as_input(std::declval<ISV&>())), ISV>);
static_assert(std::is_same_v<decltype(vw::as_input(std::declval<ISV>())), ISV>);
static_assert(std::is_same_v<decltype(vw::as_input(std::declval<ISV&>())), decltype(vw::all(std::declval<ISV&>()))>);
// The deduction guide.
static_assert(std::is_same_v<decltype(rg::as_input_view(std::declval<std::vector<int>&>())), AI>);

constexpr bool run() {
  std::vector<int> v{1, 2, 3, 4};
  AI a(v);
  if (a.size() != 4 || &a.base().base() != &v) return false;
  It it = a.begin();
  auto end = a.end();
  if (end - it != 4 || it - end != -4 || it == end) return false;
  if (*it != 1) return false;
  ++it;
  it++;
  if (*it != 3 || end - it != 2) return false;
  if (it.base() != v.begin() + 2) return false;
  // iter_move and iter_swap act on the underlying iterator.
  It other = a.begin();
  rg::iter_swap(it, other);
  if (v[0] != 3 || v[2] != 1) return false;
  int moved = rg::iter_move(other);
  if (moved != 3) return false;
  auto under = std::move(it).base();
  if (under != v.begin() + 2) return false;
  // Conversion from iterator<false> to iterator<true> (/2).
  OV ov(std::vector<int>{7, 8});
  rg::iterator_t<const OV> c = ov.begin();
  if (*c != 7 || c == std::as_const(ov).end()) return false;
  ++c;
  if (c == std::as_const(ov).end() || *c != 8 || ov.size() != 2) return false;
  // Iteration through the const view.
  const AI& ca = a;
  int sum = 0;
  for (auto i = ca.begin(); i != ca.end(); ++i) sum += *i;
  if (sum != 10) return false;
  // Algorithms over the input-only view.
  if (rg::count(v | vw::as_input, 4) != 1) return false;
  auto t = vw::iota(1) | vw::as_input | vw::take(3);
  int s = 0;
  for (int x : t) s += x;
  return s == 6;
}
static_assert(run());

int main() {
  CHECK(run());
  std::istringstream in("1 2 3");
  ISV isv(in);
  int total = 0;
  for (int x : vw::as_input(isv)) total += x;
  CHECK(total == 6);
  return 0;
}
