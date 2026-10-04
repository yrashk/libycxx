// [facet.num.get.virtuals]: floating-point extraction uses %g and strtof / strtod / strtold:
// decimal and exponent forms, a leading sign; zero and failbit when nothing converts; Stage 2
// stops at the first character that cannot continue the field.
#include <sstream>
#include <string>
#include "check.hpp"

template<class T>
static T get(const char* s, bool* failed = nullptr, std::string* rest = nullptr) {
  std::istringstream is(s);
  T v{};
  is >> v;
  if (failed) *failed = is.fail();
  if (rest) {
    is.clear();
    std::getline(is, *rest, '\0');
  }
  return v;
}

int main() {
  bool f;
  std::string rest;
  CHECK(get<double>("1.5") == 1.5);
  CHECK(get<double>("-2.25e2") == -225.0);
  CHECK(get<double>("+.5") == 0.5);
  CHECK(get<double>("5.") == 5.0);
  CHECK(get<double>("1E3") == 1000.0);
  CHECK(get<float>("0.25") == 0.25f);
  CHECK(get<long double>("3.5") == 3.5L);
  CHECK(get<double>("7,5", &f, &rest) == 7.0 && !f && rest == ",5");
  CHECK(get<double>("1.5x", &f, &rest) == 1.5 && rest == "x");
  CHECK(get<double>("abc", &f) == 0.0 && f);
  CHECK(get<double>(".", &f) == 0.0 && f);
  CHECK(get<double>("1e", &f) == 0.0 && f);  // the field "1e" is not converted entirely
  return 0;
}
