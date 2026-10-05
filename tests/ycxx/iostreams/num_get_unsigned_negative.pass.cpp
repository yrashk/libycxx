// [facet.num.get.virtuals] Stage 3: "For an unsigned integer value, the function strtoull."
// "The numeric value to be stored can be one of: ... the most positive representable value,
// if the field to be converted to an unsigned integer type represents a value that cannot be
// represented in val." "If the conversion function does not convert the entire field, or if
// the field represents a value outside the range of representable values, ios_base::failbit
// is assigned to err." For val of type unsigned int (or unsigned short), a field "-1" or
// "-4294967295" represents a value that unsigned int cannot hold, whether that value is taken
// as the mathematical value (negative) or as the result of strtoull (2^64 - 1, 2^64 - 2^32 + 1,
// both larger than UINT_MAX): UINT_MAX is stored and failbit set. ([istream.formatted.
// arithmetic]/1: operator>>(unsigned int&) and operator>>(unsigned short&) call num_get::get
// with the value itself.) "-0" is zero.
// COUNTERPART: libcxx:localization/locale.categories/category.numeric/locale.num.get/facet.num.get.members/(get_unsigned_int|get_unsigned_short|test_neg_one).pass.cpp
#include <locale>
#include <sstream>
#include <iterator>
#include <climits>
#include "check.hpp"

template <class T>
static T get(const char* s, std::ios_base::iostate& err) {
  std::istringstream is(s);
  using It = std::istreambuf_iterator<char>;
  T v = 7;
  err = std::ios_base::goodbit;
  std::use_facet<std::num_get<char>>(is.getloc()).get(It(is), It(), is, err, v);
  return v;
}

int main() {
  using B = std::ios_base;
  B::iostate err;
  CHECK(get<unsigned>("-0", err) == 0u && err == B::eofbit);
  CHECK(get<unsigned>("-4294967295", err) == UINT_MAX && (err & B::failbit));
  CHECK(get<unsigned>("-1", err) == UINT_MAX && (err & B::failbit));
  CHECK(get<unsigned short>("-1", err) == USHRT_MAX && (err & B::failbit));
  CHECK(get<unsigned short>("-65535", err) == USHRT_MAX && (err & B::failbit));
  {
    std::istringstream is("-1");
    unsigned u = 5;
    is >> u;
    CHECK(u == UINT_MAX && is.fail());
  }
  return 0;
}
