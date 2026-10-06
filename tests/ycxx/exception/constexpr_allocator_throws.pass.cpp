// std::allocator reports an impossible size during constant evaluation as at run time (P3068;
// allocate and allocate_at_least are constexpr):
// [allocator.members]/4, /8: allocate(n) and allocate_at_least(n) throw bad_array_new_length if
// numeric_limits<size_t>::max() / sizeof(T) < n; [new.badlength]: bad_array_new_length derives
// from bad_alloc. [allocator.traits.members]: allocator_traits<A>::allocate(a, n) is
// a.allocate(n), so the exception passes through it. Nothing is allocated, so nothing leaks.
// XFAIL: clang Clang 23 cannot throw during constant evaluation (P3068's core-language part)
// REQUIRES: exceptions
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include "check.hpp"

struct big {
  char bytes[64];
};

template <class T>
constexpr bool too_many() {
  constexpr std::size_t n = std::numeric_limits<std::size_t>::max() / sizeof(T) + 1;
  std::allocator<T> a;
  int ok = 0;
  try {
    (void)a.allocate(n);
  } catch (const std::bad_array_new_length&) {
    ++ok;
  }
  try {
    (void)a.allocate_at_least(n);
  } catch (const std::bad_array_new_length&) {
    ++ok;
  }
  try {
    (void)std::allocator_traits<std::allocator<T>>::allocate(a, n);
  } catch (const std::bad_alloc&) { // the base class
    ++ok;
  }
  try {
    (void)std::allocator_traits<std::allocator<T>>::allocate(a, std::numeric_limits<std::size_t>::max());
  } catch (const std::bad_array_new_length&) {
    ++ok;
  }
  // a possible size still works afterwards
  T* p = a.allocate(3);
  a.deallocate(p, 3);
  return ok == 4;
}
static_assert(too_many<int>());
static_assert(too_many<big>());
static_assert(too_many<long double>());

int main() {
  CHECK(too_many<int>());
  CHECK(too_many<big>());
}
