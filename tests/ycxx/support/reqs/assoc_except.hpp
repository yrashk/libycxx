// Generic run-time check of the exception safety guarantees of the node-based associative
// and unordered associative containers.
//   [associative.reqmts.except]/1: "no clear() function throws an exception. erase(k) does
//     not throw an exception unless that exception is thrown by the container's Compare
//     object (if any)." /2: "if an exception is thrown by any operation from within an insert
//     or emplace function inserting a single element, the insertion has no effect." /3: "no
//     swap function throws an exception unless that exception is thrown by the swap of the
//     container's Compare object (if any)."
//   [unord.req.except]/1-3: the same with "Hash or Pred object", except that /2 exempts an
//     exception thrown by the container's hash function; /4: "if an exception is thrown from
//     within a rehash() function other than by the container's hash function or comparison
//     function, the rehash() function has no effect."
//   [res.on.exception.handling]/3 (basic guarantee for the other operations, e.g. inserting
//     a range): the container stays valid and nothing leaks.
// "No effect" is checked as: the same elements, at the same addresses, in the same iteration
// order (and, for the unordered containers, the same bucket_count()), and no element object
// leaked or destroyed. Each operation is retried with the failure armed after 0, 1, 2, ...
// steps (element constructions, comparisons / equality tests, allocations) until it succeeds,
// so every point at which it can fail is exercised.
// X has key_type TKey; mapped_type (maps) TKey; Compare ThrowLess or Pred ThrowEq, hasher
// TKeyHash; allocator CountingAlloc (support/test_allocators.hpp, armed through
// alloc_counters.fail_after).
#pragma once
#include <cstddef>
#include <functional>
#include <iterator>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>
#include "test_allocators.hpp"
#include "check.hpp"  // dprintf

namespace reqs::assoc_except {

struct Injected {};  // the exception thrown by TKey and the comparison objects

inline int elem_countdown = -1;  // TKey(int) / copy construction throws when this reaches 0
inline int cmp_countdown = -1;   // ThrowLess / ThrowEq throw when this reaches 0
inline long live = 0;            // TKey objects alive

inline void tick(int& countdown) {
  if (countdown == 0) throw Injected();
  if (countdown > 0) --countdown;
}

inline void disarm() {
  elem_countdown = -1;
  cmp_countdown = -1;
  alloc_counters.fail_after = -1;
}

struct TKey {
  int v = 0;
  TKey() { ++live; }
  TKey(int x) : v(x) {
    tick(elem_countdown);
    ++live;
  }
  TKey(const TKey& o) : v(o.v) {
    tick(elem_countdown);
    ++live;
  }
  TKey(TKey&& o) noexcept : v(o.v) { ++live; }
  TKey& operator=(const TKey& o) {
    tick(elem_countdown);
    v = o.v;
    return *this;
  }
  TKey& operator=(TKey&& o) noexcept {
    v = o.v;
    return *this;
  }
  ~TKey() { --live; }
  friend bool operator==(const TKey& a, const TKey& b) { return a.v == b.v; }
};

struct ThrowLess {
  bool operator()(const TKey& a, const TKey& b) const {
    tick(cmp_countdown);
    return a.v < b.v;
  }
};
struct ThrowEq {
  bool operator()(const TKey& a, const TKey& b) const {
    tick(cmp_countdown);
    return a.v == b.v;
  }
};
struct TKeyHash {
  std::size_t operator()(const TKey& k) const noexcept { return static_cast<std::size_t>(k.v) * 2654435761u; }
};

template <class X>
concept is_map = requires { typename X::mapped_type; };
template <class X>
concept is_unordered = requires { typename X::hasher; };
template <class X>
concept is_multi = std::is_same_v<decltype(std::declval<X&>().insert(std::declval<const typename X::value_type&>())),
                                  typename X::iterator>;

template <class X>
const TKey& key_of(const typename X::value_type& v) {
  if constexpr (is_map<X>) return v.first;
  else return v;
}

template <class X>
void emplace_key(X& x, int k) {
  if constexpr (is_map<X>) x.emplace(k, k + 100);
  else x.emplace(k);
}

template <class X>
X make(std::initializer_list<int> ks) {
  disarm();
  X x;
  for (int k : ks) emplace_key(x, k);
  return x;
}

// What "no effect" compares: element addresses, keys and mapped values in iteration order.
struct Snapshot {
  std::vector<const void*> addr;
  std::vector<int> keys, mapped;
  std::size_t buckets = 0;
  long live_count = 0;
  friend bool operator==(const Snapshot&, const Snapshot&) = default;
};

template <class X>
Snapshot snap(const X& x) {
  Snapshot s;
  for (auto it = x.begin(); it != x.end(); ++it) {
    s.addr.push_back(std::addressof(*it));
    s.keys.push_back(key_of<X>(*it).v);
    if constexpr (is_map<X>) s.mapped.push_back(it->second.v);
  }
  if constexpr (is_unordered<X>) s.buckets = x.bucket_count();
  s.live_count = live;
  return s;
}

enum class Trigger { element, comparison, allocation };

inline void arm(Trigger t, int n) {
  disarm();
  if (t == Trigger::element) elem_countdown = n;
  if (t == Trigger::comparison) cmp_countdown = n;
  if (t == Trigger::allocation) alloc_counters.fail_after = n;
}

// Runs op(x) with the failure armed after n = 0, 1, 2, ... steps until it completes. After
// each failure x must be unchanged; returns false on a violation, or if op never completes.
template <class X, class Op>
bool no_effect_on_failure(X& x, Trigger t, Op op) {
  for (int n = 0; n < 200; ++n) {
    Snapshot before = snap(x);
    arm(t, n);
    try {
      op(x);
      disarm();
      return true;
    } catch (const Injected&) {
    } catch (const std::bad_alloc&) {
    }
    disarm();
    if (!(snap(x) == before)) return false;
  }
  return false;
}

// The single-element insertion and emplacement members, each inserting key 50 (absent from
// the container), and for maps try_emplace, insert_or_assign and operator[] with a new key.
template <class X>
bool single_element_insertion() {
  using V = typename X::value_type;
  const Trigger triggers[3] = {Trigger::element, Trigger::comparison, Trigger::allocation};
  int case_no = 0;
  auto ops = [&](auto&& f) {
    for (Trigger t : triggers) {
      X x = make<X>({10, 20, 30, 40, 60, 70});
      if constexpr (is_unordered<X>) x.max_load_factor(1.0f);
      if (!no_effect_on_failure(x, t, f)) {
        dprintf(2, "assoc_except: insertion case %d, trigger %d had an effect\n", case_no, static_cast<int>(t));
        return false;
      }
      disarm();
      if (x.count(TKey(50)) != 1 || x.size() != 7) return false;
    }
    ++case_no;
    return true;
  };
  auto value = [] {
    if constexpr (is_map<X>) return V(TKey(50), TKey(150));
    else return V(TKey(50));
  };
  bool ok = true;
  ok = ok && ops([&](X& x) {
    V v = value();
    x.insert(v);
  });
  ok = ok && ops([&](X& x) { x.insert(value()); });
  ok = ok && ops([&](X& x) {
    V v = value();
    x.insert(x.begin(), v);
  });
  ok = ok && ops([&](X& x) {
    if constexpr (is_map<X>) x.emplace(50, 150);
    else x.emplace(50);
  });
  ok = ok && ops([&](X& x) {
    if constexpr (is_map<X>) x.emplace_hint(x.end(), 50, 150);
    else x.emplace_hint(x.end(), 50);
  });
  if constexpr (is_map<X> && !is_multi<X>) {
    ok = ok && ops([&](X& x) { x.try_emplace(TKey(50), 150); });
    ok = ok && ops([&](X& x) { x.try_emplace(x.end(), TKey(50), 150); });
    ok = ok && ops([&](X& x) {
      const TKey k(50);
      x.insert_or_assign(k, TKey(150));
    });
    ok = ok && ops([&](X& x) {
      const TKey k(50);
      x[k] = TKey(150);
    });
  }
  // inserting a node handle allocates and constructs nothing: only the comparison object can
  // fail, and a failed insertion has no effect (the handle keeps the element)
  {
    X x = make<X>({10, 20, 30});
    X y = make<X>({50});
    auto nh = y.extract(TKey(50));
    for (int n = 0;; ++n) {
      Snapshot before = snap(x);
      arm(Trigger::comparison, n);
      try {
        x.insert(std::move(nh));
        disarm();
        break;
      } catch (const Injected&) {
      }
      disarm();
      if (!(snap(x) == before) || nh.empty()) return false;
      if (n > 100) return false;
    }
    if (!x.contains(TKey(50)) || !nh.empty()) return false;
  }
  return ok;
}

// clear(), erase(k), erase(q), extract and swap throw nothing when the comparison object and
// the hash function do not (they construct and allocate nothing): element construction and
// allocation are armed to fail at once.
template <class X>
bool non_throwing_members() {
  X x = make<X>({1, 2, 3, 4, 5, 6});
  X y = make<X>({7});
  const TKey k3(3), k5(5), k9(9);  // made before arming
  elem_countdown = 0;
  alloc_counters.fail_after = 0;
  try {
    if (x.erase(k3) != 1 || x.erase(k9) != 0) return disarm(), false;
    x.erase(x.begin());
    (void)x.extract(k5);
    x.swap(y);
    swap(x, y);
    x.swap(y);  // x: {7}, y: {2, 4, 6}
    if (x.size() != 1 || y.size() != 3) return disarm(), false;
    y.clear();
  } catch (...) {
    disarm();
    dprintf(2, "assoc_except: a member that must not throw threw\n");
    return false;
  }
  disarm();
  return x.size() == 1 && y.empty();
}

// Basic guarantee for the range members: after a failure the container is still valid
// (its iteration, size() and lookups agree) and no element leaks.
template <class X>
bool range_insertion_basic() {
  using V = typename X::value_type;
  std::vector<V> src;
  disarm();
  for (int k = 0; k < 20; ++k) {
    if constexpr (is_map<X>) src.emplace_back(TKey(k * 3), TKey(k));
    else src.emplace_back(k * 3);
  }
  for (Trigger t : {Trigger::element, Trigger::comparison, Trigger::allocation}) {
    for (int n = 0; n < 400; ++n) {
      long base = live;
      {
        X x = make<X>({1, 2, 4, 5});
        arm(t, n);
        bool done = true;
        try {
          x.insert(src.begin(), src.end());
        } catch (const Injected&) {
          done = false;
        } catch (const std::bad_alloc&) {
          done = false;
        }
        disarm();
        std::size_t count = 0;
        for (auto it = x.begin(); it != x.end(); ++it) {
          ++count;
          if (!x.contains(key_of<X>(*it))) return false;
        }
        if (count != x.size()) return false;
        if (done) {
          if (x.size() != 24u) return false;  // 0, 3, ..., 57 and 1, 2, 4, 5
          break;
        }
      }
      if (live != base) return false;  // nothing leaked
    }
  }
  return true;
}

// [unord.req.except]/4: a rehash that fails (by allocation) has no effect; nor does an
// insertion whose rehash fails ([unord.req.except]/2).
template <class X>
bool rehash_no_effect() {
  X x = make<X>({1, 2, 3, 4, 5, 6, 7, 8});
  x.max_load_factor(1.0f);
  if (!no_effect_on_failure(x, Trigger::allocation, [](X& c) { c.rehash(c.bucket_count() * 8 + 50); })) return false;
  if (!no_effect_on_failure(x, Trigger::allocation, [](X& c) { c.reserve(c.bucket_count() * 9 + 70); })) return false;
  // fill up to the load factor; the next insertion has to rehash
  X y = make<X>({});
  y.max_load_factor(1.0f);
  int k = 0;
  while (static_cast<float>(y.size() + 1) <= y.max_load_factor() * static_cast<float>(y.bucket_count()))
    emplace_key(y, k++);
  return no_effect_on_failure(y, Trigger::allocation, [&](X& c) { emplace_key(c, k); }) &&
         y.count(TKey(k)) == 1;
}

}  // namespace reqs::assoc_except
