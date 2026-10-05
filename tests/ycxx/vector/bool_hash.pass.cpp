// [vector.bool.pspc]/13: template<class Allocator> struct hash<vector<bool, Allocator>>: "The
// specialization is enabled ([unord.hash])": default constructible, copyable, callable with
// a const vector<bool>& returning size_t, equal vectors hash equal.
// REQUIRES: exceptions
#include <vector>
#include <cstddef>
#include <functional>
#include <type_traits>
#include "test_allocators.hpp"
#include "check.hpp"

using H = std::hash<std::vector<bool>>;
static_assert(std::is_default_constructible_v<H>);
static_assert(std::is_copy_constructible_v<H>);
static_assert(std::is_same_v<decltype(H{}(std::declval<const std::vector<bool>&>())), std::size_t>);
static_assert(std::is_default_constructible_v<std::hash<std::vector<bool, MinimalAlloc<bool>>>>);

int main() {
  std::vector<bool> a{true, false, true}, b{true, false, true};
  CHECK(H{}(a) == H{}(b));
  std::vector<bool> big(1000, true), big2(1000, true);
  CHECK(H{}(big) == H{}(big2));
  std::vector<bool, MinimalAlloc<bool>> m{true};
  (void)std::hash<std::vector<bool, MinimalAlloc<bool>>>{}(m);
  return 0;
}
