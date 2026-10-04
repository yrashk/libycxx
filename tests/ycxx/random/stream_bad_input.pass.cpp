// [rand.req.eng] Table 127, is >> v: "If bad input is encountered, ensures that v's state is
// unchanged by the operation and calls is.setstate(ios_base::failbit)"; [rand.req.dist]/17
// (Table 128), is >> d: "If bad input is encountered, ensures that d is unchanged by the
// operation and calls is.setstate(ios_base::failbit)". Bad input here: the textual representation
// of another engine/distribution ([rand.req.eng]: os << x writes the state as decimal numbers
// separated by spaces) with tokens removed at the end (the stream ends early) or with one token
// replaced by a non-number. "Unchanged" is checked with == (Table 127: equal states; Table 128:
// equal parameters and equal subsequent results) and by the next values produced.
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include "check.hpp"

std::vector<std::string> tokens(const std::string& s) {
  std::istringstream is(s);
  std::vector<std::string> t;
  for (std::string w; is >> w;) t.push_back(w);
  return t;
}
std::string join(const std::vector<std::string>& t, std::size_t n) {
  std::string s;
  for (std::size_t i = 0; i < n; ++i) s += (i ? " " : "") + t[i];
  return s;
}

template <class X>
std::vector<std::string> bad_inputs(const X& other) {
  std::ostringstream os;
  os << other;
  auto t = tokens(os.str());
  std::vector<std::string> bad;
  bad.push_back("");
  bad.push_back("x");
  for (std::size_t drop : {std::size_t(1), std::size_t(2), t.size() / 2, t.size() - 1})
    if (drop >= 1 && drop <= t.size()) bad.push_back(join(t, t.size() - drop));
  for (std::size_t pos : {std::size_t(0), t.size() / 2, t.size() - 1}) {
    auto u = t;
    u[pos] = "#";
    bad.push_back(join(u, u.size()));
  }
  return bad;
}

template <class E>
void check_engine() {
  E other(12345u);
  other.discard(3);
  for (const std::string& b : bad_inputs(other)) {
    E e(777u);
    e.discard(5);
    const E before = e;
    std::istringstream is(b);
    is >> e;
    CHECK(is.fail());
    CHECK(e == before);
    E copy = before;
    for (int i = 0; i < 5; ++i) CHECK(e() == copy());
  }
  // control: the complete representation restores the state
  std::ostringstream os;
  os << other;
  E e;
  std::istringstream is(os.str());
  is >> e;
  CHECK(!is.fail() && e == other);
}

template <class D>
void check_dist(const D& other, const D& start) {
  for (const std::string& b : bad_inputs(other)) {
    D d = start;
    std::mt19937 g1(5), g2(5);
    D copy = start;
    std::istringstream is(b);
    is >> d;
    CHECK(is.fail());
    CHECK(d == start);
    CHECK(d.param() == start.param());
    for (int i = 0; i < 4; ++i) CHECK(d(g1) == copy(g2));
  }
  std::ostringstream os;
  os << other;
  D d = start;
  std::istringstream is(os.str());
  is >> d;
  CHECK(!is.fail() && d == other);
}

int main() {
  check_engine<std::minstd_rand>();
  check_engine<std::mt19937>();
  check_engine<std::mt19937_64>();
  check_engine<std::ranlux24_base>();
  check_engine<std::ranlux48>();
  check_engine<std::knuth_b>();
  check_engine<std::independent_bits_engine<std::minstd_rand, 48, std::uint64_t>>();
  check_engine<std::philox4x32>();
  check_engine<std::philox4x64>();

  check_dist(std::uniform_int_distribution<int>(-5, 9), std::uniform_int_distribution<int>(1, 2));
  check_dist(std::uniform_real_distribution<double>(-1.5, 2.25), std::uniform_real_distribution<double>(0, 3));
  check_dist(std::bernoulli_distribution(0.25), std::bernoulli_distribution(0.75));
  check_dist(std::binomial_distribution<int>(7, 0.25), std::binomial_distribution<int>(3, 0.5));
  check_dist(std::geometric_distribution<int>(0.125), std::geometric_distribution<int>(0.5));
  check_dist(std::negative_binomial_distribution<int>(3, 0.5), std::negative_binomial_distribution<int>(2, 0.25));
  check_dist(std::poisson_distribution<int>(4.5), std::poisson_distribution<int>(1.5));
  check_dist(std::exponential_distribution<double>(2.5), std::exponential_distribution<double>(0.5));
  check_dist(std::gamma_distribution<double>(2.5, 1.5), std::gamma_distribution<double>(0.5, 2));
  check_dist(std::weibull_distribution<double>(1.5, 2), std::weibull_distribution<double>(3, 1));
  check_dist(std::extreme_value_distribution<double>(-1, 2), std::extreme_value_distribution<double>(1, 3));
  {
    std::normal_distribution<double> n1(1.5, 0.25), n2(-2, 4);
    std::mt19937 g;
    n2(g);  // may leave a cached second value: "additional internal data"
    check_dist(n1, n2);
  }
  check_dist(std::lognormal_distribution<double>(0.5, 0.75), std::lognormal_distribution<double>(1, 2));
  check_dist(std::chi_squared_distribution<double>(3.5), std::chi_squared_distribution<double>(1));
  check_dist(std::cauchy_distribution<double>(0.5, 2), std::cauchy_distribution<double>(1, 1));
  check_dist(std::fisher_f_distribution<double>(3, 5), std::fisher_f_distribution<double>(1, 2));
  check_dist(std::student_t_distribution<double>(4.5), std::student_t_distribution<double>(2));
  check_dist(std::discrete_distribution<int>({1.0, 2.0, 5.0}), std::discrete_distribution<int>({3.0, 1.0}));
  {
    double b1[] = {0, 1, 3, 7}, w1[] = {1, 2, 0.5};
    double b2[] = {-1, 1}, w2[] = {4};
    check_dist(std::piecewise_constant_distribution<double>(b1, b1 + 4, w1),
               std::piecewise_constant_distribution<double>(b2, b2 + 2, w2));
    double v1[] = {1, 2, 0.5, 3}, v2[] = {4, 1};
    check_dist(std::piecewise_linear_distribution<double>(b1, b1 + 4, v1),
               std::piecewise_linear_distribution<double>(b2, b2 + 2, v2));
  }
  return 0;
}
