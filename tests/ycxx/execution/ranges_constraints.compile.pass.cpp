// [algorithm.syn], [algorithms.parallel.defns]/2: the parallel overloads in namespace ranges
// are constrained on execution-policy<Ep> (is_execution_policy_v<remove_cvref_t<Ep>>),
// random_access_iterator + sized_sentinel_for, or sized-random-access-range (random_access_range
// and sized_range) for every range argument, outputs included (an output is a range or an
// iterator/sentinel pair, not a lone iterator). Calls that do not meet them are invalid
// expressions (not hard errors); the same calls with conforming arguments are valid.
#include <algorithm>
#include <execution>
#include <iterator>
#include <ranges>
#include <utility>
#include "test_iterators.hpp"

namespace r = std::ranges;
using Pol = const std::execution::parallel_policy&;
struct NotAPolicy {};

int arr[4];
int out[4];

template <class P, class R>
concept for_each_ok = requires(P&& p, R&& rg) { r::for_each(std::forward<P>(p), std::forward<R>(rg), [](int) {}); };
template <class P, class R, class O>
concept copy_ok = requires(P&& p, R&& rg, O&& o) { r::copy(std::forward<P>(p), std::forward<R>(rg), std::forward<O>(o)); };
template <class P, class R>
concept sort_ok = requires(P&& p, R&& rg) { r::sort(std::forward<P>(p), std::forward<R>(rg)); };

// conforming
static_assert(for_each_ok<Pol, int (&)[4]>);
static_assert(for_each_ok<std::execution::sequenced_policy, r::subrange<int*>>);
static_assert(copy_ok<Pol, int (&)[4], int (&)[4]>);
static_assert(sort_ok<const std::execution::unsequenced_policy&, int (&)[4]>);

// not a policy: no overload (the non-parallel ones do not take this argument list either)
static_assert(!copy_ok<NotAPolicy, int (&)[4], int (&)[4]>);

// not random access, or not sized
using FwdR = ForwardRange<int>;                                   // forward, unsized, non-common
using BidiRg = r::subrange<BidiIter<int>>;                        // bidirectional, common
using UnsizedRA = r::subrange<int*, std::unreachable_sentinel_t>;  // random access, not sized
static_assert(!for_each_ok<Pol, FwdR>);
static_assert(!for_each_ok<Pol, BidiRg>);
static_assert(!for_each_ok<Pol, UnsizedRA>);
static_assert(!sort_ok<Pol, BidiRg>);
// the output must be a sized random-access range, not an iterator
static_assert(!copy_ok<Pol, int (&)[4], int*>);
static_assert(!copy_ok<Pol, int (&)[4], UnsizedRA>);

int main() { return 0; }
