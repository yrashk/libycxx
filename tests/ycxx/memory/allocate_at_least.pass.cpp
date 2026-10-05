// [allocator.members]/6-10: allocator<T>::allocate_at_least(n) returns
// allocation_result<T*>{ptr, count} with count >= n; deallocate(p, m) accepts any
// req <= m <= count. Throws bad_array_new_length if numeric_limits<size_t>::max() /
// sizeof(T) < n. [allocator.traits.members]/3: allocator_traits::allocate_at_least returns
// a.allocate_at_least(n) if well-formed, otherwise {a.allocate(n), n}.
// [memory.syn]: allocation_result<Pointer, SizeType = size_t> { Pointer ptr; SizeType count; }
// with no other members or bases ([allocator.traits.other]).
// REQUIRES: exceptions
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::allocator<int>{}.allocate_at_least(1)), std::allocation_result<int*>>);
static_assert(std::is_same_v<std::allocation_result<int*>, std::allocation_result<int*, std::size_t>>);
static_assert(std::is_aggregate_v<std::allocation_result<int*>>);
static_assert(std::is_same_v<decltype(std::allocation_result<int*>::ptr), int*>);
static_assert(std::is_same_v<decltype(std::allocation_result<int*, short>::count), short>);
static_assert(std::is_empty_v<std::allocation_result<int*>> == false);
static_assert(sizeof(std::allocation_result<char*, std::size_t>) == sizeof(char*) + sizeof(std::size_t));
static_assert(std::is_same_v<decltype(std::allocator_traits<std::allocator<long>>::allocate_at_least(
                                 std::declval<std::allocator<long>&>(), 1)),
                             std::allocation_result<long*, std::size_t>>);

// An allocator without allocate_at_least.
template <class T>
struct Plain {
  using value_type = T;
  int* allocs;
  constexpr T* allocate(std::size_t n) { ++*allocs; return std::allocator<T>{}.allocate(n); }
  constexpr void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  friend constexpr bool operator==(const Plain&, const Plain&) = default;
};
// An allocator whose allocate_at_least over-allocates, with custom size_type.
template <class T>
struct Generous {
  using value_type = T;
  using size_type = unsigned short;
  constexpr T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  constexpr std::allocation_result<T*, unsigned short> allocate_at_least(unsigned short n) {
    return {std::allocator<T>{}.allocate(n + 8u), static_cast<unsigned short>(n + 8u)};
  }
  constexpr void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  friend constexpr bool operator==(const Generous&, const Generous&) = default;
};
static_assert(std::is_same_v<decltype(std::allocator_traits<Generous<int>>::allocate_at_least(
                                 std::declval<Generous<int>&>(), 1)),
                             std::allocation_result<int*, unsigned short>>);

constexpr bool test() {
  {
    std::allocator<int> a;
    auto [p, count] = a.allocate_at_least(5);
    if (p == nullptr || count < 5) return false;
    for (std::size_t i = 0; i < count; ++i) std::construct_at(p + i, int(i));   // all count usable
    if (p[4] != 4 || p[count - 1] != int(count - 1)) return false;
    std::destroy(p, p + count);
    a.deallocate(p, count);                    // any value in [req, count]
  }
  {
    std::allocator<int> a;
    std::allocation_result<int*> r = a.allocate_at_least(3);
    if (r.count < 3) return false;
    a.deallocate(r.ptr, 3);                    // the requested size is also fine
  }
  {
    std::allocator<int> a;
    auto r = std::allocator_traits<std::allocator<int>>::allocate_at_least(a, 7);
    if (r.ptr == nullptr || r.count < 7) return false;
    std::allocator_traits<std::allocator<int>>::deallocate(a, r.ptr, r.count);
  }
  {
    int allocs = 0;
    Plain<double> a{&allocs};
    auto r = std::allocator_traits<Plain<double>>::allocate_at_least(a, 4);
    if (r.count != 4 || r.ptr == nullptr || allocs != 1) return false;   // {a.allocate(n), n}
    std::allocator_traits<Plain<double>>::deallocate(a, r.ptr, r.count);
  }
  {
    Generous<char> a;
    auto r = std::allocator_traits<Generous<char>>::allocate_at_least(a, 2);
    if (r.count != 10) return false;           // forwarded to a.allocate_at_least
    std::allocator_traits<Generous<char>>::deallocate(a, r.ptr, r.count);
  }
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  bool threw = false;
  try {
    (void)std::allocator<std::uint32_t>{}.allocate_at_least(std::numeric_limits<std::size_t>::max() / 4 + 1);
  } catch (const std::bad_array_new_length&) {
    threw = true;
  }
  CHECK(threw);
  return 0;
}
