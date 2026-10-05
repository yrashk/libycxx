// [unord.map.overview], [unord.multimap.overview] deduction guides: from iterator ranges of
// pairs (const removed from the key), from_range, initializer_list<pair<Key, T>>, with
// optional bucket count, Hash, Pred and Allocator, plus the forms taking (n, Allocator) and
// (n, Hash, Allocator). [unord.req.general]/246: a guide does not participate when an
// integral type would be deduced as Hash ("has a Hash template parameter and an integral
// type or a type that qualifies as an allocator is deduced for that parameter").
// REQUIRES: exceptions
// COUNTERPART: libcxx:containers/unord/unord.(map/unord.map|multimap/unord.multimap).cnstr/deduct(_const)?.pass.cpp
#include <unordered_map>
#include <functional>
#include <ranges>
#include <type_traits>
#include <utility>
#include "test_allocators.hpp"

struct H {
  std::size_t operator()(int i) const { return static_cast<std::size_t>(i); }
};
using P = std::pair<const int, long>;
using A = MinimalAlloc<P>;

void f() {
  std::pair<int, long> arr[] = {{1, 2}, {3, 4}};
  std::unordered_map a(arr, arr + 2);
  static_assert(std::is_same_v<decltype(a), std::unordered_map<int, long>>);
  std::unordered_map b(arr, arr + 2, 10);
  static_assert(std::is_same_v<decltype(b), std::unordered_map<int, long>>);
  std::unordered_map c(arr, arr + 2, 10, H());
  static_assert(std::is_same_v<decltype(c), std::unordered_map<int, long, H>>);
  std::unordered_map d(arr, arr + 2, 10, H(), std::equal_to<>(), A());
  static_assert(std::is_same_v<decltype(d), std::unordered_map<int, long, H, std::equal_to<>, A>>);
  std::unordered_map e(arr, arr + 2, 10, A());
  static_assert(std::is_same_v<decltype(e), std::unordered_map<int, long, std::hash<int>, std::equal_to<int>, A>>);
  std::unordered_map g(std::from_range, arr);
  static_assert(std::is_same_v<decltype(g), std::unordered_map<int, long>>);
  std::unordered_map h(std::from_range, arr, 10, H(), A());
  static_assert(std::is_same_v<decltype(h), std::unordered_map<int, long, H, std::equal_to<int>, A>>);
  std::unordered_map i{std::pair{1, 2L}, std::pair{3, 4L}};
  static_assert(std::is_same_v<decltype(i), std::unordered_map<int, long>>);
  std::unordered_map j({std::pair{1, 2L}}, 10, H());
  static_assert(std::is_same_v<decltype(j), std::unordered_map<int, long, H>>);
  std::unordered_multimap k(arr, arr + 2);
  static_assert(std::is_same_v<decltype(k), std::unordered_multimap<int, long>>);
  std::unordered_multimap l({std::pair{1, 2L}}, 10, A());
  static_assert(std::is_same_v<decltype(l), std::unordered_multimap<int, long, std::hash<int>, std::equal_to<int>, A>>);
}

int main() { return 0; }
