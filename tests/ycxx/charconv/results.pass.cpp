// [charconv.syn]: to_chars_result { char* ptr; errc ec; } and from_chars_result { const
// char* ptr; errc ec; } have exactly those data members (no bases, no other members) and
// a defaulted friend operator==; constexpr explicit operator bool() const noexcept returns
// ec == errc{}. Both support structured bindings and aggregate initialization.
#include <charconv>
#include <system_error>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_aggregate_v<std::to_chars_result>);
static_assert(std::is_aggregate_v<std::from_chars_result>);
static_assert(std::is_same_v<decltype(std::to_chars_result::ptr), char*>);
static_assert(std::is_same_v<decltype(std::to_chars_result::ec), std::errc>);
static_assert(std::is_same_v<decltype(std::from_chars_result::ptr), const char*>);
static_assert(std::is_same_v<decltype(std::from_chars_result::ec), std::errc>);
static_assert(!std::is_convertible_v<std::to_chars_result, bool>);
static_assert(std::is_constructible_v<bool, std::to_chars_result>);
static_assert(!std::is_convertible_v<std::from_chars_result, bool>);
static_assert(std::is_constructible_v<bool, std::from_chars_result>);
static_assert(noexcept(static_cast<bool>(std::to_chars_result{})));
static_assert(noexcept(static_cast<bool>(std::from_chars_result{})));

constexpr bool test() {
  char buf[4];
  std::to_chars_result a{buf, std::errc{}};
  std::to_chars_result b{buf, std::errc{}};
  std::to_chars_result c{buf + 1, std::errc{}};
  std::to_chars_result d{buf, std::errc::value_too_large};
  if (!(a == b) || a == c || a != b || !(a != d)) return false;
  if (!a || d) return false;
  auto [p, e] = a;
  if (p != buf || e != std::errc{}) return false;
  const char* s = "12";
  std::from_chars_result f{s, std::errc{}};
  std::from_chars_result g{s, std::errc::invalid_argument};
  if (f == g || !(f == f)) return false;
  if (!f || g) return false;
  int v = 0;
  if (!std::from_chars(s, s + 2, v) || v != 12) return false;
  if (std::from_chars(s, s, v)) return false;
  if (!std::to_chars(buf, buf + 4, 42)) return false;
  if (std::to_chars(buf, buf + 1, 42)) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
