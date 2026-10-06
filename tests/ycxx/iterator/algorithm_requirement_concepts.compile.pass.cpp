// [alg.req.ind.move]/1-2: indirectly_movable<In, Out> is indirectly_writable<Out,
// iter_rvalue_reference_t<In>>; indirectly_movable_storable adds writability of iter_value_t<In>,
// movable value type, construction and assignment from the rvalue reference.
// [alg.req.ind.copy]/1-2: indirectly_copyable uses iter_reference_t<In>; the storable form
// needs writability of every value category of iter_value_t<In> and copyable<iter_value_t<In>>.
// [alg.req.ind.swap]/1: ranges::iter_swap in all four combinations.
// [alg.req.ind.cmp]/1: indirectly_comparable is indirect_binary_predicate on the projections.
// [alg.req.permutable]/1: forward_iterator, indirectly_movable_storable<I, I>,
// indirectly_swappable<I, I>. [alg.req.mergeable]/1, [alg.req.sortable]/1.
#include <forward_list>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using It = std::vector<int>::iterator;
using CIt = std::vector<int>::const_iterator;
using UIt = std::vector<std::unique_ptr<int>>::iterator;
using LIt = std::vector<long>::iterator;
using SIt = std::vector<std::string>::iterator;
using II = std::istream_iterator<int>;
using OI = std::ostream_iterator<int>;

// move / copy
static_assert(std::indirectly_movable<UIt, UIt> && std::indirectly_movable_storable<UIt, UIt>);
static_assert(!std::indirectly_copyable<UIt, UIt> && !std::indirectly_copyable_storable<UIt, UIt>);
static_assert(std::indirectly_copyable<It, LIt> && std::indirectly_copyable_storable<It, LIt>);
static_assert(std::indirectly_copyable<CIt, It> && !std::indirectly_copyable<It, CIt>);
static_assert(std::indirectly_copyable<It, OI> && std::indirectly_copyable_storable<It, OI>);
static_assert(std::indirectly_copyable<II, It> && std::indirectly_movable<II, It>);
static_assert(std::indirectly_movable<std::move_iterator<SIt>, SIt>);
static_assert(!std::indirectly_copyable<SIt, It> && !std::indirectly_movable<SIt, It>);

// a value type that is not movable: movable but not storable
struct pinned {
  pinned() = default;
  pinned(const pinned&) = delete;
  pinned& operator=(const pinned&) = delete;
};
struct pin_ref_it {
  using value_type = pinned;
  using difference_type = std::ptrdiff_t;
  pinned& operator*() const;
  pin_ref_it& operator++();
  pin_ref_it operator++(int);
  bool operator==(const pin_ref_it&) const;
};
static_assert(std::indirectly_readable<pin_ref_it>);
static_assert(!std::indirectly_movable<pin_ref_it, pin_ref_it>);
static_assert(!std::indirectly_movable_storable<pin_ref_it, pin_ref_it>);

// writable from the reference but not from the value type: copyable, not storable
struct only_from_ref {
  int v;
};
struct accepts_ref_only {
  accepts_ref_only& operator=(const only_from_ref&) const;  // const-assignable proxy
  accepts_ref_only& operator=(only_from_ref&&) const = delete;
};
struct proxy_out {
  using difference_type = std::ptrdiff_t;
  accepts_ref_only operator*() const;
  proxy_out& operator++();
  proxy_out operator++(int);
};
using RefIt = std::vector<only_from_ref>::const_iterator;  // reference: const only_from_ref&
static_assert(std::indirectly_writable<proxy_out, const only_from_ref&>);
static_assert(std::indirectly_copyable<RefIt, proxy_out>);
static_assert(!std::indirectly_copyable_storable<RefIt, proxy_out>);  // value rvalue not writable

// swap
static_assert(std::indirectly_swappable<It> && std::indirectly_swappable<It, It>);
static_assert(std::indirectly_swappable<UIt>);
static_assert(!std::indirectly_swappable<CIt> && !std::indirectly_swappable<It, CIt>);
// int& and long& are not swappable, but ranges::iter_swap falls back to exchanging through the
// value types ([iterator.cust.swap]/4.3), which needs movable-storable both ways
static_assert(std::indirectly_swappable<It, LIt>);
static_assert(!std::indirectly_swappable<It, SIt>);
static_assert(std::indirectly_swappable<std::vector<bool>::iterator>);

// comparable with projections
struct S { int key; };
using SSIt = std::vector<S>::iterator;
static_assert(std::indirectly_comparable<It, LIt, std::ranges::equal_to>);
static_assert(!std::indirectly_comparable<SSIt, It, std::ranges::equal_to>);
static_assert(std::indirectly_comparable<SSIt, It, std::ranges::equal_to, int S::*>);
static_assert(std::indirectly_comparable<It, SSIt, std::ranges::equal_to, std::identity, int S::*>);

// permutable: forward iterators only, movable-storable and swappable
static_assert(std::permutable<It> && std::permutable<UIt> && std::permutable<std::forward_list<int>::iterator>);
static_assert(!std::permutable<CIt> && !std::permutable<II>);
static_assert(std::permutable<std::vector<bool>::iterator>);
static_assert(!std::permutable<pin_ref_it>);

// mergeable
static_assert(std::mergeable<It, CIt, OI>);
static_assert(std::mergeable<II, std::list<int>::iterator, It>);
static_assert(!std::mergeable<It, It, CIt>);
static_assert(!std::mergeable<SSIt, SSIt, SSIt>);  // no order on S
static_assert(std::mergeable<SSIt, SSIt, SSIt, std::ranges::less, int S::*, int S::*>);
static_assert(std::mergeable<SSIt, SSIt, SSIt, std::ranges::greater, int S::*, int S::*>);

// sortable
static_assert(std::sortable<It> && std::sortable<UIt>);  // unique_ptr is totally ordered
static_assert(std::sortable<SSIt, std::ranges::less, int S::*>);
static_assert(!std::sortable<SSIt>);
static_assert(!std::sortable<CIt>);
static_assert(std::sortable<std::list<int>::iterator>);  // the concept does not need random access

int main() {}
