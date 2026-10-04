// [facet.num.get.virtuals] Stage 3: the value stored is "the most positive (or negative)
// representable value, if the field to be converted to a signed integer type represents a
// value too large positive (or negative) to be represented in val" / "the most positive
// representable value, if the field to be converted to an unsigned integer type represents a
// value that cannot be represented in val", and "if the field represents a value outside the
// range of representable values, ios_base::failbit is assigned to err". [istream.formatted.
// arithmetic]/2: short and int are extracted through long and clamped to their range with
// failbit.
#include <sstream>
#include <climits>
#include <string>
#include "check.hpp"

template<class T>
static T get(const std::string& s, bool* failed) {
  std::istringstream is(s);
  T v{};
  is >> v;
  *failed = is.fail();
  return v;
}

int main() {
  bool f;
  CHECK(get<int>("2147483647", &f) == INT_MAX && !f);
  CHECK(get<int>("2147483648", &f) == INT_MAX && f);
  CHECK(get<int>("-2147483649", &f) == INT_MIN && f);
  CHECK(get<short>("40000", &f) == SHRT_MAX && f);
  CHECK(get<short>("-40000", &f) == SHRT_MIN && f);
  CHECK(get<long long>("99999999999999999999", &f) == LLONG_MAX && f);
  CHECK(get<long long>("-99999999999999999999", &f) == LLONG_MIN && f);
  CHECK(get<unsigned long long>("18446744073709551615", &f) == ULLONG_MAX && !f);
  CHECK(get<unsigned long long>("18446744073709551616", &f) == ULLONG_MAX && f);
  CHECK(get<unsigned short>("70000", &f) == USHRT_MAX && f);
  CHECK(get<unsigned>("4294967296", &f) == UINT_MAX && f);
  return 0;
}
