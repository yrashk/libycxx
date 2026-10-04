// libycxx core: the parametric random number distributions of <random> ([rand.dist.uni],
// [rand.dist.bern], [rand.dist.pois], [rand.dist.norm]).
//
// The algorithms are implementation-defined ([rand.dist.general]/3):
// - uniform_int_distribution: rand_uniform_upto (Lemire's method, or rejection), never biased;
// - uniform_real_distribution: a + (b - a) u with u from generate_canonical, kept below b;
// - binomial: inversion for t min(p, 1-p) < 10, else Hormann's BTRS transformed rejection;
// - poisson: multiplication of uniforms for a mean < 10, else Hormann's PTRS;
// - geometric, exponential, weibull, extreme_value, cauchy: inversion;
// - normal: Marsaglia's polar method (the second value is kept, and is part of the state);
// - gamma: Marsaglia and Tsang's squeeze method (alpha < 1 through alpha + 1);
// - negative_binomial: a Poisson variate with a gamma-distributed mean;
// - chi_squared, fisher_f, student_t, lognormal: from gamma and normal variates.
// min() and max() of the unbounded real distributions are -inf and +inf: extreme parameters can
// overflow a variate (an exponential with a tiny lambda), so no finite bound holds in general.
// The distributions' inserters write every parameter and every piece of internal state, floating
// point values with max_digits10 significant digits, so a distribution read back continues
// exactly where the written one would ([rand.req.dist]/6).
#pragma once

#include <ycxx/core/cmath.hpp>
#include <ycxx/core/numbers.hpp>
#include <ycxx/core/random_base.hpp>

namespace ycxx::detail {

// A standard normal variate by the polar method, discarding the second value.
template <class Real, class G>
Real rand_std_normal(G& g) {
  for (;;) {
    const Real u = 2 * ::ycxx::detail::rand_canonical<Real>(g) - 1;
    const Real v = 2 * ::ycxx::detail::rand_canonical<Real>(g) - 1;
    const Real s = u * u + v * v;
    if (s < 1 && s != 0)
      return u * std::sqrt(-2 * std::log(s) / s);
  }
}

// A gamma(alpha, 1) variate: Marsaglia and Tsang, "A simple method for generating gamma
// variables" (2000).
template <class Real, class G>
Real rand_gamma(G& g, Real alpha) {
  if (alpha == Real(1))
    return -std::log1p(-::ycxx::detail::rand_canonical<Real>(g));
  if (alpha < Real(1)) {
    // gamma(alpha) = gamma(alpha + 1) * U^(1/alpha).
    const Real x = ::ycxx::detail::rand_gamma<Real>(g, alpha + 1);
    return x * std::exp(std::log(::ycxx::detail::rand_open01<Real>(g)) / alpha);
  }
  const Real d = alpha - Real(1) / 3, c = 1 / std::sqrt(9 * d);
  for (;;) {
    Real x, v;
    do {
      x = ::ycxx::detail::rand_std_normal<Real>(g);
      v = 1 + c * x;
    } while (v <= 0);
    v = v * v * v;
    const Real u = ::ycxx::detail::rand_open01<Real>(g);
    const Real x2 = x * x;
    if (u < 1 - Real(0.0331) * x2 * x2)
      return d * v;
    if (std::log(u) < x2 / 2 + d * (1 - v + std::log(v)))
      return d * v;
  }
}

// Converts a non-negative real variate to IntType, clamping at its maximum.
template <class IntType>
IntType rand_clamp_int(double x) {
  constexpr double top = static_cast<double>(std::numeric_limits<IntType>::max());
  if (!(x < top)) // also NaN
    return std::numeric_limits<IntType>::max();
  return static_cast<IntType>(x);
}

// Poisson variates with mean mu > 0 ([rand.dist.pois.poisson]): the setup of PTRS (Hormann, "The
// transformed rejection method for generating Poisson random variables", 1993) for mu >= 10, of
// the multiplication method below.
struct rand_poisson_setup {
  double mu = 1, l = 0, smu = 0, b = 0, a = 0, inv_alpha = 0, vr = 0, log_mu = 0;

  rand_poisson_setup() = default;
  explicit rand_poisson_setup(double mean) : mu(mean) {
    if (mu < 10) {
      l = std::exp(-mu);
    } else {
      smu = std::sqrt(mu);
      b = 0.931 + 2.53 * smu;
      a = -0.059 + 0.02483 * b;
      inv_alpha = 1.1239 + 1.1328 / (b - 3.4);
      vr = 0.9277 - 3.6224 / (b - 2);
      log_mu = std::log(mu);
    }
  }
  template <class G>
  double sample(G& g) const {
    if (mu < 10) {
      double p = 1;
      for (double k = 0;; ++k) {
        p *= ::ycxx::detail::rand_canonical<double>(g);
        if (p <= l)
          return k;
      }
    }
    for (;;) {
      const double u = ::ycxx::detail::rand_canonical<double>(g) - 0.5;
      const double v = ::ycxx::detail::rand_canonical<double>(g);
      const double us = 0.5 - std::fabs(u);
      const double k = std::floor((2 * a / us + b) * u + mu + 0.43);
      if (us >= 0.07 && v <= vr)
        return k;
      if (k < 0 || (us < 0.013 && v > us))
        continue;
      if (v == 0)
        continue;
      if (std::log(v) + std::log(inv_alpha) - std::log(a / (us * us) + b) <= -mu + k * log_mu - std::lgamma(k + 1))
        return k;
    }
  }
};

// Binomial variates ([rand.dist.bern.bin]): inversion when n min(p, 1-p) < 10, else BTRS (Hormann,
// "The generation of binomial random variates", 1993), on min(p, 1-p).
struct rand_binomial_setup {
  double n = 1, p = 0.5, pp = 0.5; // pp = min(p, 1 - p)
  bool flip = false, inversion = true;
  double q = 0.5, s = 0, a = 0, r0 = 0;                         // inversion
  double b = 0, c = 0, vr = 0, alpha = 0, lpq = 0, m = 0, h = 0; // BTRS

  rand_binomial_setup() = default;
  rand_binomial_setup(double t, double prob) : n(t), p(prob) {
    flip = p > 0.5;
    pp = flip ? 1 - p : p;
    q = 1 - pp;
    inversion = n * pp < 10;
    if (pp == 0)
      return;
    if (inversion) {
      s = pp / q;
      a = (n + 1) * s;
      r0 = std::pow(q, n);
    } else {
      const double spq = std::sqrt(n * pp * q);
      b = 1.15 + 2.53 * spq;
      a = -0.0873 + 0.0248 * b + 0.01 * pp;
      c = n * pp + 0.5;
      vr = 0.92 - 4.2 / b;
      alpha = (2.83 + 5.1 / b) * spq;
      lpq = std::log(pp / q);
      m = std::floor((n + 1) * pp);
      h = std::lgamma(m + 1) + std::lgamma(n - m + 1);
    }
  }
  template <class G>
  double sample(G& g) const {
    if (pp == 0 || n == 0)
      return flip ? n : 0;
    const double k = inversion ? sample_inversion(g) : sample_btrs(g);
    return flip ? n - k : k;
  }

private:
  template <class G>
  double sample_inversion(G& g) const {
    for (;;) {
      double u = ::ycxx::detail::rand_canonical<double>(g), r = r0, x = 0;
      while (u > r && x <= n) {
        u -= r;
        ++x;
        r *= a / x - s;
      }
      if (x <= n)
        return x;
    }
  }
  template <class G>
  double sample_btrs(G& g) const {
    for (;;) {
      const double u = ::ycxx::detail::rand_canonical<double>(g) - 0.5;
      double v = ::ycxx::detail::rand_canonical<double>(g);
      const double us = 0.5 - std::fabs(u);
      const double k = std::floor((2 * a / us + b) * u + c);
      if (k < 0 || k > n)
        continue;
      if (us >= 0.07 && v <= vr)
        return k;
      v *= alpha / (a / (us * us) + b);
      if (v == 0)
        continue;
      if (std::log(v) <= h - std::lgamma(k + 1) - std::lgamma(n - k + 1) + (k - m) * lpq)
        return k;
    }
  }
};

} // namespace ycxx::detail

namespace std {

// ---- [rand.dist.uni.int] --------------------------------------------------------------------------
template <class IntType = int>
class uniform_int_distribution {
  static_assert(ycxx::detail::rand_int_type<IntType>,
                "uniform_int_distribution: IntType must be a standard integer type ([rand.req.genl]/1.6)");

public:
  using result_type = IntType;
  class param_type {
  public:
    using distribution_type = uniform_int_distribution;
    param_type() : param_type(0) {}
    explicit param_type(IntType a, IntType b = numeric_limits<IntType>::max()) : a_(a), b_(b) {
      ycxx::detail::precondition(a <= b, "uniform_int_distribution: requires a <= b");
    }
    result_type a() const { return a_; }
    result_type b() const { return b_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    IntType a_, b_;
  };

  uniform_int_distribution() : uniform_int_distribution(0) {}
  explicit uniform_int_distribution(IntType a, IntType b = numeric_limits<IntType>::max()) : p_(a, b) {}
  explicit uniform_int_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const uniform_int_distribution& x, const uniform_int_distribution& y) { return x.p_ == y.p_; }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    using U = make_unsigned_t<IntType>;
    const U range = static_cast<U>(static_cast<U>(parm.b()) - static_cast<U>(parm.a()));
    const U off = static_cast<U>(ycxx::detail::rand_uniform_upto(g, range));
    return static_cast<IntType>(static_cast<U>(static_cast<U>(parm.a()) + off));
  }

  result_type a() const { return p_.a(); }
  result_type b() const { return p_.b(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return a(); }
  result_type max() const { return b(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const uniform_int_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.a(), x.b());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, uniform_int_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    IntType a{}, b{};
    if (ycxx::detail::rand_get(is, a, b)) {
      if (a <= b)
        x.p_ = param_type(a, b);
      else
        is.setstate(basic_istream<charT, traits>::failbit);
    }
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.uni.real] -------------------------------------------------------------------------
template <class RealType = double>
class uniform_real_distribution {
  static_assert(ycxx::detail::rand_real_type<RealType>,
                "uniform_real_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = RealType;
  class param_type {
  public:
    using distribution_type = uniform_real_distribution;
    param_type() : param_type(0.0) {}
    explicit param_type(RealType a, RealType b = 1.0) : a_(a), b_(b) {
      ycxx::detail::precondition(a <= b && b - a <= numeric_limits<RealType>::max(),
                                 "uniform_real_distribution: requires a <= b and b - a <= max()");
    }
    result_type a() const { return a_; }
    result_type b() const { return b_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    RealType a_, b_;
  };

  uniform_real_distribution() : uniform_real_distribution(0.0) {}
  explicit uniform_real_distribution(RealType a, RealType b = 1.0) : p_(a, b) {}
  explicit uniform_real_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const uniform_real_distribution& x, const uniform_real_distribution& y) { return x.p_ == y.p_; }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    const RealType a = parm.a(), b = parm.b();
    const RealType x = a + (b - a) * ycxx::detail::rand_canonical<RealType>(g);
    // Rounding can reach b: [rand.dist.uni.real]/1 excludes it.
    return x < b ? x : std::nextafter(b, a);
  }

  result_type a() const { return p_.a(); }
  result_type b() const { return p_.b(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return a(); }
  result_type max() const { return b(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os,
                                                  const uniform_real_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.a(), x.b());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, uniform_real_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    RealType a{}, b{};
    if (ycxx::detail::rand_get(is, a, b)) {
      if (a <= b)
        x.p_ = param_type(a, b);
      else
        is.setstate(basic_istream<charT, traits>::failbit);
    }
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.bern.bernoulli] -------------------------------------------------------------------
class bernoulli_distribution {
public:
  using result_type = bool;
  class param_type {
  public:
    using distribution_type = bernoulli_distribution;
    param_type() : param_type(0.5) {}
    explicit param_type(double p) : p_(p) {
      ycxx::detail::precondition(0 <= p && p <= 1, "bernoulli_distribution: requires 0 <= p <= 1");
    }
    double p() const { return p_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    double p_;
  };

  bernoulli_distribution() : bernoulli_distribution(0.5) {}
  explicit bernoulli_distribution(double p) : p_(p) {}
  explicit bernoulli_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const bernoulli_distribution& x, const bernoulli_distribution& y) { return x.p_ == y.p_; }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    return ycxx::detail::rand_canonical<double>(g) < parm.p();
  }

  double p() const { return p_.p(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return false; }
  result_type max() const { return true; }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const bernoulli_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.p());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, bernoulli_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    double p = 0;
    if (ycxx::detail::rand_get(is, p)) {
      if (0 <= p && p <= 1)
        x.p_ = param_type(p);
      else
        is.setstate(basic_istream<charT, traits>::failbit);
    }
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.bern.bin] -------------------------------------------------------------------------
template <class IntType = int>
class binomial_distribution {
  static_assert(ycxx::detail::rand_int_type<IntType>,
                "binomial_distribution: IntType must be a standard integer type ([rand.req.genl]/1.6)");

public:
  using result_type = IntType;
  class param_type {
  public:
    using distribution_type = binomial_distribution;
    param_type() : param_type(1) {}
    explicit param_type(IntType t, double p = 0.5) : t_(t), p_(p), s_(static_cast<double>(t), p) {
      ycxx::detail::precondition(0 <= p && p <= 1 && 0 <= t, "binomial_distribution: requires 0 <= p <= 1 and 0 <= t");
    }
    IntType t() const { return t_; }
    double p() const { return p_; }
    friend bool operator==(const param_type& x, const param_type& y) { return x.t_ == y.t_ && x.p_ == y.p_; }

  private:
    friend binomial_distribution;
    IntType t_;
    double p_;
    ycxx::detail::rand_binomial_setup s_;
  };

  binomial_distribution() : binomial_distribution(1) {}
  explicit binomial_distribution(IntType t, double p = 0.5) : p_(t, p) {}
  explicit binomial_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const binomial_distribution& x, const binomial_distribution& y) { return x.p_ == y.p_; }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    const double k = parm.s_.sample(g);
    return k >= static_cast<double>(parm.t_) ? parm.t_ : static_cast<IntType>(k);
  }

  IntType t() const { return p_.t(); }
  double p() const { return p_.p(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return 0; }
  result_type max() const { return t(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const binomial_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.t(), x.p());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, binomial_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    IntType t{};
    double p = 0;
    if (ycxx::detail::rand_get(is, t, p)) {
      if (0 <= t && 0 <= p && p <= 1)
        x.p_ = param_type(t, p);
      else
        is.setstate(basic_istream<charT, traits>::failbit);
    }
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.bern.geo] -------------------------------------------------------------------------
template <class IntType = int>
class geometric_distribution {
  static_assert(ycxx::detail::rand_int_type<IntType>,
                "geometric_distribution: IntType must be a standard integer type ([rand.req.genl]/1.6)");

public:
  using result_type = IntType;
  class param_type {
  public:
    using distribution_type = geometric_distribution;
    param_type() : param_type(0.5) {}
    explicit param_type(double p) : p_(p) {
      ycxx::detail::precondition(0 < p && p < 1, "geometric_distribution: requires 0 < p < 1");
    }
    double p() const { return p_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    double p_;
  };

  geometric_distribution() : geometric_distribution(0.5) {}
  explicit geometric_distribution(double p) : p_(p) {}
  explicit geometric_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const geometric_distribution& x, const geometric_distribution& y) { return x.p_ == y.p_; }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    // P(X >= k) = (1 - p)^k: X = floor(log(V) / log(1 - p)) with V uniform in (0, 1].
    const double v = std::log1p(-ycxx::detail::rand_canonical<double>(g));
    return ycxx::detail::rand_clamp_int<IntType>(std::floor(v / std::log1p(-parm.p())));
  }

  double p() const { return p_.p(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<IntType>::max(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const geometric_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.p());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, geometric_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    double p = 0;
    if (ycxx::detail::rand_get(is, p)) {
      if (0 < p && p < 1)
        x.p_ = param_type(p);
      else
        is.setstate(basic_istream<charT, traits>::failbit);
    }
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.bern.negbin] ----------------------------------------------------------------------
template <class IntType = int>
class negative_binomial_distribution {
  static_assert(ycxx::detail::rand_int_type<IntType>,
                "negative_binomial_distribution: IntType must be a standard integer type ([rand.req.genl]/1.6)");

public:
  using result_type = IntType;
  class param_type {
  public:
    using distribution_type = negative_binomial_distribution;
    param_type() : param_type(1) {}
    explicit param_type(IntType k, double p = 0.5) : k_(k), p_(p) {
      ycxx::detail::precondition(0 < p && p <= 1 && 0 < k,
                                 "negative_binomial_distribution: requires 0 < p <= 1 and 0 < k");
    }
    IntType k() const { return k_; }
    double p() const { return p_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    IntType k_;
    double p_;
  };

  negative_binomial_distribution() : negative_binomial_distribution(1) {}
  explicit negative_binomial_distribution(IntType k, double p = 0.5) : p_(k, p) {}
  explicit negative_binomial_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const negative_binomial_distribution& x, const negative_binomial_distribution& y) {
    return x.p_ == y.p_;
  }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    // A Poisson variate whose mean is gamma(k, (1 - p) / p) distributed.
    if (parm.p() == 1)
      return 0;
    const double mean = ycxx::detail::rand_gamma<double>(g, static_cast<double>(parm.k())) * (1 - parm.p()) / parm.p();
    if (!(mean > 0))
      return 0;
    return ycxx::detail::rand_clamp_int<IntType>(ycxx::detail::rand_poisson_setup(mean).sample(g));
  }

  IntType k() const { return p_.k(); }
  double p() const { return p_.p(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<IntType>::max(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os,
                                                  const negative_binomial_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.k(), x.p());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is,
                                                  negative_binomial_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    IntType k{};
    double p = 0;
    if (ycxx::detail::rand_get(is, k, p)) {
      if (0 < k && 0 < p && p <= 1)
        x.p_ = param_type(k, p);
      else
        is.setstate(basic_istream<charT, traits>::failbit);
    }
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.pois.poisson] ---------------------------------------------------------------------
template <class IntType = int>
class poisson_distribution {
  static_assert(ycxx::detail::rand_int_type<IntType>,
                "poisson_distribution: IntType must be a standard integer type ([rand.req.genl]/1.6)");

public:
  using result_type = IntType;
  class param_type {
  public:
    using distribution_type = poisson_distribution;
    param_type() : param_type(1.0) {}
    explicit param_type(double mean) : s_(mean) {
      ycxx::detail::precondition(0 < mean, "poisson_distribution: requires 0 < mean");
    }
    double mean() const { return s_.mu; }
    friend bool operator==(const param_type& x, const param_type& y) { return x.s_.mu == y.s_.mu; }

  private:
    friend poisson_distribution;
    ycxx::detail::rand_poisson_setup s_;
  };

  poisson_distribution() : poisson_distribution(1.0) {}
  explicit poisson_distribution(double mean) : p_(mean) {}
  explicit poisson_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const poisson_distribution& x, const poisson_distribution& y) { return x.p_ == y.p_; }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    return ycxx::detail::rand_clamp_int<IntType>(parm.s_.sample(g));
  }

  double mean() const { return p_.mean(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<IntType>::max(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const poisson_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.mean());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, poisson_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    double mean = 0;
    if (ycxx::detail::rand_get(is, mean)) {
      if (0 < mean)
        x.p_ = param_type(mean);
      else
        is.setstate(basic_istream<charT, traits>::failbit);
    }
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.pois.exp] -------------------------------------------------------------------------
template <class RealType = double>
class exponential_distribution {
  static_assert(ycxx::detail::rand_real_type<RealType>,
                "exponential_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = RealType;
  class param_type {
  public:
    using distribution_type = exponential_distribution;
    param_type() : param_type(1.0) {}
    explicit param_type(RealType lambda) : lambda_(lambda) {
      ycxx::detail::precondition(0 < lambda, "exponential_distribution: requires 0 < lambda");
    }
    RealType lambda() const { return lambda_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    RealType lambda_;
  };

  exponential_distribution() : exponential_distribution(1.0) {}
  explicit exponential_distribution(RealType lambda) : p_(lambda) {}
  explicit exponential_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const exponential_distribution& x, const exponential_distribution& y) { return x.p_ == y.p_; }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    return -std::log1p(-ycxx::detail::rand_canonical<RealType>(g)) / parm.lambda();
  }

  RealType lambda() const { return p_.lambda(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<RealType>::infinity(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const exponential_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.lambda());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, exponential_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    RealType l{};
    if (ycxx::detail::rand_get(is, l)) {
      if (0 < l)
        x.p_ = param_type(l);
      else
        is.setstate(basic_istream<charT, traits>::failbit);
    }
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.pois.gamma] -----------------------------------------------------------------------
template <class RealType = double>
class gamma_distribution {
  static_assert(ycxx::detail::rand_real_type<RealType>,
                "gamma_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = RealType;
  class param_type {
  public:
    using distribution_type = gamma_distribution;
    param_type() : param_type(1.0) {}
    explicit param_type(RealType alpha, RealType beta = 1.0) : alpha_(alpha), beta_(beta) {
      ycxx::detail::precondition(0 < alpha && 0 < beta, "gamma_distribution: requires 0 < alpha and 0 < beta");
    }
    RealType alpha() const { return alpha_; }
    RealType beta() const { return beta_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    RealType alpha_, beta_;
  };

  gamma_distribution() : gamma_distribution(1.0) {}
  explicit gamma_distribution(RealType alpha, RealType beta = 1.0) : p_(alpha, beta) {}
  explicit gamma_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const gamma_distribution& x, const gamma_distribution& y) { return x.p_ == y.p_; }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    return ycxx::detail::rand_gamma<RealType>(g, parm.alpha()) * parm.beta();
  }

  RealType alpha() const { return p_.alpha(); }
  RealType beta() const { return p_.beta(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<RealType>::infinity(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const gamma_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.alpha(), x.beta());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, gamma_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    RealType a{}, b{};
    if (ycxx::detail::rand_get(is, a, b)) {
      if (0 < a && 0 < b)
        x.p_ = param_type(a, b);
      else
        is.setstate(basic_istream<charT, traits>::failbit);
    }
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.pois.weibull] ---------------------------------------------------------------------
template <class RealType = double>
class weibull_distribution {
  static_assert(ycxx::detail::rand_real_type<RealType>,
                "weibull_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = RealType;
  class param_type {
  public:
    using distribution_type = weibull_distribution;
    param_type() : param_type(1.0) {}
    explicit param_type(RealType a, RealType b = 1.0) : a_(a), b_(b) {
      ycxx::detail::precondition(0 < a && 0 < b, "weibull_distribution: requires 0 < a and 0 < b");
    }
    RealType a() const { return a_; }
    RealType b() const { return b_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    RealType a_, b_;
  };

  weibull_distribution() : weibull_distribution(1.0) {}
  explicit weibull_distribution(RealType a, RealType b = 1.0) : p_(a, b) {}
  explicit weibull_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const weibull_distribution& x, const weibull_distribution& y) { return x.p_ == y.p_; }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    const RealType e = -std::log1p(-ycxx::detail::rand_canonical<RealType>(g));
    return parm.b() * std::pow(e, 1 / parm.a());
  }

  RealType a() const { return p_.a(); }
  RealType b() const { return p_.b(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<RealType>::infinity(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const weibull_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.a(), x.b());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, weibull_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    RealType a{}, b{};
    if (ycxx::detail::rand_get(is, a, b)) {
      if (0 < a && 0 < b)
        x.p_ = param_type(a, b);
      else
        is.setstate(basic_istream<charT, traits>::failbit);
    }
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.pois.extreme] ---------------------------------------------------------------------
template <class RealType = double>
class extreme_value_distribution {
  static_assert(ycxx::detail::rand_real_type<RealType>,
                "extreme_value_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = RealType;
  class param_type {
  public:
    using distribution_type = extreme_value_distribution;
    param_type() : param_type(0.0) {}
    explicit param_type(RealType a, RealType b = 1.0) : a_(a), b_(b) {
      ycxx::detail::precondition(0 < b, "extreme_value_distribution: requires 0 < b");
    }
    RealType a() const { return a_; }
    RealType b() const { return b_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    RealType a_, b_;
  };

  extreme_value_distribution() : extreme_value_distribution(0.0) {}
  explicit extreme_value_distribution(RealType a, RealType b = 1.0) : p_(a, b) {}
  explicit extreme_value_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const extreme_value_distribution& x, const extreme_value_distribution& y) {
    return x.p_ == y.p_;
  }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    // F(x) = exp(-exp((a - x) / b)).
    const RealType u = ycxx::detail::rand_open01<RealType>(g);
    return parm.a() - parm.b() * std::log(-std::log(u));
  }

  RealType a() const { return p_.a(); }
  RealType b() const { return p_.b(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return -numeric_limits<RealType>::infinity(); }
  result_type max() const { return numeric_limits<RealType>::infinity(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os,
                                                  const extreme_value_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.a(), x.b());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, extreme_value_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    RealType a{}, b{};
    if (ycxx::detail::rand_get(is, a, b)) {
      if (0 < b)
        x.p_ = param_type(a, b);
      else
        is.setstate(basic_istream<charT, traits>::failbit);
    }
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.norm.normal] ----------------------------------------------------------------------
template <class RealType = double>
class normal_distribution {
  static_assert(ycxx::detail::rand_real_type<RealType>,
                "normal_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = RealType;
  class param_type {
  public:
    using distribution_type = normal_distribution;
    param_type() : param_type(0.0) {}
    explicit param_type(RealType mean, RealType stddev = 1.0) : mean_(mean), stddev_(stddev) {
      ycxx::detail::precondition(0 < stddev, "normal_distribution: requires 0 < stddev");
    }
    RealType mean() const { return mean_; }
    RealType stddev() const { return stddev_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    RealType mean_, stddev_;
  };

  normal_distribution() : normal_distribution(0.0) {}
  explicit normal_distribution(RealType mean, RealType stddev = 1.0) : p_(mean, stddev) {}
  explicit normal_distribution(const param_type& parm) : p_(parm) {}
  void reset() { saved_valid_ = false; }

  friend bool operator==(const normal_distribution& x, const normal_distribution& y) {
    return x.p_ == y.p_ && x.saved_valid_ == y.saved_valid_ && (!x.saved_valid_ || x.saved_ == y.saved_);
  }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    RealType z;
    if (saved_valid_) {
      saved_valid_ = false;
      z = saved_;
    } else {
      // Marsaglia's polar method: two standard normal variates per accepted pair.
      RealType u, v, s;
      do {
        u = 2 * ycxx::detail::rand_canonical<RealType>(g) - 1;
        v = 2 * ycxx::detail::rand_canonical<RealType>(g) - 1;
        s = u * u + v * v;
      } while (!(s < 1) || s == 0);
      const RealType f = std::sqrt(-2 * std::log(s) / s);
      saved_ = v * f;
      saved_valid_ = true;
      z = u * f;
    }
    return parm.mean() + parm.stddev() * z;
  }

  RealType mean() const { return p_.mean(); }
  RealType stddev() const { return p_.stddev(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return -numeric_limits<RealType>::infinity(); }
  result_type max() const { return numeric_limits<RealType>::infinity(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const normal_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.mean(), x.stddev(), x.saved_valid_);
    if (x.saved_valid_)
      ycxx::detail::rand_put_sep(os, x.saved_);
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, normal_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    RealType mean{}, stddev{}, saved{};
    bool valid = false;
    if (!ycxx::detail::rand_get(is, mean, stddev, valid) || (valid && !ycxx::detail::rand_get(is, saved)))
      return is;
    if (!(0 < stddev)) {
      is.setstate(basic_istream<charT, traits>::failbit);
      return is;
    }
    x.p_ = param_type(mean, stddev);
    x.saved_valid_ = valid;
    x.saved_ = valid ? saved : RealType(0);
    return is;
  }

private:
  param_type p_;
  RealType saved_ = 0;
  bool saved_valid_ = false;
};

// ---- [rand.dist.norm.lognormal] -------------------------------------------------------------------
template <class RealType = double>
class lognormal_distribution {
  static_assert(ycxx::detail::rand_real_type<RealType>,
                "lognormal_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = RealType;
  class param_type {
  public:
    using distribution_type = lognormal_distribution;
    param_type() : param_type(0.0) {}
    explicit param_type(RealType m, RealType s = 1.0) : m_(m), s_(s) {
      ycxx::detail::precondition(0 < s, "lognormal_distribution: requires 0 < s");
    }
    RealType m() const { return m_; }
    RealType s() const { return s_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    RealType m_, s_;
  };

  lognormal_distribution() : lognormal_distribution(0.0) {}
  explicit lognormal_distribution(RealType m, RealType s = 1.0) : p_(m, s) {}
  explicit lognormal_distribution(const param_type& parm) : p_(parm) {}
  void reset() { nd_.reset(); }

  friend bool operator==(const lognormal_distribution& x, const lognormal_distribution& y) {
    return x.p_ == y.p_ && x.nd_ == y.nd_;
  }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    return std::exp(parm.m() + parm.s() * nd_(g));
  }

  RealType m() const { return p_.m(); }
  RealType s() const { return p_.s(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<RealType>::infinity(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const lognormal_distribution& x) {
    {
      auto st = ycxx::detail::rand_out_dist(os);
      ycxx::detail::rand_put(os, x.m(), x.s());
      os.put(os.widen(' '));
    }
    return os << x.nd_;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, lognormal_distribution& x) {
    RealType m{}, s{};
    {
      auto st = ycxx::detail::rand_in(is);
      if (!ycxx::detail::rand_get(is, m, s))
        return is;
    }
    normal_distribution<RealType> nd;
    if (!(is >> nd))
      return is;
    if (!(0 < s)) {
      is.setstate(basic_istream<charT, traits>::failbit);
      return is;
    }
    x.p_ = param_type(m, s);
    x.nd_ = nd;
    return is;
  }

private:
  param_type p_;
  normal_distribution<RealType> nd_;
};

// ---- [rand.dist.norm.chisq] -----------------------------------------------------------------------
template <class RealType = double>
class chi_squared_distribution {
  static_assert(ycxx::detail::rand_real_type<RealType>,
                "chi_squared_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = RealType;
  class param_type {
  public:
    using distribution_type = chi_squared_distribution;
    param_type() : param_type(1.0) {}
    explicit param_type(RealType n) : n_(n) {
      ycxx::detail::precondition(0 < n, "chi_squared_distribution: requires 0 < n");
    }
    RealType n() const { return n_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    RealType n_;
  };

  chi_squared_distribution() : chi_squared_distribution(1.0) {}
  explicit chi_squared_distribution(RealType n) : p_(n) {}
  explicit chi_squared_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const chi_squared_distribution& x, const chi_squared_distribution& y) { return x.p_ == y.p_; }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    return 2 * ycxx::detail::rand_gamma<RealType>(g, parm.n() / 2);
  }

  RealType n() const { return p_.n(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<RealType>::infinity(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const chi_squared_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.n());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, chi_squared_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    RealType n{};
    if (ycxx::detail::rand_get(is, n)) {
      if (0 < n)
        x.p_ = param_type(n);
      else
        is.setstate(basic_istream<charT, traits>::failbit);
    }
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.norm.cauchy] ----------------------------------------------------------------------
template <class RealType = double>
class cauchy_distribution {
  static_assert(ycxx::detail::rand_real_type<RealType>,
                "cauchy_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = RealType;
  class param_type {
  public:
    using distribution_type = cauchy_distribution;
    param_type() : param_type(0.0) {}
    explicit param_type(RealType a, RealType b = 1.0) : a_(a), b_(b) {
      ycxx::detail::precondition(0 < b, "cauchy_distribution: requires 0 < b");
    }
    RealType a() const { return a_; }
    RealType b() const { return b_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    RealType a_, b_;
  };

  cauchy_distribution() : cauchy_distribution(0.0) {}
  explicit cauchy_distribution(RealType a, RealType b = 1.0) : p_(a, b) {}
  explicit cauchy_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const cauchy_distribution& x, const cauchy_distribution& y) { return x.p_ == y.p_; }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    const RealType u = ycxx::detail::rand_open01<RealType>(g);
    return parm.a() + parm.b() * std::tan(numbers::pi_v<RealType> * (u - RealType(0.5)));
  }

  RealType a() const { return p_.a(); }
  RealType b() const { return p_.b(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return -numeric_limits<RealType>::infinity(); }
  result_type max() const { return numeric_limits<RealType>::infinity(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const cauchy_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.a(), x.b());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, cauchy_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    RealType a{}, b{};
    if (ycxx::detail::rand_get(is, a, b)) {
      if (0 < b)
        x.p_ = param_type(a, b);
      else
        is.setstate(basic_istream<charT, traits>::failbit);
    }
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.norm.f] ---------------------------------------------------------------------------
template <class RealType = double>
class fisher_f_distribution {
  static_assert(ycxx::detail::rand_real_type<RealType>,
                "fisher_f_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = RealType;
  class param_type {
  public:
    using distribution_type = fisher_f_distribution;
    param_type() : param_type(1.0) {}
    explicit param_type(RealType m, RealType n = 1) : m_(m), n_(n) {
      ycxx::detail::precondition(0 < m && 0 < n, "fisher_f_distribution: requires 0 < m and 0 < n");
    }
    RealType m() const { return m_; }
    RealType n() const { return n_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    RealType m_, n_;
  };

  fisher_f_distribution() : fisher_f_distribution(1.0) {}
  explicit fisher_f_distribution(RealType m, RealType n = 1) : p_(m, n) {}
  explicit fisher_f_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const fisher_f_distribution& x, const fisher_f_distribution& y) { return x.p_ == y.p_; }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    // (chi2(m) / m) / (chi2(n) / n), with chi2(k) = 2 gamma(k / 2).
    const RealType x = ycxx::detail::rand_gamma<RealType>(g, parm.m() / 2);
    const RealType y = ycxx::detail::rand_gamma<RealType>(g, parm.n() / 2);
    return (x * parm.n()) / (y * parm.m());
  }

  RealType m() const { return p_.m(); }
  RealType n() const { return p_.n(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<RealType>::infinity(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const fisher_f_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.m(), x.n());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, fisher_f_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    RealType m{}, n{};
    if (ycxx::detail::rand_get(is, m, n)) {
      if (0 < m && 0 < n)
        x.p_ = param_type(m, n);
      else
        is.setstate(basic_istream<charT, traits>::failbit);
    }
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.norm.t] ---------------------------------------------------------------------------
template <class RealType = double>
class student_t_distribution {
  static_assert(ycxx::detail::rand_real_type<RealType>,
                "student_t_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = RealType;
  class param_type {
  public:
    using distribution_type = student_t_distribution;
    param_type() : param_type(1.0) {}
    explicit param_type(RealType n) : n_(n) {
      ycxx::detail::precondition(0 < n, "student_t_distribution: requires 0 < n");
    }
    RealType n() const { return n_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    RealType n_;
  };

  student_t_distribution() : student_t_distribution(1.0) {}
  explicit student_t_distribution(RealType n) : p_(n) {}
  explicit student_t_distribution(const param_type& parm) : p_(parm) {}
  void reset() { nd_.reset(); }

  friend bool operator==(const student_t_distribution& x, const student_t_distribution& y) {
    return x.p_ == y.p_ && x.nd_ == y.nd_;
  }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    // Z / sqrt(chi2(n) / n).
    const RealType z = nd_(g);
    const RealType c = 2 * ycxx::detail::rand_gamma<RealType>(g, parm.n() / 2);
    return z * std::sqrt(parm.n() / c);
  }

  RealType n() const { return p_.n(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return -numeric_limits<RealType>::infinity(); }
  result_type max() const { return numeric_limits<RealType>::infinity(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const student_t_distribution& x) {
    {
      auto st = ycxx::detail::rand_out_dist(os);
      ycxx::detail::rand_put(os, x.n());
      os.put(os.widen(' '));
    }
    return os << x.nd_;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, student_t_distribution& x) {
    RealType n{};
    {
      auto st = ycxx::detail::rand_in(is);
      if (!ycxx::detail::rand_get(is, n))
        return is;
    }
    normal_distribution<RealType> nd;
    if (!(is >> nd))
      return is;
    if (!(0 < n)) {
      is.setstate(basic_istream<charT, traits>::failbit);
      return is;
    }
    x.p_ = param_type(n);
    x.nd_ = nd;
    return is;
  }

private:
  param_type p_;
  normal_distribution<RealType> nd_;
};

} // namespace std
