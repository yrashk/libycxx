// [charconv.to.chars]/7: for to_chars(first, last, value) "The conversion specifier is f
// if the absolute value of value is in the range [l, u), where l is the smallest value
// larger than or equal to 10^-4 that is representable by T and u is the largest value
// smaller than or equal to numeric_limits<T>::radix^(numeric_limits<T>::digits + 1) rounded
// down to the nearest power of 10 representable by T, otherwise e." /2 then picks the
// fewest digits within that style. For double [l, u) = [double(1e-4), 1e16) since
// double(1e-4) >= 1e-4; for float u = 1e7. So values in range use f even where e would be
// shorter (1e5 is "100000", not "1e+05").
#include <charconv>
#include <string_view>
#include <system_error>
#include "check.hpp"

char buf[64];

template <class T>
std::string_view tc(T v) {
  auto r = std::to_chars(buf, buf + sizeof buf, v);
  CHECK(r.ec == std::errc{});
  return std::string_view(buf, r.ptr);
}

int main() {
  CHECK(tc(1e5) == "100000");
  CHECK(tc(1e15) == "1000000000000000");
  CHECK(tc(-1e10) == "-10000000000");
  CHECK(tc(1e-4) == "0.0001");
  CHECK(tc(2e-4) == "0.0002");
  CHECK(tc(16777216.0f) == "1.6777216e+07");  // >= u = 1e7: e although "16777216" is shorter
  CHECK(tc(1e6f) == "1000000");
  CHECK(tc(2e-4f) == "0.0002");
  return 0;
}
