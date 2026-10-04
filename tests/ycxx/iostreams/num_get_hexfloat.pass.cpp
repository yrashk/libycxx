// [facet.num.get.virtuals] Stage 2: the atoms include "0123456789abcdefpxABCDEFPX+-" and
// Example 1: "if the conversion specifier returned by Stage 1 is %g, "0x1a.bp+07" are
// accumulated"; Stage 3 converts the field with strtod, which accepts hexadecimal
// floating-point (26.6875 * 2^7 = 3416).
#include <sstream>
#include <string>
#include "check.hpp"

int main() {
  std::istringstream is("0x1a.bp+07p");
  double d = 0;
  is >> d;
  CHECK(!is.fail());
  CHECK(d == 3416.0);
  std::string rest;
  is >> rest;
  CHECK(rest == "p");
  return 0;
}
