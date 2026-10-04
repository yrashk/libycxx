// Generic run-time check of the exception guarantee of the flat container adaptors.
//   [flat.map.overview]/5-6: "A flat_map maintains the following invariants: it contains
//     the same number of keys and values; the keys are sorted with respect to the comparison
//     object; and the value at offset off within the value container is the value associated
//     with the key at offset off within the key container. If any member function in
//     [flat.map.defn] exits via an exception the invariants of the object argument are
//     restored. For the move constructor and move assignment operator, the invariants of both
//     arguments are restored. [Note 2: This can result in the flat_map being emptied.]"
//   [flat.multimap.overview], [flat.set.overview]/5-6 ("A flat_set maintains the invariant
//     that the keys are sorted with respect to the comparison object. If any member function
//     in [flat.set.defn] exits via an exception, the invariant of the object argument is
//     restored."), [flat.multiset.overview]: the same.
// Every operation is retried with a failure armed after 0, 1, 2, ... steps -- a copy or move
// of a key or value, a comparison, an allocation -- until it completes; after each failure the
// invariants must hold again (and no element object may leak: [res.on.exception.handling]/3).
// Values are encoded so that the association is observable: the mapped value of key k is
// k + 100. X has key (and mapped) type FKey, comparison FLess and underlying containers with
// CountingAlloc (support/test_allocators.hpp).
#pragma once
#include <cstddef>
#include <iterator>
#include <new>
#include <utility>
#include <vector>
#include "test_allocators.hpp"
#include "check.hpp"  // dprintf

namespace reqs::flat_except {

struct Injected {};
inline int copy_countdown = -1;  // FKey copies and moves throw when this reaches 0
inline int cmp_countdown = -1;   // FLess throws when this reaches 0
inline long live = 0;

inline void tick(int& c) {
  if (c == 0) throw Injected();
  if (c > 0) --c;
}
inline void disarm() {
  copy_countdown = -1;
  cmp_countdown = -1;
  alloc_counters.fail_after = -1;
}

struct FKey {
  int v = 0;
  FKey() { ++live; }
  FKey(int x) : v(x) { ++live; }
  FKey(const FKey& o) : v(o.v) {
    tick(copy_countdown);
    ++live;
  }
  FKey(FKey&& o) : v(o.v) {
    tick(copy_countdown);
    ++live;
  }
  FKey& operator=(const FKey& o) {
    tick(copy_countdown);
    v = o.v;
    return *this;
  }
  FKey& operator=(FKey&& o) {
    tick(copy_countdown);
    v = o.v;
    return *this;
  }
  ~FKey() { --live; }
  friend bool operator==(const FKey& a, const FKey& b) { return a.v == b.v; }
};

struct FLess {
  bool operator()(const FKey& a, const FKey& b) const {
    tick(cmp_countdown);
    return a.v < b.v;
  }
};

template <class X>
concept is_map = requires { typename X::mapped_type; };
template <class X>
concept is_multi = std::is_same_v<decltype(std::declval<X&>().insert(std::declval<const typename X::value_type&>())),
                                  typename X::iterator>;

template <class X>
typename X::value_type v(int k) {
  if constexpr (is_map<X>) return {FKey(k), FKey(k + 100)};
  else return FKey(k);
}

// The invariants of [flat.map.overview]/5 / [flat.set.overview]/5, read through the
// underlying containers. "Sorted" is checked as non-descending: the invariant as stated does
// not include the uniqueness of the keys of flat_map / flat_set. Returns the number of FKey objects x owns, or -1 if broken.
template <class X>
long invariants(const X& x) {
  disarm();
  std::size_t n = 0;
  if constexpr (is_map<X>) {
    const auto& ks = x.keys();
    const auto& vs = x.values();
    if (ks.size() != vs.size()) return -1;
    for (std::size_t i = 0; i < ks.size(); ++i) {
      if (vs[i].v != ks[i].v + 100) return -1;
      if (i > 0 && ks[i].v < ks[i - 1].v) return -1;
    }
    n = 2 * ks.size();
  } else {
    std::size_t i = 0;
    int prev = 0;
    for (const FKey& k : x) {
      if (i > 0 && k.v < prev) return -1;
      prev = k.v;
      ++i;
    }
    n = i;
  }
  return static_cast<long>(n);
}

enum class Trigger { copy, comparison, allocation };
inline void arm(Trigger t, int n) {
  disarm();
  if (t == Trigger::copy) copy_countdown = n;
  if (t == Trigger::comparison) cmp_countdown = n;
  if (t == Trigger::allocation) alloc_counters.fail_after = n;
}

template <class X>
X make(std::initializer_list<int> ks) {
  disarm();
  X x;
  for (int k : ks) x.insert(v<X>(k));
  return x;
}

// Runs op on a fresh container (and a second one for the binary operations) with the failure
// armed after n steps, for n = 0, 1, ... until it completes; checks the invariants after
// every failure, and that the FKey objects alive are exactly those the containers own.
template <class X, class Op>
bool restores(const char* what, Op op) {
  const long others = live;
  for (Trigger t : {Trigger::copy, Trigger::comparison, Trigger::allocation}) {
    bool completed = false;
    for (int n = 0; n < 500 && !completed; ++n) {
      X x = make<X>({10, 20, 30, 40, 50, 60, 70, 80});
      X y = make<X>({15, 25, 35});
      arm(t, n);
      try {
        op(x, y);
        completed = true;
      } catch (const Injected&) {
      } catch (const std::bad_alloc&) {
      }
      disarm();
      long nx = invariants(x), ny = invariants(y);
      if (nx < 0 || ny < 0 || live != nx + ny + others) {
        dprintf(2, "flat_except: %s, trigger %d after %d steps: %s\n", what, static_cast<int>(t), n,
                nx < 0 || ny < 0 ? "invariant broken" : "element objects leaked");
        return false;
      }
    }
    if (!completed) return false;
  }
  return true;
}

template <class X>
bool test() {
  using V = typename X::value_type;
  bool ok = true;
  ok = restores<X>("insert(const value_type&)", [](X& x, X&) {
    const V e = v<X>(45);
    x.insert(e);
  }) && ok;
  ok = restores<X>("insert(value_type&&)", [](X& x, X&) { x.insert(v<X>(5)); }) && ok;
  ok = restores<X>("insert(hint, t)", [](X& x, X&) { x.insert(x.begin() + 3, v<X>(35)); }) && ok;
  ok = restores<X>("emplace", [](X& x, X&) {
    if constexpr (is_map<X>) x.emplace(FKey(55), FKey(155));
    else x.emplace(55);
  }) && ok;
  ok = restores<X>("emplace_hint", [](X& x, X&) { x.emplace_hint(x.end(), v<X>(95)); }) && ok;
  if constexpr (is_map<X> && !is_multi<X>) {
    ok = restores<X>("try_emplace", [](X& x, X&) { x.try_emplace(FKey(25), 125); }) && ok;
    ok = restores<X>("insert_or_assign", [](X& x, X&) { x.insert_or_assign(FKey(26), FKey(126)); }) && ok;
    ok = restores<X>("operator[]", [](X& x, X&) {
      FKey& m = x[FKey(27)];  // value-initialized mapped value, then assigned
      m.v = 127;
    }) && ok;
  }
  ok = restores<X>("insert(first, last)", [](X& x, X&) {
    std::vector<V> src;
    for (int k : {33, 3, 93, 63, 20}) src.push_back(v<X>(k));
    x.insert(src.begin(), src.end());
  }) && ok;
  ok = restores<X>("insert_range", [](X& x, X&) {
    std::vector<V> src;
    for (int k : {34, 4, 94}) src.push_back(v<X>(k));
    x.insert_range(src);
  }) && ok;
  ok = restores<X>("erase(k)", [](X& x, X&) { x.erase(FKey(30)); }) && ok;
  ok = restores<X>("erase(position)", [](X& x, X&) { x.erase(x.begin() + 1); }) && ok;
  ok = restores<X>("erase(first, last)", [](X& x, X&) { x.erase(x.begin() + 1, x.begin() + 3); }) && ok;
  ok = restores<X>("erase_if", [](X& x, X&) {
    std::erase_if(x, [](const auto& e) {
      if constexpr (is_map<X>) return e.first.v % 20 == 0;
      else return e.v % 20 == 0;
    });
  }) && ok;
  ok = restores<X>("copy assignment", [](X& x, X& y) { x = y; }) && ok;
  ok = restores<X>("move assignment", [](X& x, X& y) { x = std::move(y); }) && ok;
  ok = restores<X>("move construction", [](X& x, X&) {
    X z(std::move(x));
    (void)z;
  }) && ok;
  ok = restores<X>("copy construction", [](X& x, X&) {
    X z(x);
    (void)z;
  }) && ok;
  return ok;
}

}  // namespace reqs::flat_except
