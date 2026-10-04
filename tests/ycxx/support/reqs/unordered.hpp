// Generic checks for the unordered associative containers (unordered_set, unordered_multiset,
// unordered_map, unordered_multimap), written from [unord.req.general] and [container.node].
// Each function is a template over the container type X and is instantiated by the tests in
// tests/ycxx/unordered_{map,set}. Elements are built as in reqs/associative.hpp: kv<X>(k) has
// key val<key_type>(k) (and, for maps, mapped value val<mapped_type>(m)). The hash functions
// below are constexpr so that the checks can also run in constant expressions.
#pragma once
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_iterators.hpp"
#include "reqs/associative.hpp"

namespace reqs::unordered {

using reqs::associative::is_map;
using reqs::associative::is_multi;
using reqs::associative::key;
using reqs::associative::key_of;
using reqs::associative::kv;

// A constexpr hash for int, long and Elem keys. The `salt` member makes it stateful: equal
// salts give equal hashes, so containers with differently salted hashers order their
// elements differently while agreeing on which keys are equal.
struct Hash {
  std::size_t salt = 0;
  constexpr Hash() = default;
  constexpr explicit Hash(std::size_t s) : salt(s) {}
  constexpr std::size_t operator()(long v) const { return static_cast<std::size_t>(v) * 2654435761u + salt; }
  constexpr std::size_t operator()(const Elem& e) const { return (*this)(static_cast<long>(e.value())); }
};
// A stateful equality predicate (the state is only observed through key_eq()).
struct Eq {
  int tag = 0;
  constexpr Eq() = default;
  constexpr explicit Eq(int t) : tag(t) {}
  template <class T>
  constexpr bool operator()(const T& a, const T& b) const {
    return a == b;
  }
};

template <class X>
constexpr X mk(std::initializer_list<int> ks) {
  X c;
  for (int k : ks) c.insert(kv<X>(k));
  return c;
}

// Number of elements with key key(k), by iteration.
template <class X>
constexpr std::size_t count_key(const X& c, int k) {
  std::size_t n = 0;
  for (const auto& v : c) n += key_of<X>(v) == key<X>(k) ? 1 : 0;
  return n;
}

// The elements with equivalent keys are adjacent in the iteration order ([unord.req.general]/6).
// Every one of the `distinct` keys of c starts at least one run of equal keys in the
// iteration order, so the groups are adjacent iff there are exactly `distinct` runs. One
// pass, cheap enough to call after every insertion in a constant expression.
template <class X>
constexpr bool groups_adjacent(const X& c, std::size_t distinct) {
  std::size_t runs = 0;
  auto prev = c.cbegin();
  for (auto i = c.cbegin(); i != c.cend(); prev = i, ++i)
    if (i == c.cbegin() || !(key_of<X>(*prev) == key_of<X>(*i))) ++runs;
  return runs == distinct;
}

// [unord.req.general]/7-23, /241: member types; forward iterators, constant for the sets;
// local_iterator / const_local_iterator have the category, value, difference, pointer and
// reference types of iterator / const_iterator; node_type and insert_return_type.
template <class X>
constexpr bool types() {
  using K = typename X::key_type;
  using V = typename X::value_type;
  using It = typename X::iterator;
  using CIt = typename X::const_iterator;
  using LIt = typename X::local_iterator;
  using CLIt = typename X::const_local_iterator;
  static_assert(std::is_same_v<typename X::reference, V&> && std::is_same_v<typename X::const_reference, const V&>);
  static_assert(std::forward_iterator<It> && std::forward_iterator<CIt>);
  static_assert(std::is_convertible_v<It, CIt>);
  static_assert(std::is_same_v<std::iter_reference_t<CIt>, const V&>);
  if constexpr (is_map<X>) {
    static_assert(std::is_same_v<V, std::pair<const K, typename X::mapped_type>>);
    static_assert(std::is_same_v<std::iter_reference_t<It>, V&>);
  } else {
    static_assert(std::is_same_v<V, K>);
    static_assert(std::is_same_v<std::iter_reference_t<It>, const K&>);
  }
  using TI = std::iterator_traits<It>;
  using TL = std::iterator_traits<LIt>;
  using TC = std::iterator_traits<CIt>;
  using TCL = std::iterator_traits<CLIt>;
  static_assert(std::is_same_v<typename TL::iterator_category, typename TI::iterator_category>);
  static_assert(std::is_same_v<typename TL::value_type, typename TI::value_type>);
  static_assert(std::is_same_v<typename TL::difference_type, typename TI::difference_type>);
  static_assert(std::is_same_v<typename TL::pointer, typename TI::pointer>);
  static_assert(std::is_same_v<typename TL::reference, typename TI::reference>);
  static_assert(std::is_same_v<typename TCL::iterator_category, typename TC::iterator_category>);
  static_assert(std::is_same_v<typename TCL::value_type, typename TC::value_type>);
  static_assert(std::is_same_v<typename TCL::reference, typename TC::reference>);
  static_assert(std::is_same_v<typename TCL::pointer, typename TC::pointer>);
  static_assert(std::is_same_v<typename X::allocator_type::value_type, V>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>().hash_function()), typename X::hasher>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>().key_eq()), typename X::key_equal>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>().load_factor()), float>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>().max_load_factor()), float>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().begin(0)), LIt>);
  static_assert(std::is_same_v<decltype(std::declval<const X&>().begin(0)), CLIt>);
  static_assert(std::is_same_v<decltype(std::declval<X&>().cend(0)), CLIt>);
  using N = typename X::node_type;
  static_assert(std::is_same_v<typename N::allocator_type, typename X::allocator_type>);
  if constexpr (is_map<X>) {
    static_assert(std::is_same_v<typename N::key_type, K> && std::is_same_v<typename N::mapped_type, typename X::mapped_type>);
  } else {
    static_assert(std::is_same_v<typename N::value_type, V>);
  }
  if constexpr (is_multi<X>) {
    static_assert(!requires { typename X::insert_return_type; });
  } else {
    using R = typename X::insert_return_type;
    static_assert(std::is_same_v<decltype(R::position), It> && std::is_same_v<decltype(R::inserted), bool> &&
                  std::is_same_v<decltype(R::node), N>);
  }
  return true;
}

// [unord.req.general]/24-71, [unord.map.cnstr]: the constructors with bucket count, hash
// function and predicate, from iterator ranges, ranges and initializer lists; "at least n
// buckets"; max_load_factor() is 1.0 after construction; copy construction and copy
// assignment copy the hash function, predicate and maximum load factor; a = il.
// X must have Hash and Eq as hasher and key_equal.
template <class X>
constexpr bool construct() {
  using V = typename X::value_type;
  const Hash hf(7);
  const Eq eq(3);
  V arr[] = {kv<X>(5), kv<X>(1), kv<X>(3), kv<X>(1, 30), kv<X>(9)};
  const std::size_t n = is_multi<X> ? 5 : 4;
  X a;
  if (!a.empty() || a.max_load_factor() != 1.0f) return false;
  X b(50);
  if (b.bucket_count() < 50 || b.max_load_factor() != 1.0f) return false;
  X c(20, hf);
  if (c.bucket_count() < 20 || c.hash_function().salt != 7 || c.key_eq().tag != 0) return false;
  X d(20, hf, eq);
  if (d.hash_function().salt != 7 || d.key_eq().tag != 3) return false;
  X e(arr, arr + 5);
  if (e.size() != n || count_key(e, 1) != (is_multi<X> ? 2u : 1u) || count_key(e, 9) != 1) return false;
  X f(InputIter<V>(arr), InputIter<V>(arr + 5), 64, hf, eq);
  if (f.size() != n || f.bucket_count() < 64 || f.hash_function().salt != 7 || f.key_eq().tag != 3) return false;
  if (f.max_load_factor() != 1.0f) return false;
  X g(std::from_range, InputRange<V>{arr, arr + 5});
  X h(std::from_range, ForwardRange<V>{arr, arr + 5}, 30, hf, eq);
  if (g.size() != n || h.size() != n || h.bucket_count() < 30 || h.key_eq().tag != 3) return false;
  X i{kv<X>(5), kv<X>(1), kv<X>(3), kv<X>(1, 30), kv<X>(9)};
  X j({kv<X>(2), kv<X>(2)}, 40, hf, eq);
  if (i.size() != n || j.size() != (is_multi<X> ? 2u : 1u) || j.bucket_count() < 40 || j.hash_function().salt != 7)
    return false;
  if (!(i == e) || !(g == e) || !(h == e)) return false;
  // copies carry hash function, predicate and maximum load factor
  h.max_load_factor(0.5f);
  const float mlf = h.max_load_factor();
  X cp(h);
  if (cp.hash_function().salt != 7 || cp.key_eq().tag != 3 || cp.max_load_factor() != mlf || !(cp == h)) return false;
  X as;
  X& r = (as = h);
  if (&r != &as || as.hash_function().salt != 7 || as.key_eq().tag != 3 || as.max_load_factor() != mlf) return false;
  X& r2 = (as = {kv<X>(4), kv<X>(4), kv<X>(6)});
  if (&r2 != &as || as.size() != (is_multi<X> ? 3u : 2u) || count_key(as, 4) != (is_multi<X> ? 2u : 1u)) return false;
  X mv(std::move(cp));
  return mv.hash_function().salt == 7 && mv.key_eq().tag == 3 && mv.size() == n;
}

// [unord.req.general]/78-114: emplace / insert results for unique and equivalent keys,
// hinted insertion, range and initializer-list insertion; elements with equivalent keys
// stay adjacent ([unord.req.general]/6).
template <class X>
constexpr bool insert_emplace() {
  using V = typename X::value_type;
  X a;
  const V t5 = kv<X>(5);
  if constexpr (is_multi<X>) {
    auto i1 = a.insert(t5);
    auto i2 = a.emplace(kv<X>(5, 50));
    auto i3 = a.insert(a.cbegin(), kv<X>(5, 60));
    auto i4 = a.emplace_hint(a.cend(), kv<X>(2));
    static_assert(std::is_same_v<decltype(i1), typename X::iterator>);
    if (!(key_of<X>(*i1) == key<X>(5)) || !(key_of<X>(*i4) == key<X>(2)) || i2 == i1 || i3 == i2) return false;
    if (a.size() != 4 || count_key(a, 5) != 3) return false;
    if constexpr (is_map<X>) {
      if (!(i2->second == val<typename X::mapped_type>(50)) || !(i3->second == val<typename X::mapped_type>(60)))
        return false;
    }
  } else {
    auto [i1, b1] = a.insert(t5);
    auto [i2, b2] = a.emplace(kv<X>(5, 50));
    if (!b1 || b2 || i1 != i2) return false;
    auto i3 = a.insert(a.cbegin(), kv<X>(5, 60));
    auto i4 = a.emplace_hint(a.cend(), kv<X>(2));
    if (i3 != i1 || !(key_of<X>(*i4) == key<X>(2)) || a.size() != 2) return false;
    if constexpr (is_map<X>) {
      if (!(i1->second == val<typename X::mapped_type>(6))) return false;  // the first value is kept
    }
  }
  V arr[] = {kv<X>(7), kv<X>(1), kv<X>(5, 70)};
  a.insert(arr, arr + 3);
  a.insert_range(InputRange<V>{arr, arr + 1});
  a.insert({kv<X>(0), kv<X>(9)});
  if (a.size() != (is_multi<X> ? 10u : 6u)) return false;
  if (count_key(a, 7) != (is_multi<X> ? 2u : 1u)) return false;
  X big;
  for (int i = 0; i < 60; ++i) {
    big.insert(kv<X>(i % 30));
    if (!groups_adjacent(big, i < 30 ? i + 1 : 30)) return false;  // keys 0 .. min(i, 29)
  }
  if (big.size() != (is_multi<X> ? 60u : 30u)) return false;
  return groups_adjacent(a, 6);  // keys 0, 1, 2, 5, 7, 9
}

// [unord.req.general]/172-191: find, count, contains, equal_range on X and const X.
template <class X>
constexpr bool lookup() {
  using It = typename X::iterator;
  using CIt = typename X::const_iterator;
  X a = mk<X>({1, 3, 3, 5, 7, 7, 7, 9});
  const X& ca = a;
  static_assert(std::is_same_v<decltype(a.find(key<X>(1))), It> && std::is_same_v<decltype(ca.find(key<X>(1))), CIt>);
  static_assert(std::is_same_v<decltype(a.equal_range(key<X>(1))), std::pair<It, It>>);
  static_assert(std::is_same_v<decltype(ca.equal_range(key<X>(1))), std::pair<CIt, CIt>>);
  static_assert(std::is_same_v<decltype(ca.count(key<X>(1))), typename X::size_type>);
  static_assert(std::is_same_v<decltype(ca.contains(key<X>(1))), bool>);
  for (int q = 0; q <= 10; ++q) {
    const auto kq = key<X>(q);
    const std::size_t expected = count_key(a, q);
    if (ca.count(kq) != expected || ca.contains(kq) != (expected != 0)) return false;
    auto f = a.find(kq);
    if (expected == 0 ? f != a.end() : !(key_of<X>(*f) == kq)) return false;
    auto [lo, hi] = ca.equal_range(kq);
    if (static_cast<std::size_t>(std::distance(lo, hi)) != expected) return false;
    if (expected == 0 && (lo != ca.end() || hi != ca.end())) return false;
    for (auto it = lo; it != hi; ++it)
      if (!(key_of<X>(*it) == kq)) return false;
  }
  return true;
}

// [unord.req.general]/148-171, /242: erase(k) returns the number erased, erase(q) / erase(r)
// return the iterator that followed the erased element, erase(q1, q2) returns the iterator
// following the erased range; erasure keeps the relative order of the remaining elements and
// invalidates only the erased elements; clear().
template <class X>
constexpr bool erase() {
  X a = mk<X>({1, 2, 2, 3, 4, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  const std::size_t total = a.size();
  // remember the order
  const typename X::value_type* order[20];
  int n = 0;
  for (auto& v : a) order[n++] = std::addressof(v);
  auto check_order = [&](const X& x) {
    int k = 0;
    for (auto& v : x) {
      while (k < n && order[k] != std::addressof(v)) ++k;
      if (k == n) return false;
      ++k;
    }
    return true;
  };
  if (a.erase(key<X>(4)) != (is_multi<X> ? 3u : 1u) || a.erase(key<X>(4)) != 0 || !check_order(a)) return false;
  auto first = a.begin();
  auto second = std::next(first);
  auto r = a.erase(first);
  if (r != second || !check_order(a)) return false;
  typename X::const_iterator q = std::next(a.cbegin(), 2);
  auto after = std::next(q);
  auto r2 = a.erase(q);
  if (r2 != after || !check_order(a)) return false;
  auto q1 = std::next(a.cbegin());
  auto q2 = std::next(q1, 3);
  auto r3 = a.erase(q1, q2);
  if (r3 != q2 || a.size() != total - (is_multi<X> ? 3 : 1) - 5 || !check_order(a)) return false;
  auto r4 = a.erase(a.cbegin(), a.cbegin());
  if (r4 != a.begin()) return false;
  a.clear();
  return a.empty() && a.begin() == a.end() && a.size() == 0;
}

// [unord.req.general]/9, /192-239: bucket interface and load factor. Every element is in
// bucket(key) and is reached through begin(n)/end(n) of that bucket; bucket sizes add up
// to size(); empty buckets have begin(n) == end(n); load_factor() is size() / bucket_count();
// the container keeps load_factor() <= max_load_factor() as it grows; rehash(n) and
// reserve(n) meet their postconditions; rehashing keeps references valid and the relative
// order of equivalent elements.
template <class X>
constexpr bool buckets() {
  X a;
  a.max_load_factor(0.75f);
  const float mlf = a.max_load_factor();
  if (!(mlf > 0.0f)) return false;
  for (int i = 0; i < 50; ++i) {
    a.insert(kv<X>(i % 40, i));
    if (a.load_factor() > a.max_load_factor()) return false;
  }
  if (a.max_bucket_count() < a.bucket_count()) return false;
  const auto check = [](const X& c) {
    std::size_t total = 0;
    for (std::size_t n = 0; n < c.bucket_count(); ++n) {
      std::size_t in_bucket = 0;
      for (auto it = c.begin(n); it != c.end(n); ++it) {
        ++in_bucket;
        if (c.bucket(key_of<X>(*it)) != n) return false;
      }
      if (in_bucket != c.bucket_size(n)) return false;
      if (in_bucket == 0 && c.cbegin(n) != c.cend(n)) return false;
      total += in_bucket;
    }
    if (total != c.size()) return false;
    for (const auto& v : c) {
      std::size_t b = c.bucket(key_of<X>(v));
      if (b >= c.bucket_count()) return false;
      bool found = false;
      for (auto it = c.begin(b); it != c.end(b); ++it) found = found || std::addressof(*it) == std::addressof(v);
      if (!found) return false;
    }
    float lf = static_cast<float>(c.size()) / static_cast<float>(c.bucket_count());
    float diff = c.load_factor() - lf;
    return diff < 0.001f && diff > -0.001f;
  };
  if (!check(a)) return false;
  // references survive rehashing; equivalent elements keep their relative order
  const typename X::value_type* addr[50];
  int n = 0;
  for (auto& v : a) addr[n++] = std::addressof(v);
  const auto group_order_kept = [&](const X& c) {
    // within each key group the elements appear in the same relative order as in addr
    for (int k = 0; k < 40; ++k) {
      auto [lo, hi] = c.equal_range(key<X>(k));
      int last = -1;
      for (auto it = lo; it != hi; ++it) {
        int pos = 0;
        while (pos < n && addr[pos] != std::addressof(*it)) ++pos;
        if (pos == n || pos < last) return false;
        last = pos;
      }
    }
    return true;
  };
  a.rehash(a.bucket_count() * 4 + 3);
  if (!check(a) || !group_order_kept(a)) return false;
  a.rehash(0);
  if (a.bucket_count() < a.size() / a.max_load_factor()) return false;
  a.reserve(500);
  if (a.bucket_count() < 500 / a.max_load_factor() - 1) return false;
  a.rehash(1000);
  if (a.bucket_count() < 1000) return false;
  if (!check(a) || !group_order_kept(a)) return false;
  int found = 0;
  for (auto& v : a)
    for (int k = 0; k < n; ++k) found += std::addressof(v) == addr[k] ? 1 : 0;
  if (found != n) return false;
  X e;
  if (e.bucket_count() > 0) {
    for (std::size_t b = 0; b < e.bucket_count(); ++b)
      if (e.bucket_size(b) != 0 || e.begin(b) != e.end(b)) return false;
  }
  return e.load_factor() == 0.0f;
}

// [unord.req.general]/242-243: insertion keeps references valid, and keeps iterators valid
// when (N + n) <= z * B (no rehash needed).
template <class X>
constexpr bool validity() {
  X a;
  a.reserve(100);
  const std::size_t B = a.bucket_count();
  for (int i = 0; i < 10; ++i) a.insert(kv<X>(i));
  auto it3 = a.find(key<X>(3));
  const auto* p3 = std::addressof(*it3);
  const auto end_before = a.end();
  (void)end_before;
  for (int i = 10; i < 40; ++i) a.insert(kv<X>(i));  // 40 <= 1.0 * B, B >= 100
  if (a.bucket_count() != B) return false;  // no rehash happened
  if (std::addressof(*it3) != p3 || !(key_of<X>(*it3) == key<X>(3))) return false;
  int steps = 0;
  for (auto it = it3; it != a.end() && steps <= 40; ++it) ++steps;
  if (steps > 40 || steps < 1) return false;
  for (int i = 40; i < 300; ++i) a.insert(kv<X>(i % 80));
  auto [lo, hi] = a.equal_range(key<X>(3));
  for (auto it = lo; it != hi; ++it)
    if (std::addressof(*it) == p3) return true;
  return false;
}

// [unord.req.general]/240: a == b iff same size and every equivalent-key group of a is a
// permutation of the matching group of b, independent of bucket counts, hash function salt
// or insertion order.
template <class X>
constexpr bool equality() {
  X a(5, Hash(1));
  X b(200, Hash(99));
  for (int i = 0; i < 30; ++i) a.insert(kv<X>(i));
  for (int i = 29; i >= 0; --i) b.insert(kv<X>(i));
  if (!(a == b) || a != b || !(b == a)) return false;
  b.erase(key<X>(7));
  if (a == b || !(a != b)) return false;
  b.insert(kv<X>(7, 70));
  if constexpr (is_map<X>) {
    if (a == b) return false;  // same key, different mapped value
  } else {
    if (!(a == b)) return false;
  }
  if constexpr (is_multi<X>) {
    X m1, m2;
    for (int v : {1, 2, 3}) m1.insert(kv<X>(4, v));
    for (int v : {3, 1, 2}) m2.insert(kv<X>(4, v));
    if (!(m1 == m2)) return false;  // permutations of the same group
    m2.insert(kv<X>(4, 1));
    if (m1 == m2) return false;
    m1.insert(kv<X>(4, 2));
    if constexpr (is_map<X>) {
      if (m1 == m2) return false;  // groups {1,2,3,2} vs {3,1,2,1}
    } else {
      if (!(m1 == m2)) return false;
    }
  }
  X e1, e2(100);
  return e1 == e2;
}

// erase_if ([unord.set.erasure] etc.): equivalent to erasing every element for which pred
// is true, returning the number erased.
template <class X>
constexpr bool erase_if() {
  X a = mk<X>({1, 2, 3, 4, 5, 6, 7, 8});
  if constexpr (is_multi<X>) a.insert(kv<X>(4, 9));
  int calls = 0;
  auto n = std::erase_if(a, [&](const typename X::value_type& v) {
    ++calls;
    return key_of<X>(v) == key<X>(4) || key_of<X>(v) == key<X>(7);
  });
  static_assert(std::is_same_v<decltype(n), typename X::size_type>);
  return n == (is_multi<X> ? 3u : 2u) && calls == (is_multi<X> ? 9 : 8) && a.size() == 6 && !a.contains(key<X>(4));
}

// [unord.req.general]/115-147, /244, [container.node]: node handles and merge.
template <class X>
bool node_handles() {
  using N = typename X::node_type;
  X a = mk<X>({1, 2, 3, 4});
  auto i2 = a.find(key<X>(2));
  const auto* p2 = std::addressof(*i2);
  N nh = a.extract(i2);
  if (nh.empty() || a.size() != 3 || a.contains(key<X>(2)) || !(nh.get_allocator() == a.get_allocator())) return false;
  if constexpr (is_map<X>) {
    if (std::addressof(nh.key()) != std::addressof(p2->first)) return false;
    nh.key() = key<X>(9);
  } else {
    if (std::addressof(nh.value()) != p2) return false;
    nh.value() = key<X>(9);
  }
  if constexpr (is_multi<X>) {
    auto it = a.insert(std::move(nh));
    if (!nh.empty() || std::addressof(*it) != p2) return false;
  } else {
    auto r = a.insert(std::move(nh));
    if (!r.inserted || std::addressof(*r.position) != p2 || !r.node.empty() || !nh.empty()) return false;
    N empty;
    auto r2 = a.insert(std::move(empty));
    if (r2.inserted || r2.position != a.end() || !r2.node.empty()) return false;
    X b = mk<X>({3});
    auto r3 = a.insert(b.extract(key<X>(3)));
    if (r3.inserted || r3.node.empty() || !(key_of<X>(*r3.position) == key<X>(3))) return false;
    auto h = a.insert(a.cbegin(), std::move(r3.node));
    if (r3.node.empty() || !(key_of<X>(*h) == key<X>(3))) return false;  // failed: unchanged
  }
  if (!a.contains(key<X>(9)) || a.size() != 4) return false;
  if (!a.extract(key<X>(7)).empty()) return false;
  N n4 = a.extract(key<X>(4));
  if (n4.empty() || a.size() != 3) return false;
  auto it = a.insert(a.cbegin(), std::move(n4));
  if (!n4.empty() || !(key_of<X>(*it) == key<X>(4)) || a.insert(a.cbegin(), N()) != a.end()) return false;
  return a.size() == 4;
}

template <class X, class Y>
bool merge() {
  X a = mk<X>({1, 3, 5});
  Y b(10, Hash(5));
  for (int k : {2, 3, 4, 5, 5, 6}) b.insert(kv<Y>(k, 50 + k));
  const auto* p2 = std::addressof(*b.find(key<Y>(2)));
  const std::size_t bsize = b.size();
  a.merge(b);
  if constexpr (is_multi<X>) {
    if (!b.empty() || a.size() != 3 + bsize) return false;
  } else {
    if (a.size() != 6 || b.size() != bsize - 3) return false;
    if (b.count(key<Y>(3)) != 1 || b.count(key<Y>(2)) != 0) return false;
  }
  if (std::addressof(*a.find(key<X>(2))) != p2) return false;
  if constexpr (is_map<X>) {
    if (!(a.find(key<X>(2))->second == val<typename X::mapped_type>(52))) return false;
  }
  Y c;
  c.insert(kv<Y>(0));
  a.merge(std::move(c));
  return c.empty() && a.contains(key<X>(0));
}

}  // namespace reqs::unordered
