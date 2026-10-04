// [range.repeat.view], [range.repeat.iterator]: repeat_view's concepts, its iterator types
// (/1: difference_type is index-type when signed-integer-like, otherwise
// IOTA-DIFF-T(index-type); index-type is ptrdiff_t when unbounded), the constructors
// (including piecewise_construct), the deduction guide, size(), and that every iterator
// refers to the one stored value.
#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "range_support.hpp"

namespace rg = std::ranges;

using RB = rg::repeat_view<int, int>;
static_assert(rg::view<RB> && rg::random_access_range<RB> && rg::common_range<RB> && rg::sized_range<RB>);
static_assert(!rg::contiguous_range<RB> && !rg::borrowed_range<RB>);
static_assert(std::is_same_v<rg::range_reference_t<RB>, const int&>);
static_assert(std::is_same_v<rg::range_value_t<RB>, int>);
static_assert(std::is_same_v<rg::iterator_t<RB>::iterator_concept, std::random_access_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<RB>::iterator_category, std::random_access_iterator_tag>);
static_assert(std::is_same_v<rg::range_difference_t<RB>, int>);
static_assert(std::is_same_v<decltype(RB().size()), unsigned>);
static_assert(noexcept(*std::declval<const rg::iterator_t<RB>&>()));
static_assert(noexcept(std::declval<const rg::iterator_t<RB>&>()[0]));

using RU = rg::repeat_view<int>;
static_assert(std::is_same_v<RU, rg::repeat_view<int, std::unreachable_sentinel_t>>);
static_assert(rg::random_access_range<RU> && !rg::common_range<RU> && !rg::sized_range<RU>);
static_assert(std::is_same_v<rg::sentinel_t<RU>, std::unreachable_sentinel_t>);
static_assert(std::is_same_v<rg::range_difference_t<RU>, std::ptrdiff_t>);
static_assert(noexcept(RU().end()));

// Unsigned bound: difference_type is IOTA-DIFF-T(unsigned), a wider signed type.
using RUn = rg::repeat_view<int, unsigned>;
static_assert(std::signed_integral<rg::range_difference_t<RUn>>);
static_assert(sizeof(rg::range_difference_t<RUn>) > sizeof(unsigned));

// Bound must be integer-like or unreachable_sentinel_t; T must be a cv-unqualified object.
template <class T, class B>
concept valid_repeat = requires { typename rg::repeat_view<T, B>; };
static_assert(valid_repeat<int, long>);
static_assert(!valid_repeat<const int, int>);
static_assert(!valid_repeat<int, double>);

// views::repeat(E) and views::repeat(E, F); the deduction guide.
static_assert(std::is_same_v<decltype(std::views::repeat(1)), rg::repeat_view<int>>);
static_assert(std::is_same_v<decltype(std::views::repeat(1.5, 3L)), rg::repeat_view<double, long>>);
static_assert(std::is_same_v<decltype(rg::repeat_view(1, 2u)), rg::repeat_view<int, unsigned>>);

struct MoveOnly {
  int v;
  constexpr explicit MoveOnly(int x) : v(x) {}
  MoveOnly(MoveOnly&&) = default;
  MoveOnly& operator=(MoveOnly&&) = default;
};
static_assert(rg::view<rg::repeat_view<MoveOnly, int>>);
static_assert(!std::copyable<rg::repeat_view<MoveOnly, int>>);

struct Pair {
  int a, b;
  constexpr Pair(int x, int y) : a(x), b(y) {}
};

constexpr bool test() {
  {
    auto r = std::views::repeat(17, 4);
    CHECK(range_equals(r, {17, 17, 17, 17}));
    CHECK(r.size() == 4u && r[3] == 17 && r.end() - r.begin() == 4);
    CHECK(&*r.begin() == &r.begin()[2]); // all iterators refer to the stored value
    auto i = r.begin();
    i += 3;
    CHECK(i - r.begin() == 3 && (r.begin() <=> i) < 0 && i != r.end());
    ++i;
    CHECK(i == r.end());
    --i;
    CHECK(r.end() - i == 1);
  }
  {
    auto r = std::views::repeat(5, 0);
    CHECK(r.empty() && r.begin() == r.end());
  }
  {
    auto u = std::views::repeat('x');
    auto i = u.begin() + 1000;
    CHECK(*i == 'x' && i - u.begin() == 1000);
  }
  {
    rg::repeat_view<MoveOnly, int> m(MoveOnly(3), 2);
    CHECK(m.size() == 2u && (*m.begin()).v == 3);
    rg::repeat_view<Pair, int> p(std::piecewise_construct, std::tuple<int, int>(1, 2), std::tuple<int>(3));
    CHECK(p.size() == 3u && p[2].a == 1 && p[2].b == 2);
    rg::repeat_view<Pair> pu(std::piecewise_construct, std::tuple<int, int>(4, 5));
    CHECK(pu.begin()[7].b == 5);
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
