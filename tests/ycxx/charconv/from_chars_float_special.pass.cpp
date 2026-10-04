// [charconv.from.chars]/6: the pattern is strtod's subject sequence, which includes
// "INF or INFINITY, ignoring case" and "NAN or NAN(n-char-sequence opt), ignoring case"
// (ISO C 7.24.2.6), optionally preceded by '-' ('+' only in the exponent). The longest
// matching prefix is consumed. This holds for every chars_format value.
#include <charconv>
#include <cmath>
#include <limits>
#include <string_view>
#include <system_error>
#include "check.hpp"

using F = std::chars_format;

template <class T>
void check_type() {
  const T inf = std::numeric_limits<T>::infinity();
  auto run = [](std::string_view s, T& v, F f) {
    auto r = std::from_chars(s.data(), s.data() + s.size(), v, f);
    CHECK(r.ec == std::errc{});
    return r.ptr - s.data();
  };
  for (F f : {F::general, F::fixed, F::scientific, F::hex}) {
    T v = 0;
    CHECK(run("inf", v, f) == 3 && v == inf);
    CHECK(run("INF", v, f) == 3 && v == inf);
    CHECK(run("-Inf", v, f) == 4 && v == -inf);
    CHECK(run("infinity", v, f) == 8 && v == inf);
    CHECK(run("InFiNiTy", v, f) == 8 && v == inf);
    CHECK(run("-infinityx", v, f) == 9 && v == -inf);
    CHECK(run("infinit", v, f) == 3 && v == inf);  // only "inf" matches
    CHECK(run("infx", v, f) == 3 && v == inf);
    CHECK(run("nan", v, f) == 3 && std::isnan(v));
    v = 0;
    CHECK(run("NaN", v, f) == 3 && std::isnan(v));
    v = 0;
    CHECK(run("-nan", v, f) == 4 && std::isnan(v));
    v = 0;
    CHECK(run("nan(123)", v, f) == 8 && std::isnan(v));
    v = 0;
    CHECK(run("nan(abc_DEF09)", v, f) == 14 && std::isnan(v));
    v = 0;
    CHECK(run("nan()", v, f) == 5 && std::isnan(v));
    v = 0;
    CHECK(run("nan(12", v, f) == 3 && std::isnan(v));  // unterminated: just "nan"
    v = 0;
    CHECK(run("nan(1 2)", v, f) == 3 && std::isnan(v));
    // '+' is not allowed and "in" alone does not match
    T w = 5;
    std::string_view p = "+inf";
    auto r = std::from_chars(p.data(), p.data() + p.size(), w, f);
    CHECK(r.ec == std::errc::invalid_argument && r.ptr == p.data() && w == 5);
    p = "in";
    r = std::from_chars(p.data(), p.data() + p.size(), w, f);
    CHECK(r.ec == std::errc::invalid_argument && r.ptr == p.data() && w == 5);
    p = "na";
    r = std::from_chars(p.data(), p.data() + p.size(), w, f);
    CHECK(r.ec == std::errc::invalid_argument && w == 5);
  }
}

int main() {
  check_type<float>();
  check_type<double>();
  check_type<long double>();
  return 0;
}
