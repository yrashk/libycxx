// [range.filter]: filter_view's concepts (at most bidirectional, never sized, common as V,
// not const-iterable over a forward range), iterator_concept (/2) and iterator_category
// (/3: bidirectional_iterator_tag if C derives from it, forward_iterator_tag if C derives
// from that, otherwise C), the begin() cache (/5), operator--, pred() and base().
// COUNTERPART: libcxx:ranges/range.adaptors/range.filter/(iterator|sentinel)/.*
#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;

struct Even {
  int* calls = nullptr;
  constexpr bool operator()(int x) const {
    if (calls) ++*calls;
    return x % 2 == 0;
  }
};

using FV1 = rg::filter_view<BorrowedView<int>, Even>;
static_assert(rg::bidirectional_range<FV1> && !rg::random_access_range<FV1>);
static_assert(rg::common_range<FV1> && !rg::sized_range<FV1> && !rg::borrowed_range<FV1>);
static_assert(!rg::range<const FV1>); // begin() const only for non-forward const V
using I1 = rg::iterator_t<FV1>;
static_assert(std::is_same_v<I1::iterator_concept, std::bidirectional_iterator_tag>);
static_assert(std::is_same_v<I1::iterator_category, std::bidirectional_iterator_tag>);
static_assert(std::is_same_v<std::iter_reference_t<I1>, int&>);
static_assert(std::is_same_v<decltype(std::declval<const I1&>().base()), int* const&>);
static_assert(noexcept(std::declval<const I1&>().base()));
static_assert(std::is_same_v<decltype(std::declval<I1&>().operator->()), int*>);

// Forward, non-common base.
using FwdV = ArchetypeView<ForwardIter<int>, PtrSentinel<int>>;
using FV2 = rg::filter_view<FwdV, Even>;
static_assert(rg::forward_range<FV2> && !rg::bidirectional_range<FV2> && !rg::common_range<FV2>);
static_assert(std::is_same_v<rg::iterator_t<FV2>::iterator_concept, std::forward_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<FV2>::iterator_category, std::forward_iterator_tag>);

// iota: models random_access, but its iterator_category is input_iterator_tag: /3.4.
using FV3 = rg::filter_view<rg::iota_view<int, int>, Even>;
static_assert(std::is_same_v<rg::iterator_t<FV3>::iterator_concept, std::bidirectional_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<FV3>::iterator_category, std::input_iterator_tag>);

// Input base: no iterator_category, void postfix ++.
using InV = ArchetypeView<InputIter<int>, PtrSentinel<int>>;
using FV4 = rg::filter_view<InV, Even>;
static_assert(rg::input_range<FV4> && !rg::forward_range<FV4>);
static_assert(std::is_same_v<rg::iterator_t<FV4>::iterator_concept, std::input_iterator_tag>);
static_assert(!has_iterator_category<rg::iterator_t<FV4>>);
static_assert(std::is_void_v<decltype(std::declval<rg::iterator_t<FV4>&>()++)>);

// Deduction guide.
static_assert(std::is_same_v<decltype(rg::filter_view(std::declval<int (&)[3]>(), Even{})),
                             rg::filter_view<rg::ref_view<int[3]>, Even>>);

constexpr bool test() {
  int a[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  {
    int calls = 0;
    auto f = a | std::views::filter(Even{&calls});
    CHECK(calls == 0);
    auto b = f.begin();
    CHECK(*b == 2 && calls == 2);
    auto b2 = f.begin();
    CHECK(b2 == b && calls == 2); // cached (/5)
    CHECK(range_equals(f, {2, 4, 6, 8}));
    auto e = f.end();
    --e;
    CHECK(*e == 8);
    --e;
    CHECK(*e == 6);
    auto old = e--;
    CHECK(*old == 6 && *e == 4);
    CHECK(f.front() == 2 && f.back() == 8 && !f.empty() && bool(f));
    CHECK(f.base().base()[0] == 1 && f.pred().calls == &calls);
    *f.begin() = 20;
    CHECK(a[1] == 20);
    CHECK(rg::iter_move(f.begin()) == 20);
    static_assert(std::is_same_v<decltype(rg::iter_move(f.begin())), int&&>);
    auto i = f.begin();
    auto j = std::ranges::next(i);
    rg::iter_swap(i, j);
    CHECK(a[1] == 4 && a[3] == 20);
  }
  {
    auto f = a | std::views::filter([](int x) { return x > 100; });
    CHECK(f.empty() && f.begin() == f.end());
  }
  {
    int b[5] = {2, 3, 4, 5, 6};
    FV2 f(FwdV(ForwardIter<int>(b), PtrSentinel<int>{b + 5}), Even{});
    CHECK(range_equals(f, {2, 4, 6}));
  }
  {
    auto f = std::views::iota(0, 10) | std::views::filter(Even{});
    CHECK(range_equals(f, {0, 2, 4, 6, 8}));
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
