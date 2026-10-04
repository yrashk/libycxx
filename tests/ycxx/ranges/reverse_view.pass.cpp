// [range.reverse]: views::reverse undoes a reverse_view (/2.1) and a subrange of
// reverse_iterators (/2.3), keeps an optional (/2.2), and otherwise yields reverse_view,
// whose iterators are reverse_iterators; begin() of a non-common view walks to the end once
// and caches it (/3); borrowed as its view; sized as its view.
#include <iterator>
#include <optional>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

using R1 = decltype(std::declval<int (&)[3]>() | vw::reverse);
static_assert(std::is_same_v<R1, rg::reverse_view<rg::ref_view<int[3]>>>);
static_assert(std::is_same_v<rg::iterator_t<R1>, std::reverse_iterator<int*>>);
static_assert(rg::random_access_range<R1> && rg::common_range<R1> && rg::sized_range<R1>);
static_assert(!rg::contiguous_range<R1> && rg::borrowed_range<R1>);
static_assert(std::is_same_v<decltype(std::declval<R1>() | vw::reverse), rg::ref_view<int[3]>>);

using SRR = rg::subrange<std::reverse_iterator<int*>>;
static_assert(std::is_same_v<decltype(std::declval<SRR>() | vw::reverse), rg::subrange<int*>>);
using SRRU = rg::subrange<std::reverse_iterator<BidiIter<int>>>;
static_assert(std::is_same_v<decltype(std::declval<SRRU>() | vw::reverse), rg::subrange<BidiIter<int>>>);
static_assert(std::is_same_v<decltype(std::optional<int>() | vw::reverse), std::optional<int>>);

using BidiNC = ArchetypeView<BidiIter<int>, PtrSentinel<int>>;
using R2 = rg::reverse_view<BidiNC>;
static_assert(rg::bidirectional_range<R2> && rg::common_range<R2> && !rg::range<const R2>);

template <class V>
concept reversible = requires { typename rg::reverse_view<V>; };
static_assert(!reversible<ArchetypeView<ForwardIter<int>>>);

// Counts increments of a bidirectional iterator.
struct CountIt {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p = nullptr;
  int* incs = nullptr;
  constexpr int& operator*() const { return *p; }
  constexpr CountIt& operator++() {
    ++p;
    ++*incs;
    return *this;
  }
  constexpr CountIt operator++(int) {
    auto t = *this;
    ++*this;
    return t;
  }
  constexpr CountIt& operator--() {
    --p;
    return *this;
  }
  constexpr CountIt operator--(int) {
    auto t = *this;
    --p;
    return t;
  }
  friend constexpr bool operator==(const CountIt& a, const CountIt& b) { return a.p == b.p; }
  friend constexpr bool operator==(const CountIt& a, const PtrSentinel<int>& s) { return a.p == s.p; }
};
static_assert(std::bidirectional_iterator<CountIt>);
struct CountView : rg::view_base {
  int* b = nullptr;
  int* e = nullptr;
  int* incs = nullptr;
  constexpr CountIt begin() const { return {b, incs}; }
  constexpr PtrSentinel<int> end() const { return {e}; }
};

constexpr bool test() {
  int a[4] = {1, 2, 3, 4};
  {
    auto r = a | vw::reverse;
    CHECK(range_equals(r, {4, 3, 2, 1}) && r.size() == 4u && r[0] == 4 && r.back() == 1);
    CHECK(r.begin().base() == a + 4 && r.end().base() == a);
    auto rr = r | vw::reverse;
    CHECK(rr.data() == a);
  }
  {
    int incs = 0;
    rg::reverse_view<CountView> r(CountView{{}, a, a + 4, &incs});
    auto b1 = r.begin();
    CHECK(incs == 4 && *b1 == 4);
    auto b2 = r.begin();
    CHECK(incs == 4 && b1 == b2); // cached
    CHECK(range_equals(r, {4, 3, 2, 1}));
  }
  {
    SRR s(std::reverse_iterator<int*>(a + 3), std::reverse_iterator<int*>(a));
    auto u = s | vw::reverse;
    CHECK(u.begin() == a && u.end() == a + 3);
  }
  {
    auto r = vw::iota(0, 4) | vw::reverse | vw::take(2);
    CHECK(range_equals(r, {3, 2}));
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
