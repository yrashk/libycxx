// [range.lazy.split]: lazy_split_view splits lazily and works on input ranges when the
// pattern is a tiny-range (sized with a constant size() <= 1); outer iterator_concept is
// forward for forward bases, input otherwise; iterator_category (input_iterator_tag) only for
// forward bases; trailing delimiter gives a trailing empty range.
// COUNTERPART: libcxx:ranges/range.adaptors/range.lazy.split/range.lazy.split.inner/ctor.outer_iterator.pass.cpp
// COUNTERPART: libcxx:ranges/range.adaptors/range.lazy.split/range.lazy.split.outer.value/ctor.iter.pass.cpp
// COUNTERPART: libcxx:ranges/range.adaptors/range.lazy.split/range.lazy.split.outer/ctor.parent(_base)?.pass.cpp
#include <array>
#include <iterator>
#include <ranges>
#include <string_view>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;
using namespace std::string_view_literals;

using L1 = decltype("a b"sv | vw::lazy_split(' '));
static_assert(std::is_same_v<L1, rg::lazy_split_view<std::string_view, rg::single_view<char>>>);
static_assert(rg::forward_range<L1> && rg::common_range<L1> && rg::forward_range<const L1>);
static_assert(std::is_same_v<rg::iterator_t<L1>::iterator_concept, std::forward_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<L1>::iterator_category, std::input_iterator_tag>);
using Inner1 = rg::range_reference_t<L1>;
static_assert(rg::forward_range<Inner1> && !rg::common_range<Inner1>);
static_assert(std::is_same_v<rg::iterator_t<Inner1>::iterator_category, std::forward_iterator_tag>);

using InV = ArchetypeView<InputIter<const char>, PtrSentinel<const char>>;
using L2 = rg::lazy_split_view<InV, rg::single_view<char>>;
static_assert(rg::input_range<L2> && !rg::forward_range<L2>);
static_assert(std::is_same_v<rg::iterator_t<L2>::iterator_concept, std::input_iterator_tag>);
static_assert(!has_iterator_category<rg::iterator_t<L2>>);
static_assert(std::is_same_v<rg::sentinel_t<L2>, std::default_sentinel_t>);
// A non-tiny pattern cannot split an input range.
template <class V, class P>
concept can_lazy_split = requires { typename rg::lazy_split_view<V, P>; };
static_assert(can_lazy_split<InV, rg::single_view<char>> && can_lazy_split<InV, rg::empty_view<char>>);
static_assert(!can_lazy_split<InV, std::string_view>);
static_assert(can_lazy_split<std::string_view, std::string_view>);

// Writes the pieces separated by '|' into out; returns the length.
template <class R>
constexpr int flatten(R&& r, char* out) {
  int n = 0;
  bool first = true;
  for (auto&& piece : r) {
    if (!first) out[n++] = '|';
    first = false;
    for (char c : piece) out[n++] = c;
  }
  return n;
}

constexpr bool test() {
  char buf[64] = {};
  int n = flatten("the quick fox"sv | vw::lazy_split(' '), buf);
  CHECK(std::string_view(buf, n) == "the|quick|fox");
  n = flatten("a::b::"sv | vw::lazy_split("::"sv), buf);
  CHECK(std::string_view(buf, n) == "a|b|");
  n = flatten("ab"sv | vw::lazy_split(""sv), buf);
  CHECK(std::string_view(buf, n) == "a|b");
  n = flatten(",x"sv | vw::lazy_split(','), buf);
  CHECK(std::string_view(buf, n) == "|x");
  const char text[] = "1 22 333";
  L2 in(InV(InputIter<const char>(text), PtrSentinel<const char>{text + 8}), rg::single_view<char>(' '));
  n = flatten(in, buf);
  CHECK(std::string_view(buf, n) == "1|22|333");
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
