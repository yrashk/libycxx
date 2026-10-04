// [rand.req.dist] Table 128 and /4-10 for every distribution of [rand.dist]:
// result_type is arithmetic; D() behaves like any other default-constructed D; D(p) behaves like a
// distribution constructed from the values used for p; x.param() returns p with D(p).param() == p;
// d.param(p) gives d.param() == p; d(g) and d(g, p) return values in [min(), max()] (glb/lub of
// the possible values); x == y is an equivalence relation that implies equal future sequences for
// equal engines; reset() makes later uses independent of earlier engine values; D and P are
// copyable, P is equality comparable and has P::distribution_type = D; the sequence is not affected
// by calls of const member functions (/5).
#include <random>
#include <concepts>
#include <type_traits>
#include "check.hpp"

template <class D>
void check(const D& other) {
  using T = typename D::result_type;
  using P = typename D::param_type;
  static_assert(std::is_arithmetic_v<T>);
  static_assert(std::is_same_v<typename P::distribution_type, D>);
  static_assert(std::is_copy_constructible_v<D> && std::is_copy_assignable_v<D>);
  static_assert(std::is_copy_constructible_v<P> && std::is_copy_assignable_v<P>);
  static_assert(requires(D d, const D x, const D y, const P p, std::mt19937 g) {
    { d.reset() } -> std::same_as<void>;
    { x.param() } -> std::same_as<P>;
    { d.param(p) } -> std::same_as<void>;
    { d(g) } -> std::same_as<T>;
    { d(g, p) } -> std::same_as<T>;
    { x.min() } -> std::same_as<T>;
    { x.max() } -> std::same_as<T>;
    { x == y } -> std::same_as<bool>;
    { x != y } -> std::same_as<bool>;
    { p == p } -> std::same_as<bool>;
    { p != p } -> std::same_as<bool>;
  });

  const D a, b;
  CHECK(a == b && !(a != b));
  CHECK(a.param() == b.param());
  CHECK(D(a.param()).param() == a.param());
  CHECK(D(other.param()).param() == other.param());
  CHECK(D(other.param()) == other);
  CHECK(other.param() != a.param());
  CHECK(other != a);

  D d;
  d.param(other.param());
  CHECK(d.param() == other.param());
  if (d == other) {  // == implies equal sequences for equal engines
    D x(d), y(other);
    std::mt19937 g1(2), g2(2);
    for (int i = 0; i < 50; ++i) CHECK(x(g1) == y(g2));
  }
  D e(other);
  CHECK(e == other);
  e = a;
  CHECK(e == a);

  // Equal distributions with equal engines give equal sequences, also with const calls between.
  {
    D x(other), y(other);
    std::mt19937_64 g1(7), g2(7);
    for (int i = 0; i < 500; ++i) {
      T u = x(g1);
      (void)y.min(); (void)y.max(); (void)y.param(); (void)(y == x);
      T v = y(g2);
      CHECK(u == v || (u != u && v != v));
      CHECK(x.min() <= u && u <= x.max());
    }
  }
  // Values of d(g) and d(g, p) lie within [min(), max()] of the parameters used.
  {
    D x;
    std::mt19937 g(3);
    const P p = other.param();
    const D withp(p);
    for (int i = 0; i < 2000; ++i) {
      T u = x(g);
      CHECK(x.min() <= u && u <= x.max());
      T v = x(g, p);
      CHECK(withp.min() <= v && v <= withp.max());
    }
    CHECK(x.param() == D().param());  // d(g, p) does not change d's parameters
  }
  // reset(): later values do not depend on engine values produced before the reset.
  {
    D x(other), y(other);
    std::mt19937 noise(99);
    for (int i = 0; i < 3; ++i) (void)y(noise);
    y.reset();
    x.reset();
    std::mt19937 g1(5), g2(5);
    for (int i = 0; i < 200; ++i) {
      T u = x(g1), v = y(g2);
      CHECK(u == v);
    }
  }
  // x == y implies equal future sequences.
  {
    D x(other), y(other);
    std::mt19937 g(11);
    (void)y(g);
    if (x == y) {
      std::mt19937 g1(1), g2(1);
      for (int i = 0; i < 50; ++i) CHECK(x(g1) == y(g2));
    }
  }
}

int main() {
  check(std::uniform_int_distribution<>(-5, 17));
  check(std::uniform_int_distribution<unsigned long long>(3, ~0ull));
  check(std::uniform_int_distribution<short>(-300, 300));
  check(std::uniform_real_distribution<>(-2.5, 3.0));
  check(std::uniform_real_distribution<float>(1.0f, 2.0f));
  check(std::bernoulli_distribution(0.3));
  check(std::binomial_distribution<>(20, 0.3));
  check(std::binomial_distribution<long>(1000, 0.6));
  check(std::geometric_distribution<>(0.2));
  check(std::negative_binomial_distribution<>(3, 0.4));
  check(std::poisson_distribution<>(4.5));
  check(std::poisson_distribution<unsigned>(60.0));
  check(std::exponential_distribution<>(2.0));
  check(std::gamma_distribution<>(2.5, 1.5));
  check(std::gamma_distribution<float>(0.5f, 2.0f));
  check(std::weibull_distribution<>(1.5, 2.0));
  check(std::extreme_value_distribution<>(1.0, 2.0));
  check(std::normal_distribution<>(3.0, 2.0));
  check(std::normal_distribution<long double>(-1.0L, 0.5L));
  check(std::lognormal_distribution<>(0.5, 0.25));
  check(std::chi_squared_distribution<>(3.0));
  check(std::cauchy_distribution<>(1.0, 0.5));
  check(std::fisher_f_distribution<>(5.0, 7.0));
  check(std::student_t_distribution<>(4.0));
  check(std::discrete_distribution<>{1.0, 2.0, 3.0, 4.0});
  const double b[] = {0.0, 1.0, 3.0}, wc[] = {1.0, 2.0}, wl[] = {1.0, 2.0, 0.5};
  check(std::piecewise_constant_distribution<>(b, b + 3, wc));
  check(std::piecewise_linear_distribution<>(b, b + 3, wl));
}
