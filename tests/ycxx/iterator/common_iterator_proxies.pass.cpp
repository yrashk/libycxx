// [common.iter.access]/3-5: common_iterator::operator->: (5.1) for a pointer or an I with
// operator-> returns the iterator; (5.2) if iter_reference_t<I> is a reference type returns
// addressof(*it); (5.3) otherwise returns a proxy holding iter_value_t<I> whose operator->
// returns a const iter_value_t<I>*. The requires-clause rejects an iterator whose prvalue
// reference cannot construct its value type.
// [common.iter.nav]/5: operator++(int) for a non-forward I returns get<I>(v_)++ if *i++ can
// be referenced, or if the value type cannot be constructed from the reference; otherwise a
// postfix-proxy whose operator* returns a const iter_value_t<I>& to the value read before the
// increment, and the iterator is incremented. For forward I, operator++(int) returns a
// common_iterator copy ([common.iterator] synopsis).
// [common.iter.const]/3-4, [common.iter.cmp]: a common_iterator holding a sentinel can be
// assigned an iterator state and vice versa (converting assignment from common_iterator<I2,S2>).
#include <iterator>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Pt { int x, y; };

// Input iterator whose reference is a prvalue Pt; *i++ yields a prvalue (can-reference fails
// for void only, so make i++ return void to force the postfix-proxy).
struct PrvalIn {
  using value_type = Pt;
  using difference_type = std::ptrdiff_t;
  const Pt* p = nullptr;
  int* reads = nullptr;
  Pt operator*() const { if (reads) ++*reads; return *p; }
  PrvalIn& operator++() { ++p; return *this; }
  void operator++(int) { ++p; }
};
struct PrvalEnd {
  const Pt* e;
  friend bool operator==(const PrvalIn& i, PrvalEnd s) { return i.p == s.e; }
};
static_assert(std::input_iterator<PrvalIn> && !std::forward_iterator<PrvalIn>);
static_assert(std::sentinel_for<PrvalEnd, PrvalIn>);

// Input iterator whose i++ returns a reference-yielding thing: *i++ can be referenced.
struct RefIn {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p = nullptr;
  int& operator*() const { return *p; }
  RefIn& operator++() { ++p; return *this; }
  RefIn operator++(int) { RefIn t = *this; ++p; return t; }
};
struct RefEnd {
  int* e;
  friend bool operator==(const RefIn& i, RefEnd s) { return i.p == s.e; }
};
static_assert(std::input_iterator<RefIn> && !std::forward_iterator<RefIn>);  // no ==

// Forward iterator whose reference is a real reference but has no operator->.
struct FwdRef {
  using value_type = Pt;
  using difference_type = std::ptrdiff_t;
  Pt* p = nullptr;
  Pt& operator*() const { return *p; }
  FwdRef& operator++() { ++p; return *this; }
  FwdRef operator++(int) { FwdRef t = *this; ++p; return t; }
  friend bool operator==(const FwdRef&, const FwdRef&) = default;
};
struct FwdEnd {
  Pt* e;
  friend bool operator==(const FwdRef& i, FwdEnd s) { return i.p == s.e; }
};
static_assert(std::forward_iterator<FwdRef>);

// Prvalue reference not convertible to the value type: no operator->.
struct Opaque { explicit Opaque(int) {} };
struct WeirdIn {
  using value_type = Opaque;
  using difference_type = std::ptrdiff_t;
  int* p;
  int operator*() const { return *p; }
  WeirdIn& operator++() { ++p; return *this; }
  void operator++(int) { ++p; }
};
struct WeirdEnd {
  friend bool operator==(const WeirdIn&, WeirdEnd) { return true; }
};

template <class C>
concept has_arrow = requires(const C& c) { c.operator->(); };

int main() {
  {
    Pt pts[3] = {{1, 2}, {3, 4}, {5, 6}};
    int reads = 0;
    using C = std::common_iterator<PrvalIn, PrvalEnd>;
    C it(PrvalIn{pts, &reads});
    auto arrow = it.operator->();
    static_assert(!std::is_pointer_v<decltype(arrow)>);
    static_assert(std::is_same_v<decltype(arrow.operator->()), const Pt*>);
    CHECK(arrow->x == 1 && it->y == 2);
    // postfix-proxy: the value before the increment is kept.
    auto old = it++;
    static_assert(std::is_same_v<decltype(*old), const Pt&>);
    static_assert(noexcept(*old));
    CHECK((*old).x == 1 && (*it).x == 3);
    ++it;
    CHECK(it != C(PrvalEnd{pts + 3}) && it->x == 5);
    ++it;
    CHECK(it == C(PrvalEnd{pts + 3}));
    // Assigning across states.
    C s(PrvalEnd{pts + 3});
    s = C(PrvalIn{pts, nullptr});
    CHECK(s->x == 1);
    s = C(PrvalEnd{pts + 3});
    CHECK(s == C(PrvalEnd{pts + 3}));
  }
  {
    int a[3] = {7, 8, 9};
    using C = std::common_iterator<RefIn, RefEnd>;
    C it(RefIn{a});
    auto r = it++;  // *i++ can be referenced: returns get<I>(v_)++ (a RefIn)
    static_assert(std::is_same_v<decltype(r), RefIn>);
    CHECK(*r == 7 && *it == 8);
    static_assert(std::is_same_v<decltype(it.operator->()), int*>);  // (5.2): addressof(*it)
    CHECK(it.operator->() == a + 1);
  }
  {
    Pt pts[2] = {{1, 2}, {3, 4}};
    using C = std::common_iterator<FwdRef, FwdEnd>;
    C it(FwdRef{pts});
    static_assert(std::is_same_v<decltype(it.operator->()), Pt*>);
    CHECK(it.operator->() == pts && it->y == 2);
    it->x = 10;
    CHECK(pts[0].x == 10);
    auto old = it++;
    static_assert(std::is_same_v<decltype(old), C>);
    CHECK(old->x == 10 && it->x == 3);
    CHECK(std::ranges::distance(old, C(FwdEnd{pts + 2})) == 2);
  }
  static_assert(!has_arrow<std::common_iterator<WeirdIn, WeirdEnd>>);
  static_assert(has_arrow<std::common_iterator<PrvalIn, PrvalEnd>>);
  return 0;
}
