// [stacktrace.entry]: stacktrace_entry models regular and three_way_comparable<strong_ordering>;
// a default-constructed entry is empty (operator bool is false, explicit); == is true for two
// empty entries; native_handle() is stable; description()/source_file() return strings and
// source_line() a uint_least32_t (empty / 0 when no information is available). All
// constructors, operator bool, native_handle and comparisons are constexpr and noexcept.
#include <stacktrace>
#include <compare>
#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>
#include "check.hpp"

using E = std::stacktrace_entry;
static_assert(std::regular<E> && std::three_way_comparable<E, std::strong_ordering>);
static_assert(!std::is_convertible_v<E, bool> && std::is_constructible_v<bool, E>);
static_assert(std::is_nothrow_default_constructible_v<E> && std::is_nothrow_copy_constructible_v<E>);
static_assert(std::is_same_v<decltype(E().description()), std::string>);
static_assert(std::is_same_v<decltype(E().source_file()), std::string>);
static_assert(std::is_same_v<decltype(E().source_line()), std::uint_least32_t>);
static_assert(std::is_same_v<decltype(E().native_handle()), E::native_handle_type>);
static_assert(noexcept(E() == E()) && noexcept(E() <=> E()) && noexcept(E().native_handle()));
static_assert(std::is_same_v<decltype(E() <=> E()), std::strong_ordering>);

constexpr bool empty_entries() {
  E a, b;
  return !a && a == b && (a <=> b) == 0 && a.native_handle() == b.native_handle();
}
static_assert(empty_entries());

int main() {
  E e;
  CHECK(!e && e == E());
  CHECK(e.native_handle() == e.native_handle());
  (void)e.description();
  (void)e.source_file();
  (void)e.source_line();
  return 0;
}
