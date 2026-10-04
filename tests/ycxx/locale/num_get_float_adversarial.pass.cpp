// [facet.num.get.virtuals] Stage 2: "char c = src[find(atoms, atoms + sizeof(src) - 1, ct) -
// atoms]" with "static const char src[] = "0123456789abcdefpxABCDEFPX+-";" -- only those
// characters (and the decimal point) can be accumulated, so "inf" / "nan" are never a field;
// a character is accumulated if it "is allowed as the next character of an input field of
// the conversion specifier returned by Stage 1" (%g; Example 1 accumulates "0x1a.bp+07"). No
// limit is placed on the length of the field.
// Stage 3: converted with strtof / strtod / strtold; the value stored is "zero, if the
// conversion function does not convert the entire field" or "the converted value, otherwise";
// "If the conversion function does not convert the entire field, or if the field represents a
// value outside the range of representable values, ios_base::failbit is assigned to err."
// ISO C 7.24.1.5 (strtod): if the correct value overflows, plus or minus HUGE_VAL (HUGE_VALF,
// HUGE_VALL) is returned; hexadecimal input is converted exactly when representable; decimal
// input is correctly rounded (round-to-nearest).
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
  std::string rest;

  // In range at the extremes.
  CHECK(get<double>("1.7976931348623157e308", err) == std::numeric_limits<double>::max() && err == B::eofbit);
  CHECK(get<float>("3.40282347e38", err) == std::numeric_limits<float>::max() && err == B::eofbit);
  // Subnormals and underflow to zero (failbit is not checked: whether ERANGE underflow is
  // "outside the range of representable values" is not settled here).
  CHECK(get<double>("4.9406564584124654e-324", err) == std::numeric_limits<double>::denorm_min());
  CHECK(get<float>("1.40129846e-45", err) == std::numeric_limits<float>::denorm_min());
  CHECK(get<double>("1e-400", err) == 0.0);
  CHECK(get<double>("-1e-400", err) == 0.0);

  // inf / nan are not fields: nothing is accumulated, zero and failbit, input left alone.
  CHECK(get<double>("inf", err, &rest) == 0.0 && err == B::failbit && rest == "inf");
  CHECK(get<double>("nan", err, &rest) == 0.0 && err == B::failbit && rest == "nan");
  CHECK(get<double>("-inf", err, &rest) == 0.0 && (err & B::failbit) && rest == "inf");
  CHECK(get<double>("INFINITY", err, &rest) == 0.0 && err == B::failbit && rest == "INFINITY");

  // Very long fields are converted as a whole.
  const std::string zeros(600, '0');
  CHECK(get<double>("0." + zeros + "1e600", err) == 0.1 && err == B::eofbit);
  CHECK(get<double>(zeros + "2.5", err) == 2.5 && err == B::eofbit);
  CHECK(get<long>(zeros + "123", err) == 123 && err == B::eofbit);
  CHECK(get<double>("1" + zeros + "e-600", err) == 1.0 && err == B::eofbit);
  // Just above / at the halfway point between 1 and its successor 1 + 2^-52:
  // 1 + 2^-53 = 1.00000000000000011102230246251565404236316680908203125 exactly.
  const std::string half = "1.00000000000000011102230246251565404236316680908203125";
  const double up = 1.0 + std::numeric_limits<double>::epsilon();
  CHECK(get<double>(half, err) == 1.0);  // tie: to even
  CHECK(get<double>(half + zeros + "1", err) == up);
  CHECK(get<double>(half + zeros, err) == 1.0);
  CHECK(get<double>("1.00000000000000011102230246251565404236316680908203124" + std::string(400, '9'), err) == 1.0);
  // 1 + 3 * 2^-53 is the tie between 1 + 2^-52 and 1 + 2^-51: to even (1 + 2^-51).
  CHECK(get<double>("1.00000000000000033306690738754696212708950042724609375", err) == 1.0 + 2 * std::numeric_limits<double>::epsilon());


  // Fields that are not converted entirely: zero and failbit.
  CHECK(get<double>("1e+", err) == 0.0 && (err & B::failbit));
  CHECK(get<double>("-", err) == 0.0 && (err & B::failbit));
  CHECK(get<double>("+-1", err) == 0.0 && (err & B::failbit));
  CHECK(get<double>("1.5e3x", err, &rest) == 1500.0 && err == B::goodbit && rest == "x");
  return 0;
}
