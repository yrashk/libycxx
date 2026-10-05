// num_put of floating-point values with floatfield == fixed | scientific (std::hexfloat).
// [facet.num.put.virtuals] Stage 1: the characters are those "that would be printed by printf
// ... given this conversion specifier" in the "C" locale; Table 98: %a, or %A with uppercase;
// Table 99: the length modifier L for long double; Table 100: + with showpos; the precision is
// given only "if floatfield != (ios_base::fixed | ios_base::scientific)", so the output is
// exact whatever precision() is. ISO C 7.23.6.1 (a, A): "[-]0xh.hhhhp±d", the digit before
// the point being nonzero for a normalized value and otherwise unspecified, the exponent at
// least one digit; A uses X and P and upper-case digits; infinity is "inf"/"INF". Checked by
// reading the value back with strtold / strtod and by the form of the text, so that the test
// does not depend on which leading digit the implementation picks.
// COUNTERPART: libcxx:localization/locale.categories/category.numeric/locale.nm.put/facet.num.put.members/put_long_double.hex.pass.cpp
#include <cctype>
#include <cstdlib>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <type_traits>
#include "check.hpp"

template <class T>
std::string put(T v, std::ios_base::fmtflags extra = {}, int prec = 3) {
  std::ostringstream os;
  os.imbue(std::locale::classic());
  os.precision(prec);
  os.setf(std::ios_base::fixed | std::ios_base::scientific, std::ios_base::floatfield);
  os.setf(extra);
  os << v;
  return os.str();
}

// [-]0xh[.h...]p(+|-)d...
bool well_formed(const std::string& s, bool upper) {
  std::size_t i = 0;
  if (i < s.size() && (s[i] == '-' || s[i] == '+')) ++i;
  if (s.compare(i, 2, upper ? "0X" : "0x") != 0) return false;
  i += 2;
  auto hex = [&](char c) {
    return std::isdigit(static_cast<unsigned char>(c)) || (upper ? (c >= 'A' && c <= 'F') : (c >= 'a' && c <= 'f'));
  };
  if (i >= s.size() || !hex(s[i++])) return false;
  if (i < s.size() && s[i] == '.') {
    ++i;
    if (i >= s.size() || !hex(s[i])) return false;
    while (i < s.size() && hex(s[i])) ++i;
  }
  if (i >= s.size() || s[i++] != (upper ? 'P' : 'p')) return false;
  if (i >= s.size() || (s[i] != '+' && s[i] != '-')) return false;
  ++i;
  if (i >= s.size()) return false;
  for (; i < s.size(); ++i)
    if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
  return true;
}

template <class T>
T read_back(const std::string& s) {
  if constexpr (std::is_same_v<T, long double>)
    return std::strtold(s.c_str(), nullptr);
  else
    return std::strtod(s.c_str(), nullptr);
}

template <class T>
void check(T v) {
  const std::string s = put(v);
  CHECK(well_formed(s, false));
  CHECK(read_back<T>(s) == v);
  CHECK(put(v, {}, 0) == s && put(v, {}, 30) == s);  // the precision does not apply
  const std::string u = put(v, std::ios_base::uppercase);
  CHECK(well_formed(u, true));
  CHECK(read_back<T>(u) == v);
  const std::string p = put(v, std::ios_base::showpos);
  CHECK(v < 0 ? p == s : p == "+" + s);
}

int main() {
  using L = std::numeric_limits<long double>;
  const long double lds[] = {1.0L, -1.0L, 0.0L, 0.1L, 1234567.875L, -3.0e-4000L, 1.5e4000L,
                             L::max(), L::min(), L::denorm_min(), L::epsilon()};
  for (long double v : lds) check(v);
  using D = std::numeric_limits<double>;
  const double ds[] = {1.0, -2.5, 0.1, 6.02214076e23, D::max(), D::denorm_min()};
  for (double v : ds) check(v);
  CHECK(put(L::infinity()) == "inf" && put(-L::infinity(), std::ios_base::uppercase) == "-INF");
  CHECK(put(D::infinity(), std::ios_base::showpos) == "+inf");
}
