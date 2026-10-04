// [facet.num.get.virtuals] Stage 2 accumulates hexadecimal floating fields for %g (Example 1:
// "0x1a.bp+07" is accumulated; the atoms include x, X, p, P and a-f, A-F) and Stage 3 converts
// them with strtof / strtod / strtold, which convert hexadecimal input exactly when it is
// representable and otherwise round to nearest; an overflowing field gives HUGE_VAL with failbit.
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
  CHECK(get<double>("0x1.fffffffffffffp1023", err) == std::numeric_limits<double>::max() && !(err & B::failbit));
  CHECK(get<double>("0x1p-1074", err) == std::numeric_limits<double>::denorm_min());
  CHECK(get<double>("0x0.0000000000001p-1022", err) == std::numeric_limits<double>::denorm_min());
  CHECK(get<double>("0X1.8P1", err) == 3.0 && err == B::eofbit);
  CHECK(get<double>("-0x1P+3", err) == -8.0 && err == B::eofbit);
  CHECK(get<double>("0x.8", err) == 0.5);
  CHECK(get<double>("0x10", err) == 16.0);
  CHECK(get<float>("0x1.000001p0", err) == 1.0f);  // rounds (tie to even) to float
  CHECK(get<long double>("0x1p-2", err) == 0.25L);
  CHECK(get<double>("0x1p1024", err) == HUGE_VAL && (err & B::failbit));
  return 0;
}
