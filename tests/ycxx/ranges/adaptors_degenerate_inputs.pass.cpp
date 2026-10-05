// Range factories and adaptors on empty inputs and with degenerate arguments.
//   Every adaptor of an empty range is empty: begin() == end(), size() == 0 where sized, and no
//   element is accessed (the element type here counts accesses through a projection-free
//   transform, and an input-only empty range is used as well).
//   [range.zip.overview]/2: views::zip() is "auto(views::empty<tuple<>>)".
//   [range.cartesian.overview]/2: views::cartesian_product() is
//     "views::single(tuple())" -- one element, the empty tuple.
//   [range.adjacent.overview]/2: views::adjacent<0>(E) is "((void)E, auto(views::empty<tuple<>>))";
//     [range.adjacent.transform.overview]/2: adjacent_transform<0>(E, F) is
//     "((void)E, views::zip_transform(F))", which is empty_view<decay_t<invoke_result_t<F&>>>.
//   [range.zip.transform.overview]/2: zip_transform(F) with no ranges is
//     "((void)F, auto(views::empty<decay_t<invoke_result_t<FD&>>>))".
//   [range.concat.overview]/2: concat(E) with one range is views::all(E); concat of empty ranges
//     is empty.
//   [range.take.overview], [range.drop.overview]: take(0) is empty, drop(n) with n >= size is
//     empty; [range.slide.view]: a slide_view whose base has fewer than n elements is empty
//     (size: "auto sz = ranges::distance(base_) - n + 1; if (sz < 0) sz = 0;");
//   [range.chunk.view.fwd]: chunk(n) of k (0 < k <= n) elements is one chunk of k elements.
//   [range.split.view]/[range.split.iterator]: splitting an empty range gives no subranges,
//     a trailing delimiter gives a trailing empty subrange; [range.lazy.split.outer] likewise.
//   [range.join.with.view]: join_with of two empty inner ranges is the pattern alone.
//   [range.repeat.view]: repeat(v, 0) is empty; [range.iota.view]: iota(0, 0) is empty.
//   [range.istream.view]: istream_view over an empty stream is empty.
//   [alg.fold]: fold_left of an empty range returns the init value, fold_left_first an empty
//     optional; [alg.min.max] minmax_element of an empty range returns {first, first}.
#include <algorithm>
#include <forward_list>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"

namespace v = std::views;

static int accesses = 0;
static int touch(int x) {
  ++accesses;
  return x;
}

template <class R>
bool empty_view(R&& r) {
  if constexpr (std::ranges::sized_range<R>)
    if (std::ranges::size(r) != 0) return false;
  return std::ranges::begin(r) == std::ranges::end(r) && std::ranges::distance(r) == 0;
}

int main() {
  std::vector<int> e;
  std::forward_list<int> fe;
  const std::vector<std::pair<int, std::string>> ep;
  auto t = e | v::transform(touch);  // any element access is counted

  CHECK(empty_view(t | v::filter([](int) { return true; })));
  CHECK(empty_view(t | v::take(3)));
  CHECK(empty_view(t | v::drop(3)));
  CHECK(empty_view(t | v::take_while([](int) { return true; })));
  CHECK(empty_view(t | v::drop_while([](int) { return false; })));
  CHECK(empty_view(t | v::reverse));
  CHECK(empty_view(t | v::common));
  CHECK(empty_view(t | v::as_const));
  CHECK(empty_view(t | v::as_rvalue));
  CHECK(empty_view(t | v::enumerate));
  CHECK(empty_view(t | v::stride(2)));
  CHECK(empty_view(t | v::slide(2)));
  CHECK(empty_view(t | v::chunk(2)));
  CHECK(empty_view(t | v::chunk_by(std::ranges::less{})));
  CHECK(empty_view(t | v::adjacent<2>));
  CHECK(empty_view(t | v::pairwise_transform(std::plus{})));
  CHECK(empty_view(v::zip(t, std::vector<int>{1, 2})));
  CHECK(empty_view(v::zip_transform(std::plus{}, t, std::vector<int>{1})));
  CHECK(empty_view(v::cartesian_product(t, std::vector<int>{1, 2})));
  CHECK(empty_view(v::cartesian_product(std::vector<int>{1, 2}, t)));
  CHECK(empty_view(v::concat(t, t)));
  CHECK(empty_view(t | v::cache_latest));
  CHECK(empty_view(t | v::as_input));
  CHECK(empty_view(fe | v::chunk(3)));
  CHECK(empty_view(fe | v::slide(1)));
  CHECK(empty_view(ep | v::keys));
  CHECK(empty_view(ep | v::values));
  CHECK(empty_view(ep | v::elements<1>));
  CHECK(empty_view(std::vector<std::vector<int>>() | v::join));
  CHECK(empty_view(std::vector<std::vector<int>>(3) | v::join));
  CHECK(empty_view(std::vector<std::string>() | v::join_with(',')));
  CHECK(std::ranges::equal(std::vector<std::string>(2) | v::join_with(','), std::string_view(",")));
  CHECK(std::ranges::equal(std::vector<std::string>(1) | v::join_with(std::string_view("--")), std::string_view("")));
  CHECK(empty_view(std::string_view() | v::split(',')));
  CHECK(empty_view(std::string_view() | v::lazy_split(',')));
  CHECK(std::ranges::distance(std::string_view("a,") | v::split(',')) == 2);
  CHECK(std::ranges::distance(std::string_view(",") | v::lazy_split(',')) == 2);
  CHECK(std::ranges::distance(std::string_view("a") | v::split(std::string_view())) == 1);
  CHECK(accesses == 0);

  // Degenerate arguments.
  std::vector<int> three{1, 2, 3};
  CHECK(empty_view(three | v::take(0)));
  CHECK(std::ranges::equal(three | v::drop(0), three));
  CHECK(empty_view(three | v::drop(100)));
  CHECK(empty_view(three | v::slide(4)));
  CHECK((three | v::slide(4)).size() == 0);
  CHECK((three | v::slide(3)).size() == 1);
  CHECK(std::ranges::distance(three | v::chunk(100)) == 1 && std::ranges::equal(*(three | v::chunk(100)).begin(), three));
  CHECK(std::ranges::equal(three | v::stride(100), std::vector<int>{1}));
  CHECK(empty_view(v::repeat(7, 0)));
  CHECK(empty_view(v::iota(5, 5)));
  CHECK(empty_view(v::counted(three.begin(), 0)));
  CHECK(empty_view(three | v::adjacent<4>));

  auto z0 = v::zip();
  static_assert(std::is_same_v<decltype(z0), std::ranges::empty_view<std::tuple<>>>);
  CHECK(empty_view(z0));
  auto c0 = v::cartesian_product();
  static_assert(std::is_same_v<decltype(c0), std::ranges::single_view<std::tuple<>>>);
  CHECK(std::ranges::distance(c0) == 1);
  auto a0 = three | v::adjacent<0>;
  static_assert(std::is_same_v<decltype(a0), std::ranges::empty_view<std::tuple<>>>);
  auto at0 = three | v::adjacent_transform<0>([] { return 1.5; });
  static_assert(std::is_same_v<decltype(at0), std::ranges::empty_view<double>>);
  auto zt0 = v::zip_transform([] { return 'c'; });
  static_assert(std::is_same_v<decltype(zt0), std::ranges::empty_view<char>>);
  CHECK(empty_view(at0) && empty_view(zt0));
  auto c1 = v::concat(three);
  static_assert(std::is_same_v<decltype(c1), std::ranges::ref_view<std::vector<int>>>);

  std::istringstream in("");
  CHECK(empty_view(std::ranges::istream_view<int>(in)));
  std::istringstream ws("   \n\t ");
  auto iv = std::ranges::istream_view<int>(ws);
  CHECK(iv.begin() == iv.end());

  CHECK(std::ranges::fold_left(e, 42, std::plus{}) == 42);
  CHECK(!std::ranges::fold_left_first(e, std::plus{}).has_value());
  CHECK(!std::ranges::fold_right_last(e, std::plus{}).has_value());
  auto [lo, hi] = std::ranges::minmax_element(e);
  CHECK(lo == e.end() && hi == e.end());
  CHECK(std::ranges::to<std::vector<int>>(t).empty());
  CHECK(accesses == 0);
  return 0;
}
