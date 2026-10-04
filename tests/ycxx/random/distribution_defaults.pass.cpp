// [rand.dist]: the default parameters of each distribution (default constructors and default
// arguments), the same defaults for param_type ([rand.req.dist]/9), and that the accessors return
// the values the object was constructed with.
#include <random>
#include <vector>
#include <limits>
#include "check.hpp"

int main() {
  using std::numeric_limits;
  {
    std::uniform_int_distribution<> d;
    CHECK(d.a() == 0 && d.b() == numeric_limits<int>::max());
    std::uniform_int_distribution<> e(5);
    CHECK(e.a() == 5 && e.b() == numeric_limits<int>::max());
    std::uniform_int_distribution<unsigned char> u;
    CHECK(u.a() == 0 && u.b() == 255);
    std::uniform_int_distribution<>::param_type p(5);
    CHECK(p.a() == 5 && p.b() == numeric_limits<int>::max());
    CHECK(std::uniform_int_distribution<>::param_type() == d.param());
  }
  {
    std::uniform_real_distribution<> d;
    CHECK(d.a() == 0.0 && d.b() == 1.0);
    std::uniform_real_distribution<> e(-3.0);
    CHECK(e.a() == -3.0 && e.b() == 1.0);
    std::uniform_real_distribution<float>::param_type p(0.5f);
    CHECK(p.a() == 0.5f && p.b() == 1.0f);
  }
  CHECK(std::bernoulli_distribution().p() == 0.5);
  CHECK(std::bernoulli_distribution::param_type().p() == 0.5);
  CHECK(std::binomial_distribution<>().t() == 1 && std::binomial_distribution<>().p() == 0.5);
  CHECK(std::binomial_distribution<>(7).p() == 0.5);
  CHECK(std::binomial_distribution<>::param_type(7).t() == 7);
  CHECK(std::geometric_distribution<>().p() == 0.5);
  CHECK(std::negative_binomial_distribution<>().k() == 1 && std::negative_binomial_distribution<>().p() == 0.5);
  CHECK(std::negative_binomial_distribution<>(4).p() == 0.5);
  CHECK(std::poisson_distribution<>().mean() == 1.0);
  CHECK(std::exponential_distribution<>().lambda() == 1.0);
  CHECK(std::gamma_distribution<>().alpha() == 1.0 && std::gamma_distribution<>().beta() == 1.0);
  CHECK(std::gamma_distribution<>(3.0).beta() == 1.0);
  CHECK(std::weibull_distribution<>().a() == 1.0 && std::weibull_distribution<>().b() == 1.0);
  CHECK(std::weibull_distribution<>(2.0).b() == 1.0);
  CHECK(std::extreme_value_distribution<>().a() == 0.0 && std::extreme_value_distribution<>().b() == 1.0);
  CHECK(std::extreme_value_distribution<>(2.0).b() == 1.0);
  CHECK(std::normal_distribution<>().mean() == 0.0 && std::normal_distribution<>().stddev() == 1.0);
  CHECK(std::normal_distribution<>(5.0).stddev() == 1.0);
  CHECK(std::normal_distribution<>::param_type(5.0).stddev() == 1.0);
  CHECK(std::lognormal_distribution<>().m() == 0.0 && std::lognormal_distribution<>().s() == 1.0);
  CHECK(std::lognormal_distribution<>(2.0).s() == 1.0);
  CHECK(std::chi_squared_distribution<>().n() == 1.0);
  CHECK(std::cauchy_distribution<>().a() == 0.0 && std::cauchy_distribution<>().b() == 1.0);
  CHECK(std::cauchy_distribution<>(2.0).b() == 1.0);
  CHECK(std::fisher_f_distribution<>().m() == 1.0 && std::fisher_f_distribution<>().n() == 1.0);
  CHECK(std::fisher_f_distribution<>(3.0).n() == 1.0);
  CHECK(std::student_t_distribution<>().n() == 1.0);

  // Accessors return the constructor arguments, for D and for param_type.
  std::binomial_distribution<long> bd(40, 0.125);
  CHECK(bd.t() == 40 && bd.p() == 0.125 && bd.param().t() == 40 && bd.param().p() == 0.125);
  std::gamma_distribution<float> gd(2.5f, 0.75f);
  CHECK(gd.alpha() == 2.5f && gd.beta() == 0.75f && gd.param().alpha() == 2.5f);
  std::fisher_f_distribution<> fd(3.5, 9.25);
  CHECK(fd.m() == 3.5 && fd.n() == 9.25 && fd.param().n() == 9.25);
  std::normal_distribution<> nd;
  nd.param(std::normal_distribution<>::param_type(-1.5, 0.25));
  CHECK(nd.mean() == -1.5 && nd.stddev() == 0.25);

  // The default sampling distributions ([rand.dist.samp]).
  std::discrete_distribution<> dd;
  std::vector<double> pr = dd.probabilities();
  CHECK(pr.size() == 1 && pr[0] == 1.0);
  CHECK(dd.min() == 0 && dd.max() == 0);
  std::piecewise_constant_distribution<> pc;
  std::vector<double> iv = pc.intervals(), dv = pc.densities();
  CHECK(iv.size() == 2 && iv[0] == 0.0 && iv[1] == 1.0);
  CHECK(dv.size() == 1 && dv[0] == 1.0);
  std::piecewise_linear_distribution<> pl;
  iv = pl.intervals();
  dv = pl.densities();
  CHECK(iv.size() == 2 && iv[0] == 0.0 && iv[1] == 1.0);
  CHECK(dv.size() >= 2 && dv[0] == 1.0 && dv[1] == 1.0);  // rho_0 = rho_1 = 1
}
