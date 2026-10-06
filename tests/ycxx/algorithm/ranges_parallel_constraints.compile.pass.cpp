// Constraints of the ranges:: parallel algorithm overloads.
// [algorithms.parallel.defns]/2: the execution policy parameter satisfies execution-policy,
// is_execution_policy_v<remove_cvref_t<Ep>> (lvalue, const and rvalue policies alike).
// [range.refinements]/8: their range parameters are sized-random-access-range (random_access_range
// and sized_range); the iterator forms take random_access_iterator with a sized_sentinel_for.
// [alg.copy] and the other algorithms with an output take a bounded output (result,
// result_last) or a sized-random-access-range result_r: there is no overload with an unbounded
// output iterator (copy(exec, first, last, result) is ill-formed).
#include <algorithm>
#include <deque>
#include <execution>
#include <iterator>
#include <list>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace rg = std::ranges;
namespace ex = std::execution;
using V = std::vector<int>;
using L = std::list<int>;
using D = std::deque<int>;
using Unsized = rg::subrange<int*, std::unreachable_sentinel_t>;  // random access, not sized
using Taken = decltype(std::views::iota(0) | std::views::take_while([](int i) { return i < 3; }));

template <class P, class R>
concept can_sort = requires(P&& p, R&& r) { rg::sort(std::forward<P>(p), std::forward<R>(r)); };
template <class P, class R, class O>
concept can_copy = requires(P&& p, R&& r, O&& o) { rg::copy(std::forward<P>(p), std::forward<R>(r), std::forward<O>(o)); };
template <class P, class R>
concept can_find = requires(P&& p, R&& r) { rg::find(std::forward<P>(p), std::forward<R>(r), 0); };
template <class P, class R>
concept can_for_each = requires(P&& p, R&& r) { rg::for_each(std::forward<P>(p), std::forward<R>(r), [](int) {}); };
template <class P, class I, class S>
concept can_find_it = requires(P&& p, I i, S s) { rg::find(std::forward<P>(p), i, s, 0); };
template <class P, class I, class S, class O>
concept can_copy_unbounded = requires(P&& p, I i, S s, O o) { rg::copy(std::forward<P>(p), i, s, o); };
template <class P, class R, class O>
concept can_transform = requires(P&& p, R&& r, O&& o) {
  rg::transform(std::forward<P>(p), std::forward<R>(r), std::forward<O>(o), [](int x) { return x; });
};

// every kind of policy expression
static_assert(can_sort<const ex::sequenced_policy&, V&>);
static_assert(can_sort<ex::parallel_policy&, V&>);
static_assert(can_sort<ex::parallel_unsequenced_policy, V&>);
static_assert(can_sort<const ex::unsequenced_policy&&, V&>);
static_assert(can_sort<ex::parallel_policy, std::span<int>>);
static_assert(can_sort<ex::parallel_policy, D&>);
// ranges that are not sized-random-access-range
static_assert(!can_sort<ex::parallel_policy, L&>);
static_assert(!can_find<ex::parallel_policy, L&>);
static_assert(!can_for_each<ex::parallel_policy, L&>);
static_assert(!can_find<ex::parallel_policy, Unsized>);
static_assert(!can_find<ex::parallel_policy, Taken>);
static_assert(can_find<ex::parallel_policy, V&> && can_for_each<ex::parallel_policy, const V&>);
// iterator forms need a sized sentinel
static_assert(can_find_it<ex::parallel_policy, int*, int*>);
static_assert(!can_find_it<ex::parallel_policy, int*, std::unreachable_sentinel_t>);
static_assert(!can_find_it<ex::parallel_policy, L::iterator, L::iterator>);
// outputs: bounded sized random access ranges only
static_assert(can_copy<ex::parallel_policy, V&, V&>);
static_assert(can_copy<ex::parallel_policy, const V&, std::span<int>>);
static_assert(!can_copy<ex::parallel_policy, V&, L&>);
static_assert(!can_copy<ex::parallel_policy, V&, const V&>);
static_assert(!can_copy<ex::parallel_policy, L&, V&>);
static_assert(!can_copy_unbounded<ex::parallel_policy, int*, int*, int*>);
static_assert(!can_copy_unbounded<ex::parallel_policy, int*, int*, std::back_insert_iterator<V>>);
static_assert(!can_copy<ex::parallel_policy, V&, std::back_insert_iterator<V>>);
static_assert(can_transform<ex::parallel_policy, V&, V&> && !can_transform<ex::parallel_policy, V&, L&>);
// without a policy, the same calls are the sequential overloads
static_assert(requires(L& l, V& v) { rg::copy(l, v.begin()); rg::find(l, 0); rg::sort(v); });
// a non-policy first argument does not select the parallel overloads
static_assert(!can_sort<int, V&>);

int main() {}
