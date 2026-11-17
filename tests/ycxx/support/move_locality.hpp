// An element type that logs the target of every assignment, and the measure of how a container
// operation walks its elements: the number of "far jumps", assignments to an element whose
// index is more than 2 away from the indices of each of the previous 4 elements assigned to.
// An operation that moves elements along one or two sequential sweeps (shifting, reversing)
// makes a few; one that follows the cycles of a rotation by k (f[j] = f[j + k], j += k) makes
// one per element moved for any k > 2. For libycxx's own suite; independent of every other
// test suite.
#pragma once
#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace locality {

struct Stats {
  long ctors = 0, assigns = 0, live = 0;
  std::vector<const void*> targets; // of the assignments, in order
};
inline Stats stats;
inline bool logging = false;

struct E {
  int v;
  E(int x) : v(x) { ++stats.ctors, ++stats.live; }
  E(const E& o) : v(o.v) { ++stats.ctors, ++stats.live; }
  E(E&& o) noexcept : v(o.v) { ++stats.ctors, ++stats.live; }
  E& operator=(const E& o) {
    v = o.v;
    note();
    return *this;
  }
  E& operator=(E&& o) noexcept {
    v = o.v;
    note();
    return *this;
  }
  ~E() { --stats.live; }
  void note() {
    ++stats.assigns;
    if (logging) stats.targets.push_back(this);
  }
};

inline void start() {
  stats.ctors = stats.assigns = 0;
  stats.targets.clear();
  logging = true;
}

// The far jumps among the logged assignments to the elements of c (others, to temporaries, are
// skipped). Element addresses are those of c after the operation, which the operation did not
// change (no reallocation; a deque's blocks never move).
template <class C>
long far_jumps(const C& c) {
  logging = false;
  std::vector<std::pair<const void*, long>> index;
  long i = 0;
  for (const E& e : c) index.push_back({&e, i++});
  std::sort(index.begin(), index.end());
  long recent[4];
  int nrecent = 0;
  long jumps = 0;
  for (const void* t : stats.targets) {
    const auto it = std::lower_bound(index.begin(), index.end(), std::pair<const void*, long>(t, -1));
    if (it == index.end() || it->first != t) continue;
    const long x = it->second;
    bool near = false;
    for (int k = 0; k != nrecent; ++k)
      if (x - recent[k] <= 2 && recent[k] - x <= 2) near = true;
    if (!near && nrecent != 0) ++jumps;
    if (nrecent == 4) {
      for (int k = 1; k != 4; ++k) recent[k - 1] = recent[k];
      --nrecent;
    }
    recent[nrecent++] = x;
  }
  return jumps;
}

// A sized range whose iterator is single-pass, over [b, e).
template <class It, class T>
struct SizedInput {
  T* b;
  T* e;
  It begin() const { return It(b); }
  auto end() const { return It(e); }
  std::size_t size() const { return static_cast<std::size_t>(e - b); }
};

} // namespace locality
