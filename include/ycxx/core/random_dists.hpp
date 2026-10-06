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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// A standard normal variate by the polar method, discarding the second value.
template <class _Real, class _Gp>
_Real __rand_std_normal(_Gp& __g) {
  for (;;) {
    const _Real __u = 2 * ::__ycxx::__detail::__rand_canonical<_Real>(__g) - 1;
    const _Real __v = 2 * ::__ycxx::__detail::__rand_canonical<_Real>(__g) - 1;
    const _Real s = __u * __u + __v * __v;
    if (s < 1 && s != 0)
      return __u * std::sqrt(-2 * std::log(s) / s);
  }
}

// A gamma(alpha, 1) variate: Marsaglia and Tsang, "A simple method for generating gamma
// variables" (2000).
template <class _Real, class _Gp>
_Real __rand_gamma(_Gp& __g, _Real alpha) {
  if (alpha == _Real(1))
    return -std::log1p(-::__ycxx::__detail::__rand_canonical<_Real>(__g));
  if (alpha < _Real(1)) {
    // gamma(alpha) = gamma(alpha + 1) * U^(1/alpha).
    const _Real __x = ::__ycxx::__detail::__rand_gamma<_Real>(__g, alpha + 1);
    return __x * std::exp(std::log(::__ycxx::__detail::__rand_open01<_Real>(__g)) / alpha);
  }
  const _Real d = alpha - _Real(1) / 3, c = 1 / std::sqrt(9 * d);
  for (;;) {
    _Real __x, __v;
    do {
      __x = ::__ycxx::__detail::__rand_std_normal<_Real>(__g);
      __v = 1 + c * __x;
    } while (__v <= 0);
    __v = __v * __v * __v;
    const _Real __u = ::__ycxx::__detail::__rand_open01<_Real>(__g);
    const _Real __x2 = __x * __x;
    if (__u < 1 - _Real(0.0331) * __x2 * __x2)
      return d * __v;
    if (std::log(__u) < __x2 / 2 + d * (1 - __v + std::log(__v)))
      return d * __v;
  }
}

// Converts a non-negative real variate to IntType, clamping at its maximum.
template <class _IntType>
_IntType __rand_clamp_int(double __x) {
  constexpr double top = static_cast<double>(std::numeric_limits<_IntType>::max());
  if (!(__x < top)) // also NaN
    return std::numeric_limits<_IntType>::max();
  return static_cast<_IntType>(__x);
}

// Poisson variates with mean mu > 0 ([rand.dist.pois.poisson]): the setup of PTRS (Hormann, "The
// transformed rejection method for generating Poisson random variables", 1993) for mu >= 10, of
// the multiplication method below.
struct __rand_poisson_setup {
  double __mu = 1, __l = 0, __smu = 0, b = 0, a = 0, __inv_alpha = 0, __vr = 0, __log_mu = 0;

  __rand_poisson_setup() = default;
  explicit __rand_poisson_setup(double mean) : __mu(mean) {
    if (__mu < 10) {
      __l = std::exp(-__mu);
    } else {
      __smu = std::sqrt(__mu);
      b = 0.931 + 2.53 * __smu;
      a = -0.059 + 0.02483 * b;
      __inv_alpha = 1.1239 + 1.1328 / (b - 3.4);
      __vr = 0.9277 - 3.6224 / (b - 2);
      __log_mu = std::log(__mu);
    }
  }
  template <class _Gp>
  double sample(_Gp& __g) const {
    if (__mu < 10) {
      double p = 1;
      for (double k = 0;; ++k) {
        p *= ::__ycxx::__detail::__rand_canonical<double>(__g);
        if (p <= __l)
          return k;
      }
    }
    for (;;) {
      const double __u = ::__ycxx::__detail::__rand_canonical<double>(__g) - 0.5;
      const double __v = ::__ycxx::__detail::__rand_canonical<double>(__g);
      const double us = 0.5 - std::fabs(__u);
      const double k = std::floor((2 * a / us + b) * __u + __mu + 0.43);
      if (us >= 0.07 && __v <= __vr)
        return k;
      if (k < 0 || (us < 0.013 && __v > us))
        continue;
      if (__v == 0)
        continue;
      if (std::log(__v) + std::log(__inv_alpha) - std::log(a / (us * us) + b) <= -__mu + k * __log_mu - std::lgamma(k + 1))
        return k;
    }
  }
};

// Binomial variates ([rand.dist.bern.bin]): inversion when n min(p, 1-p) < 10, else BTRS (Hormann,
// "The generation of binomial random variates", 1993), on min(p, 1-p).
struct __rand_binomial_setup {
  double n = 1, p = 0.5, __pp = 0.5; // pp = min(p, 1 - p)
  bool flip = false, __inversion = true;
  double __q = 0.5, s = 0, a = 0, __r0 = 0;                         // inversion
  double b = 0, c = 0, __vr = 0, alpha = 0, __lpq = 0, m = 0, h = 0; // BTRS

  __rand_binomial_setup() = default;
  __rand_binomial_setup(double t, double __prob) : n(t), p(__prob) {
    flip = p > 0.5;
    __pp = flip ? 1 - p : p;
    __q = 1 - __pp;
    __inversion = n * __pp < 10;
    if (__pp == 0)
      return;
    if (__inversion) {
      s = __pp / __q;
      a = (n + 1) * s;
      __r0 = std::pow(__q, n);
    } else {
      const double __spq = std::sqrt(n * __pp * __q);
      b = 1.15 + 2.53 * __spq;
      a = -0.0873 + 0.0248 * b + 0.01 * __pp;
      c = n * __pp + 0.5;
      __vr = 0.92 - 4.2 / b;
      alpha = (2.83 + 5.1 / b) * __spq;
      __lpq = std::log(__pp / __q);
      m = std::floor((n + 1) * __pp);
      h = std::lgamma(m + 1) + std::lgamma(n - m + 1);
    }
  }
  template <class _Gp>
  double sample(_Gp& __g) const {
    if (__pp == 0 || n == 0)
      return flip ? n : 0;
    const double k = __inversion ? __sample_inversion(__g) : __sample_btrs(__g);
    return flip ? n - k : k;
  }

private:
  template <class _Gp>
  double __sample_inversion(_Gp& __g) const {
    for (;;) {
      double __u = ::__ycxx::__detail::__rand_canonical<double>(__g), r = __r0, __x = 0;
      while (__u > r && __x <= n) {
        __u -= r;
        ++__x;
        r *= a / __x - s;
      }
      if (__x <= n)
        return __x;
    }
  }
  template <class _Gp>
  double __sample_btrs(_Gp& __g) const {
    for (;;) {
      const double __u = ::__ycxx::__detail::__rand_canonical<double>(__g) - 0.5;
      double __v = ::__ycxx::__detail::__rand_canonical<double>(__g);
      const double us = 0.5 - std::fabs(__u);
      const double k = std::floor((2 * a / us + b) * __u + c);
      if (k < 0 || k > n)
        continue;
      if (us >= 0.07 && __v <= __vr)
        return k;
      __v *= alpha / (a / (us * us) + b);
      if (__v == 0)
        continue;
      if (std::log(__v) <= h - std::lgamma(k + 1) - std::lgamma(n - k + 1) + (k - m) * __lpq)
        return k;
    }
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// ---- [rand.dist.uni.int] --------------------------------------------------------------------------
template <class _IntType = int>
class uniform_int_distribution {
  static_assert(__ycxx::__detail::__rand_int_type<_IntType>,
                "uniform_int_distribution: IntType must be a standard integer type ([rand.req.genl]/1.6)");

public:
  using result_type = _IntType;
  class param_type {
  public:
    using distribution_type = uniform_int_distribution;
    param_type() : param_type(0) {}
    explicit param_type(_IntType a, _IntType b = numeric_limits<_IntType>::max()) : __a_(a), __b_(b) {
      __ycxx::__detail::__precondition(a <= b, "uniform_int_distribution: requires a <= b");
    }
    result_type a() const { return __a_; }
    result_type b() const { return __b_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    _IntType __a_, __b_;
  };

  uniform_int_distribution() : uniform_int_distribution(0) {}
  explicit uniform_int_distribution(_IntType a, _IntType b = numeric_limits<_IntType>::max()) : __p_(a, b) {}
  explicit uniform_int_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const uniform_int_distribution& __x, const uniform_int_distribution& y) { return __x.__p_ == y.__p_; }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    using _Up = make_unsigned_t<_IntType>;
    const _Up range = static_cast<_Up>(static_cast<_Up>(__parm.b()) - static_cast<_Up>(__parm.a()));
    const _Up __off = static_cast<_Up>(__ycxx::__detail::__rand_uniform_upto(__g, range));
    return static_cast<_IntType>(static_cast<_Up>(static_cast<_Up>(__parm.a()) + __off));
  }

  result_type a() const { return __p_.a(); }
  result_type b() const { return __p_.b(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return a(); }
  result_type max() const { return b(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const uniform_int_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.a(), __x.b());
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, uniform_int_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    _IntType a{}, b{};
    if (__ycxx::__detail::__rand_get(is, a, b)) {
      if (a <= b)
        __x.__p_ = param_type(a, b);
      else
        is.setstate(basic_istream<__charT, __traits>::failbit);
    }
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.uni.real] -------------------------------------------------------------------------
template <class _RealType = double>
class uniform_real_distribution {
  static_assert(__ycxx::__detail::__rand_real_type<_RealType>,
                "uniform_real_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = _RealType;
  class param_type {
  public:
    using distribution_type = uniform_real_distribution;
    param_type() : param_type(0.0) {}
    explicit param_type(_RealType a, _RealType b = 1.0) : __a_(a), __b_(b) {
      __ycxx::__detail::__precondition(a <= b && b - a <= numeric_limits<_RealType>::max(),
                                 "uniform_real_distribution: requires a <= b and b - a <= max()");
    }
    result_type a() const { return __a_; }
    result_type b() const { return __b_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    _RealType __a_, __b_;
  };

  uniform_real_distribution() : uniform_real_distribution(0.0) {}
  explicit uniform_real_distribution(_RealType a, _RealType b = 1.0) : __p_(a, b) {}
  explicit uniform_real_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const uniform_real_distribution& __x, const uniform_real_distribution& y) { return __x.__p_ == y.__p_; }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    const _RealType a = __parm.a(), b = __parm.b();
    const _RealType __x = a + (b - a) * __ycxx::__detail::__rand_canonical<_RealType>(__g);
    // Rounding can reach b: [rand.dist.uni.real]/1 excludes it.
    return __x < b ? __x : std::nextafter(b, a);
  }

  result_type a() const { return __p_.a(); }
  result_type b() const { return __p_.b(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return a(); }
  result_type max() const { return b(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os,
                                                  const uniform_real_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.a(), __x.b());
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, uniform_real_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    _RealType a{}, b{};
    if (__ycxx::__detail::__rand_get(is, a, b)) {
      if (a <= b)
        __x.__p_ = param_type(a, b);
      else
        is.setstate(basic_istream<__charT, __traits>::failbit);
    }
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.bern.bernoulli] -------------------------------------------------------------------
class bernoulli_distribution {
public:
  using result_type = bool;
  class param_type {
  public:
    using distribution_type = bernoulli_distribution;
    param_type() : param_type(0.5) {}
    explicit param_type(double p) : __p_(p) {
      __ycxx::__detail::__precondition(0 <= p && p <= 1, "bernoulli_distribution: requires 0 <= p <= 1");
    }
    double p() const { return __p_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    double __p_;
  };

  bernoulli_distribution() : bernoulli_distribution(0.5) {}
  explicit bernoulli_distribution(double p) : __p_(p) {}
  explicit bernoulli_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const bernoulli_distribution& __x, const bernoulli_distribution& y) { return __x.__p_ == y.__p_; }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    return __ycxx::__detail::__rand_canonical<double>(__g) < __parm.p();
  }

  double p() const { return __p_.p(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return false; }
  result_type max() const { return true; }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const bernoulli_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.p());
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, bernoulli_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    double p = 0;
    if (__ycxx::__detail::__rand_get(is, p)) {
      if (0 <= p && p <= 1)
        __x.__p_ = param_type(p);
      else
        is.setstate(basic_istream<__charT, __traits>::failbit);
    }
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.bern.bin] -------------------------------------------------------------------------
template <class _IntType = int>
class binomial_distribution {
  static_assert(__ycxx::__detail::__rand_int_type<_IntType>,
                "binomial_distribution: IntType must be a standard integer type ([rand.req.genl]/1.6)");

public:
  using result_type = _IntType;
  class param_type {
  public:
    using distribution_type = binomial_distribution;
    param_type() : param_type(1) {}
    explicit param_type(_IntType t, double p = 0.5) : __t_(t), __p_(p), __s_(static_cast<double>(t), p) {
      __ycxx::__detail::__precondition(0 <= p && p <= 1 && 0 <= t, "binomial_distribution: requires 0 <= p <= 1 and 0 <= t");
    }
    _IntType t() const { return __t_; }
    double p() const { return __p_; }
    friend bool operator==(const param_type& __x, const param_type& y) { return __x.__t_ == y.__t_ && __x.__p_ == y.__p_; }

  private:
    friend binomial_distribution;
    _IntType __t_;
    double __p_;
    __ycxx::__detail::__rand_binomial_setup __s_;
  };

  binomial_distribution() : binomial_distribution(1) {}
  explicit binomial_distribution(_IntType t, double p = 0.5) : __p_(t, p) {}
  explicit binomial_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const binomial_distribution& __x, const binomial_distribution& y) { return __x.__p_ == y.__p_; }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    const double k = __parm.__s_.sample(__g);
    return k >= static_cast<double>(__parm.__t_) ? __parm.__t_ : static_cast<_IntType>(k);
  }

  _IntType t() const { return __p_.t(); }
  double p() const { return __p_.p(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return 0; }
  result_type max() const { return t(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const binomial_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.t(), __x.p());
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, binomial_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    _IntType t{};
    double p = 0;
    if (__ycxx::__detail::__rand_get(is, t, p)) {
      if (0 <= t && 0 <= p && p <= 1)
        __x.__p_ = param_type(t, p);
      else
        is.setstate(basic_istream<__charT, __traits>::failbit);
    }
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.bern.geo] -------------------------------------------------------------------------
template <class _IntType = int>
class geometric_distribution {
  static_assert(__ycxx::__detail::__rand_int_type<_IntType>,
                "geometric_distribution: IntType must be a standard integer type ([rand.req.genl]/1.6)");

public:
  using result_type = _IntType;
  class param_type {
  public:
    using distribution_type = geometric_distribution;
    param_type() : param_type(0.5) {}
    explicit param_type(double p) : __p_(p) {
      __ycxx::__detail::__precondition(0 < p && p < 1, "geometric_distribution: requires 0 < p < 1");
    }
    double p() const { return __p_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    double __p_;
  };

  geometric_distribution() : geometric_distribution(0.5) {}
  explicit geometric_distribution(double p) : __p_(p) {}
  explicit geometric_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const geometric_distribution& __x, const geometric_distribution& y) { return __x.__p_ == y.__p_; }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    // P(X >= k) = (1 - p)^k: X = floor(log(V) / log(1 - p)) with V uniform in (0, 1].
    const double __v = std::log1p(-__ycxx::__detail::__rand_canonical<double>(__g));
    return __ycxx::__detail::__rand_clamp_int<_IntType>(std::floor(__v / std::log1p(-__parm.p())));
  }

  double p() const { return __p_.p(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<_IntType>::max(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const geometric_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.p());
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, geometric_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    double p = 0;
    if (__ycxx::__detail::__rand_get(is, p)) {
      if (0 < p && p < 1)
        __x.__p_ = param_type(p);
      else
        is.setstate(basic_istream<__charT, __traits>::failbit);
    }
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.bern.negbin] ----------------------------------------------------------------------
template <class _IntType = int>
class negative_binomial_distribution {
  static_assert(__ycxx::__detail::__rand_int_type<_IntType>,
                "negative_binomial_distribution: IntType must be a standard integer type ([rand.req.genl]/1.6)");

public:
  using result_type = _IntType;
  class param_type {
  public:
    using distribution_type = negative_binomial_distribution;
    param_type() : param_type(1) {}
    explicit param_type(_IntType k, double p = 0.5) : __k_(k), __p_(p) {
      __ycxx::__detail::__precondition(0 < p && p <= 1 && 0 < k,
                                 "negative_binomial_distribution: requires 0 < p <= 1 and 0 < k");
    }
    _IntType k() const { return __k_; }
    double p() const { return __p_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    _IntType __k_;
    double __p_;
  };

  negative_binomial_distribution() : negative_binomial_distribution(1) {}
  explicit negative_binomial_distribution(_IntType k, double p = 0.5) : __p_(k, p) {}
  explicit negative_binomial_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const negative_binomial_distribution& __x, const negative_binomial_distribution& y) {
    return __x.__p_ == y.__p_;
  }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    // A Poisson variate whose mean is gamma(k, (1 - p) / p) distributed.
    if (__parm.p() == 1)
      return 0;
    const double mean = __ycxx::__detail::__rand_gamma<double>(__g, static_cast<double>(__parm.k())) * (1 - __parm.p()) / __parm.p();
    if (!(mean > 0))
      return 0;
    return __ycxx::__detail::__rand_clamp_int<_IntType>(__ycxx::__detail::__rand_poisson_setup(mean).sample(__g));
  }

  _IntType k() const { return __p_.k(); }
  double p() const { return __p_.p(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<_IntType>::max(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os,
                                                  const negative_binomial_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.k(), __x.p());
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is,
                                                  negative_binomial_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    _IntType k{};
    double p = 0;
    if (__ycxx::__detail::__rand_get(is, k, p)) {
      if (0 < k && 0 < p && p <= 1)
        __x.__p_ = param_type(k, p);
      else
        is.setstate(basic_istream<__charT, __traits>::failbit);
    }
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.pois.poisson] ---------------------------------------------------------------------
template <class _IntType = int>
class poisson_distribution {
  static_assert(__ycxx::__detail::__rand_int_type<_IntType>,
                "poisson_distribution: IntType must be a standard integer type ([rand.req.genl]/1.6)");

public:
  using result_type = _IntType;
  class param_type {
  public:
    using distribution_type = poisson_distribution;
    param_type() : param_type(1.0) {}
    explicit param_type(double mean) : __s_(mean) {
      __ycxx::__detail::__precondition(0 < mean, "poisson_distribution: requires 0 < mean");
    }
    double mean() const { return __s_.__mu; }
    friend bool operator==(const param_type& __x, const param_type& y) { return __x.__s_.__mu == y.__s_.__mu; }

  private:
    friend poisson_distribution;
    __ycxx::__detail::__rand_poisson_setup __s_;
  };

  poisson_distribution() : poisson_distribution(1.0) {}
  explicit poisson_distribution(double mean) : __p_(mean) {}
  explicit poisson_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const poisson_distribution& __x, const poisson_distribution& y) { return __x.__p_ == y.__p_; }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    return __ycxx::__detail::__rand_clamp_int<_IntType>(__parm.__s_.sample(__g));
  }

  double mean() const { return __p_.mean(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<_IntType>::max(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const poisson_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.mean());
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, poisson_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    double mean = 0;
    if (__ycxx::__detail::__rand_get(is, mean)) {
      if (0 < mean)
        __x.__p_ = param_type(mean);
      else
        is.setstate(basic_istream<__charT, __traits>::failbit);
    }
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.pois.exp] -------------------------------------------------------------------------
template <class _RealType = double>
class exponential_distribution {
  static_assert(__ycxx::__detail::__rand_real_type<_RealType>,
                "exponential_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = _RealType;
  class param_type {
  public:
    using distribution_type = exponential_distribution;
    param_type() : param_type(1.0) {}
    explicit param_type(_RealType lambda) : __lambda_(lambda) {
      __ycxx::__detail::__precondition(0 < lambda, "exponential_distribution: requires 0 < lambda");
    }
    _RealType lambda() const { return __lambda_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    _RealType __lambda_;
  };

  exponential_distribution() : exponential_distribution(1.0) {}
  explicit exponential_distribution(_RealType lambda) : __p_(lambda) {}
  explicit exponential_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const exponential_distribution& __x, const exponential_distribution& y) { return __x.__p_ == y.__p_; }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    return -std::log1p(-__ycxx::__detail::__rand_canonical<_RealType>(__g)) / __parm.lambda();
  }

  _RealType lambda() const { return __p_.lambda(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<_RealType>::infinity(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const exponential_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.lambda());
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, exponential_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    _RealType __l{};
    if (__ycxx::__detail::__rand_get(is, __l)) {
      if (0 < __l)
        __x.__p_ = param_type(__l);
      else
        is.setstate(basic_istream<__charT, __traits>::failbit);
    }
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.pois.gamma] -----------------------------------------------------------------------
template <class _RealType = double>
class gamma_distribution {
  static_assert(__ycxx::__detail::__rand_real_type<_RealType>,
                "gamma_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = _RealType;
  class param_type {
  public:
    using distribution_type = gamma_distribution;
    param_type() : param_type(1.0) {}
    explicit param_type(_RealType alpha, _RealType beta = 1.0) : __alpha_(alpha), __beta_(beta) {
      __ycxx::__detail::__precondition(0 < alpha && 0 < beta, "gamma_distribution: requires 0 < alpha and 0 < beta");
    }
    _RealType alpha() const { return __alpha_; }
    _RealType beta() const { return __beta_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    _RealType __alpha_, __beta_;
  };

  gamma_distribution() : gamma_distribution(1.0) {}
  explicit gamma_distribution(_RealType alpha, _RealType beta = 1.0) : __p_(alpha, beta) {}
  explicit gamma_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const gamma_distribution& __x, const gamma_distribution& y) { return __x.__p_ == y.__p_; }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    return __ycxx::__detail::__rand_gamma<_RealType>(__g, __parm.alpha()) * __parm.beta();
  }

  _RealType alpha() const { return __p_.alpha(); }
  _RealType beta() const { return __p_.beta(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<_RealType>::infinity(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const gamma_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.alpha(), __x.beta());
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, gamma_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    _RealType a{}, b{};
    if (__ycxx::__detail::__rand_get(is, a, b)) {
      if (0 < a && 0 < b)
        __x.__p_ = param_type(a, b);
      else
        is.setstate(basic_istream<__charT, __traits>::failbit);
    }
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.pois.weibull] ---------------------------------------------------------------------
template <class _RealType = double>
class weibull_distribution {
  static_assert(__ycxx::__detail::__rand_real_type<_RealType>,
                "weibull_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = _RealType;
  class param_type {
  public:
    using distribution_type = weibull_distribution;
    param_type() : param_type(1.0) {}
    explicit param_type(_RealType a, _RealType b = 1.0) : __a_(a), __b_(b) {
      __ycxx::__detail::__precondition(0 < a && 0 < b, "weibull_distribution: requires 0 < a and 0 < b");
    }
    _RealType a() const { return __a_; }
    _RealType b() const { return __b_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    _RealType __a_, __b_;
  };

  weibull_distribution() : weibull_distribution(1.0) {}
  explicit weibull_distribution(_RealType a, _RealType b = 1.0) : __p_(a, b) {}
  explicit weibull_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const weibull_distribution& __x, const weibull_distribution& y) { return __x.__p_ == y.__p_; }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    const _RealType e = -std::log1p(-__ycxx::__detail::__rand_canonical<_RealType>(__g));
    return __parm.b() * std::pow(e, 1 / __parm.a());
  }

  _RealType a() const { return __p_.a(); }
  _RealType b() const { return __p_.b(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<_RealType>::infinity(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const weibull_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.a(), __x.b());
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, weibull_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    _RealType a{}, b{};
    if (__ycxx::__detail::__rand_get(is, a, b)) {
      if (0 < a && 0 < b)
        __x.__p_ = param_type(a, b);
      else
        is.setstate(basic_istream<__charT, __traits>::failbit);
    }
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.pois.extreme] ---------------------------------------------------------------------
template <class _RealType = double>
class extreme_value_distribution {
  static_assert(__ycxx::__detail::__rand_real_type<_RealType>,
                "extreme_value_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = _RealType;
  class param_type {
  public:
    using distribution_type = extreme_value_distribution;
    param_type() : param_type(0.0) {}
    explicit param_type(_RealType a, _RealType b = 1.0) : __a_(a), __b_(b) {
      __ycxx::__detail::__precondition(0 < b, "extreme_value_distribution: requires 0 < b");
    }
    _RealType a() const { return __a_; }
    _RealType b() const { return __b_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    _RealType __a_, __b_;
  };

  extreme_value_distribution() : extreme_value_distribution(0.0) {}
  explicit extreme_value_distribution(_RealType a, _RealType b = 1.0) : __p_(a, b) {}
  explicit extreme_value_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const extreme_value_distribution& __x, const extreme_value_distribution& y) {
    return __x.__p_ == y.__p_;
  }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    // F(x) = exp(-exp((a - x) / b)).
    const _RealType __u = __ycxx::__detail::__rand_open01<_RealType>(__g);
    return __parm.a() - __parm.b() * std::log(-std::log(__u));
  }

  _RealType a() const { return __p_.a(); }
  _RealType b() const { return __p_.b(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return -numeric_limits<_RealType>::infinity(); }
  result_type max() const { return numeric_limits<_RealType>::infinity(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os,
                                                  const extreme_value_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.a(), __x.b());
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, extreme_value_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    _RealType a{}, b{};
    if (__ycxx::__detail::__rand_get(is, a, b)) {
      if (0 < b)
        __x.__p_ = param_type(a, b);
      else
        is.setstate(basic_istream<__charT, __traits>::failbit);
    }
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.norm.normal] ----------------------------------------------------------------------
template <class _RealType = double>
class normal_distribution {
  static_assert(__ycxx::__detail::__rand_real_type<_RealType>,
                "normal_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = _RealType;
  class param_type {
  public:
    using distribution_type = normal_distribution;
    param_type() : param_type(0.0) {}
    explicit param_type(_RealType mean, _RealType stddev = 1.0) : __mean_(mean), __stddev_(stddev) {
      __ycxx::__detail::__precondition(0 < stddev, "normal_distribution: requires 0 < stddev");
    }
    _RealType mean() const { return __mean_; }
    _RealType stddev() const { return __stddev_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    _RealType __mean_, __stddev_;
  };

  normal_distribution() : normal_distribution(0.0) {}
  explicit normal_distribution(_RealType mean, _RealType stddev = 1.0) : __p_(mean, stddev) {}
  explicit normal_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() { __saved_valid_ = false; }

  friend bool operator==(const normal_distribution& __x, const normal_distribution& y) {
    return __x.__p_ == y.__p_ && __x.__saved_valid_ == y.__saved_valid_ && (!__x.__saved_valid_ || __x.__saved_ == y.__saved_);
  }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    _RealType __z;
    if (__saved_valid_) {
      __saved_valid_ = false;
      __z = __saved_;
    } else {
      // Marsaglia's polar method: two standard normal variates per accepted pair.
      _RealType __u, __v, s;
      do {
        __u = 2 * __ycxx::__detail::__rand_canonical<_RealType>(__g) - 1;
        __v = 2 * __ycxx::__detail::__rand_canonical<_RealType>(__g) - 1;
        s = __u * __u + __v * __v;
      } while (!(s < 1) || s == 0);
      const _RealType __f = std::sqrt(-2 * std::log(s) / s);
      __saved_ = __v * __f;
      __saved_valid_ = true;
      __z = __u * __f;
    }
    return __parm.mean() + __parm.stddev() * __z;
  }

  _RealType mean() const { return __p_.mean(); }
  _RealType stddev() const { return __p_.stddev(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return -numeric_limits<_RealType>::infinity(); }
  result_type max() const { return numeric_limits<_RealType>::infinity(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const normal_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.mean(), __x.stddev(), __x.__saved_valid_);
    if (__x.__saved_valid_)
      __ycxx::__detail::__rand_put_sep(__os, __x.__saved_);
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, normal_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    _RealType mean{}, stddev{}, __saved{};
    bool valid = false;
    if (!__ycxx::__detail::__rand_get(is, mean, stddev, valid) || (valid && !__ycxx::__detail::__rand_get(is, __saved)))
      return is;
    if (!(0 < stddev)) {
      is.setstate(basic_istream<__charT, __traits>::failbit);
      return is;
    }
    __x.__p_ = param_type(mean, stddev);
    __x.__saved_valid_ = valid;
    __x.__saved_ = valid ? __saved : _RealType(0);
    return is;
  }

private:
  param_type __p_;
  _RealType __saved_ = 0;
  bool __saved_valid_ = false;
};

// ---- [rand.dist.norm.lognormal] -------------------------------------------------------------------
template <class _RealType = double>
class lognormal_distribution {
  static_assert(__ycxx::__detail::__rand_real_type<_RealType>,
                "lognormal_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = _RealType;
  class param_type {
  public:
    using distribution_type = lognormal_distribution;
    param_type() : param_type(0.0) {}
    explicit param_type(_RealType m, _RealType s = 1.0) : __m_(m), __s_(s) {
      __ycxx::__detail::__precondition(0 < s, "lognormal_distribution: requires 0 < s");
    }
    _RealType m() const { return __m_; }
    _RealType s() const { return __s_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    _RealType __m_, __s_;
  };

  lognormal_distribution() : lognormal_distribution(0.0) {}
  explicit lognormal_distribution(_RealType m, _RealType s = 1.0) : __p_(m, s) {}
  explicit lognormal_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() { __nd_.reset(); }

  friend bool operator==(const lognormal_distribution& __x, const lognormal_distribution& y) {
    return __x.__p_ == y.__p_ && __x.__nd_ == y.__nd_;
  }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    return std::exp(__parm.m() + __parm.s() * __nd_(__g));
  }

  _RealType m() const { return __p_.m(); }
  _RealType s() const { return __p_.s(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<_RealType>::infinity(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const lognormal_distribution& __x) {
    {
      auto __st = __ycxx::__detail::__rand_out_dist(__os);
      __ycxx::__detail::__rand_put(__os, __x.m(), __x.s());
      __os.put(__os.widen(' '));
    }
    return __os << __x.__nd_;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, lognormal_distribution& __x) {
    _RealType m{}, s{};
    {
      auto __st = __ycxx::__detail::__rand_in(is);
      if (!__ycxx::__detail::__rand_get(is, m, s))
        return is;
    }
    normal_distribution<_RealType> __nd;
    if (!(is >> __nd))
      return is;
    if (!(0 < s)) {
      is.setstate(basic_istream<__charT, __traits>::failbit);
      return is;
    }
    __x.__p_ = param_type(m, s);
    __x.__nd_ = __nd;
    return is;
  }

private:
  param_type __p_;
  normal_distribution<_RealType> __nd_;
};

// ---- [rand.dist.norm.chisq] -----------------------------------------------------------------------
template <class _RealType = double>
class chi_squared_distribution {
  static_assert(__ycxx::__detail::__rand_real_type<_RealType>,
                "chi_squared_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = _RealType;
  class param_type {
  public:
    using distribution_type = chi_squared_distribution;
    param_type() : param_type(1.0) {}
    explicit param_type(_RealType n) : __n_(n) {
      __ycxx::__detail::__precondition(0 < n, "chi_squared_distribution: requires 0 < n");
    }
    _RealType n() const { return __n_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    _RealType __n_;
  };

  chi_squared_distribution() : chi_squared_distribution(1.0) {}
  explicit chi_squared_distribution(_RealType n) : __p_(n) {}
  explicit chi_squared_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const chi_squared_distribution& __x, const chi_squared_distribution& y) { return __x.__p_ == y.__p_; }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    return 2 * __ycxx::__detail::__rand_gamma<_RealType>(__g, __parm.n() / 2);
  }

  _RealType n() const { return __p_.n(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<_RealType>::infinity(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const chi_squared_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.n());
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, chi_squared_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    _RealType n{};
    if (__ycxx::__detail::__rand_get(is, n)) {
      if (0 < n)
        __x.__p_ = param_type(n);
      else
        is.setstate(basic_istream<__charT, __traits>::failbit);
    }
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.norm.cauchy] ----------------------------------------------------------------------
template <class _RealType = double>
class cauchy_distribution {
  static_assert(__ycxx::__detail::__rand_real_type<_RealType>,
                "cauchy_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = _RealType;
  class param_type {
  public:
    using distribution_type = cauchy_distribution;
    param_type() : param_type(0.0) {}
    explicit param_type(_RealType a, _RealType b = 1.0) : __a_(a), __b_(b) {
      __ycxx::__detail::__precondition(0 < b, "cauchy_distribution: requires 0 < b");
    }
    _RealType a() const { return __a_; }
    _RealType b() const { return __b_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    _RealType __a_, __b_;
  };

  cauchy_distribution() : cauchy_distribution(0.0) {}
  explicit cauchy_distribution(_RealType a, _RealType b = 1.0) : __p_(a, b) {}
  explicit cauchy_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const cauchy_distribution& __x, const cauchy_distribution& y) { return __x.__p_ == y.__p_; }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    const _RealType __u = __ycxx::__detail::__rand_open01<_RealType>(__g);
    return __parm.a() + __parm.b() * std::tan(numbers::pi_v<_RealType> * (__u - _RealType(0.5)));
  }

  _RealType a() const { return __p_.a(); }
  _RealType b() const { return __p_.b(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return -numeric_limits<_RealType>::infinity(); }
  result_type max() const { return numeric_limits<_RealType>::infinity(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const cauchy_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.a(), __x.b());
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, cauchy_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    _RealType a{}, b{};
    if (__ycxx::__detail::__rand_get(is, a, b)) {
      if (0 < b)
        __x.__p_ = param_type(a, b);
      else
        is.setstate(basic_istream<__charT, __traits>::failbit);
    }
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.norm.f] ---------------------------------------------------------------------------
template <class _RealType = double>
class fisher_f_distribution {
  static_assert(__ycxx::__detail::__rand_real_type<_RealType>,
                "fisher_f_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = _RealType;
  class param_type {
  public:
    using distribution_type = fisher_f_distribution;
    param_type() : param_type(1.0) {}
    explicit param_type(_RealType m, _RealType n = 1) : __m_(m), __n_(n) {
      __ycxx::__detail::__precondition(0 < m && 0 < n, "fisher_f_distribution: requires 0 < m and 0 < n");
    }
    _RealType m() const { return __m_; }
    _RealType n() const { return __n_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    _RealType __m_, __n_;
  };

  fisher_f_distribution() : fisher_f_distribution(1.0) {}
  explicit fisher_f_distribution(_RealType m, _RealType n = 1) : __p_(m, n) {}
  explicit fisher_f_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const fisher_f_distribution& __x, const fisher_f_distribution& y) { return __x.__p_ == y.__p_; }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    // (chi2(m) / m) / (chi2(n) / n), with chi2(k) = 2 gamma(k / 2).
    const _RealType __x = __ycxx::__detail::__rand_gamma<_RealType>(__g, __parm.m() / 2);
    const _RealType y = __ycxx::__detail::__rand_gamma<_RealType>(__g, __parm.n() / 2);
    return (__x * __parm.n()) / (y * __parm.m());
  }

  _RealType m() const { return __p_.m(); }
  _RealType n() const { return __p_.n(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return 0; }
  result_type max() const { return numeric_limits<_RealType>::infinity(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const fisher_f_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.m(), __x.n());
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, fisher_f_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    _RealType m{}, n{};
    if (__ycxx::__detail::__rand_get(is, m, n)) {
      if (0 < m && 0 < n)
        __x.__p_ = param_type(m, n);
      else
        is.setstate(basic_istream<__charT, __traits>::failbit);
    }
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.norm.t] ---------------------------------------------------------------------------
template <class _RealType = double>
class student_t_distribution {
  static_assert(__ycxx::__detail::__rand_real_type<_RealType>,
                "student_t_distribution: RealType must be float, double or long double ([rand.req.genl]/1.5)");

public:
  using result_type = _RealType;
  class param_type {
  public:
    using distribution_type = student_t_distribution;
    param_type() : param_type(1.0) {}
    explicit param_type(_RealType n) : __n_(n) {
      __ycxx::__detail::__precondition(0 < n, "student_t_distribution: requires 0 < n");
    }
    _RealType n() const { return __n_; }
    friend bool operator==(const param_type&, const param_type&) = default;

  private:
    _RealType __n_;
  };

  student_t_distribution() : student_t_distribution(1.0) {}
  explicit student_t_distribution(_RealType n) : __p_(n) {}
  explicit student_t_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() { __nd_.reset(); }

  friend bool operator==(const student_t_distribution& __x, const student_t_distribution& y) {
    return __x.__p_ == y.__p_ && __x.__nd_ == y.__nd_;
  }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    // Z / sqrt(chi2(n) / n).
    const _RealType __z = __nd_(__g);
    const _RealType c = 2 * __ycxx::__detail::__rand_gamma<_RealType>(__g, __parm.n() / 2);
    return __z * std::sqrt(__parm.n() / c);
  }

  _RealType n() const { return __p_.n(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return -numeric_limits<_RealType>::infinity(); }
  result_type max() const { return numeric_limits<_RealType>::infinity(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const student_t_distribution& __x) {
    {
      auto __st = __ycxx::__detail::__rand_out_dist(__os);
      __ycxx::__detail::__rand_put(__os, __x.n());
      __os.put(__os.widen(' '));
    }
    return __os << __x.__nd_;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, student_t_distribution& __x) {
    _RealType n{};
    {
      auto __st = __ycxx::__detail::__rand_in(is);
      if (!__ycxx::__detail::__rand_get(is, n))
        return is;
    }
    normal_distribution<_RealType> __nd;
    if (!(is >> __nd))
      return is;
    if (!(0 < n)) {
      is.setstate(basic_istream<__charT, __traits>::failbit);
      return is;
    }
    __x.__p_ = param_type(n);
    __x.__nd_ = __nd;
    return is;
  }

private:
  param_type __p_;
  normal_distribution<_RealType> __nd_;
};

} // namespace std
