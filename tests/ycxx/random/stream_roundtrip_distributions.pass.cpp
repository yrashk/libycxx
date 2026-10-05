// [rand.req.dist]/6: "If a textual representation is written using os << x and that
// representation is restored into the same or a different object y of the same type using
// is >> y, repeated invocations of y(g) shall produce the same sequence of numbers as would
// repeated invocations of x(g)." Table 128, os << x: "Writes to os a textual representation
// for the parameters and the additional internal data of x." "Postconditions: The os.fmtflags
// and fill character are unchanged." is >> d: "Restores from is the parameters and additional
// internal data of the lvalue d." The only preconditions are on the locale and the stream's
// template arguments, so the round trip must be exact whatever formatting state the streams
// carry: here precision(2) with fixed (or hexfloat, or scientific|uppercase|showpos) on output
// and hex on input. The parameters are values with no short decimal form (1/3, 0.1, pi/7), and
// each distribution is round-tripped fresh, after one call (normal_distribution and the
// distributions built on it may keep a second value: "additional internal data"), and after
// several calls. x == y ([rand.req.dist] Table 128: same parameters and same future sequences)
// is checked too.
#include <random>
#include <sstream>
#include <vector>
#include "check.hpp"

using B = std::ios_base;

template <class D, class Ch>
void roundtrip_one(const D& x, B::fmtflags out_flags, const std::mt19937_64& g0) {
  std::basic_ostringstream<Ch> os;
  os.flags(out_flags);
  os.precision(2);
  os.fill(Ch('#'));
  os << x;
  CHECK(os.good());
  CHECK(os.flags() == out_flags);
  CHECK(os.fill() == Ch('#'));

  std::basic_istringstream<Ch> is(os.str());
  is.flags(B::hex | B::skipws);
  D y;
  is >> y;
  CHECK(!is.fail());
  CHECK(is.flags() == (B::hex | B::skipws));
  CHECK(y == x);
  CHECK(y.param() == x.param());
  D xc = x;
  auto g1 = g0, g2 = g0;
  for (int i = 0; i < 200; ++i) CHECK(xc(g1) == y(g2));
}

template <class D>
void check(D x) {
  std::mt19937_64 g(2024);
  const B::fmtflags hostile[] = {B::fixed, B::fixed | B::scientific,
                                 B::scientific | B::uppercase | B::showpos | B::showpoint,
                                 B::hex | B::showbase | B::left};
  for (int calls : {0, 1, 3, 10}) {
    for (int i = 0; i < calls; ++i) x(g);
    for (auto f : hostile) {
      roundtrip_one<D, char>(x, f, g);
      roundtrip_one<D, wchar_t>(x, f, g);
    }
  }
}

int main() {
  const double third = 1.0 / 3, tenth = 0.1, pi7 = 3.14159265358979323846 / 7;
  check(std::uniform_int_distribution<long long>(-123456789012345LL, 98765432109876LL));
  check(std::uniform_int_distribution<unsigned short>(3, 60000));
  check(std::uniform_real_distribution<double>(-third, pi7));
  check(std::uniform_real_distribution<float>(tenth, 7.3f));
  check(std::bernoulli_distribution(third));
  check(std::binomial_distribution<int>(1000, tenth));
  check(std::geometric_distribution<long>(pi7));
  check(std::negative_binomial_distribution<int>(7, third));
  check(std::poisson_distribution<int>(pi7 * 100));
  check(std::poisson_distribution<int>(third));
  check(std::exponential_distribution<double>(third));
  check(std::gamma_distribution<double>(third, pi7));
  check(std::gamma_distribution<double>(7.1, tenth));
  check(std::weibull_distribution<double>(pi7, third));
  check(std::extreme_value_distribution<double>(-third, tenth));
  check(std::normal_distribution<double>(third, pi7));
  check(std::normal_distribution<float>(tenth, 3.3f));
  check(std::normal_distribution<long double>(third, tenth));
  check(std::lognormal_distribution<double>(tenth, third));
  check(std::chi_squared_distribution<double>(third * 10));
  check(std::cauchy_distribution<double>(third, tenth));
  check(std::fisher_f_distribution<double>(third * 10, pi7 * 10));
  check(std::student_t_distribution<double>(pi7 * 10));
  check(std::discrete_distribution<int>({third, tenth, pi7, 1.0, 0.0, 2.0}));
  {
    std::vector<double> b{-third, tenth, pi7, 1.0}, w{third, 0.0, pi7};
    check(std::piecewise_constant_distribution<double>(b.begin(), b.end(), w.begin()));
    std::vector<double> wl{third, tenth, 0.0, pi7};
    check(std::piecewise_linear_distribution<double>(b.begin(), b.end(), wl.begin()));
  }
  return 0;
}
