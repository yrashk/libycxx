// [move.iter.op.comp]: move_iterator<I1> == / < / <=> move_iterator<I2> compare the bases
// (heterogeneous, <=> requires three_way_comparable_with); "template<sentinel_for<Iterator>
// S> friend constexpr bool operator==(const move_iterator& x, const move_sentinel<S>& y)"
// returns x.base() == y.base(). [move.iter.nonmember]: x - y is x.base() - y.base() for
// move_iterators and for a sized move_sentinel ("requires sized_sentinel_for<S, Iterator>").
// [move.sentinel]/1-2: move_iterator with move_sentinel models sentinel_for, so ranges
// algorithms accept the pair and move the elements: ranges::copy moves each string
// ([move.iter.elem]/1: operator* is ranges::iter_move(current)), and ranges::distance counts
// with a non-sized sentinel and subtracts with a sized one.
// [move.iterator]/1: iterator_concept is input_iterator_tag unless the base models
// random_access/bidirectional/forward_iterator, in which case it is that tag;
// iterator_category is iterator_traits<Iterator>::iterator_category (input_iterator_tag for
// iota_view's iterator) unless that derives from random_access_iterator_tag.
#include <iterator>
#include <algorithm>
#include <compare>
#include <cstddef>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>
#include "check.hpp"

struct NotSized {  // sentinel for std::string*, not sized
  std::string* e;
  friend bool operator==(std::string* p, NotSized s) { return p == s.e; }
};
static_assert(std::sentinel_for<NotSized, std::string*> && !std::sized_sentinel_for<NotSized, std::string*>);

int main() {
  const char* const longs = "a string that is too long for any small-string buffer, 0123456789";
  std::string src[4] = {longs, longs, longs, "x"};
  using MI = std::move_iterator<std::string*>;
  using MS = std::move_sentinel<NotSized>;
  static_assert(std::sentinel_for<MS, MI> && !std::sized_sentinel_for<MS, MI>);
  static_assert(std::sized_sentinel_for<std::move_sentinel<std::string*>, MI>);

  MI first(src);
  MS last(NotSized{src + 3});
  CHECK(std::ranges::distance(first, last) == 3);
  std::vector<std::string> out;
  auto res = std::ranges::copy(first, last, std::back_inserter(out));
  CHECK(res.in == MI(src + 3) && res.in == MS(NotSized{src + 3}));
  CHECK(out.size() == 3 && out[0] == longs && out[2] == longs);
  CHECK(src[3] == "x");

  // Sized move_sentinel: difference both ways.
  std::move_sentinel<std::string*> ms(src + 4);
  CHECK(ms - first == 4 && first - ms == -4);
  CHECK(std::ranges::distance(first, ms) == 4);

  // Heterogeneous comparisons between move_iterators.
  int a[4] = {1, 2, 3, 4};
  std::move_iterator<int*> m1(a + 1);
  std::move_iterator<const int*> m3(static_cast<const int*>(a + 3));
  CHECK(m1 != m3 && m1 < m3 && m3 > m1 && m1 <= m3 && !(m1 >= m3));
  CHECK((m1 <=> m3) == std::strong_ordering::less);
  CHECK(m3 - m1 == 2 && m1 - m3 == -2);
  CHECK(m1 == std::move_iterator<const int*>(static_cast<const int*>(a + 1)));

  // Moving algorithms through a move_iterator over a view.
  std::vector<std::string> strs{longs, "b", longs};
  auto mv = std::ranges::subrange(std::make_move_iterator(strs.begin()), std::move_sentinel(strs.end()));
  static_assert(std::ranges::input_range<decltype(mv)>);
  static_assert(std::is_same_v<std::ranges::range_reference_t<decltype(mv)>, std::string&&>);
  std::vector<std::string> dst(mv.begin(), std::make_move_iterator(strs.end()));
  CHECK(dst.size() == 3 && dst[0] == longs && dst[1] == "b" && dst[2] == longs);

  // Iterator concept / category.
  static_assert(std::is_same_v<MI::iterator_concept, std::random_access_iterator_tag>);
  auto iv = std::views::iota(0, 3);
  using IotaMI = std::move_iterator<std::ranges::iterator_t<decltype(iv)>>;
  static_assert(std::is_same_v<IotaMI::iterator_concept, std::random_access_iterator_tag>);
  static_assert(std::is_same_v<IotaMI::iterator_category, std::input_iterator_tag>);
  auto filt = iv | std::views::filter([](int) { return true; });
  using FiltMI = std::move_iterator<std::ranges::iterator_t<decltype(filt)>>;
  static_assert(std::is_same_v<FiltMI::iterator_concept, std::bidirectional_iterator_tag>);
  return 0;
}
