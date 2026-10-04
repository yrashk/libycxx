// [range.transform]: transform_view's concepts (the base's category, capped below
// contiguous; sized and common as V; const-iterable when F is const-invocable),
// iterator_concept (/1) and iterator_category (/2: C for a reference result, except
// contiguous -> random_access; input_iterator_tag for a prvalue result), and the iterator's
// operations.
#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;

struct Sq {
  constexpr int operator()(int x) const { return x * x; }
};
struct Ref {
  constexpr int& operator()(int& x) const { return x; }
};
struct MutableF {
  int k = 0;
  constexpr int operator()(int x) { return x + k; }
};

using T1 = rg::transform_view<BorrowedView<int>, Sq>;
static_assert(rg::random_access_range<T1> && !rg::contiguous_range<T1>);
static_assert(rg::sized_range<T1> && rg::common_range<T1> && !rg::borrowed_range<T1>);
static_assert(rg::random_access_range<const T1>);
static_assert(std::is_same_v<rg::range_reference_t<T1>, int> && std::is_same_v<rg::range_value_t<T1>, int>);
using I1 = rg::iterator_t<T1>;
static_assert(std::is_same_v<I1::iterator_concept, std::random_access_iterator_tag>);
static_assert(std::is_same_v<I1::iterator_category, std::input_iterator_tag>); // prvalue result
static_assert(std::is_convertible_v<I1, rg::iterator_t<const T1>>);
static_assert(!std::is_convertible_v<rg::iterator_t<const T1>, I1>);

using T2 = rg::transform_view<BorrowedView<int>, Ref>;
static_assert(std::is_same_v<rg::range_reference_t<T2>, int&>);
static_assert(std::is_same_v<rg::iterator_t<T2>::iterator_category, std::random_access_iterator_tag>);
static_assert(std::is_same_v<rg::range_rvalue_reference_t<T2>, int&&>);

using FwdV = ArchetypeView<ForwardIter<int>, PtrSentinel<int>>;
using T3 = rg::transform_view<FwdV, Ref>;
static_assert(rg::forward_range<T3> && !rg::bidirectional_range<T3> && !rg::common_range<T3>);
static_assert(std::is_same_v<rg::iterator_t<T3>::iterator_category, std::forward_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<T3>::iterator_concept, std::forward_iterator_tag>);

using InV = ArchetypeView<InputIter<int>, PtrSentinel<int>>;
using T4 = rg::transform_view<InV, Sq>;
static_assert(rg::input_range<T4> && !rg::forward_range<T4>);
static_assert(!has_iterator_category<rg::iterator_t<T4>>);

using T5 = rg::transform_view<BorrowedView<int>, MutableF>;
static_assert(rg::random_access_range<T5> && !rg::range<const T5>);

using T6 = rg::transform_view<rg::iota_view<int>, Sq>;
static_assert(!rg::common_range<T6> && !rg::sized_range<T6>);

// F must be regular_invocable with the reference type and yield a referenceable type.
struct VoidF {
  void operator()(int) const {}
};
template <class F>
concept can_transform = requires { typename rg::transform_view<BorrowedView<int>, F>; };
static_assert(can_transform<Sq> && !can_transform<VoidF>);

constexpr bool test() {
  int a[5] = {0, 1, 2, 3, 4};
  {
    auto t = a | std::views::transform(Sq{});
    CHECK(range_equals(t, {0, 1, 4, 9, 16}));
    CHECK(t.size() == 5u && t[3] == 9 && t.back() == 16);
    auto i = t.begin();
    i += 2;
    CHECK(*i == 4 && i[1] == 9 && *(i - 1) == 1 && *(1 + i) == 9);
    CHECK(t.end() - t.begin() == 5 && i - t.begin() == 2);
    CHECK(t.begin() < i && i > t.begin() && (i <=> t.begin()) > 0);
    CHECK(i.base() == a + 2);
    --i;
    CHECK(*i == 1);
    const auto& ct = t;
    CHECK(*ct.begin() == 0);
    rg::iterator_t<const decltype(t)> ci = t.begin();
    CHECK(*ci == 0);
  }
  {
    auto r = a | std::views::transform(Ref{});
    *r.begin() = 10;
    CHECK(a[0] == 10);
    CHECK(rg::iter_move(r.begin() + 1) == 1);
  }
  {
    auto m = a | std::views::transform(MutableF{100});
    CHECK(m.front() == 110);
  }
  {
    auto c = std::views::iota(1) | std::views::transform(Sq{}) | std::views::take(3);
    CHECK(range_equals(c, {1, 4, 9}));
  }
  {
    int b[3] = {1, 2, 3};
    T3 t(FwdV(ForwardIter<int>(b), PtrSentinel<int>{b + 3}), Ref{});
    CHECK(count_elements(t) == 3);
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
