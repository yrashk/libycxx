// libycxx core: the sampling distributions of <random> ([rand.dist.samp]): discrete,
// piecewise_constant and piecewise_linear.
//
// Each param_type keeps the distribution's parameters as the draft states them (p_k, or b_k and
// rho_k) and, derived from exactly those values, the cumulative probabilities of the outcomes or
// intervals; a variate picks the outcome or interval by a binary search for a canonical uniform
// value. Since the cumulative table is a function of the stored parameters alone, a
// distribution read back from a stream ([rand.req.dist]/6) behaves exactly like the written one.
#pragma once

#include <initializer_list>
#include <ycxx/core/cmath.hpp>
#include <ycxx/core/iterator_core.hpp>
#include <ycxx/core/random_base.hpp>
#include <ycxx/core/vector.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// The cumulative sums of `__mass`, normalized so that the last entry with a positive mass, and every
// entry after it, is exactly 1 (so that a uniform value in [0, 1) never selects a trailing
// zero-mass entry).
inline std::vector<double> __rand_cumulative(const std::vector<double>& __mass) {
  std::vector<double> __cdf(__mass.size());
  double sum = 0;
  size_t last = 0;
  for (size_t k = 0; k < __mass.size(); ++k) {
    sum += __mass[k];
    __cdf[k] = sum;
    if (__mass[k] > 0)
      last = k;
  }
  for (size_t k = 0; k < __cdf.size(); ++k)
    __cdf[k] = k >= last ? 1.0 : __cdf[k] / sum;
  return __cdf;
}
// The first index k with u < cdf[k].
inline size_t __rand_find(const std::vector<double>& __cdf, double __u) {
  size_t __lo = 0, __hi = __cdf.size() - 1;
  while (__lo < __hi) {
    const size_t __mid = __lo + (__hi - __lo) / 2;
    if (__u < __cdf[__mid])
      __hi = __mid;
    else
      __lo = __mid + 1;
  }
  return __lo;
}

// Weights must be non-negative and finite, with a positive sum ([rand.dist.samp.discrete]/2).
inline void __rand_check_weights(const std::vector<double>& __w, const char* __msg) {
  double sum = 0;
  bool ok = true;
  for (double __x : __w) {
    ok = ok && __x >= 0 && __x <= std::numeric_limits<double>::max();
    sum += __x;
  }
  ::__ycxx::__detail::__precondition(ok && sum > 0, __msg);
}

template <class _It>
concept __rand_input_iter = requires { typename std::iterator_traits<_It>::value_type; };

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// ---- [rand.dist.samp.discrete] --------------------------------------------------------------------
template <class _IntType = int>
class discrete_distribution {
  static_assert(__ycxx::__detail::__rand_int_type<_IntType>,
                "discrete_distribution: IntType must be a standard integer type ([rand.req.genl]/1.6)");

public:
  using result_type = _IntType;
  class param_type {
  public:
    using distribution_type = discrete_distribution;
    param_type() : __p_{1.0}, __cdf_{1.0} {}
    template <class _InputIterator>
      requires __ycxx::__detail::__rand_input_iter<_InputIterator>
    param_type(_InputIterator __firstW, _InputIterator __lastW) {
      static_assert(is_convertible_v<typename iterator_traits<_InputIterator>::value_type, double>,
                    "discrete_distribution: the weights must be convertible to double");
      for (; __firstW != __lastW; ++__firstW)
        __p_.push_back(static_cast<double>(*__firstW));
      init();
    }
    param_type(initializer_list<double> __wl) : param_type(__wl.begin(), __wl.end()) {}
    template <class _UnaryOperation>
    param_type(size_t __nw, double __xmin, double __xmax, _UnaryOperation __fw) {
      static_assert(is_invocable_r_v<double, _UnaryOperation&, double>,
                    "discrete_distribution: fw must be callable with a double, returning a double");
      const size_t n = __nw == 0 ? 1 : __nw;
      const double __delta = (__xmax - __xmin) / static_cast<double>(n);
      __ycxx::__detail::__precondition(0 < __delta, "discrete_distribution: requires 0 < (xmax - xmin) / n");
      if (__nw != 0) {
        __p_.reserve(n);
        for (size_t k = 0; k < n; ++k)
          __p_.push_back(static_cast<double>(__fw(__xmin + static_cast<double>(k) * __delta + __delta / 2)));
      }
      init();
    }

    vector<double> probabilities() const { return __p_; }
    friend bool operator==(const param_type& __x, const param_type& y) { return __x.__p_ == y.__p_; }

  private:
    friend discrete_distribution;
    struct __exact_tag {};
    param_type(__exact_tag, vector<double>&& p) : __p_(std::move(p)), __cdf_(__ycxx::__detail::__rand_cumulative(__p_)) {}

    void init() {
      if (__p_.empty())
        __p_.push_back(1.0);
      __ycxx::__detail::__rand_check_weights(__p_, "discrete_distribution: the weights must be non-negative and finite, "
                                           "with a positive sum");
      double sum = 0;
      for (double __w : __p_)
        sum += __w;
      for (double& __w : __p_)
        __w /= sum;
      __cdf_ = __ycxx::__detail::__rand_cumulative(__p_);
    }

    vector<double> __p_;
    vector<double> __cdf_;
  };

  discrete_distribution() {}
  template <class _InputIterator>
    requires __ycxx::__detail::__rand_input_iter<_InputIterator>
  discrete_distribution(_InputIterator __firstW, _InputIterator __lastW) : __p_(__firstW, __lastW) {}
  discrete_distribution(initializer_list<double> __wl) : __p_(__wl) {}
  template <class _UnaryOperation>
  discrete_distribution(size_t __nw, double __xmin, double __xmax, _UnaryOperation __fw) : __p_(__nw, __xmin, __xmax, std::move(__fw)) {}
  explicit discrete_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const discrete_distribution& __x, const discrete_distribution& y) { return __x.__p_ == y.__p_; }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    return static_cast<_IntType>(__ycxx::__detail::__rand_find(__parm.__cdf_, __ycxx::__detail::__rand_canonical<double>(__g)));
  }

  vector<double> probabilities() const { return __p_.probabilities(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return 0; }
  result_type max() const { return static_cast<_IntType>(__p_.__p_.size() - 1); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const discrete_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.__p_.__p_.size());
    __ycxx::__detail::__rand_put_seq(__os, __x.__p_.__p_);
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, discrete_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    size_t n = 0;
    if (!__ycxx::__detail::__rand_get(is, n))
      return is;
    vector<double> p;
    double sum = 0;
    for (size_t k = 0; k < n; ++k) {
      double __v = 0;
      if (!__ycxx::__detail::__rand_get(is, __v))
        return is;
      if (!(__v >= 0 && __v <= 1)) {
        is.setstate(basic_istream<__charT, __traits>::failbit);
        return is;
      }
      sum += __v;
      p.push_back(__v);
    }
    if (n == 0 || !(sum > 0)) {
      is.setstate(basic_istream<__charT, __traits>::failbit);
      return is;
    }
    __x.__p_ = param_type(typename param_type::__exact_tag{}, std::move(p));
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.samp.pconst] ----------------------------------------------------------------------
template <class _RealType = double>
class piecewise_constant_distribution {
  static_assert(__ycxx::__detail::__rand_real_type<_RealType>,
                "piecewise_constant_distribution: RealType must be float, double or long double "
                "([rand.req.genl]/1.5)");

public:
  using result_type = _RealType;
  class param_type {
  public:
    using distribution_type = piecewise_constant_distribution;
    param_type() : __b_{0, 1}, __rho_{1}, __cdf_{1.0} {}
    template <class _InputIteratorB, class _InputIteratorW>
      requires __ycxx::__detail::__rand_input_iter<_InputIteratorB>
    param_type(_InputIteratorB __firstB, _InputIteratorB __lastB, _InputIteratorW __firstW) {
      static_assert(is_convertible_v<typename iterator_traits<_InputIteratorB>::value_type, double> &&
                        is_convertible_v<typename iterator_traits<_InputIteratorW>::value_type, double>,
                    "piecewise_constant_distribution: the boundaries and weights must be convertible to double");
      for (; __firstB != __lastB; ++__firstB)
        __b_.push_back(static_cast<_RealType>(*__firstB));
      if (__b_.size() < 2) {
        *this = param_type();
        return;
      }
      vector<double> __w;
      for (size_t k = 0; k + 1 < __b_.size(); ++k) {
        __w.push_back(static_cast<double>(*__firstW));
        if (k + 2 < __b_.size()) // no increment past the last weight used (an input iterator reads ahead)
          ++__firstW;
      }
      init(__w);
    }
    template <class _UnaryOperation>
    param_type(initializer_list<_RealType> __bl, _UnaryOperation __fw) {
      static_assert(is_invocable_r_v<double, _UnaryOperation&, double>,
                    "piecewise_constant_distribution: fw must be callable with a double, returning a double");
      if (__bl.size() < 2) {
        *this = param_type();
        return;
      }
      __b_.assign(__bl.begin(), __bl.end());
      vector<double> __w;
      for (size_t k = 0; k + 1 < __b_.size(); ++k)
        __w.push_back(static_cast<double>(__fw((__b_[k + 1] + __b_[k]) / 2)));
      init(__w);
    }
    template <class _UnaryOperation>
    param_type(size_t __nw, _RealType __xmin, _RealType __xmax, _UnaryOperation __fw) {
      static_assert(is_invocable_r_v<double, _UnaryOperation&, double>,
                    "piecewise_constant_distribution: fw must be callable with a double, returning a double");
      const size_t n = __nw == 0 ? 1 : __nw;
      const _RealType __delta = (__xmax - __xmin) / static_cast<_RealType>(n);
      __ycxx::__detail::__precondition(0 < __delta, "piecewise_constant_distribution: requires 0 < (xmax - xmin) / n");
      vector<double> __w;
      for (size_t k = 0; k <= n; ++k)
        __b_.push_back(k == n ? __xmax : __xmin + static_cast<_RealType>(k) * __delta);
      for (size_t k = 0; k < n; ++k)
        __w.push_back(static_cast<double>(__fw(__b_[k] + __delta / 2)));
      init(__w);
    }

    vector<result_type> intervals() const { return __b_; }
    vector<result_type> densities() const { return __rho_; }
    friend bool operator==(const param_type& __x, const param_type& y) { return __x.__b_ == y.__b_ && __x.__rho_ == y.__rho_; }

  private:
    friend piecewise_constant_distribution;
    struct __exact_tag {};
    param_type(__exact_tag, vector<_RealType>&& b, vector<_RealType>&& __rho) : __b_(std::move(b)), __rho_(std::move(__rho)) {
      __build();
    }

    void init(const vector<double>& __w) {
      for (size_t k = 0; k + 1 < __b_.size(); ++k)
        __ycxx::__detail::__precondition(__b_[k] < __b_[k + 1], "piecewise_constant_distribution: requires b_i < b_i+1");
      __ycxx::__detail::__rand_check_weights(__w, "piecewise_constant_distribution: the weights must be non-negative and "
                                          "finite, with a positive sum");
      double sum = 0;
      for (double __x : __w)
        sum += __x;
      __rho_.resize(__w.size());
      for (size_t k = 0; k < __w.size(); ++k)
        __rho_[k] = static_cast<_RealType>(__w[k] / (sum * static_cast<double>(__b_[k + 1] - __b_[k])));
      __build();
    }
    void __build() {
      vector<double> __mass(__rho_.size());
      for (size_t k = 0; k < __rho_.size(); ++k)
        __mass[k] = static_cast<double>(__rho_[k]) * static_cast<double>(__b_[k + 1] - __b_[k]);
      __cdf_ = __ycxx::__detail::__rand_cumulative(__mass);
    }

    vector<_RealType> __b_;
    vector<_RealType> __rho_;
    vector<double> __cdf_;
  };

  piecewise_constant_distribution() {}
  template <class _InputIteratorB, class _InputIteratorW>
    requires __ycxx::__detail::__rand_input_iter<_InputIteratorB>
  piecewise_constant_distribution(_InputIteratorB __firstB, _InputIteratorB __lastB, _InputIteratorW __firstW)
      : __p_(__firstB, __lastB, __firstW) {}
  template <class _UnaryOperation>
  piecewise_constant_distribution(initializer_list<_RealType> __bl, _UnaryOperation __fw) : __p_(__bl, std::move(__fw)) {}
  template <class _UnaryOperation>
  piecewise_constant_distribution(size_t __nw, _RealType __xmin, _RealType __xmax, _UnaryOperation __fw)
      : __p_(__nw, __xmin, __xmax, std::move(__fw)) {}
  explicit piecewise_constant_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const piecewise_constant_distribution& __x, const piecewise_constant_distribution& y) {
    return __x.__p_ == y.__p_;
  }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    const size_t k = __ycxx::__detail::__rand_find(__parm.__cdf_, __ycxx::__detail::__rand_canonical<double>(__g));
    const _RealType __lo = __parm.__b_[k], __hi = __parm.__b_[k + 1];
    const _RealType __x = __lo + (__hi - __lo) * __ycxx::__detail::__rand_canonical<_RealType>(__g);
    return __x < __hi ? __x : std::nextafter(__hi, __lo);
  }

  vector<result_type> intervals() const { return __p_.intervals(); }
  vector<result_type> densities() const { return __p_.densities(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return __p_.__b_.front(); }
  result_type max() const { return __p_.__b_.back(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os,
                                                  const piecewise_constant_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.__p_.__rho_.size());
    __ycxx::__detail::__rand_put_seq(__os, __x.__p_.__b_);
    __ycxx::__detail::__rand_put_seq(__os, __x.__p_.__rho_);
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is,
                                                  piecewise_constant_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    size_t n = 0;
    if (!__ycxx::__detail::__rand_get(is, n))
      return is;
    vector<_RealType> b, __rho;
    for (size_t k = 0; k <= n; ++k) {
      _RealType __v{};
      if (!__ycxx::__detail::__rand_get(is, __v))
        return is;
      if (!b.empty() && !(b.back() < __v)) {
        is.setstate(basic_istream<__charT, __traits>::failbit);
        return is;
      }
      b.push_back(__v);
    }
    double __total = 0;
    for (size_t k = 0; k < n; ++k) {
      _RealType __v{};
      if (!__ycxx::__detail::__rand_get(is, __v))
        return is;
      if (!(__v >= 0) || !(__v <= numeric_limits<_RealType>::max())) {
        is.setstate(basic_istream<__charT, __traits>::failbit);
        return is;
      }
      __total += static_cast<double>(__v);
      __rho.push_back(__v);
    }
    if (n == 0 || !(__total > 0)) {
      is.setstate(basic_istream<__charT, __traits>::failbit);
      return is;
    }
    __x.__p_ = param_type(typename param_type::__exact_tag{}, std::move(b), std::move(__rho));
    return is;
  }

private:
  param_type __p_;
};

// ---- [rand.dist.samp.plinear] ---------------------------------------------------------------------
template <class _RealType = double>
class piecewise_linear_distribution {
  static_assert(__ycxx::__detail::__rand_real_type<_RealType>,
                "piecewise_linear_distribution: RealType must be float, double or long double "
                "([rand.req.genl]/1.5)");

public:
  using result_type = _RealType;
  class param_type {
  public:
    using distribution_type = piecewise_linear_distribution;
    param_type() : __b_{0, 1}, __rho_{1, 1}, __cdf_{1.0} {}
    template <class _InputIteratorB, class _InputIteratorW>
      requires __ycxx::__detail::__rand_input_iter<_InputIteratorB>
    param_type(_InputIteratorB __firstB, _InputIteratorB __lastB, _InputIteratorW __firstW) {
      static_assert(is_convertible_v<typename iterator_traits<_InputIteratorB>::value_type, double> &&
                        is_convertible_v<typename iterator_traits<_InputIteratorW>::value_type, double>,
                    "piecewise_linear_distribution: the boundaries and weights must be convertible to double");
      for (; __firstB != __lastB; ++__firstB)
        __b_.push_back(static_cast<_RealType>(*__firstB));
      if (__b_.size() < 2) {
        *this = param_type();
        return;
      }
      vector<double> __w;
      for (size_t k = 0; k < __b_.size(); ++k) {
        __w.push_back(static_cast<double>(*__firstW));
        if (k + 1 < __b_.size()) // no increment past the last weight used
          ++__firstW;
      }
      init(__w);
    }
    template <class _UnaryOperation>
    param_type(initializer_list<_RealType> __bl, _UnaryOperation __fw) {
      static_assert(is_invocable_r_v<double, _UnaryOperation&, double>,
                    "piecewise_linear_distribution: fw must be callable with a double, returning a double");
      if (__bl.size() < 2) {
        *this = param_type();
        return;
      }
      __b_.assign(__bl.begin(), __bl.end());
      vector<double> __w;
      for (size_t k = 0; k < __b_.size(); ++k)
        __w.push_back(static_cast<double>(__fw(__b_[k])));
      init(__w);
    }
    template <class _UnaryOperation>
    param_type(size_t __nw, _RealType __xmin, _RealType __xmax, _UnaryOperation __fw) {
      static_assert(is_invocable_r_v<double, _UnaryOperation&, double>,
                    "piecewise_linear_distribution: fw must be callable with a double, returning a double");
      const size_t n = __nw == 0 ? 1 : __nw;
      const _RealType __delta = (__xmax - __xmin) / static_cast<_RealType>(n);
      __ycxx::__detail::__precondition(0 < __delta, "piecewise_linear_distribution: requires 0 < (xmax - xmin) / n");
      vector<double> __w;
      for (size_t k = 0; k <= n; ++k) {
        __b_.push_back(k == n ? __xmax : __xmin + static_cast<_RealType>(k) * __delta);
        __w.push_back(static_cast<double>(__fw(__xmin + static_cast<_RealType>(k) * __delta)));
      }
      init(__w);
    }

    vector<result_type> intervals() const { return __b_; }
    vector<result_type> densities() const { return __rho_; }
    friend bool operator==(const param_type& __x, const param_type& y) { return __x.__b_ == y.__b_ && __x.__rho_ == y.__rho_; }

  private:
    friend piecewise_linear_distribution;
    struct __exact_tag {};
    param_type(__exact_tag, vector<_RealType>&& b, vector<_RealType>&& __rho) : __b_(std::move(b)), __rho_(std::move(__rho)) {
      __build();
    }

    void init(const vector<double>& __w) {
      for (size_t k = 0; k + 1 < __b_.size(); ++k)
        __ycxx::__detail::__precondition(__b_[k] < __b_[k + 1], "piecewise_linear_distribution: requires b_i < b_i+1");
      bool ok = true;
      double sum = 0;
      for (size_t k = 0; k < __w.size(); ++k) {
        ok = ok && __w[k] >= 0 && __w[k] <= numeric_limits<double>::max();
        if (k + 1 < __w.size())
          sum += (__w[k] + __w[k + 1]) * static_cast<double>(__b_[k + 1] - __b_[k]) / 2;
      }
      __ycxx::__detail::__precondition(ok && sum > 0, "piecewise_linear_distribution: the weights must be non-negative "
                                                "and finite, with a positive integral");
      __rho_.resize(__w.size());
      for (size_t k = 0; k < __w.size(); ++k)
        __rho_[k] = static_cast<_RealType>(__w[k] / sum);
      __build();
    }
    void __build() {
      vector<double> __mass(__rho_.size() - 1);
      for (size_t k = 0; k + 1 < __rho_.size(); ++k)
        __mass[k] = (static_cast<double>(__rho_[k]) + static_cast<double>(__rho_[k + 1])) *
                  static_cast<double>(__b_[k + 1] - __b_[k]) / 2;
      __cdf_ = __ycxx::__detail::__rand_cumulative(__mass);
    }

    vector<_RealType> __b_;
    vector<_RealType> __rho_;
    vector<double> __cdf_;
  };

  piecewise_linear_distribution() {}
  template <class _InputIteratorB, class _InputIteratorW>
    requires __ycxx::__detail::__rand_input_iter<_InputIteratorB>
  piecewise_linear_distribution(_InputIteratorB __firstB, _InputIteratorB __lastB, _InputIteratorW __firstW)
      : __p_(__firstB, __lastB, __firstW) {}
  template <class _UnaryOperation>
  piecewise_linear_distribution(initializer_list<_RealType> __bl, _UnaryOperation __fw) : __p_(__bl, std::move(__fw)) {}
  template <class _UnaryOperation>
  piecewise_linear_distribution(size_t __nw, _RealType __xmin, _RealType __xmax, _UnaryOperation __fw)
      : __p_(__nw, __xmin, __xmax, std::move(__fw)) {}
  explicit piecewise_linear_distribution(const param_type& __parm) : __p_(__parm) {}
  void reset() {}

  friend bool operator==(const piecewise_linear_distribution& __x, const piecewise_linear_distribution& y) {
    return __x.__p_ == y.__p_;
  }

  template <class _URBG>
  result_type operator()(_URBG& __g) {
    return (*this)(__g, __p_);
  }
  template <class _URBG>
  result_type operator()(_URBG& __g, const param_type& __parm) {
    const size_t k = __ycxx::__detail::__rand_find(__parm.__cdf_, __ycxx::__detail::__rand_canonical<double>(__g));
    const _RealType __lo = __parm.__b_[k], __hi = __parm.__b_[k + 1];
    const _RealType __r0 = __parm.__rho_[k], __r1 = __parm.__rho_[k + 1];
    // Inverse of the linear density's distribution on [0, 1): F(t) = (r0 t + (r1 - r0) t^2 / 2)
    // / ((r0 + r1) / 2), solved in the cancellation-free form.
    const _RealType __u = __ycxx::__detail::__rand_canonical<_RealType>(__g);
    _RealType t;
    if (__r0 == __r1) {
      t = __u;
    } else {
      const _RealType num = __u * (__r0 + __r1);
      const _RealType den = __r0 + std::sqrt(__r0 * __r0 + (__r1 - __r0) * num);
      t = den > 0 ? num / den : _RealType(0);
    }
    const _RealType __x = __lo + (__hi - __lo) * t;
    return __x < __hi ? __x : std::nextafter(__hi, __lo);
  }

  vector<result_type> intervals() const { return __p_.intervals(); }
  vector<result_type> densities() const { return __p_.densities(); }
  param_type param() const { return __p_; }
  void param(const param_type& __parm) { __p_ = __parm; }
  result_type min() const { return __p_.__b_.front(); }
  result_type max() const { return __p_.__b_.back(); }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os,
                                                  const piecewise_linear_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_out_dist(__os);
    __ycxx::__detail::__rand_put(__os, __x.__p_.__b_.size() - 1);
    __ycxx::__detail::__rand_put_seq(__os, __x.__p_.__b_);
    __ycxx::__detail::__rand_put_seq(__os, __x.__p_.__rho_);
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, piecewise_linear_distribution& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    size_t n = 0;
    if (!__ycxx::__detail::__rand_get(is, n))
      return is;
    vector<_RealType> b, __rho;
    for (size_t k = 0; k <= n; ++k) {
      _RealType __v{};
      if (!__ycxx::__detail::__rand_get(is, __v))
        return is;
      if (!b.empty() && !(b.back() < __v)) {
        is.setstate(basic_istream<__charT, __traits>::failbit);
        return is;
      }
      b.push_back(__v);
    }
    double __total = 0;
    for (size_t k = 0; k <= n; ++k) {
      _RealType __v{};
      if (!__ycxx::__detail::__rand_get(is, __v))
        return is;
      if (!(__v >= 0) || !(__v <= numeric_limits<_RealType>::max())) {
        is.setstate(basic_istream<__charT, __traits>::failbit);
        return is;
      }
      __total += static_cast<double>(__v);
      __rho.push_back(__v);
    }
    if (n == 0 || !(__total > 0)) {
      is.setstate(basic_istream<__charT, __traits>::failbit);
      return is;
    }
    __x.__p_ = param_type(typename param_type::__exact_tag{}, std::move(b), std::move(__rho));
    return is;
  }

private:
  param_type __p_;
};

} // namespace std
