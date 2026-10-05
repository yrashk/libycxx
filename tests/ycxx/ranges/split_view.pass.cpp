// [range.split]: split_view splits into subrange<iterator_t<V>> elements; a trailing
// delimiter produces a trailing empty element ([range.split.iterator]/4); an empty pattern
// splits into single elements (find-next, /5); the iterator is forward with
// iterator_category input_iterator_tag; split_view is not const-iterable; deduction guides.
// COUNTERPART: libcxx:ranges/range.adaptors/range.split/(iterator|sentinel)/.*
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

using S1 = decltype("a b"sv | vw::split(' '));
static_assert(std::is_same_v<S1, rg::split_view<std::string_view, rg::single_view<char>>>);
static_assert(rg::forward_range<S1> && !rg::bidirectional_range<S1> && rg::common_range<S1>);
static_assert(!rg::range<const S1> && !rg::sized_range<S1>);
static_assert(std::is_same_v<rg::range_value_t<S1>, rg::subrange<std::string_view::const_iterator>>);
static_assert(std::is_same_v<rg::range_reference_t<S1>, rg::range_value_t<S1>>);
static_assert(std::is_same_v<rg::iterator_t<S1>::iterator_concept, std::forward_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<S1>::iterator_category, std::input_iterator_tag>);
using S2 = decltype("a, b"sv | vw::split(", "sv));
static_assert(std::is_same_v<S2, rg::split_view<std::string_view, std::string_view>>);
static_assert(!rg::common_range<rg::split_view<rg::iota_view<int>, rg::single_view<int>>>);

template <std::size_t N>
constexpr bool pieces_are(auto&& sv, const std::array<std::string_view, N>& expect) {
  std::size_t k = 0;
  for (auto piece : sv) {
    if (k == N) return false;
    if (std::string_view(piece.begin(), piece.end()) != expect[k]) return false;
    ++k;
  }
  return k == N;
}

constexpr bool test() {
  CHECK(pieces_are("the quick brown fox"sv | vw::split(' '), std::array<std::string_view, 4>{"the", "quick", "brown", "fox"}));
  CHECK(pieces_are("a,,b,"sv | vw::split(','), std::array<std::string_view, 4>{"a", "", "b", ""}));
  CHECK(pieces_are(",a"sv | vw::split(','), std::array<std::string_view, 2>{"", "a"}));
  CHECK(pieces_are(","sv | vw::split(','), std::array<std::string_view, 2>{"", ""}));
  CHECK(pieces_are(""sv | vw::split(','), std::array<std::string_view, 0>{}));
  CHECK(pieces_are("abc"sv | vw::split(""sv), std::array<std::string_view, 3>{"a", "b", "c"}));
  CHECK(pieces_are("x::y::"sv | vw::split("::"sv), std::array<std::string_view, 3>{"x", "y", ""}));
  CHECK(pieces_are("nodelim"sv | vw::split(';'), std::array<std::string_view, 1>{"nodelim"}));
  {
    auto s = "a b"sv | vw::split(' ');
    auto b1 = s.begin();
    CHECK(b1 == s.begin()); // begin() is cached ([range.split.view]/4)
    CHECK(b1.base() == s.base().begin());
    auto old = b1++;
    CHECK(old == s.begin() && b1 != old);
  }
  {
    int a[7] = {1, 0, 2, 3, 0, 0, 4};
    auto s = a | vw::split(0);
    auto i = s.begin();
    CHECK(range_equals(*i, {1}));
    ++i;
    CHECK(range_equals(*i, {2, 3}));
    ++i;
    CHECK((*i).empty());
    ++i;
    CHECK(range_equals(*i, {4}));
    ++i;
    CHECK(i == s.end());
    CHECK((*s.begin()).begin() == a);
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
