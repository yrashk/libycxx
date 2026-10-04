// [range.iter.op.advance]/4: ranges::advance(i, bound): (4.1) if assignable_from<I&, S>,
// "i = std::move(bound)" (no increments); (4.2) otherwise if sized_sentinel_for<S, I>,
// "ranges::advance(i, bound - i)" (for a random-access I, one i += n); (4.3) otherwise
// increments while i != bound.
// /6-7: ranges::advance(i, n, bound): (6.1) with a sized sentinel, "If |n| >= |bound - i|,
// equivalent to ranges::advance(i, bound)", otherwise ranges::advance(i, n); returns n - M.
// |n| is the mathematical absolute value, so n = numeric_limits<difference_type>::min() and
// max() are valid counts (the result is n - M, representable) -- checked in constant
// evaluation, where signed overflow is diagnosed. (6.2) without one, increments (n >= 0) or
// decrements (n < 0) "while bool(i != bound) is true ... but at most |n| times".
// [range.iter.op.next], [range.iter.op.prev]: next(i, n, bound) / prev(i, n, bound) are
// advance(i, n, bound) / advance(i, -n, bound).
// [range.iter.op.distance]/1-3: distance(first, last) counts increments for a non-sized
// sentinel (first taken by value: a move-only iterator is accepted as an rvalue), otherwise
// "last - first" (an array argument decays).
#include <iterator>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>
#include "check.hpp"

// Random-access iterator counting the operations applied to it.
struct Ops { int inc = 0, dec = 0, jump = 0; };
struct RA {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::random_access_iterator_tag;
  int* p = nullptr;
  Ops* ops = nullptr;
  constexpr int& operator*() const { return *p; }
  constexpr int& operator[](difference_type n) const { return p[n]; }
  constexpr RA& operator++() { ++ops->inc; ++p; return *this; }
  constexpr RA operator++(int) { RA t = *this; ++*this; return t; }
  constexpr RA& operator--() { ++ops->dec; --p; return *this; }
  constexpr RA operator--(int) { RA t = *this; --*this; return t; }
  constexpr RA& operator+=(difference_type n) { ++ops->jump; p += n; return *this; }
  constexpr RA& operator-=(difference_type n) { ++ops->jump; p -= n; return *this; }
  constexpr friend RA operator+(RA i, difference_type n) { i += n; return i; }
  constexpr friend RA operator+(difference_type n, RA i) { i += n; return i; }
  constexpr friend RA operator-(RA i, difference_type n) { i -= n; return i; }
  constexpr friend difference_type operator-(const RA& a, const RA& b) { return a.p - b.p; }
  constexpr friend bool operator==(const RA& a, const RA& b) { return a.p == b.p; }
  constexpr friend auto operator<=>(const RA& a, const RA& b) { return a.p <=> b.p; }
};
static_assert(std::random_access_iterator<RA>);

// A sized sentinel that is not assignable to RA.
struct SizedEnd {
  int* e;
  constexpr friend bool operator==(const RA& i, SizedEnd s) { return i.p == s.e; }
  constexpr friend std::ptrdiff_t operator-(SizedEnd s, const RA& i) { return s.e - i.p; }
  constexpr friend std::ptrdiff_t operator-(const RA& i, SizedEnd s) { return i.p - s.e; }
};
static_assert(std::sized_sentinel_for<SizedEnd, RA> && !std::assignable_from<RA&, SizedEnd>);

// A non-sized sentinel.
struct PlainEnd {
  int* e;
  constexpr friend bool operator==(const RA& i, PlainEnd s) { return i.p == s.e; }
};
static_assert(std::sentinel_for<PlainEnd, RA> && !std::sized_sentinel_for<PlainEnd, RA>);

constexpr bool run() {
  int a[10] = {};
  constexpr std::ptrdiff_t MIN = std::numeric_limits<std::ptrdiff_t>::min();
  constexpr std::ptrdiff_t MAX = std::numeric_limits<std::ptrdiff_t>::max();
  Ops ops;
  RA i{a + 2, &ops};

  std::ranges::advance(i, RA{a + 9, &ops});  // (4.1): assignment
  if (i.p != a + 9 || ops.inc || ops.dec || ops.jump) return false;

  i = RA{a + 2, &ops};
  std::ranges::advance(i, SizedEnd{a + 8});  // (4.2): one jump
  if (i.p != a + 8 || ops.inc || ops.jump != 1) return false;

  ops = {};
  i = RA{a + 2, &ops};
  std::ranges::advance(i, PlainEnd{a + 5});  // (4.3): increments
  if (i.p != a + 5 || ops.inc != 3) return false;

  // (6.1) extremes with a sized sentinel.
  i = RA{a + 2, &ops};
  if (std::ranges::advance(i, MAX, SizedEnd{a + 8}) != MAX - 6 || i.p != a + 8) return false;
  i = RA{a + 8, &ops};
  if (std::ranges::advance(i, MIN, RA{a + 1, &ops}) != MIN + 7 || i.p != a + 1) return false;
  i = RA{a + 8, &ops};
  if (std::ranges::advance(i, MIN + 1, RA{a + 1, &ops}) != MIN + 8 || i.p != a + 1) return false;
  i = RA{a + 8, &ops};
  if (std::ranges::advance(i, -3, RA{a + 1, &ops}) != 0 || i.p != a + 5) return false;
  i = RA{a + 8, &ops};
  if (std::ranges::advance(i, -7, RA{a + 1, &ops}) != 0 || i.p != a + 1) return false;
  i = RA{a + 4, &ops};
  if (std::ranges::advance(i, 0, SizedEnd{a + 4}) != 0 || i.p != a + 4) return false;
  if (std::ranges::advance(i, 0, SizedEnd{a + 9}) != 0 || i.p != a + 4) return false;

  // (6.2) extremes with a non-sized sentinel: stops at the bound.
  i = RA{a + 2, &ops};
  if (std::ranges::advance(i, MAX, PlainEnd{a + 8}) != MAX - 6 || i.p != a + 8) return false;
  i = RA{a + 3, &ops};
  if (std::ranges::advance(i, 4, PlainEnd{a + 9}) != 0 || i.p != a + 7) return false;

  // next / prev with bounds.
  RA b{a + 1, &ops};
  if (std::ranges::next(RA{a + 6, &ops}, MIN, b).p != a + 1) return false;
  if (std::ranges::prev(RA{a + 6, &ops}, MAX, b).p != a + 1) return false;
  if (std::ranges::prev(RA{a + 6, &ops}, -2, RA{a + 9, &ops}).p != a + 8) return false;
  if (std::ranges::prev(RA{a + 6, &ops}, -5, RA{a + 9, &ops}).p != a + 9) return false;
  if (std::ranges::next(RA{a + 6, &ops}, MAX, PlainEnd{a + 7}).p != a + 7) return false;

  // distance.
  if (std::ranges::distance(RA{a + 1, &ops}, PlainEnd{a + 7}) != 6) return false;
  if (std::ranges::distance(RA{a + 7, &ops}, SizedEnd{a + 1}) != -6) return false;  // last - first
  if (std::ranges::distance(a, a + 10) != 10) return false;  // array decays
  const int ca[3] = {};
  if (std::ranges::distance(ca, ca + 3) != 3) return false;
  return true;
}
static_assert(run());

// (6.2.2) bidirectional, non-sized, same type: decrements at most -n times.
struct Bidi {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p = nullptr;
  constexpr int& operator*() const { return *p; }
  constexpr Bidi& operator++() { ++p; return *this; }
  constexpr Bidi operator++(int) { Bidi t = *this; ++p; return t; }
  constexpr Bidi& operator--() { --p; return *this; }
  constexpr Bidi operator--(int) { Bidi t = *this; --p; return t; }
  constexpr friend bool operator==(const Bidi&, const Bidi&) = default;
};
static_assert(std::bidirectional_iterator<Bidi> && !std::sized_sentinel_for<Bidi, Bidi>);

constexpr bool bidi() {
  int a[10] = {};
  constexpr std::ptrdiff_t MIN = std::numeric_limits<std::ptrdiff_t>::min();
  Bidi i{a + 8};
  if (std::ranges::advance(i, MIN, Bidi{a + 3}) != MIN + 5 || i.p != a + 3) return false;
  i = Bidi{a + 8};
  if (std::ranges::advance(i, -2, Bidi{a + 3}) != 0 || i.p != a + 6) return false;
  if (std::ranges::distance(Bidi{a + 2}, Bidi{a + 9}) != 7) return false;
  return true;
}
static_assert(bidi());

// distance with a move-only input iterator passed as an rvalue.
struct MoveIn {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p = nullptr;
  constexpr explicit MoveIn(int* q) : p(q) {}
  MoveIn(MoveIn&&) = default;
  MoveIn& operator=(MoveIn&&) = default;
  constexpr int& operator*() const { return *p; }
  constexpr MoveIn& operator++() { ++p; return *this; }
  constexpr void operator++(int) { ++p; }
};
struct MoveEnd {
  int* e;
  constexpr friend bool operator==(const MoveIn& i, MoveEnd s) { return i.p == s.e; }
};
static_assert(std::input_iterator<MoveIn> && !std::copyable<MoveIn>);

constexpr bool move_only() {
  int a[5] = {};
  MoveIn it(a);
  return std::ranges::distance(std::move(it), MoveEnd{a + 5}) == 5;
}
static_assert(move_only());

int main() {
  CHECK(run() && bidi() && move_only());
  return 0;
}
