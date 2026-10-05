// As containers/range_insert_iterator_throws, for hive and inplace_vector: the SOURCE range
// throws part-way (iterator increment or dereference, filter predicate).
//   [hive.modifiers]/7-10: insert_range / insert(i, j) give no no-effect guarantee, but every
//     element object constructed is destroyed exactly once (a live count returns to zero) and
//     the hive stays valid (size() equals the iteration length).
//   [inplace.vector.modifiers]/3: "If an exception is thrown other than by the copy
//     constructor, move constructor, assignment operator, or move assignment operator of T or
//     by any InputIterator operation, there are no effects. Otherwise, if an exception is
//     thrown, then size() >= n and elements in the range begin() + [0, n) are not modified"
//     (n: the size before the call).
// Each source throws on its n-th operation, for every n until the operation completes.
// REQUIRES: exceptions
#include <cstddef>
#include <hive>
#include <inplace_vector>
#include <iterator>
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


template <class C, class Op>
void sweep(Op op, int line, bool prefix = true) {
  bool completed = false;
  for (int limit = 0; limit < 400 && !completed; ++limit) {
    {
      C c{E(100), E(200), E(300)};
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
      std::size_t n = static_cast<std::size_t>(std::ranges::distance(c));
      CHECK(n == c.size());
      if (threw) {
        if (live != base + static_cast<int>(n) - 3) {
          dprintf(2, "line %d: limit %d: %d element objects alive, the container holds %zu\n", line, limit,
                  live - base + 3, n);
          ++failures;
        }
        if constexpr (requires { c[0]; }) if (prefix) {
          CHECK(n >= 3);
          CHECK(c[0] == E(100) && c[1] == E(200) && c[2] == E(300));
        }
      }
    }
    if (live != 0) {
      dprintf(2, "line %d: limit %d: %d objects never destroyed\n", line, limit, live);
      ++failures;
      live = 0;
      break;
    }
  }
}

template <class C>
void ctors(int line) {
  for (int limit = 0; limit < 400; ++limit) {
    budget = limit;
    bool threw = false;
    try {
      { C a(std::from_range, in_range()); }
      { C b(std::from_range, fwd_range()); }
      { C c(std::from_range, ra_range()); }
      { auto r = in_range(); C d(r.begin(), r.end()); }
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

int main() {
  using H = std::hive<E>;
  sweep<H>([](H& c) { c.insert_range(in_range()); }, __LINE__);
  sweep<H>([](H& c) { c.insert_range(fwd_range()); }, __LINE__);
  sweep<H>([](H& c) { c.insert_range(ra_range()); }, __LINE__);
  sweep<H>([](H& c) { auto r = in_range(); c.insert(r.begin(), r.end()); }, __LINE__);
  sweep<H>([](H& c) { c.assign_range(fwd_range()); }, __LINE__);
  ctors<H>(__LINE__);

  using IV = std::inplace_vector<E, 16>;
  sweep<IV>([](IV& c) { c.append_range(in_range()); }, __LINE__);
  sweep<IV>([](IV& c) { c.append_range(fwd_range()); }, __LINE__);
  sweep<IV>([](IV& c) { c.append_range(ra_range()); }, __LINE__);
  sweep<IV>([](IV& c) { c.insert_range(c.end(), fwd_range()); }, __LINE__);
  sweep<IV>([](IV& c) { auto r = in_range(); c.insert(c.end(), r.begin(), r.end()); }, __LINE__);
  sweep<IV>([](IV& c) { c.assign_range(fwd_range()); }, __LINE__, false);  // not an insertion
  ctors<IV>(__LINE__);
  CHECK(failures == 0);
  CHECK(live == 0);
  return 0;
}
