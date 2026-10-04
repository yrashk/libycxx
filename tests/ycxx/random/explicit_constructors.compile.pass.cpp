// [rand.req.genl]/3: "Throughout [rand], any constructor that can be called with a single argument
// and that meets a requirement specified in this subclause shall be declared explicit."
// Engines: E(s), E(q); adaptors also A(const Engine&), A(Engine&&); distributions: D(p) and the
// parameter constructors callable with one argument; random_device(const string&).
#include <random>
#include <cstdint>
#include <string>
#include <type_traits>

template <class T, class Arg>
constexpr bool explicit_only = std::is_constructible_v<T, Arg> && !std::is_convertible_v<Arg, T>;

template <class E>
constexpr bool engine_ok = explicit_only<E, typename E::result_type> && explicit_only<E, std::seed_seq&> &&
                           std::is_default_constructible_v<E>;

static_assert(engine_ok<std::minstd_rand>);
static_assert(engine_ok<std::mt19937>);
static_assert(engine_ok<std::mt19937_64>);
static_assert(engine_ok<std::ranlux24_base>);
static_assert(engine_ok<std::ranlux48>);
static_assert(engine_ok<std::knuth_b>);
static_assert(engine_ok<std::philox4x32>);
static_assert(engine_ok<std::philox4x64>);
static_assert(engine_ok<std::independent_bits_engine<std::mt19937, 16, std::uint32_t>>);

static_assert(explicit_only<std::ranlux24, const std::ranlux24_base&>);
static_assert(explicit_only<std::ranlux24, std::ranlux24_base&&>);
static_assert(explicit_only<std::knuth_b, const std::minstd_rand0&>);
static_assert(explicit_only<std::knuth_b, std::minstd_rand0&&>);
static_assert(explicit_only<std::independent_bits_engine<std::mt19937, 16, std::uint32_t>, const std::mt19937&>);
static_assert(explicit_only<std::independent_bits_engine<std::mt19937, 16, std::uint32_t>, std::mt19937&&>);
static_assert(explicit_only<std::random_device, const std::string&>);

template <class D>
constexpr bool param_explicit = explicit_only<D, const typename D::param_type&>;

static_assert(param_explicit<std::uniform_int_distribution<>> && explicit_only<std::uniform_int_distribution<>, int>);
static_assert(param_explicit<std::uniform_real_distribution<>> && explicit_only<std::uniform_real_distribution<>, double>);
static_assert(param_explicit<std::bernoulli_distribution> && explicit_only<std::bernoulli_distribution, double>);
static_assert(param_explicit<std::binomial_distribution<>> && explicit_only<std::binomial_distribution<>, int>);
static_assert(param_explicit<std::geometric_distribution<>> && explicit_only<std::geometric_distribution<>, double>);
static_assert(param_explicit<std::negative_binomial_distribution<>> &&
              explicit_only<std::negative_binomial_distribution<>, int>);
static_assert(param_explicit<std::poisson_distribution<>> && explicit_only<std::poisson_distribution<>, double>);
static_assert(param_explicit<std::exponential_distribution<>> && explicit_only<std::exponential_distribution<>, double>);
static_assert(param_explicit<std::gamma_distribution<>> && explicit_only<std::gamma_distribution<>, double>);
static_assert(param_explicit<std::weibull_distribution<>> && explicit_only<std::weibull_distribution<>, double>);
static_assert(param_explicit<std::extreme_value_distribution<>> &&
              explicit_only<std::extreme_value_distribution<>, double>);
static_assert(param_explicit<std::normal_distribution<>> && explicit_only<std::normal_distribution<>, double>);
static_assert(param_explicit<std::lognormal_distribution<>> && explicit_only<std::lognormal_distribution<>, double>);
static_assert(param_explicit<std::chi_squared_distribution<>> && explicit_only<std::chi_squared_distribution<>, double>);
static_assert(param_explicit<std::cauchy_distribution<>> && explicit_only<std::cauchy_distribution<>, double>);
static_assert(param_explicit<std::fisher_f_distribution<>> && explicit_only<std::fisher_f_distribution<>, double>);
static_assert(param_explicit<std::student_t_distribution<>> && explicit_only<std::student_t_distribution<>, double>);
static_assert(param_explicit<std::discrete_distribution<>>);
static_assert(param_explicit<std::piecewise_constant_distribution<>>);
static_assert(param_explicit<std::piecewise_linear_distribution<>>);

int main() {}
