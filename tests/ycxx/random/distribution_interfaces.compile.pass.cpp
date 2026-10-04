// [rand.dist]: result types and default template arguments of each distribution, its parameter
// accessors with their types, and [rand.req.dist]/9: "For each of the constructors of D taking
// arguments corresponding to parameters of the distribution, P shall have a corresponding
// constructor subject to the same requirements and taking arguments identical in number, type, and
// default values. Moreover, for each of the member functions of D that return values corresponding
// to parameters of the distribution, P shall have a corresponding member function with the
// identical name, type, and semantics."
#include <random>
#include <concepts>
#include <initializer_list>
#include <type_traits>
#include <vector>

using namespace std;

static_assert(is_same_v<uniform_int_distribution<>::result_type, int>);
static_assert(is_same_v<uniform_real_distribution<>::result_type, double>);
static_assert(is_same_v<bernoulli_distribution::result_type, bool>);
static_assert(is_same_v<binomial_distribution<>::result_type, int>);
static_assert(is_same_v<geometric_distribution<>::result_type, int>);
static_assert(is_same_v<negative_binomial_distribution<>::result_type, int>);
static_assert(is_same_v<poisson_distribution<>::result_type, int>);
static_assert(is_same_v<exponential_distribution<>::result_type, double>);
static_assert(is_same_v<gamma_distribution<>::result_type, double>);
static_assert(is_same_v<weibull_distribution<>::result_type, double>);
static_assert(is_same_v<extreme_value_distribution<>::result_type, double>);
static_assert(is_same_v<normal_distribution<>::result_type, double>);
static_assert(is_same_v<lognormal_distribution<>::result_type, double>);
static_assert(is_same_v<chi_squared_distribution<>::result_type, double>);
static_assert(is_same_v<cauchy_distribution<>::result_type, double>);
static_assert(is_same_v<fisher_f_distribution<>::result_type, double>);
static_assert(is_same_v<student_t_distribution<>::result_type, double>);
static_assert(is_same_v<discrete_distribution<>::result_type, int>);
static_assert(is_same_v<piecewise_constant_distribution<>::result_type, double>);
static_assert(is_same_v<piecewise_linear_distribution<>::result_type, double>);
static_assert(is_same_v<uniform_int_distribution<long long>::result_type, long long>);
static_assert(is_same_v<normal_distribution<float>::result_type, float>);

// Both D and P: constructible from the parameter lists, with the accessors.
template <class X>
concept uni_int = requires(const X x) {
  X(1);
  X(1, 2);
  { x.a() } -> same_as<int>;
  { x.b() } -> same_as<int>;
};
template <class X>
concept uni_real = requires(const X x) {
  X(1.0);
  X(1.0, 2.0);
  { x.a() } -> same_as<double>;
  { x.b() } -> same_as<double>;
};
template <class X>
concept bern = requires(const X x) {
  X(0.5);
  { x.p() } -> same_as<double>;
};
template <class X>
concept binom = requires(const X x) {
  X(3);
  X(3, 0.5);
  { x.t() } -> same_as<int>;
  { x.p() } -> same_as<double>;
};
template <class X>
concept geo = requires(const X x) {
  X(0.5);
  { x.p() } -> same_as<double>;
};
template <class X>
concept negbin = requires(const X x) {
  X(3);
  X(3, 0.5);
  { x.k() } -> same_as<int>;
  { x.p() } -> same_as<double>;
};
template <class X>
concept pois = requires(const X x) {
  X(2.0);
  { x.mean() } -> same_as<double>;
};
template <class X, class R>
concept expo = requires(const X x) {
  X(R(2));
  { x.lambda() } -> same_as<R>;
};
template <class X, class R>
concept gam = requires(const X x) {
  X(R(2));
  X(R(2), R(3));
  { x.alpha() } -> same_as<R>;
  { x.beta() } -> same_as<R>;
};
template <class X, class R>
concept ab = requires(const X x) {  // weibull, extreme_value, cauchy
  X(R(2));
  X(R(2), R(3));
  { x.a() } -> same_as<R>;
  { x.b() } -> same_as<R>;
};
template <class X, class R>
concept norm = requires(const X x) {
  X(R(2));
  X(R(2), R(3));
  { x.mean() } -> same_as<R>;
  { x.stddev() } -> same_as<R>;
};
template <class X, class R>
concept lognorm = requires(const X x) {
  X(R(2));
  X(R(2), R(3));
  { x.m() } -> same_as<R>;
  { x.s() } -> same_as<R>;
};
template <class X, class R>
concept one_n = requires(const X x) {  // chi_squared, student_t
  X(R(2));
  { x.n() } -> same_as<R>;
};
template <class X, class R>
concept fisher = requires(const X x) {
  X(R(2));
  X(R(2), R(3));
  { x.m() } -> same_as<R>;
  { x.n() } -> same_as<R>;
};
template <class X>
concept discrete = requires(const X x, const double* it, initializer_list<double> il, double (*fw)(double)) {
  X();
  X(it, it);
  X(il);
  X(size_t(3), 0.0, 1.0, fw);
  { x.probabilities() } -> same_as<vector<double>>;
};
template <class X, class R>
concept piecewise = requires(const X x, const R* bi, const double* wi, initializer_list<R> bl, double (*fw)(double)) {
  X();
  X(bi, bi, wi);
  X(bl, fw);
  X(size_t(3), R(0), R(1), fw);
  { x.intervals() } -> same_as<vector<R>>;
  { x.densities() } -> same_as<vector<R>>;
};

template <class D> using P = typename D::param_type;

static_assert(uni_int<uniform_int_distribution<>> && uni_int<P<uniform_int_distribution<>>>);
static_assert(uni_real<uniform_real_distribution<>> && uni_real<P<uniform_real_distribution<>>>);
static_assert(bern<bernoulli_distribution> && bern<P<bernoulli_distribution>>);
static_assert(binom<binomial_distribution<>> && binom<P<binomial_distribution<>>>);
static_assert(geo<geometric_distribution<>> && geo<P<geometric_distribution<>>>);
static_assert(negbin<negative_binomial_distribution<>> && negbin<P<negative_binomial_distribution<>>>);
static_assert(pois<poisson_distribution<>> && pois<P<poisson_distribution<>>>);
static_assert(expo<exponential_distribution<float>, float> && expo<P<exponential_distribution<float>>, float>);
static_assert(gam<gamma_distribution<>, double> && gam<P<gamma_distribution<>>, double>);
static_assert(ab<weibull_distribution<>, double> && ab<P<weibull_distribution<>>, double>);
static_assert(ab<extreme_value_distribution<>, double> && ab<P<extreme_value_distribution<>>, double>);
static_assert(ab<cauchy_distribution<float>, float> && ab<P<cauchy_distribution<float>>, float>);
static_assert(norm<normal_distribution<>, double> && norm<P<normal_distribution<>>, double>);
static_assert(lognorm<lognormal_distribution<>, double> && lognorm<P<lognormal_distribution<>>, double>);
static_assert(one_n<chi_squared_distribution<>, double> && one_n<P<chi_squared_distribution<>>, double>);
static_assert(one_n<student_t_distribution<>, double> && one_n<P<student_t_distribution<>>, double>);
static_assert(fisher<fisher_f_distribution<>, double> && fisher<P<fisher_f_distribution<>>, double>);
static_assert(discrete<discrete_distribution<>> && discrete<P<discrete_distribution<>>>);
static_assert(piecewise<piecewise_constant_distribution<>, double> &&
              piecewise<P<piecewise_constant_distribution<>>, double>);
static_assert(piecewise<piecewise_linear_distribution<>, double> &&
              piecewise<P<piecewise_linear_distribution<>>, double>);

// Default constructors of D and P are both available.
static_assert(is_default_constructible_v<P<uniform_int_distribution<>>>);
static_assert(is_default_constructible_v<P<normal_distribution<>>>);
static_assert(is_default_constructible_v<P<bernoulli_distribution>>);
static_assert(is_default_constructible_v<P<discrete_distribution<>>>);

int main() {}
