// [format.string.std] Table 110 (none, no precision): to_chars(first, last, value), whose
// plain overload ([charconv.to.chars]/7) uses "The conversion specifier is f if the
// absolute value of value is in the range [l, u), where l is the smallest value larger than
// or equal to 10^-4 that is representable by T and u is the largest value smaller than or
// equal to numeric_limits<T>::radix^(numeric_limits<T>::digits + 1) rounded down to the
// nearest power of 10 representable by T, otherwise e." For double [l, u) is [1e-4, 1e16),
// for float [1e-4, 1e7). So 1e5 is "100000", not the shorter "1e+05".
#include <format>
#include <string>
#include "check.hpp"

int main() {
  CHECK(std::format("{}", 1e5) == "100000");
  CHECK(std::format("{}", 1e-4) == "0.0001");
  CHECK(std::format("{}", 1e-4f) == "0.0001");
  CHECK(std::format("{}", 1e10) == "10000000000");
  CHECK(std::format("{}", 1e15) == "1000000000000000");
  CHECK(std::format("{}", 1e6f) == "1000000");
  CHECK(std::format("{}", 1e7f) == "1e+07");
  CHECK(std::format("{}", 1.5e9) == "1500000000");
}
