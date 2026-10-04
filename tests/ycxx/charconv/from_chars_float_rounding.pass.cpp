// [charconv.from.chars]/1: value "is set to the parsed value, after rounding according to
// round_to_nearest ([round.style])" -- correctly rounded, ties to even -- regardless of the
// number of digits; /6: "In any case, the resulting value is one of at most two
// floating-point values closest to the value of the string matching the pattern."
#include <charconv>
#include <cfloat>
#include <string>
#include <string_view>
#include <system_error>
#include "check.hpp"

template <class T>
T parse(std::string_view s, std::chars_format f = std::chars_format::general) {
  T v{};
  auto r = std::from_chars(s.data(), s.data() + s.size(), v, f);
  CHECK(r.ec == std::errc{});
  CHECK(r.ptr == s.data() + s.size());
  return v;
}

int main() {
  // exact decimal values of doubles parse back exactly
  CHECK(parse<double>("0.1000000000000000055511151231257827021181583404541015625") == 0.1);
  CHECK(parse<double>("0.1") == 0.1);
  CHECK(parse<double>("0.3") == 0.3);
  CHECK(parse<double>("0.30000000000000004") == 0.1 + 0.2);
  // ties to even at 2^53: 2^53 + 1 lies halfway between 2^53 and 2^53 + 2
  CHECK(parse<double>("9007199254740993") == 9007199254740992.0);
  CHECK(parse<double>("9007199254740995") == 9007199254740996.0);
  // just above the halfway point rounds up
  CHECK(parse<double>("9007199254740993.0000000000000000000001") == 9007199254740994.0);
  // halfway between 1 and the next double: 1 + 2^-53, ties to even (1.0)
  CHECK(parse<double>("1.00000000000000011102230246251565404236316680908203125") == 1.0);
  CHECK(parse<double>("1.00000000000000011102230246251565404236316680908203126") == 1.0 + DBL_EPSILON);
  // hex: 1 + 2^-53 exactly halfway, 1 + 3 * 2^-54 above
  CHECK(parse<double>("1.00000000000008", std::chars_format::hex) == 1.0);
  CHECK(parse<double>("1.00000000000018", std::chars_format::hex) == 1.0 + 2 * DBL_EPSILON);
  CHECK(parse<double>("1.000000000000080000001", std::chars_format::hex) == 1.0 + DBL_EPSILON);
  // float ties: 2^24 + 1 rounds to 2^24, 2^24 + 3 to 2^24 + 4
  CHECK(parse<float>("16777217") == 16777216.0f);
  CHECK(parse<float>("16777219") == 16777220.0f);
  CHECK(parse<float>("0.1") == 0.1f);
  // subnormals and the smallest values
  CHECK(parse<double>("4.9406564584124654e-324") == DBL_TRUE_MIN);
  CHECK(parse<double>("5e-324") == DBL_TRUE_MIN);
  CHECK(parse<double>("2.2250738585072011e-308") == 2.2250738585072009e-308);
  CHECK(parse<float>("1.4e-45") == FLT_TRUE_MIN);
  // many leading zeros, long significands, large exponents with compensating digits
  CHECK(parse<double>(std::string("0.") + std::string(300, '0') + "1") == 1e-301);
  CHECK(parse<double>(std::string("1") + std::string(300, '0')) == 1e300);
  CHECK(parse<double>(std::string("1") + std::string(300, '0') + "e-300") == 1.0);
  CHECK(parse<double>("0.000001e6") == 1.0);
  CHECK(parse<double>("123456789012345678901234567890") == 123456789012345678901234567890.0);
  CHECK(parse<double>("1.7976931348623158e308") == DBL_MAX);  // below the overflow threshold
  return 0;
}
