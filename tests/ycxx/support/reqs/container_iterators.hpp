// Generic requirement checks extracted from tests/ycxx/containers/container_iterators.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <compare>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "container_values.hpp"

namespace reqs::container_iterators {

template <class X>
constexpr bool test() {
  using It = typename X::iterator;
  using CIt = typename X::const_iterator;
  X b = make<X>({5, 6, 7, 8});
  const X& cb = b;
  static_assert(std::is_same_v<decltype(b.begin()), It> && std::is_same_v<decltype(b.end()), It>);
  static_assert(std::is_same_v<decltype(cb.begin()), CIt> && std::is_same_v<decltype(cb.end()), CIt>);
  static_assert(std::is_same_v<decltype(b.cbegin()), CIt> && std::is_same_v<decltype(b.cend()), CIt>);
  static_assert(std::is_same_v<decltype(cb.cbegin()), CIt>);

  if (!(*b.begin() == val<typename X::value_type>(5))) return false;
  if (b.cbegin() != cb.begin() || b.cend() != cb.end()) return false;
  if (std::addressof(*b.cbegin()) != std::addressof(*b.begin())) return false;
  if (std::distance(b.begin(), b.end()) != 4) return false;
  It last = b.begin();
  for (int i = 0; i < 3; ++i) ++last;
  if (!(*last == val<typename X::value_type>(8))) return false;
  ++last;
  if (last != b.end()) return false;

  // multi-pass: a copy of an iterator advances independently
  It a = b.begin();
  It a2 = a;
  ++a;
  if (!(*a2 == val<typename X::value_type>(5)) || a == a2) return false;
  if (std::addressof(*b.begin()) != std::addressof(*a2)) return false;

  // value-initialized iterators compare equal
  if (!(It() == It()) || !(CIt() == CIt()) || It{} != It{}) return false;

  if constexpr (std::random_access_iterator<It>) {
    // random access: <=> is strong_ordering, mixed iterator / const_iterator operands
    It i = b.begin() + 1;
    CIt ci = cb.begin() + 1;
    CIt cj = cb.begin() + 3;
    static_assert(std::is_same_v<decltype(i <=> i), std::strong_ordering>);
    static_assert(std::is_same_v<decltype(i <=> ci), std::strong_ordering>);
    static_assert(std::is_same_v<decltype(ci <=> i), std::strong_ordering>);
    if (!(i == ci) || !(ci == i) || i != ci || ci != i) return false;
    if ((i <=> ci) != 0 || (ci <=> i) != 0) return false;
    if (!(i < cj) || !(i <= cj) || cj < i || !(cj > i) || !(cj >= i) || (i <=> cj) >= 0) return false;
    if (!(ci <= i) || !(ci >= i)) return false;
    if (cj - i != 2 || i - cj != -2 || i - ci != 0 || ci - i != 0) return false;
    It j = b.begin() + 3;
    if ((cj <=> j) != std::strong_ordering::equal || (j <=> ci) != std::strong_ordering::greater) return false;
  }

  X e;
  const X& ce = e;
  if (e.begin() != e.end() || ce.begin() != ce.end() || e.cbegin() != e.cend() || !e.empty()) return false;
  if (e.begin() != e.cbegin()) return false;
  return true;
}

}  // namespace reqs::container_iterators
