// Exception specifications in [vector.overview]: default constructor
// noexcept(noexcept(Allocator())); vector(const Allocator&) noexcept; move constructor
// noexcept; move assignment noexcept(POCMA || is_always_equal); iterator functions, empty,
// size, max_size, capacity, data, get_allocator and clear are noexcept; swap is
// noexcept(POCS || is_always_equal); non-member swap is noexcept(noexcept(x.swap(y))).
// REQUIRES: exceptions
#include <vector>
#include <type_traits>
#include <utility>
#include "test_allocators.hpp"

template <class T>
struct ThrowingDefaultAlloc {
  using value_type = T;
  ThrowingDefaultAlloc() noexcept(false) {}
  template <class U>
  ThrowingDefaultAlloc(const ThrowingDefaultAlloc<U>&) noexcept {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) { std::allocator<T>{}.deallocate(p, n); }
  friend bool operator==(ThrowingDefaultAlloc, ThrowingDefaultAlloc) { return true; }
};

using V = std::vector<int>;
static_assert(std::is_nothrow_default_constructible_v<V>);
static_assert(!std::is_nothrow_default_constructible_v<std::vector<int, ThrowingDefaultAlloc<int>>>);
static_assert(std::is_default_constructible_v<std::vector<int, ThrowingDefaultAlloc<int>>>);
static_assert(std::is_nothrow_constructible_v<std::vector<int, ThrowingDefaultAlloc<int>>,
                                              const ThrowingDefaultAlloc<int>&>);
static_assert(std::is_nothrow_move_constructible_v<V>);
static_assert(std::is_nothrow_move_constructible_v<std::vector<int, IdAlloc<int>>>);
static_assert(std::is_nothrow_move_assignable_v<V>);
static_assert(std::is_nothrow_move_assignable_v<std::vector<int, IdAlloc<int, false, true, false>>>);
static_assert(std::is_nothrow_swappable_v<V>);
static_assert(std::is_nothrow_swappable_v<std::vector<int, IdAlloc<int, false, false, true>>>);

extern V& m;
extern const V& c;
static_assert(noexcept(m.begin()) && noexcept(c.begin()) && noexcept(m.end()) && noexcept(c.end()));
static_assert(noexcept(m.rbegin()) && noexcept(c.rbegin()) && noexcept(m.rend()) && noexcept(c.rend()));
static_assert(noexcept(c.cbegin()) && noexcept(c.cend()) && noexcept(c.crbegin()) && noexcept(c.crend()));
static_assert(noexcept(c.empty()) && noexcept(c.size()) && noexcept(c.max_size()) && noexcept(c.capacity()));
static_assert(noexcept(m.data()) && noexcept(c.data()) && noexcept(c.get_allocator()) && noexcept(m.clear()));
static_assert(noexcept(m.swap(m)));
static_assert(noexcept(swap(m, m)));

// Element types do not affect these: a vector of a type with a throwing move is still
// nothrow-move-constructible.
struct ThrowingMove {
  ThrowingMove(ThrowingMove&&) noexcept(false);
  ThrowingMove& operator=(ThrowingMove&&) noexcept(false);
};
static_assert(std::is_nothrow_move_constructible_v<std::vector<ThrowingMove>>);
static_assert(std::is_nothrow_move_assignable_v<std::vector<ThrowingMove>>);
static_assert(std::is_nothrow_swappable_v<std::vector<ThrowingMove>>);
