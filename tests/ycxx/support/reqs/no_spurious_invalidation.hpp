// Generic requirement checks extracted from tests/ycxx/containers/no_spurious_invalidation.pass.cpp so they can be
// instantiated for every container; see that file for the draft wording they check.
#pragma once
#include <iterator>
#include <memory>
#include <ranges>
#include "container_values.hpp"

namespace reqs::no_spurious_invalidation {

template <class X>
constexpr bool test() {
  using T = typename X::value_type;
  X a = make<X>({1, 2, 3, 4, 5});
  X other = make<X>({1, 2, 3});
  const T* addr[5];
  {
    int k = 0;
    for (auto& x : a) addr[k++] = std::addressof(x);
  }
  auto it = nth(a, 2);
  (void)a.size(); (void)a.empty(); (void)a.max_size();
  (void)a.begin(); (void)a.end(); (void)a.cbegin(); (void)a.cend();
  (void)a.rbegin(); (void)a.rend(); (void)a.crbegin(); (void)a.crend();
  (void)a.front(); (void)a.back();
  if constexpr (std::random_access_iterator<typename X::iterator>) {
    (void)a[1]; (void)a.at(3);
  }
  if constexpr (std::contiguous_iterator<typename X::iterator>) {
    (void)a.capacity(); (void)a.data(); (void)std::data(a); (void)std::ranges::data(a);
  }
  if constexpr (requires { typename X::allocator_type; }) (void)a.get_allocator();
  (void)(a == other); (void)(a != other); (void)(other == a);
  (void)std::size(a); (void)std::ssize(a); (void)std::empty(a);
  (void)std::begin(a); (void)std::end(a); (void)std::rbegin(a);
  (void)std::ranges::begin(a); (void)std::ranges::end(a); (void)std::ranges::size(a);
  (void)std::ranges::cbegin(a);
  X copy(a);  // copying from a does not change a
  (void)copy;
  int k = 0;
  for (auto& x : a) {
    if (std::addressof(x) != addr[k] || !(x == val<T>(k + 1))) return false;
    ++k;
  }
  if (std::addressof(*it) != addr[2] || !(*it == val<T>(3))) return false;
  return k == 5;
}

}  // namespace reqs::no_spurious_invalidation
