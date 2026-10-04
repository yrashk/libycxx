// Shared helpers for the batch-21 generic checks over every associative ([associative.reqmts])
// and unordered associative ([unord.req]) container, including the flat container adaptors.
// Elements are described by small ints: an element "k" has key key_type(k) and, for maps,
// mapped value mapped_type(k + 100), so that duplicates in a source range are identical and
// the result of an operation does not depend on which of several equivalent source elements
// is kept. Observation is by iteration and by the lookup members only.
// Independent of every other test suite.
#pragma once
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>
#include <utility>
#include "container_values.hpp"
#include "move_only_elem.hpp"

namespace reqs::assoc {

template <class X>
concept is_map = requires { typename X::mapped_type; };

template <class X>
concept is_unordered = requires { typename X::hasher; };

template <class X>
concept is_flat = requires { typename X::containers; } || requires { typename X::container_type; };

// unique keys: insert(const value_type&) returns pair<iterator, bool>
template <class X>
concept is_multi = std::is_same_v<decltype(std::declval<X&>().insert(std::declval<const typename X::value_type&>())),
                                  typename X::iterator>;

template <class X>
constexpr const typename X::key_type& key_of(const auto& v) {
  if constexpr (is_map<X>) return v.first;
  else return v;
}

// The element with index k, as a value_type prvalue.
template <class X>
constexpr typename X::value_type elem(int k) {
  if constexpr (is_map<X>)
    return typename X::value_type(typename X::key_type(k), typename X::mapped_type(k + 100));
  else
    return typename X::value_type(k);
}

// The source type used for ranges of "k": pair<int, int>{k, k + 100} for maps, int for sets.
template <class X>
using src_t = std::conditional_t<is_map<X>, std::pair<int, int>, int>;

template <class X>
constexpr src_t<X> src(int k) {
  if constexpr (is_map<X>) return {k, k + 100};
  else return k;
}

// contents(c, {k0, k1, ...}): c holds exactly the multiset of keys key_type(ki) (every
// element counted by iteration); for maps every mapped value is mapped_type(key + 100); the
// ordered containers iterate in non-descending order of value_comp().
template <class X>
constexpr bool contents(const X& c, std::initializer_list<int> ks) {
  using K = typename X::key_type;
  std::size_t n = 0;
  for (auto it = c.begin(); it != c.end(); ++it) ++n;
  if (n != ks.size() || c.size() != n) return false;
  for (int k : ks) {
    std::size_t want = 0, have = 0;
    for (int j : ks) want += (j == k);
    for (auto it = c.begin(); it != c.end(); ++it) have += (key_of<X>(*it) == K(k));
    if (want != have) return false;
  }
  if constexpr (is_map<X>) {
    for (auto it = c.begin(); it != c.end(); ++it) {
      int k = value_of(it->first);
      if (!(it->second == typename X::mapped_type(k + 100))) return false;
    }
  }
  if constexpr (!is_unordered<X>) {
    auto it = c.begin();
    if (it != c.end()) {
      for (auto nx = std::next(it); nx != c.end(); ++it, ++nx)
        if (c.value_comp()(*nx, *it)) return false;
    }
  }
  return true;
}

// A stateless hash for int, long, Elem and MOElem keys (by value).
struct VHash {
  constexpr std::size_t operator()(const auto& k) const {
    return static_cast<std::size_t>(static_cast<long>(value_of(k))) * 2654435761u;
  }
};

}  // namespace reqs::assoc
