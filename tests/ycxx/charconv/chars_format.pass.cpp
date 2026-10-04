// [charconv.syn]: chars_format is an enum class with elements scientific, fixed, hex and
// general = fixed | scientific; "The type chars_format is a bitmask type ([bitmask.types])
// with elements scientific, fixed, and hex", so |, &, ^, ~, |=, &=, ^= are available and the
// elements are distinct, nonzero and non-overlapping.
#include <charconv>
#include <type_traits>
#include "check.hpp"

using F = std::chars_format;
static_assert(std::is_enum_v<F> && !std::is_convertible_v<F, int>);

constexpr bool test() {
  F s = F::scientific, f = F::fixed, h = F::hex, g = F::general;
  if (s == f || s == h || f == h) return false;
  if ((s & f) != F{} || (s & h) != F{} || (f & h) != F{}) return false;
  if (s == F{} || f == F{} || h == F{}) return false;
  if ((f | s) != g) return false;
  if ((g & f) != f || (g & s) != s || (g & h) != F{}) return false;
  if ((g ^ f) != s) return false;
  if ((~f & g) != s) return false;
  F x = f;
  x |= s;
  if (x != g) return false;
  x &= s;
  if (x != s) return false;
  x ^= h;
  if (x != (s | h)) return false;
  static_assert(std::is_same_v<decltype(f | s), F>);
  static_assert(std::is_same_v<decltype(f & s), F>);
  static_assert(std::is_same_v<decltype(~f), F>);
  static_assert(std::is_same_v<decltype(x |= s), F&>);
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
