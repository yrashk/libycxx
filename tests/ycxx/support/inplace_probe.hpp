// Probes for "constructs in place from the forwarded arguments" in libycxx's own suite.
// Independent of every other test suite.
//   probe::Arg     an argument whose value category the constructed object records: a
//                  constructor taking Arg& records 1, const Arg& 2, Arg&& 3, const Arg&& 4.
//   probe::Probe   copyable and movable; counts every constructor, copy, move, assignment and
//                  destruction in probe::counts.
//   probe::Pinned  neither copyable nor movable (only Cpp17EmplaceConstructible): an operation
//                  that compiles with it cannot have made a temporary and moved it.
// probe::reset() zeroes the counters; probe::counts.extra() is the number of copy and move
// constructions and assignments since then.
#pragma once
#include <cstddef>

namespace probe {

struct Counts {
  int made = 0;        // constructions from arguments (not copies or moves)
  int copies = 0;      // copy constructions
  int moves = 0;       // move constructions
  int copy_assigns = 0;
  int move_assigns = 0;
  int destroyed = 0;
  constexpr int extra() const { return copies + moves + copy_assigns + move_assigns; }
};
inline Counts counts;
inline void reset() { counts = Counts{}; }

struct Arg {
  int v = 0;
  bool moved_from = false;
};

// How the constructor received its Arg (0: no Arg).
enum Cat : int { none = 0, lref = 1, clref = 2, rref = 3, crref = 4 };

template <bool Movable>
struct Basic {
  int key = 0;
  int cat = none;
  int extra = 0;
  Basic() { ++counts.made; }
  Basic(int k) : key(k) { ++counts.made; }
  Basic(int k, int e) : key(k), extra(e) { ++counts.made; }
  Basic(int k, Arg& a) : key(k), cat(lref), extra(a.v) { ++counts.made; }
  Basic(int k, const Arg& a) : key(k), cat(clref), extra(a.v) { ++counts.made; }
  Basic(int k, Arg&& a) : key(k), cat(rref), extra(a.v) {
    a.moved_from = true;
    ++counts.made;
  }
  Basic(int k, const Arg&& a) : key(k), cat(crref), extra(a.v) { ++counts.made; }
  Basic(const Basic& o)
    requires Movable
      : key(o.key), cat(o.cat), extra(o.extra) {
    ++counts.copies;
  }
  Basic(Basic&& o) noexcept
    requires Movable
      : key(o.key), cat(o.cat), extra(o.extra) {
    ++counts.moves;
  }
  Basic& operator=(const Basic& o)
    requires Movable
  {
    key = o.key;
    cat = o.cat;
    extra = o.extra;
    ++counts.copy_assigns;
    return *this;
  }
  Basic& operator=(Basic&& o) noexcept
    requires Movable
  {
    key = o.key;
    cat = o.cat;
    extra = o.extra;
    ++counts.move_assigns;
    return *this;
  }
  Basic(const Basic&)
    requires(!Movable)
  = delete;
  Basic& operator=(const Basic&)
    requires(!Movable)
  = delete;
  ~Basic() { ++counts.destroyed; }
  friend bool operator==(const Basic& a, const Basic& b) { return a.key == b.key; }
  friend bool operator<(const Basic& a, const Basic& b) { return a.key < b.key; }
};
using Probe = Basic<true>;
using Pinned = Basic<false>;

struct Hash {
  template <bool M>
  std::size_t operator()(const Basic<M>& p) const { return static_cast<std::size_t>(p.key); }
  std::size_t operator()(int k) const { return static_cast<std::size_t>(k); }
};

}  // namespace probe
