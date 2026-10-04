// [syserr.hash]/1: hash<error_code> and hash<error_condition> are enabled ([unord.hash]):
// default constructible, copyable, swappable, invocable on const lvalues with result size_t,
// and equal arguments give equal results ([unord.hash]/5, [hash.requirements]).
// Equality is [syserr.compare]/1, /3: same category and value.
#include <system_error>
#include <cerrno>
#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include "check.hpp"

template <class T>
void enabled() {
  using H = std::hash<T>;
  static_assert(std::is_default_constructible_v<H>);
  static_assert(std::is_copy_constructible_v<H>);
  static_assert(std::is_move_constructible_v<H>);
  static_assert(std::is_copy_assignable_v<H>);
  static_assert(std::is_move_assignable_v<H>);
  static_assert(std::is_swappable_v<H>);
  static_assert(std::is_destructible_v<H>);
  static_assert(std::is_same_v<std::invoke_result_t<const H&, const T&>, std::size_t>);
  static_assert(std::is_same_v<std::invoke_result_t<H&, T&>, std::size_t>);
}

struct Cat : std::error_category {
  const char* name() const noexcept override { return "c"; }
  std::string message(int) const override { return ""; }
};

int main() {
  enabled<std::error_code>();
  enabled<std::error_condition>();
  Cat c;
  std::hash<std::error_code> hc;
  std::hash<std::error_condition> hk;
  const std::error_code a(EINVAL, std::generic_category());
  std::error_code b = std::make_error_code(std::errc::invalid_argument);
  CHECK(a == b);
  CHECK(hc(a) == hc(b));
  CHECK(hc(a) == std::hash<std::error_code>()(a));  // deterministic within the program
  std::error_code d(7, c), e;
  e.assign(7, c);
  CHECK(hc(d) == hc(e));
  std::error_code x, y(0, std::system_category());
  CHECK(hc(x) == hc(y));
  const std::error_condition k1(EINVAL, std::generic_category());
  std::error_condition k2 = std::errc::invalid_argument;
  CHECK(hk(k1) == hk(k2));
  std::error_condition k3, k4(0, std::generic_category());
  CHECK(hk(k3) == hk(k4));
  return 0;
}
