// [range.join.with]: join_with_view inserts the pattern (a single element or a range)
// between the inner ranges, including empty ones; the deduction guides (a range pattern ->
// all_t, an element -> single_view); iterator_concept and iterator_category per
// [range.join.with.iterator]/1-2; begin() const as specified.
#include <array>
#include <iterator>
#include <ranges>
#include <string_view>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"

namespace rg = std::ranges;
namespace vw = std::views;
using namespace std::string_view_literals;
using Words = std::array<std::string_view, 4>;

using JE = decltype(std::declval<Words&>() | vw::join_with('-'));
static_assert(std::is_same_v<JE, rg::join_with_view<rg::ref_view<Words>, rg::single_view<char>>>);
static_assert(rg::bidirectional_range<JE> && rg::common_range<JE> && !rg::sized_range<JE>);
static_assert(rg::bidirectional_range<const JE>);
static_assert(std::is_same_v<rg::range_reference_t<JE>, const char&>);
static_assert(std::is_same_v<rg::iterator_t<JE>::iterator_concept, std::bidirectional_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<JE>::iterator_category, std::bidirectional_iterator_tag>);

using JR = decltype(std::declval<Words&>() | vw::join_with(", "sv));
static_assert(std::is_same_v<JR, rg::join_with_view<rg::ref_view<Words>, std::string_view>>);

// The pattern yields char& and the inner ranges const char&: common reference const char&
// is a reference, so iterator_category is computed from the categories (/2.3).
static_assert(std::is_same_v<decltype(rg::join_with_view(std::declval<Words&>(), std::declval<rg::single_view<char>>())), JE>);

// Inner prvalue ranges: input only, no iterator_category.
inline constexpr auto upto = [](int n) { return vw::iota(0, n); };
using JP = decltype(vw::iota(1, 4) | vw::transform(upto) | vw::join_with(-1));
static_assert(rg::input_range<JP> && !rg::forward_range<JP> && !rg::range<const JP>);
static_assert(!has_iterator_category<rg::iterator_t<JP>>);

template <class R>
bool constexpr str_equals(R&& r, std::string_view s) {
  auto i = rg::begin(r);
  auto e = rg::end(r);
  for (char c : s) {
    if (i == e || *i != c) return false;
    ++i;
  }
  return i == e;
}

constexpr bool test() {
  Words w = {"the", "quick", "brown", "fox"};
  CHECK(str_equals(w | vw::join_with('-'), "the-quick-brown-fox"));
  CHECK(str_equals(w | vw::join_with(", "sv), "the, quick, brown, fox"));
  Words e = {"", "a", "", ""};
  CHECK(str_equals(e | vw::join_with('+'), "+a++"));
  auto j = w | vw::join_with('-');
  auto it = j.end();
  --it;
  CHECK(*it == 'x');
  for (int n = 0; n < 3; ++n) --it;
  CHECK(*it == '-');
  CHECK(j.front() == 't' && j.back() == 'x');
  std::array<std::string_view, 1> one = {"solo"};
  CHECK(str_equals(one | vw::join_with('-'), "solo"));
  std::array<std::string_view, 0> none = {};
  CHECK(str_equals(none | vw::join_with('-'), ""));
  CHECK(range_equals(vw::iota(1, 4) | vw::transform(upto) | vw::join_with(-1), {0, -1, 0, 1, -1, 0, 1, 2}));
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
