// [bit.shift]/2-5: shl(x, s) = x * 2^s and shr(x, s) = x * 2^-s, rounded towards negative
// infinity, then static_cast<T>; "Constraints: Each of T and S is a signed or unsigned integer
// type", so a negative amount of a narrow signed type (signed char, short) means the same as the
// same value as an int: shl(x, -1) halves x, shr(x, -1) doubles it. (bit/oracle_cxx26 checks
// every T/S combination exhaustively; this is the focused case.)
#include <bit>
#include <cstdint>
#include "check.hpp"

template <class S>
constexpr bool amounts_of() {
  if (std::shl(std::int8_t(-128), S(-1)) != -64) return false;
  if (std::shl(std::int8_t(-128), S(-6)) != -2) return false;
  if (std::shl(std::int8_t(100), S(-2)) != 25) return false;
  if (std::shl(std::uint8_t(200), S(-3)) != 25) return false;
  if (std::shl(200u, S(-3)) != 25u) return false;
  if (std::shl(-7, S(-1)) != -4) return false;  // floor(-3.5)
  if (std::shr(std::int8_t(5), S(-1)) != 10) return false;
  if (std::shr(std::uint16_t(3), S(-4)) != 48) return false;
  if (std::shr(1, S(-30)) != (1 << 30)) return false;
  if (std::shl(std::int8_t(-1), S(-100)) != -1 || std::shr(std::int8_t(1), S(-100)) != 0) return false;
  return true;
}

static_assert(amounts_of<int>());  // control
static_assert(amounts_of<signed char>());
static_assert(amounts_of<short>());
static_assert(amounts_of<long long>());

int main() {
  CHECK(amounts_of<signed char>() && amounts_of<short>() && amounts_of<int>() && amounts_of<long long>());
  return 0;
}
