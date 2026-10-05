// Sequence-container range operations whose SOURCE throws part-way (an iterator increment or
// dereference, or a filter predicate), not the element type.
//   [deque.modifiers]/3: "If an exception is thrown other than by the copy constructor, move
//     constructor, assignment operator, or move assignment operator of T, there are no effects"
//     (insert(p, i, j), insert_range, append_range, prepend_range). Iterator operations are not
//     exempted for deque (unlike [vector.modifiers]/2, which adds "or by any InputIterator
//     operation").
//   [list.modifiers]/2 and [forward.list.modifiers]/1: "If an exception is thrown, there are no
//     effects" / "no effect on the container".
//   For vector (and for every constructor and assign_range) the effects are unspecified, but
//   no element object may be lost: each constructed element is destroyed exactly once
//   ([res.on.exception.handling]; [container.reqmts]: the container's destructor, and a
//   constructor that exits via an exception, destroy the elements it constructed). A live
//   count must return to zero.
// Sources: a single-pass input range, a forward non-sized range (filter_view with a throwing
// predicate), a random-access sized range with a throwing dereference. Each throws on its n-th
// operation, for every n until the operation completes.
// REQUIRES: exceptions
#include <cstddef>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <ranges>
#include <vector>
#include "check.hpp"

static int live = 0, budget = -1, failures = 0;
static void tick() {
  if (budget >= 0 && budget-- == 0) throw 9;
}
struct E {
  int v;
  E(int x) : v(x) { ++live; }
  E(const E& o) : v(o.v) { ++live; }
  E& operator=(const E&) = default;
  ~E() { --live; }
  friend bool operator==(const E&, const E&) = default;
};

static const int src[8] = {1, 2, 3, 4, 5, 6, 7, 8};

// Single-pass input iterator: ++ and * tick.
struct In {
  using value_type = E;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::input_iterator_tag;
  using reference = E;
  using pointer = void;
  const int* p = nullptr;
  E operator*() const {
    tick();
    return E(*p);
  }
  In& operator++() {
    tick();
    ++p;
    return *this;
  }
  In operator++(int) {
    In t = *this;
    ++*this;
    return t;
  }
  friend bool operator==(const In& a, const In& b) { return a.p == b.p; }
};
static_assert(std::input_iterator<In> && !std::forward_iterator<In>);

// Random access over src; operator* ticks.
struct RA {
  using value_type = E;
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::random_access_iterator_tag;
  using iterator_concept = std::random_access_iterator_tag;
  using reference = E;
  using pointer = void;
  const int* p = nullptr;
  E operator*() const {
    tick();
    return E(*p);
  }
  E operator[](difference_type n) const { return *(*this + n); }
  RA& operator++() { return ++p, *this; }
  RA operator++(int) { auto t = *this; ++p; return t; }
  RA& operator--() { return --p, *this; }
  RA operator--(int) { auto t = *this; --p; return t; }
  RA& operator+=(difference_type n) { return p += n, *this; }
  RA& operator-=(difference_type n) { return p -= n, *this; }
  friend RA operator+(RA a, difference_type n) { return a += n; }
  friend RA operator+(difference_type n, RA a) { return a += n; }
  friend RA operator-(RA a, difference_type n) { return a -= n; }
  friend difference_type operator-(RA a, RA b) { return a.p - b.p; }
  friend auto operator<=>(RA a, RA b) { return a.p <=> b.p; }
  friend bool operator==(RA a, RA b) { return a.p == b.p; }
};
static_assert(std::random_access_iterator<RA>);

auto in_range() { return std::ranges::subrange(In{src}, In{src + 8}); }
auto ra_range() { return std::ranges::subrange(RA{src}, RA{src + 8}); }
auto keep = [](int x) {
  tick();
  return x % 3 != 0;
};
auto fwd_range() {
  return std::views::all(src) | std::views::filter(keep) | std::views::transform([](int x) { return E(x); });
}

template <class C>
C initial() {
  return C{E(100), E(200), E(300)};
}

// Every failure is reported (on stderr; "line" is the caller's line, times 1000 plus the
// operation's line for those in common_ops); the test fails at the end.
static bool fail(bool& failed) {
  ++failures;
  failed = true;
  return false;
}

// strong: on an exception, c must equal its initial value.
template <class C, class Op>
void sweep(Op op, bool strong, int line) {
  bool completed = false, failed = false;
  for (int limit = 0; limit < 400 && !completed && !failed; ++limit) {
    {
      C c = initial<C>();
      const int base = live;
      budget = limit;
      bool threw = false;
      try {
        op(c);
      } catch (int e) {
        CHECK(e == 9);
        threw = true;
      }
      budget = -1;
      completed = !threw;
      if (threw && strong && !(c == initial<C>())) {
        dprintf(2, "line %d: limit %d: the container changed although the operation threw\n", line, limit);
        fail(failed);
      }
      if (threw) {
        std::size_t n = static_cast<std::size_t>(std::ranges::distance(c));
        if (live != base + static_cast<int>(n) - 3) {
          dprintf(2, "line %d: limit %d: %d element objects alive, the container holds %zu (was 3)\n", line, limit,
                  live - base + 3, n);
          fail(failed);
        }
      }
    }
    if (live != 0) {
      dprintf(2, "line %d: limit %d: %d objects never destroyed\n", line, limit, live);
      fail(failed);
      live = 0;
    }
  }
  CHECK(completed || failed);
}

template <class C>
void constructors(int line) {
  for (int limit = 0; limit < 400; ++limit) {
    budget = limit;
    bool threw = false;
    try {
      { C a(std::from_range, in_range()); }
      { C b(std::from_range, fwd_range()); }
      { C c(std::from_range, ra_range()); }
      { auto r = in_range(); C d(r.begin(), r.end()); }
      { auto r = ra_range(); C e(r.begin(), r.end()); }
      { auto f = fwd_range() | std::ranges::to<C>(); }
    } catch (int) {
      threw = true;
    }
    budget = -1;
    if (live != 0) {
      dprintf(2, "line %d: constructor, limit %d: %d objects never destroyed\n", line, limit, live);
      ++failures;
      live = 0;
      break;
    }
    if (!threw) break;
  }
}

template <class C>
void common_ops(bool strong, int line) {
  sweep<C>([](C& c) { c.append_range(in_range()); }, strong, line * 1000 + __LINE__);
  sweep<C>([](C& c) { c.append_range(fwd_range()); }, strong, line * 1000 + __LINE__);
  sweep<C>([](C& c) { c.append_range(ra_range()); }, strong, line * 1000 + __LINE__);
  sweep<C>([](C& c) { c.insert_range(std::next(c.begin()), in_range()); }, strong, line * 1000 + __LINE__);
  sweep<C>([](C& c) { c.insert_range(std::next(c.begin()), fwd_range()); }, strong, line * 1000 + __LINE__);
  sweep<C>([](C& c) { c.insert_range(std::next(c.begin()), ra_range()); }, strong, line * 1000 + __LINE__);
  sweep<C>([](C& c) { c.insert_range(c.begin(), fwd_range()); }, strong, line * 1000 + __LINE__);
  sweep<C>([](C& c) { auto r = in_range(); c.insert(std::next(c.begin(), 2), r.begin(), r.end()); }, strong, line * 1000 + __LINE__);
  sweep<C>([](C& c) { auto r = ra_range(); c.insert(c.end(), r.begin(), r.end()); }, strong, line * 1000 + __LINE__);
  sweep<C>([](C& c) { c.assign_range(fwd_range()); }, false, line * 1000 + __LINE__);
  sweep<C>([](C& c) { c.assign_range(in_range()); }, false, line * 1000 + __LINE__);
  constructors<C>(line);
}

int main() {
  common_ops<std::deque<E>>(true, __LINE__);
  sweep<std::deque<E>>([](auto& c) { c.prepend_range(in_range()); }, true, __LINE__);
  sweep<std::deque<E>>([](auto& c) { c.prepend_range(fwd_range()); }, true, __LINE__);
  sweep<std::deque<E>>([](auto& c) { c.prepend_range(ra_range()); }, true, __LINE__);

  common_ops<std::list<E>>(true, __LINE__);
  sweep<std::list<E>>([](auto& c) { c.prepend_range(fwd_range()); }, true, __LINE__);

  common_ops<std::vector<E>>(false, __LINE__);

  using FL = std::forward_list<E>;
  sweep<FL>([](FL& c) { c.prepend_range(in_range()); }, true, __LINE__);
  sweep<FL>([](FL& c) { c.prepend_range(fwd_range()); }, true, __LINE__);
  sweep<FL>([](FL& c) { c.insert_range_after(c.begin(), fwd_range()); }, true, __LINE__);
  sweep<FL>([](FL& c) { c.insert_range_after(c.begin(), in_range()); }, true, __LINE__);
  sweep<FL>([](FL& c) { auto r = ra_range(); c.insert_after(c.begin(), r.begin(), r.end()); }, true, __LINE__);
  sweep<FL>([](FL& c) { c.assign_range(fwd_range()); }, false, __LINE__);
  constructors<FL>(__LINE__);
  CHECK(failures == 0);
  CHECK(live == 0);
  return 0;
}
