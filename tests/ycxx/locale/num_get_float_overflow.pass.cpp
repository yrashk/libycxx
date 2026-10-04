// [facet.num.get.virtuals] Stage 3: the stored value is "the most positive (or negative)
// representable value, if the field to be converted to a signed integer type represents a
// value too large" (integers only) ... "the converted value, otherwise", and "if the field
// represents a value outside the range of representable values, ios_base::failbit is
// assigned to err". The conversion functions are strtof / strtod (ISO C 7.24.1.5: "If the
// correct value overflows and default rounding is in effect, plus or minus HUGE_VAL, HUGE_VALF,
// or HUGE_VALL is returned"), so an overflowing floating field stores +-HUGE_VAL with failbit.
#include <locale>
#include <sstream>
#include <iterator>
#include <cmath>
#include <limits>
#include <string>
#include "check.hpp"

template<class T>
static T get(const std::string& s, std::ios_base::iostate& err, std::string* rest = nullptr) {
  std::istringstream is(s);
  using It = std::istreambuf_iterator<char>;
  T v = T(-77);
  err = std::ios_base::goodbit;
  It it = std::use_facet<std::num_get<char>>(is.getloc()).get(It(is), It(), is, err, v);
  if (rest) {
    rest->clear();
    for (; it != It(); ++it) rest->push_back(*it);
  }
  return v;
}

int main() {
  using B = std::ios_base;
  B::iostate err;
  // Overflow: HUGE_VAL with failbit.
  CHECK(get<double>("1e400", err) == HUGE_VAL && (err & B::failbit));
  CHECK(get<double>("-1e400", err) == -HUGE_VAL && (err & B::failbit));
  CHECK(get<float>("1e39", err) == HUGE_VALF && (err & B::failbit));
  CHECK(get<float>("-3.5e38", err) == -HUGE_VALF && (err & B::failbit));
  return 0;
}
