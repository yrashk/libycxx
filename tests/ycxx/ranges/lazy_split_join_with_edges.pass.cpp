// [range.lazy.split.outer]/6-8: the outer iterator's operator++ "if (current == end) {
// trailing_empty_ = false; return *this; } const auto [pcur, pend] = subrange{parent_->pattern_};
// if (pcur == pend) ++current; else if constexpr (tiny-range<Pattern>) { current = ranges::
// find(std::move(current), end, *pcur); if (current != end) { ++current; if (current == end)
// trailing_empty_ = true; } } ...". [range.lazy.split.view]: a single element pattern is
// single_view (the deduction guide / views::lazy_split(E, F) with range_value_t F), a
// tiny-range, so input ranges can be split. An empty input gives no ranges; each delimiter
// separates, so "a  b" has an empty middle range and a trailing delimiter gives a trailing
// empty range.
// [range.join.with.view]/[range.join.with.iterator]: join_with of a range of prvalue ranges
// stores the current inner range in a non-propagating-cache (the view is then an input range,
// [range.join.with.iterator]/1); the pattern is emitted between consecutive inner ranges,
// empty ones included; when the outer and inner ranges are bidirectional and common the
// iterator is bidirectional and operator-- skips backwards over empty inner ranges and
// patterns ([range.join.with.iterator]/14-16).
#include <iterator>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include "check.hpp"

using namespace std::literals;

template <class R>
static std::vector<std::string> pieces(R&& r) {
  std::vector<std::string> out;
  for (auto&& inner : r) {
    std::string s;
    for (char c : inner) s.push_back(c);
    out.push_back(s);
  }
  return out;
}

template <class R>
static std::vector<int> ints(R&& r) {
  std::vector<int> out;
  for (auto&& x : r) out.push_back(x);
  return out;
}

int main() {
  using VS = std::vector<std::string>;
  CHECK((pieces("a b  c"sv | std::views::lazy_split(' ')) == VS{"a", "b", "", "c"}));
  CHECK((pieces("a "sv | std::views::lazy_split(' ')) == VS{"a", ""}));
  CHECK((pieces(" "sv | std::views::lazy_split(' ')) == VS{"", ""}));
  CHECK((pieces("  "sv | std::views::lazy_split(' ')) == VS{"", "", ""}));
  CHECK(pieces(""sv | std::views::lazy_split(' ')).empty());
  CHECK((pieces("abc"sv | std::views::lazy_split(' ')) == VS{"abc"}));
  CHECK((pieces("a,b"sv | std::views::lazy_split(std::views::single(','))) == VS{"a", "b"}));
  CHECK((pieces("abc"sv | std::views::lazy_split(std::views::empty<char>)) == VS{"a", "b", "c"}));
  CHECK((pieces("a, b,"sv | std::views::lazy_split(", "sv)) == VS{"a", "b,"}));
  CHECK((pieces("a, b, "sv | std::views::lazy_split(", "sv)) == VS{"a", "b", ""}));
  {
    // An input range split on a single character.
    std::istringstream is("x;;yz;");
    auto in = std::views::istream<char>(is) | std::views::lazy_split(';');
    static_assert(!std::ranges::forward_range<decltype(in)>);
    CHECK((pieces(in) == VS{"x", "", "yz", ""}));
  }

  {
    // join_with over prvalue inner ranges.
    auto rows = std::views::iota(0, 4) | std::views::transform([](int n) { return std::vector<int>(n, n); });
    auto j = rows | std::views::join_with(-1);
    static_assert(!std::ranges::forward_range<decltype(j)>);
    CHECK((ints(j) == std::vector<int>{-1, 1, -1, 2, 2, -1, 3, 3, 3}));
    auto j2 = rows | std::views::join_with(std::vector<int>{8, 9});
    CHECK((ints(j2) == std::vector<int>{8, 9, 1, 8, 9, 2, 2, 8, 9, 3, 3, 3}));
    auto empties = std::views::iota(0, 3) | std::views::transform([](int) { return std::vector<int>{}; });
    CHECK((ints(empties | std::views::join_with(7)) == std::vector<int>{7, 7}));
    auto none = std::views::iota(0, 0) | std::views::transform([](int) { return std::vector<int>{1}; });
    CHECK(ints(none | std::views::join_with(7)).empty());
    auto strs = std::views::iota(1, 4) | std::views::transform([](int n) { return std::string(n, 'a' + n); });
    std::string joined;
    for (char c : strs | std::views::join_with(", "sv)) joined.push_back(c);
    CHECK(joined == "b, cc, ddd");
  }
  {
    // Bidirectional join_with: backwards over empty inner ranges and the pattern.
    std::vector<std::vector<int>> vv = {{}, {1, 2}, {}, {}, {3}, {}};
    std::vector<int> pat = {0, 0};
    auto j = std::views::join_with(vv, pat);  // ref_views: copyable, so views::reverse(j) works
    static_assert(std::ranges::bidirectional_range<decltype(j)> && std::ranges::common_range<decltype(j)>);
    const std::vector<int> fwd = {0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 3, 0, 0};
    CHECK(ints(j) == fwd);
    std::vector<int> back;
    for (auto it = j.end(); it != j.begin();) back.push_back(*--it);
    CHECK((back == std::vector<int>(fwd.rbegin(), fwd.rend())));
    CHECK((ints(j | std::views::reverse) == std::vector<int>(fwd.rbegin(), fwd.rend())));
    std::vector<std::vector<int>> all_empty = {{}, {}};
    auto e = all_empty | std::views::join_with(5);
    auto it = e.end();
    CHECK(*--it == 5 && it == e.begin());
    std::vector<std::vector<int>> single_empty = {{}};
    auto se = single_empty | std::views::join_with(5);
    CHECK(se.begin() == se.end());
  }
  return 0;
}
