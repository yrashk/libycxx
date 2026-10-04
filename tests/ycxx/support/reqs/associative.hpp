// Generic checks for the associative containers (set, multiset, map, multimap), written from
// [associative.reqmts.general] and [container.node]. Each function is a template over the
// container type X and is instantiated by the per-container tests in tests/ycxx/{map,set}.
// Elements are built from small integer indices: kv<X>(k) is the value_type with key
// val<key_type>(k) (and, for maps, mapped value val<mapped_type>(m)), so keys are ordered
// like their indices under less<>.
#pragma once
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <ranges>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "test_iterators.hpp"

namespace reqs::associative {

template <class X>
concept is_map = requires { typename X::mapped_type; };

// Node-based containers (map, set, ...) keep iterators valid across insertion; the flat
// containers do not ([flat.map.overview]/2.2).
template <class X>
concept node_based = requires { typename X::node_type; };

template <class X>
concept is_multi = std::is_same_v<decltype(std::declval<X&>().insert(std::declval<const typename X::value_type&>())),
                                  typename X::iterator>;

template <class X>
constexpr typename X::value_type kv(int k, int m = -1) {
  using V = typename X::value_type;
  if constexpr (is_map<X>)
    return V(val<typename X::key_type>(k), val<typename X::mapped_type>(m < 0 ? k + 1 : m));
  else
    return val<typename X::key_type>(k);
}

template <class X>
constexpr typename X::key_type key(int k) {
  return val<typename X::key_type>(k);
}

template <class X>
constexpr const typename X::key_type& key_of(const typename X::value_type& v) {
  if constexpr (is_map<X>) return v.first;
  else return v;
}

// keys_are(c, {k0, k1, ...}): iteration yields exactly the keys key(k0), key(k1), ...
template <class X>
constexpr bool keys_are(const X& c, std::initializer_list<int> ks) {
  auto it = c.begin();
  for (int k : ks) {
    if (it == c.end() || !(key_of<X>(*it) == key<X>(k))) return false;
    ++it;
  }
  return it == c.end() && c.size() == ks.size();
}

// for maps: the mapped values in iteration order are val(m0), val(m1), ...
template <class X>
constexpr bool mapped_are(const X& c, std::initializer_list<int> ms) {
  auto it = c.begin();
  for (int m : ms) {
    if (it == c.end() || !(it->second == val<typename X::mapped_type>(m))) return false;
    ++it;
  }
  return it == c.end();
}

template <class X>
constexpr X mk(std::initializer_list<int> ks) {
  X c;
  for (int k : ks) c.insert(kv<X>(k));
  return c;
}

// [associative.reqmts.general]/6, /8-17 and the synopses: member types.
template <class X>
constexpr bool types() {
  using K = typename X::key_type;
  using V = typename X::value_type;
  using It = typename X::iterator;
  using CIt = typename X::const_iterator;
  static_assert(std::is_same_v<typename X::reference, V&> && std::is_same_v<typename X::const_reference, const V&>);
  if constexpr (is_map<X>) {
    static_assert(std::is_same_v<V, std::pair<const K, typename X::mapped_type>>);
    static_assert(std::is_same_v<std::iter_reference_t<It>, V&>);
    static_assert(std::is_invocable_r_v<bool, typename X::value_compare, const V&, const V&>);
    static_assert(!std::is_default_constructible_v<typename X::value_compare>);
  } else {
    static_assert(std::is_same_v<V, K>);
    static_assert(std::is_same_v<typename X::value_compare, typename X::key_compare>);
    // both iterators are constant iterators
    static_assert(std::is_same_v<std::iter_reference_t<It>, const K&>);
    static_assert(!std::indirectly_writable<It, const K&>);
  }
  static_assert(std::is_same_v<std::iter_reference_t<CIt>, const V&>);
  static_assert(std::bidirectional_iterator<It> && std::bidirectional_iterator<CIt>);
  static_assert(!std::random_access_iterator<It>);
  static_assert(std::derived_from<typename std::iterator_traits<It>::iterator_category, std::bidirectional_iterator_tag>);
  static_assert(std::is_convertible_v<It, CIt>);
  static_assert(std::is_same_v<typename X::reverse_iterator, std::reverse_iterator<It>>);
  static_assert(std::is_same_v<typename X::const_reverse_iterator, std::reverse_iterator<CIt>>);
  static_assert(std::is_same_v<typename X::allocator_type::value_type, V>);
  static_assert(std::is_signed_v<typename X::difference_type> && std::is_unsigned_v<typename X::size_type>);
  static_assert(std::is_same_v<typename X::difference_type, std::iter_difference_t<It>>);
  using N = typename X::node_type;
  static_assert(std::is_same_v<typename N::allocator_type, typename X::allocator_type>);
  if constexpr (is_map<X>) {
    static_assert(std::is_same_v<typename N::key_type, K>);
    static_assert(std::is_same_v<typename N::mapped_type, typename X::mapped_type>);
    static_assert(!requires { typename N::value_type; });
  } else {
    static_assert(std::is_same_v<typename N::value_type, V>);
    static_assert(!requires { typename N::key_type; } && !requires { typename N::mapped_type; });
  }
  static_assert(std::is_nothrow_default_constructible_v<N> && std::is_nothrow_move_constructible_v<N>);
  static_assert(!std::is_copy_constructible_v<N> && !std::is_copy_assignable_v<N>);
  if constexpr (is_multi<X>) {
    static_assert(!requires { typename X::insert_return_type; });
    static_assert(std::is_same_v<decltype(std::declval<X&>().insert(std::declval<N>())), It>);
  } else {
    using R = typename X::insert_return_type;
    static_assert(std::is_same_v<decltype(std::declval<X&>().insert(std::declval<N>())), R>);
    static_assert(std::is_same_v<decltype(R::position), It>);
    static_assert(std::is_same_v<decltype(R::inserted), bool>);
    static_assert(std::is_same_v<decltype(R::node), N>);
    static_assert(std::is_aggregate_v<R>);
  }
  return true;
}

// [associative.reqmts.general]/18-36 (with the default comparator): X(), X(i, j),
// X(from_range, rg), X(il), copy; a = il; sorted iteration and unique / equivalent keys.
template <class X>
constexpr bool construct() {
  using V = typename X::value_type;
  V arr[] = {kv<X>(5), kv<X>(1), kv<X>(3), kv<X>(1, 30), kv<X>(9)};
  const bool multi = is_multi<X>;
  X a(arr, arr + 5);
  if (multi ? !keys_are(a, {1, 1, 3, 5, 9}) : !keys_are(a, {1, 3, 5, 9})) return false;
  if constexpr (is_map<X>) {
    if (multi ? !mapped_are(a, {2, 30, 4, 6, 10}) : !mapped_are(a, {2, 4, 6, 10})) return false;
  }
  X b(InputIter<V>(arr), InputIter<V>(arr + 5));
  if (b != a) return false;
  X c(std::from_range, InputRange<V>{arr, arr + 5});
  if (c != a) return false;
  X d(std::from_range, ForwardRange<V>{arr, arr + 5});
  if (d != a) return false;
  X e{kv<X>(5), kv<X>(1), kv<X>(3), kv<X>(1, 30), kv<X>(9)};
  if (e != a) return false;
  X f(arr, arr + 5, typename X::key_compare());
  if (f != a) return false;
  X g({kv<X>(2), kv<X>(2)}, typename X::key_compare());
  if (g.size() != (multi ? 2u : 1u)) return false;
  X h;
  if (!h.empty() || h.begin() != h.end()) return false;
  X& r = (h = {kv<X>(7), kv<X>(4), kv<X>(7)});
  if (&r != &h || (multi ? !keys_are(h, {4, 7, 7}) : !keys_are(h, {4, 7}))) return false;
  h = {};
  if (!h.empty()) return false;
  // sorted input of many elements (the linear-time path) and reverse-sorted input
  auto many = std::views::iota(0, 60) | std::views::transform([](int i) { return kv<X>(i); });
  X s(many.begin(), many.end());
  int k = 0;
  for (const auto& v : s)
    if (!(key_of<X>(v) == key<X>(k++))) return false;
  X rs(std::from_range, many | std::views::reverse);
  return rs == s && s.size() == 60;
}

// [associative.reqmts.general]/47-69, /75-83: emplace, insert(t), insert(i, j),
// insert_range(rg), insert(il). For unique keys the result is pair<iterator, bool> and an
// existing equivalent element is left alone; for equivalent keys the result is an iterator
// and the new element goes at the end of the range of equivalent elements (/4: insert and
// emplace preserve the relative ordering of equivalent elements).
template <class X>
constexpr bool insert_emplace() {
  using V = typename X::value_type;
  X a;
  const V t5 = kv<X>(5);
  if constexpr (is_multi<X>) {
    auto i1 = a.insert(t5);
    auto i2 = a.insert(kv<X>(5, 50));
    auto i3 = a.emplace(kv<X>(5, 60));
    auto i4 = a.emplace(kv<X>(2));
    static_assert(std::is_same_v<decltype(i1), typename X::iterator>);
    if (!keys_are(a, {2, 5, 5, 5}) || i4 != a.begin()) return false;
    if constexpr (node_based<X>) {
      if (std::next(i1) != i2 || std::next(i2) != i3 || std::next(i3) != a.end()) return false;
    }
    if constexpr (is_map<X>) {
      if (!mapped_are(a, {3, 6, 50, 60})) return false;
    }
  } else {
    auto [i1, b1] = a.insert(t5);
    if (!b1 || i1 != a.begin()) return false;
    auto [i2, b2] = a.insert(kv<X>(5, 50));
    if (b2 || i2 != i1) return false;
    auto [i3, b3] = a.emplace(kv<X>(5, 60));
    if (b3 || i3 != i1) return false;
    auto [i4, b4] = a.emplace(kv<X>(2));
    if (!b4 || i4 != a.begin()) return false;
    static_assert(std::is_same_v<decltype(a.insert(t5)), std::pair<typename X::iterator, bool>>);
    static_assert(std::is_same_v<decltype(a.emplace(t5)), std::pair<typename X::iterator, bool>>);
    if (!keys_are(a, {2, 5})) return false;
    if constexpr (is_map<X>) {
      if (!mapped_are(a, {3, 6})) return false;  // the existing value was kept
    }
  }
  V arr[] = {kv<X>(7), kv<X>(1), kv<X>(5, 70)};
  static_assert(std::is_same_v<decltype(a.insert(arr, arr)), void>);
  static_assert(std::is_same_v<decltype(a.insert_range(arr)), void>);
  a.insert(arr, arr + 3);
  if (is_multi<X> ? !keys_are(a, {1, 2, 5, 5, 5, 5, 7}) : !keys_are(a, {1, 2, 5, 7})) return false;
  if constexpr (is_map<X> && is_multi<X>) {
    if (!mapped_are(a, {2, 3, 6, 50, 60, 70, 8})) return false;
  }
  X b;
  b.insert_range(InputRange<V>{arr, arr + 3});
  b.insert_range(ForwardRange<V>{arr, arr + 1});
  b.insert({kv<X>(0), kv<X>(9)});
  if (is_multi<X> ? !keys_are(b, {0, 1, 5, 7, 7, 9}) : !keys_are(b, {0, 1, 5, 7, 9})) return false;
  // many elements, in a scrambled order
  X c;
  for (int i = 0; i < 64; ++i) c.insert(kv<X>((i * 37) % 64));
  int k = 0;
  for (const auto& v : c)
    if (!(key_of<X>(v) == key<X>(k++))) return false;
  return k == 64;
}

// [associative.reqmts.general]/57-60, /70-74: a.insert(p, t) and a.emplace_hint(p, args)
// return an iterator to the element with the key of t; for unique keys nothing is inserted
// if the key exists; the element is inserted "as close as possible to the position just
// prior to p", which for equivalent keys fixes its place among the equivalent elements.
template <class X>
constexpr bool hint() {
  X a = mk<X>({2, 4, 6});
  auto it = a.insert(std::next(a.cbegin()), kv<X>(3));  // correct hint
  if (!(key_of<X>(*it) == key<X>(3)) || std::prev(it) != a.begin()) return false;
  it = a.insert(a.cbegin(), kv<X>(5));  // wrong hint
  if (!(key_of<X>(*it) == key<X>(5)) || std::next(it, 2) != a.end()) return false;
  it = a.emplace_hint(a.cend(), kv<X>(7));
  if (std::next(it) != a.end() || !keys_are(a, {2, 3, 4, 5, 6, 7})) return false;
  it = a.emplace_hint(a.cbegin(), kv<X>(1));
  if (it != a.begin()) return false;
  if constexpr (is_multi<X>) {
    // equivalent keys: the hint decides the position among the equivalent elements
    X m;
    m.insert(kv<X>(5, 10));
    m.insert(kv<X>(5, 11));
    auto h = m.insert(m.cbegin(), kv<X>(5, 12));  // just before the first 5
    if (h != m.begin()) return false;
    // now 12, 10, 11: hint at 11 puts the new element between 10 and 11
    h = m.emplace_hint(std::next(m.cbegin(), 2), kv<X>(5, 13));
    if (std::distance(m.begin(), h) != 2) return false;
    h = m.insert(m.cend(), kv<X>(5, 14));  // after all of them
    if (std::next(h) != m.end()) return false;
    if constexpr (is_map<X>) {
      if (!mapped_are(m, {12, 10, 13, 11, 14})) return false;
    }
    if (m.size() != 5 || m.count(key<X>(5)) != 5) return false;
  } else {
    auto before = a.size();
    auto e = a.insert(a.cbegin(), kv<X>(4, 40));
    if (a.size() != before || !(key_of<X>(*e) == key<X>(4))) return false;
    e = a.emplace_hint(a.cend(), kv<X>(2, 40));
    if (a.size() != before || e != std::next(a.begin())) return false;
    if constexpr (is_map<X>) {
      if (!(e->second == val<typename X::mapped_type>(3))) return false;
    }
  }
  return true;
}

// [associative.reqmts.general]/141-174: find, count, contains, lower_bound, upper_bound,
// equal_range, on X and const X (iterator vs const_iterator results).
template <class X>
constexpr bool lookup() {
  using It = typename X::iterator;
  using CIt = typename X::const_iterator;
  using S = typename X::size_type;
  X a = mk<X>({1, 3, 3, 5, 7, 7, 7, 9});
  const X& ca = a;
  const auto k = key<X>(3);
  static_assert(std::is_same_v<decltype(a.find(k)), It> && std::is_same_v<decltype(ca.find(k)), CIt>);
  static_assert(std::is_same_v<decltype(a.lower_bound(k)), It> && std::is_same_v<decltype(ca.lower_bound(k)), CIt>);
  static_assert(std::is_same_v<decltype(a.upper_bound(k)), It> && std::is_same_v<decltype(ca.upper_bound(k)), CIt>);
  static_assert(std::is_same_v<decltype(a.equal_range(k)), std::pair<It, It>>);
  static_assert(std::is_same_v<decltype(ca.equal_range(k)), std::pair<CIt, CIt>>);
  static_assert(std::is_same_v<decltype(ca.count(k)), S> && std::is_same_v<decltype(ca.contains(k)), bool>);
  const bool multi = is_multi<X>;
  for (int q = 0; q <= 10; ++q) {
    const auto kq = key<X>(q);
    S expected = 0;
    for (const auto& v : a) expected += key_of<X>(v) == kq ? 1 : 0;
    if (ca.count(kq) != expected || ca.contains(kq) != (expected != 0)) return false;
    auto f = a.find(kq);
    if (expected == 0 ? f != a.end() : !(key_of<X>(*f) == kq)) return false;
    auto lb = ca.lower_bound(kq);
    auto ub = ca.upper_bound(kq);
    // lower_bound: first element not less than kq; upper_bound: first greater than kq
    for (auto it = ca.begin(); it != lb; ++it)
      if (!(key_of<X>(*it) < kq)) return false;
    if (lb != ca.end() && key_of<X>(*lb) < kq) return false;
    for (auto it = ca.begin(); it != ub; ++it)
      if (kq < key_of<X>(*it)) return false;
    if (ub != ca.end() && !(kq < key_of<X>(*ub))) return false;
    if (static_cast<S>(std::distance(lb, ub)) != expected) return false;
    auto er = ca.equal_range(kq);
    if (er.first != lb || er.second != ub) return false;
    auto mer = a.equal_range(kq);
    if (CIt(mer.first) != lb || CIt(mer.second) != ub) return false;
    // find in a multi container returns one of the equivalent elements
    if (expected != 0 && (CIt(f) == ub || std::distance(lb, CIt(f)) < 0)) return false;
  }
  if (multi && (ca.count(key<X>(7)) != 3 || std::distance(ca.lower_bound(key<X>(7)), ca.upper_bound(key<X>(7))) != 3))
    return false;
  if (!multi && ca.count(key<X>(7)) != 1) return false;
  X e;
  return e.find(k) == e.end() && e.lower_bound(k) == e.end() && e.count(k) == 0;
}

// [associative.reqmts.general]/118-140: erase(k) returns the number erased; erase(q) and
// erase(r) return the iterator following the erased element; erase(q1, q2) returns q2;
// clear(). /175: erase invalidates only the erased elements.
template <class X>
constexpr bool erase() {
  using S = typename X::size_type;
  X a = mk<X>({1, 2, 2, 3, 4, 4, 4, 5, 6});
  const bool multi = is_multi<X>;
  auto keep = a.find(key<X>(5));
  const auto* pkeep = std::addressof(*keep);
  static_assert(std::is_same_v<decltype(a.erase(key<X>(1))), S>);
  if (a.erase(key<X>(4)) != (multi ? 3u : 1u)) return false;
  if (a.erase(key<X>(4)) != 0) return false;
  auto it = a.erase(a.lower_bound(key<X>(2)));  // iterator overload
  if (!(key_of<X>(*it) == key<X>(multi ? 2 : 3))) return false;
  typename X::const_iterator cq = a.find(key<X>(3));
  it = a.erase(cq);  // const_iterator overload
  if (it != keep) return false;
  it = a.erase(std::prev(a.end()));
  if (it != a.end()) return false;
  if (multi ? !keys_are(a, {1, 2, 5}) : !keys_are(a, {1, 5})) return false;
  if (std::addressof(*keep) != pkeep) return false;
  a.insert(kv<X>(8));
  a.insert(kv<X>(9));
  it = a.erase(a.cbegin(), keep);
  if (it != keep || !keys_are(a, {5, 8, 9})) return false;
  it = a.erase(keep, keep);
  if (it != keep || a.size() != 3) return false;
  it = a.erase(std::next(a.cbegin()), a.cend());
  if (it != a.end() || !keys_are(a, {5})) return false;
  a.clear();
  if (!a.empty() || a.begin() != a.end()) return false;
  a.insert(kv<X>(1));
  return a.size() == 1;
}

// [associative.reqmts.general]/175: insertion does not affect the validity of iterators and
// references; erasure invalidates only the erased elements.
template <class X>
constexpr bool stability() {
  X a = mk<X>({10, 20, 30});
  auto i10 = a.begin();
  auto i20 = std::next(i10);
  auto i30 = std::next(i20);
  const auto* p20 = std::addressof(*i20);
  for (int i = 0; i < 50; ++i) a.insert(kv<X>(i));
  if (std::addressof(*i20) != p20 || !(key_of<X>(*i10) == key<X>(10)) || !(key_of<X>(*i30) == key<X>(30))) return false;
  for (int i = 0; i < 50; ++i)
    if (i != 10 && i != 20 && i != 30) a.erase(key<X>(i));
  if (is_multi<X>) {
    a.erase(std::next(i10));
    a.erase(std::next(i20));
    a.erase(std::next(i30));
  }
  if (a.size() != 3 || a.begin() != i10 || std::next(i10) != i20 || std::next(i20) != i30) return false;
  return std::next(i30) == a.end() && std::addressof(*i20) == p20;
}

// [associative.reqmts.general]/84-111, [container.node]: extract(q) / extract(k) remove an
// element into a node handle without copying it; insert(nh) / insert(p, nh) put it back;
// an empty handle inserts nothing; for unique keys a failed insertion returns the handle in
// insert_return_type::node; the key of an extracted map / set element can be changed
// through the handle. Node handles are move-only, empty() / operator bool report
// ownership, and get_allocator() returns the container's allocator.
template <class X>
bool node_handles() {
  using N = typename X::node_type;
  X a = mk<X>({1, 2, 3, 4});
  auto i2 = a.find(key<X>(2));
  const auto* p2 = std::addressof(*i2);
  N nh = a.extract(i2);
  if (nh.empty() || !nh || a.size() != 3 || a.contains(key<X>(2))) return false;
  if (!(nh.get_allocator() == a.get_allocator())) return false;
  if constexpr (is_map<X>) {
    if (std::addressof(nh.key()) != std::addressof(p2->first) || !(nh.mapped() == val<typename X::mapped_type>(3)))
      return false;
    nh.key() = key<X>(9);  // change the key of the extracted element
    nh.mapped() = val<typename X::mapped_type>(40);
  } else {
    if (std::addressof(nh.value()) != p2) return false;
    nh.value() = key<X>(9);
  }
  if constexpr (is_multi<X>) {
    auto it = a.insert(std::move(nh));
    if (!nh.empty() || std::addressof(*it) != p2 || !(key_of<X>(*it) == key<X>(9))) return false;
  } else {
    auto r = a.insert(std::move(nh));
    if (!r.inserted || !r.node.empty() || std::addressof(*r.position) != p2 || !nh.empty()) return false;
    if (std::next(r.position) != a.end()) return false;
  }
  if (!keys_are(a, {1, 3, 4, 9})) return false;
  if constexpr (is_map<X>) {
    if (!(a.find(key<X>(9))->second == val<typename X::mapped_type>(40))) return false;
  }
  // extract by key
  N n3 = a.extract(key<X>(3));
  N none = a.extract(key<X>(7));
  if (n3.empty() || !none.empty() || static_cast<bool>(none) || !keys_are(a, {1, 4, 9})) return false;
  // inserting an empty handle
  if constexpr (is_multi<X>) {
    if (a.insert(std::move(none)) != a.end()) return false;
  } else {
    auto r = a.insert(std::move(none));
    if (r.inserted || r.position != a.end() || !r.node.empty()) return false;
  }
  if (a.insert(a.cbegin(), N()) != a.end() || a.size() != 3) return false;
  // move construction / assignment / swap of handles
  N moved(std::move(n3));
  if (!n3.empty() || moved.empty()) return false;
  N other;
  other = std::move(moved);
  if (!moved.empty() || other.empty()) return false;
  swap(other, moved);
  if (!other.empty() || moved.empty()) return false;
  moved.swap(other);
  // insert with hint
  auto it = a.insert(a.cbegin(), std::move(other));
  if (!(key_of<X>(*it) == key<X>(3)) || !other.empty() || !keys_are(a, {1, 3, 4, 9})) return false;
  // duplicate key
  X b = mk<X>({3, 4});
  N dup = b.extract(b.begin());
  if constexpr (is_multi<X>) {
    a.insert(std::move(dup));
    if (a.count(key<X>(3)) != 2 || !dup.empty()) return false;
  } else {
    auto r = a.insert(std::move(dup));
    if (r.inserted || r.node.empty() || !(key_of<X>(*r.position) == key<X>(3)) || a.count(key<X>(3)) != 1) return false;
    // the returned handle still owns the element and can go elsewhere
    X c;
    auto r2 = c.insert(std::move(r.node));
    if (!r2.inserted || !keys_are(c, {3})) return false;
    // insert(p, nh) that fails leaves nh unchanged
    N d4 = b.extract(key<X>(4));
    auto h = a.insert(a.cend(), std::move(d4));
    if (d4.empty() || !(key_of<X>(*h) == key<X>(4)) || a.count(key<X>(4)) != 1) return false;
  }
  return true;
}


// [associative.reqmts.general]/112-117: a.merge(a2) extracts each element of a2 and inserts
// it into a with a's comparison object; with unique keys an element whose key is already in
// a stays in a2. Pointers, references and (same iterator type) iterators to transferred
// elements now refer to them as members of a. Y is any container type with compatible
// nodes ([container.node.overview], Table 75); both lvalue and rvalue sources.
template <class X, class Y>
constexpr bool merge() {
  X a = mk<X>({1, 3, 5});
  Y b;
  for (int k : {2, 3, 4, 5, 5, 6}) b.insert(kv<Y>(k, 50 + k));
  auto i2 = b.find(key<Y>(2));
  const auto* p2 = std::addressof(*i2);
  const auto bsize = b.size();
  a.merge(b);
  const bool am = is_multi<X>;
  const bool bm = is_multi<Y>;
  if (am) {
    if (!b.empty() || !keys_are(a, bm ? std::initializer_list<int>{1, 2, 3, 3, 4, 5, 5, 5, 6}
                                         : std::initializer_list<int>{1, 2, 3, 3, 4, 5, 5, 6}))
      return false;
  } else {
    if (!keys_are(a, {1, 2, 3, 4, 5, 6})) return false;
    // the elements with keys already in a stay in b
    if (b.size() != (bm ? 3u : 2u) || b.count(key<Y>(3)) != 1 || b.count(key<Y>(5)) != (bm ? 2u : 1u)) return false;
  }
  if (a.size() + b.size() != 3 + bsize) return false;
  if (std::addressof(*a.find(key<X>(2))) != p2) return false;
  if constexpr (std::is_same_v<typename X::iterator, typename Y::iterator>) {
    if (i2 != std::next(a.begin())) return false;
  }
  if constexpr (is_map<X>) {
    if (!(a.find(key<X>(2))->second == val<typename X::mapped_type>(52))) return false;
    if (!(a.find(key<X>(1))->second == val<typename X::mapped_type>(2))) return false;
  }
  Y c;
  c.insert(kv<Y>(0));
  a.merge(std::move(c));
  if (!c.empty() || a.begin() == a.end() || !(key_of<X>(*a.begin()) == key<X>(0))) return false;
  Y e;
  a.merge(e);
  return e.empty();
}

// [map.erasure], [multimap.erasure], [set.erasure], [multiset.erasure]: erase_if(c, pred)
// erases the elements for which pred is true, in order, and returns the number erased
// (original size minus new size).
template <class X>
constexpr bool erase_if() {
  X a = mk<X>({1, 2, 3, 4, 5, 6, 7, 8});
  if constexpr (is_multi<X>) a.insert(kv<X>(4, 9));
  static_assert(std::is_same_v<decltype(std::erase_if(a, [](const auto&) { return true; })), typename X::size_type>);
  int calls = 0;
  auto n = std::erase_if(a, [&](const typename X::value_type& v) {
    ++calls;
    return key_of<X>(v) == key<X>(4) || key_of<X>(v) == key<X>(7) || key_of<X>(v) == key<X>(1);
  });
  if (n != (is_multi<X> ? 4u : 3u) || calls != (is_multi<X> ? 9 : 8)) return false;
  if (!keys_are(a, {2, 3, 5, 6, 8})) return false;
  return std::erase_if(a, [](const auto&) { return false; }) == 0 && a.size() == 5;
}

// A stateful comparator: ascending or descending according to its state.
struct Dir {
  bool desc = false;
  int* copies = nullptr;
  constexpr Dir() = default;
  constexpr explicit Dir(bool d, int* c = nullptr) : desc(d), copies(c) {}
  constexpr Dir(const Dir& o) : desc(o.desc), copies(o.copies) {
    if (copies) ++*copies;
  }
  constexpr Dir& operator=(const Dir&) = default;
  template <class T>
  constexpr bool operator()(const T& a, const T& b) const {
    return desc ? b < a : a < b;
  }
};

// [associative.reqmts.general]/18-46, /179: X(c), X(i, j, c), X(from_range, rg, c),
// X(il, c) use a copy of c; key_comp() returns the comparison object the container was
// constructed with and value_comp() is built from it; copies (construction and assignment)
// use the comparison object of the source. X must have Dir as key_compare.
template <class X>
constexpr bool comparator() {
  using V = typename X::value_type;
  const Dir desc(true);
  X a(desc);
  static_assert(std::is_same_v<decltype(a.key_comp()), typename X::key_compare>);
  static_assert(std::is_same_v<decltype(a.value_comp()), typename X::value_compare>);
  if (!a.key_comp().desc) return false;
  for (int k : {3, 1, 2}) a.insert(kv<X>(k));
  if (!keys_are(a, {3, 2, 1})) return false;
  if (!a.value_comp()(kv<X>(5), kv<X>(4)) || a.value_comp()(kv<X>(4), kv<X>(5))) return false;
  V arr[] = {kv<X>(1), kv<X>(4), kv<X>(2)};
  X b(arr, arr + 3, desc);
  X c(std::from_range, arr, desc);
  X d({kv<X>(1), kv<X>(4), kv<X>(2)}, desc);
  if (!keys_are(b, {4, 2, 1}) || !keys_are(c, {4, 2, 1}) || !keys_are(d, {4, 2, 1})) return false;
  X asc(arr, arr + 3);
  if (asc.key_comp().desc || !keys_are(asc, {1, 2, 4})) return false;
  // copies use the source's comparison object
  X cp(b);
  if (!cp.key_comp().desc || !keys_are(cp, {4, 2, 1})) return false;
  asc = b;
  if (!asc.key_comp().desc) return false;
  asc.insert(kv<X>(3));
  if (!keys_are(asc, {4, 3, 2, 1})) return false;
  X mv(std::move(cp));
  if (!mv.key_comp().desc) return false;
  X asc2(arr, arr + 3);
  asc2 = std::move(mv);
  if (!asc2.key_comp().desc || !keys_are(asc2, {4, 2, 1})) return false;
  // swap exchanges the comparison objects as well
  X up(arr, arr + 3);
  up.swap(asc2);
  if (!up.key_comp().desc || asc2.key_comp().desc || !keys_are(asc2, {1, 2, 4})) return false;
  // the comparison object is copied, not referenced
  Dir local(true);
  X e(local);
  local.desc = false;
  e.insert(kv<X>(1));
  e.insert(kv<X>(2));
  return keys_are(e, {2, 1});
}


// Key type for the heterogeneous-lookup checks: implicitly constructible from int, and the
// constructions from int are counted at run time.
struct Id {
  int v;
  static inline int from_int = 0;
  constexpr Id(int x) : v(x) {
    if !consteval { ++from_int; }
  }
  constexpr Id(const Id&) = default;
  constexpr Id& operator=(const Id&) = default;
  friend constexpr bool operator==(const Id&, const Id&) = default;
  friend constexpr auto operator<=>(const Id&, const Id&) = default;
};
// Transparent comparator comparing Id with Id and with int (in both orders).
struct IdLess {
  using is_transparent = int;
  constexpr bool operator()(const Id& a, const Id& b) const { return a.v < b.v; }
  constexpr bool operator()(const Id& a, int b) const { return a.v < b; }
  constexpr bool operator()(int a, const Id& b) const { return a < b.v; }
};
// The same ordering without is_transparent; it cannot compare an Id with an int.
struct IdLessPlain {
  constexpr bool operator()(const Id& a, const Id& b) const { return a.v < b.v; }
};

// [associative.reqmts.general]/7.8, /104-107, /122-125, /144-174, /180: with a transparent
// comparator, find, count, contains, lower_bound, upper_bound, equal_range, erase and
// extract accept any key-comparable value without converting it to key_type; without
// is_transparent those templates do not participate, so the argument is converted to
// key_type. erase and extract templates do not participate when the argument converts to
// iterator or const_iterator (so erase(it) / extract(it) still mean the position forms).
// X has key_type Id and key_compare IdLess; XP the same with IdLessPlain.
template <class X, class XP>
bool transparent() {
  using It = typename X::iterator;
  using CIt = typename X::const_iterator;
  X a;
  for (int k : {1, 3, 3, 5, 7}) a.insert(kv<X>(k));
  // kv<X>(k) has key val<Id>(k) == Id(k)
  const int k3 = val<Id>(3).v, k4 = val<Id>(4).v, k5 = val<Id>(5).v, k7 = val<Id>(7).v, k1 = val<Id>(1).v;
  const X& ca = a;
  Id::from_int = 0;
  static_assert(std::is_same_v<decltype(a.find(k3)), It> && std::is_same_v<decltype(ca.find(k3)), CIt>);
  static_assert(std::is_same_v<decltype(a.equal_range(k3)), std::pair<It, It>>);
  static_assert(std::is_same_v<decltype(ca.equal_range(k3)), std::pair<CIt, CIt>>);
  const typename X::size_type n3 = is_multi<X> ? 2 : 1;
  if (a.find(k3) == a.end() || ca.find(k4) != ca.end() || key_of<X>(*a.find(k5)).v != k5) return false;
  if (ca.count(k3) != n3 || ca.count(k4) != 0 || !ca.contains(k7) || ca.contains(k4)) return false;
  if (a.lower_bound(k4) != a.find(k5) || ca.upper_bound(k3) != ca.find(k5)) return false;
  auto er = a.equal_range(k3);
  if (static_cast<typename X::size_type>(std::distance(er.first, er.second)) != n3 || er.second != a.find(k5)) return false;
  if (Id::from_int != 0) return false;  // no conversions to key_type
  if (a.erase(k3) != n3 || a.contains(k3)) return false;
  auto nh = a.extract(k7);
  if (nh.empty() || a.contains(k7) || !a.extract(k4).empty()) return false;
  if (Id::from_int != 0) return false;
  // erase(iterator) / extract(iterator) are still the position forms
  static_assert(std::is_same_v<decltype(a.erase(a.begin())), It>);
  static_assert(std::is_same_v<decltype(a.erase(a.cbegin())), It>);
  auto it = a.erase(a.begin());
  if (!(key_of<X>(*it) == val<Id>(5)) || a.size() != 1) return false;
  auto nh2 = a.extract(a.cbegin());
  if (nh2.empty() || !a.empty()) return false;
  (void)k1;

  // not transparent: the int is converted to Id for every call
  XP p;
  for (int k : {1, 3, 5}) p.insert(kv<XP>(k));
  Id::from_int = 0;
  if (p.find(k3) == p.end() || p.count(k5) != 1 || !p.contains(k1) || p.lower_bound(k4) != p.find(k5)) return false;
  if (Id::from_int < 5) return false;
  Id::from_int = 0;
  if (p.erase(k3) != 1 || p.extract(k5).empty() || Id::from_int < 2) return false;
  return p.size() == 1;
}

}  // namespace reqs::associative
