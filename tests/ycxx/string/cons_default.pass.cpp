// [basic.string.general]: basic_string() noexcept(noexcept(Allocator())) delegates to
// basic_string(Allocator()); explicit basic_string(const Allocator&) noexcept.
// [string.cons]/1: Postconditions: size() is equal to 0.
// [basic.string.general]/3: data() + size() points at a null terminator.
// [container.alloc.reqmts]/9,11: u.get_allocator() == A() / == m.
#include <string>
#include <type_traits>
#include "test_allocators.hpp"
#include "check.hpp"

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

static_assert(std::is_nothrow_default_constructible_v<std::string>);
static_assert(std::is_nothrow_constructible_v<std::string, const std::allocator<char>&>);
static_assert(!std::is_convertible_v<const std::allocator<char>&, std::string>);  // explicit
using TS = std::basic_string<char, std::char_traits<char>, ThrowingDefaultAlloc<char>>;
static_assert(std::is_default_constructible_v<TS>);
static_assert(!std::is_nothrow_default_constructible_v<TS>);
static_assert(std::is_nothrow_constructible_v<TS, const ThrowingDefaultAlloc<char>&>);

template <class C>
constexpr bool test() {
  std::basic_string<C> s;
  if (s.size() != 0 || s.length() != 0 || !s.empty()) return false;
  if (s.data()[0] != C() || s.c_str()[0] != C()) return false;
  if (s.begin() != s.end()) return false;
  if (s.capacity() < s.size()) return false;
  std::basic_string<C> t{};
  if (!t.empty()) return false;
  std::basic_string<C> u(std::allocator<C>{});
  if (!u.empty() || u.get_allocator() != std::allocator<C>()) return false;
  return true;
}
static_assert(test<char>());
static_assert(test<wchar_t>());
static_assert(test<char8_t>());
static_assert(test<char16_t>());
static_assert(test<char32_t>());

int main() {
  CHECK(test<char>());
  CHECK(test<char32_t>());
  using S = std::basic_string<char, std::char_traits<char>, IdAlloc<char>>;
  S a(IdAlloc<char>(7));
  CHECK(a.empty());
  CHECK(a.get_allocator().id == 7);
  CHECK(a.c_str()[0] == '\0');
  S b;
  CHECK(b.get_allocator() == IdAlloc<char>());
  return 0;
}
