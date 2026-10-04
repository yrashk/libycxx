// [new.delete.placement] (C++26, P2747): "constexpr void* operator new(std::size_t size,
// void* ptr) noexcept;" "Returns: ptr." and the array form; placement new is usable in
// constant expressions when the storage is suitable. [new.syn] allocation errors:
// bad_alloc / bad_array_new_length what() and special members.
#include <new>
#include <cstddef>
#include <exception>
#include <type_traits>
#include "check.hpp"

struct P {
  int a, b;
};

static_assert(noexcept(::operator new(sizeof(int), static_cast<void*>(nullptr))));
static_assert(noexcept(::operator new[](sizeof(int), static_cast<void*>(nullptr))));

constexpr int test() {
  int storage = 0;
  int* p = ::new (static_cast<void*>(&storage)) int(42);
  if (p != &storage) return -1;
  P obj{0, 0};
  P* q = ::new (static_cast<void*>(&obj)) P{1, 2};  // replaces obj (trivially destructible)
  return *p + q->a + q->b;
}
static_assert(test() == 45);

static_assert(std::is_nothrow_default_constructible_v<std::bad_alloc>);
static_assert(std::is_nothrow_copy_constructible_v<std::bad_alloc>);
static_assert(std::is_nothrow_default_constructible_v<std::bad_array_new_length>);
static_assert(noexcept(std::declval<const std::bad_alloc&>().what()));

int main() {
  CHECK(test() == 45);
  alignas(int) unsigned char buf[sizeof(int) * 3];
  void* r = ::operator new(sizeof buf, static_cast<void*>(buf));
  CHECK(r == buf);
  void* ra = ::operator new[](sizeof buf, static_cast<void*>(buf));
  CHECK(ra == buf);
  std::bad_alloc ba;
  CHECK(ba.what() != nullptr);
  std::bad_array_new_length bl;
  CHECK(bl.what() != nullptr);
  const std::exception& e = bl;
  CHECK(e.what() != nullptr);
  return 0;
}
