// [complex.ops]/9-15: operator>> extracts u, (u) or (u,v) and sets failbit on bad input;
// operator<< inserts as if by formatting '(' << real << ',' << imag << ')' into an
// ostringstream with the stream's flags, locale and precision, then inserting the string
// (so the field width applies to the whole representation).
#include <complex>
#include <sstream>
#include <string>
#include "check.hpp"

using C = std::complex<double>;

int main() {
  {
    std::ostringstream os;
    os << C(1, -2.5);
    CHECK(os.str() == "(1,-2.5)");
  }
  {
    std::ostringstream os;
    os.precision(3);
    os << std::complex<float>(3.14159f, 2.0f);
    CHECK(os.str() == "(3.14,2)");
  }
  {
    std::ostringstream os;
    os << std::showpoint << std::fixed;
    os.precision(1);
    os << C(1, 2);
    CHECK(os.str() == "(1.0,2.0)");
  }
  {
    std::ostringstream os;
    os.width(10);
    os << C(1, 2);  // width applies to the whole string "(1,2)"
    CHECK(os.str() == "     (1,2)");
    os << C(3, 4);  // width was reset
    CHECK(os.str() == "     (1,2)(3,4)");
  }
  {
    std::wostringstream os;
    os << std::complex<long double>(0.5L, 4);
    CHECK(os.str() == L"(0.5,4)");
  }
  {
    std::istringstream is("(1.5,-2) 3 (4)  ( 5 , 6 )");
    C a, b, c, d;
    is >> a >> b >> c >> d;
    CHECK(!is.fail());
    CHECK(a == C(1.5, -2));
    CHECK(b == C(3, 0));
    CHECK(c == C(4, 0));
    CHECK(d == C(5, 6));
  }
  {
    std::istringstream is("(1;2)");
    C a(9, 9);
    is >> a;
    CHECK(is.fail());
  }
  {
    std::istringstream is("(1,2");
    C a;
    is >> a;
    CHECK(is.fail());
  }
  {
    std::istringstream is("x");
    std::complex<float> a;
    is >> a;
    CHECK(is.fail());
  }
  {
    std::wistringstream is(L"(7,8)");
    std::complex<float> a;
    is >> a;
    CHECK(!is.fail() && a == std::complex<float>(7, 8));
  }
  return 0;
}
