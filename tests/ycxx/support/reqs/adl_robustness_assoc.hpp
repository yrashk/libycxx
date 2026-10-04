// Generic check, instantiated per associative / unordered associative / flat container: the
// container works with key and mapped types whose associated namespaces contain poisoned
// function templates (support/adl_poison.hpp).
// [contents]/3: "Whenever an unqualified name other than swap, make_error_code,
// make_error_condition, from_stream, or submdspan_mapping is used in the specification of a
// declaration D in [library] ... its meaning is established as-if by performing unqualified
// name lookup in the context of D", so argument-dependent lookup must not find evil::move,
// evil::find, evil::lower_bound, ::addressof, ... from inside the library. The element
// requirements ([container.alloc.reqmts]/2, [utility.arg.requirements]) and the iterator
// requirements ([iterator.requirements]) include neither a unary & nor a comma operator, so
// evil::Val (both deleted) and evil::Iter (both deleted) must be usable.
// Exercised: every constructor form, assignment, comparison, swap, insertion, emplacement,
// hinted insertion, the range members, lookups, erasure, erase_if, node handles and merge
// (node-based), the map element access and try_emplace / insert_or_assign, the bucket
// interface (unordered), extract / replace (flat).
#pragma once
#include <compare>
#include <cstddef>
#include <functional>
#include <iterator>
#include <ranges>
#include <utility>
#include "adl_poison.hpp"

namespace reqs::adl_robustness_assoc {

struct ValHash {
  constexpr std::size_t operator()(const evil::Val& v) const { return static_cast<std::size_t>(v.v) * 2654435761u; }
  constexpr std::size_t operator()(const GVal& v) const { return static_cast<std::size_t>(v.v) * 2654435761u; }
};

template <class X>
concept is_map = requires { typename X::mapped_type; };
template <class X>
concept is_unordered = requires { typename X::hasher; };
template <class X>
concept node_based = requires { typename X::node_type; };
template <class X>
concept is_multi = std::is_same_v<decltype(std::declval<X&>().insert(std::declval<const typename X::value_type&>())),
                                  typename X::iterator>;

template <class X>
constexpr std::ptrdiff_t length(const X& x) {
  std::ptrdiff_t n = 0;
  for (auto it = x.begin(); it != x.end(); ++it) ++n;
  return n;
}

// K is evil::Val or GVal (constructible from int, ==, <=>); a map's mapped type is K too.
template <class X>
constexpr bool test() {
  using K = typename X::key_type;
  using V = typename X::value_type;
  auto v = [](int k) {
    if constexpr (is_map<X>) return V(K(k), K(k + 100));
    else return V(K(k));
  };
  V arr[6] = {v(5), v(3), v(1), v(4), v(1), v(2)};
  using It = evil::Iter<V>;
  const std::ptrdiff_t uniq = is_multi<X> ? 6 : 5;

  X a;
  X b(It(arr), It(arr + 6));
  X c(std::from_range, arr);
  X d{v(1), v(2)};
  X e(b);
  X f(std::move(e));
  if (length(b) != uniq || !(b == c) || b != c || length(d) != 2 || !(f == b)) return false;
  if constexpr (!is_unordered<X>) {
    if (b < c || !(b <= c) || (b <=> c) != 0) return false;
  }
  a = b;
  a = std::move(f);
  a = {v(9), v(8)};
  a.swap(d);
  std::swap(a, d);
  std::ranges::swap(a, d);
  if (length(a) != 2 || length(d) != 2) return false;

  a.insert(v(10));
  const V cv = v(11);
  a.insert(cv);
  a.insert(a.begin(), v(12));
  a.insert(It(arr), It(arr + 3));
  a.insert({v(13), v(14)});
  a.insert_range(arr);
  if constexpr (is_map<X>) {
    a.emplace(K(15), K(115));
    a.emplace_hint(a.end(), K(16), K(116));
  } else {
    a.emplace(15);
    a.emplace_hint(a.end(), 16);
  }
  if (!a.contains(K(15)) || a.count(K(16)) != 1 || a.find(K(99)) != a.end()) return false;
  auto er = a.equal_range(K(1));
  if (std::distance(er.first, er.second) != (is_multi<X> ? 4 : 1)) return false;  // a, insert(i, j), insert_range
  if constexpr (!is_unordered<X>) {
    if (!(a.lower_bound(K(2)) != a.upper_bound(K(2)))) return false;
    if (a.rbegin() == a.rend()) return false;
  }
  if constexpr (is_map<X> && !is_multi<X>) {
    a[K(20)] = K(120);
    if (!(a.at(K(20)) == K(120))) return false;
    a.try_emplace(K(21), 121);
    a.try_emplace(a.end(), K(22), 122);
    a.insert_or_assign(K(21), K(221));
    a.insert_or_assign(a.begin(), K(23), K(123));
    if (!(a.at(K(21)) == K(221))) return false;
  }
  a.erase(K(10));
  a.erase(a.find(K(11)));
  a.erase(a.find(K(12)), std::next(a.find(K(12))));
  if (a.contains(K(10)) || a.contains(K(11)) || a.contains(K(12))) return false;

  if constexpr (node_based<X>) {
    auto nh = a.extract(K(13));
    X g;
    g.insert(std::move(nh));
    g.insert(g.end(), a.extract(a.find(K(14))));
    a.merge(g);
    if (!a.contains(K(13)) || !a.contains(K(14)) || !g.empty()) return false;
  } else {
    auto cont = std::move(a).extract();
    if constexpr (is_map<X>) a.replace(std::move(cont.keys), std::move(cont.values));
    else a.replace(std::move(cont));
    if (!a.contains(K(13))) return false;
  }
  if constexpr (is_unordered<X>) {
    a.rehash(100);
    a.reserve(200);
    std::size_t n = a.bucket(K(13));
    bool found = false;
    for (auto it = a.begin(n); it != a.end(n); ++it) {
      if constexpr (is_map<X>) found = found || it->first == K(13);
      else found = found || *it == K(13);
    }
    if (!found || a.bucket_size(n) == 0 || a.load_factor() <= 0) return false;
  }
  auto removed = std::erase_if(a, [](const V& x) {
    if constexpr (is_map<X>) return x.first == K(1);
    else return x == K(1);
  });
  if (removed != (is_multi<X> ? 4u : 1u) || a.contains(K(1))) return false;
  a.clear();
  return a.empty() && a.begin() == a.end();
}

}  // namespace reqs::adl_robustness_assoc
