// Generic check, instantiated per node-based associative / unordered associative container
// whose allocator's pointer is a class type (FancyAlloc, support/fancy_ptr.hpp).
// [container.reqmts]/64: "all containers defined in this Clause obtain memory using an
// allocator", Note 2: "containers and iterators do not store references to allocated
// elements other than through the allocator's pointer type, i.e., as objects of type P or
// pointer_traits<P>::template rebind<unspecified>, where P is
// allocator_traits<allocator_type>::pointer." [allocator.requirements.general]/2-5 allows P
// to be any type meeting Cpp17NullablePointer, Cpp17RandomAccessIterator and
// contiguous_iterator. The member types pointer / const_pointer of map, multimap, set,
// multiset and the unordered containers are allocator_traits<Allocator>::pointer /
// const_pointer ([map.overview], [set.overview], [unord.map.overview], ...).
// Exercised: construction forms, copy / move / assignment / swap, insertion and erasure of
// enough elements to rebalance / rehash, bidirectional iteration, lookups, node handles and
// merge (node_handles), erase_if, the bucket interface, clear. The functions are constexpr ([map.overview] etc.:
// every member is constexpr, and FancyAlloc is usable in constant evaluation).
#pragma once
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "assoc_common.hpp"
#include "fancy_ptr.hpp"

namespace reqs::fancy_alloc_assoc {

using namespace reqs::assoc;

template <class X>
constexpr void put(X& x, int k) {
  if constexpr (is_map<X>) x.emplace(typename X::key_type(k), typename X::mapped_type(k + 100));
  else x.emplace(k);
}

template <class X>
constexpr bool test() {
  using V = typename X::value_type;
  using K = typename X::key_type;
  static_assert(std::is_same_v<typename X::allocator_type, FancyAlloc<V>>);
  static_assert(std::is_same_v<typename X::pointer, FancyPtr<V>>);
  static_assert(std::is_same_v<typename X::const_pointer, FancyPtr<const V>>);
  const bool multi = is_multi<X>;

  X a;
  for (int k = 0; k < 64; ++k) put(a, (k * 37) % 64);  // pseudo-random order
  if (a.size() != 64) return false;
  for (int k = 0; k < 64; ++k)
    if (a.count(K(k)) != 1) return false;
  if constexpr (!is_unordered<X>) {  // ordered, both directions
    int k = 0;
    for (auto it = a.begin(); it != a.end(); ++it, ++k)
      if (!(key_of<X>(*it) == K(k))) return false;
    for (auto it = a.end(); it != a.begin(); --k)
      if (!(key_of<X>(*--it) == K(k - 1))) return false;
    auto r = a.rbegin();
    if (!(key_of<X>(*r) == K(63))) return false;
    if (!(key_of<X>(*a.lower_bound(K(10))) == K(10)) || !(key_of<X>(*a.upper_bound(K(10))) == K(11))) return false;
  }
  X b(a);
  X c(std::move(b));
  X d(a, FancyAlloc<V>());
  X e{elem<X>(1), elem<X>(2)};
  if (!(c == a) || !(d == a) || !contents(e, {1, 2})) return false;
  b = a;
  b = std::move(d);
  b = {elem<X>(5)};
  b.swap(e);
  if (!contents(e, {5}) || !contents(b, {1, 2})) return false;
  using std::swap;
  swap(b, e);
  if (!contents(b, {5}) || !contents(e, {1, 2})) return false;

  // erase every other element, by key, by iterator and by range
  for (int k = 0; k < 64; k += 4) a.erase(K(k));
  for (int k = 2; k < 64; k += 4) a.erase(a.find(K(k)));
  if (a.size() != 32) return false;
  if constexpr (!is_unordered<X>) {
    a.erase(a.begin(), a.find(K(9)));  // 1, 3, 5, 7
    if (a.size() != 28 || !(key_of<X>(*a.begin()) == K(9))) return false;
  }
  if (a.contains(K(10)) || !a.contains(K(11))) return false;
  put(a, 11);
  if (a.count(K(11)) != (multi ? 2u : 1u)) return false;
  auto er = a.equal_range(K(11));
  if (std::distance(er.first, er.second) != (multi ? 2 : 1)) return false;
  // a hinted insertion and an emplace_hint
  a.insert(a.begin(), elem<X>(200));
  a.emplace_hint(a.end(), elem<X>(201));
  if (!a.contains(K(200)) || !a.contains(K(201))) return false;

  std::erase_if(a, [](const V& v) { return value_of(key_of<X>(v)) >= 200; });
  if (a.contains(K(200)) || a.contains(K(300))) return false;

  if constexpr (is_unordered<X>) {
    a.rehash(500);
    if (a.bucket_count() < 500) return false;
    std::size_t total = 0;
    for (std::size_t n = 0; n < a.bucket_count(); ++n) {
      std::size_t in_bucket = 0;
      for (auto it = a.begin(n); it != a.end(n); ++it) {
        if (a.bucket(key_of<X>(*it)) != n) return false;
        ++in_bucket;
      }
      if (in_bucket != a.bucket_size(n)) return false;
      total += in_bucket;
    }
    if (total != a.size()) return false;
    a.reserve(2);
  }
  a.clear();
  return a.empty() && a.begin() == a.end();
}

// Node handles and merge with the fancy pointer: extract(k), extract(q), insert(nh),
// insert(p, nh), merge ([associative.reqmts.general]/84-117, [unord.req.general],
// [container.node]: the handle's ptr_ is allocator_traits<...>::rebind_traits<node>::pointer).
template <class X>
constexpr bool node_handles() {
  using K = typename X::key_type;
  X a;
  for (int k : {200, 11, 13, 3}) put(a, k);
  auto nh = a.extract(K(11));
  if (nh.empty()) return false;
  X f;
  f.insert(std::move(nh));
  f.insert(f.end(), a.extract(a.find(K(13))));
  if (!contents(f, {11, 13})) return false;
  X g;
  put(g, 13);
  put(g, 300);
  a.merge(g);
  if (!a.contains(K(300)) || !a.contains(K(13)) || !g.empty()) return false;  // 13 was extracted from a

  return a.size() == 4 && f.size() == 2;
}

}  // namespace reqs::fancy_alloc_assoc
