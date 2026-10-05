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

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// The cumulative sums of `mass`, normalized so that the last entry with a positive mass, and every
// entry after it, is exactly 1 (so that a uniform value in [0, 1) never selects a trailing
// zero-mass entry).
inline std::vector<double> rand_cumulative(const std::vector<double>& mass) {
  std::vector<double> cdf(mass.size());
  double sum = 0;
  size_t last = 0;
  for (size_t k = 0; k < mass.size(); ++k) {
    sum += mass[k];
    cdf[k] = sum;
    if (mass[k] > 0)
      last = k;
  }
  for (size_t k = 0; k < cdf.size(); ++k)
    cdf[k] = k >= last ? 1.0 : cdf[k] / sum;
  return cdf;
}
// The first index k with u < cdf[k].
inline size_t rand_find(const std::vector<double>& cdf, double u) {
  size_t lo = 0, hi = cdf.size() - 1;
  while (lo < hi) {
    const size_t mid = lo + (hi - lo) / 2;
    if (u < cdf[mid])
      hi = mid;
    else
      lo = mid + 1;
  }
  return lo;
}

// Weights must be non-negative and finite, with a positive sum ([rand.dist.samp.discrete]/2).
inline void rand_check_weights(const std::vector<double>& w, const char* msg) {
  double sum = 0;
  bool ok = true;
  for (double x : w) {
    ok = ok && x >= 0 && x <= std::numeric_limits<double>::max();
    sum += x;
  }
  ::ycxx::detail::precondition(ok && sum > 0, msg);
}

template <class It>
concept rand_input_iter = requires { typename std::iterator_traits<It>::value_type; };

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// ---- [rand.dist.samp.discrete] --------------------------------------------------------------------
template <class IntType = int>
class discrete_distribution {
  static_assert(ycxx::detail::rand_int_type<IntType>,
                "discrete_distribution: IntType must be a standard integer type ([rand.req.genl]/1.6)");

public:
  using result_type = IntType;
  class param_type {
  public:
    using distribution_type = discrete_distribution;
    param_type() : p_{1.0}, cdf_{1.0} {}
    template <class InputIterator>
      requires ycxx::detail::rand_input_iter<InputIterator>
    param_type(InputIterator firstW, InputIterator lastW) {
      static_assert(is_convertible_v<typename iterator_traits<InputIterator>::value_type, double>,
                    "discrete_distribution: the weights must be convertible to double");
      for (; firstW != lastW; ++firstW)
        p_.push_back(static_cast<double>(*firstW));
      init();
    }
    param_type(initializer_list<double> wl) : param_type(wl.begin(), wl.end()) {}
    template <class UnaryOperation>
    param_type(size_t nw, double xmin, double xmax, UnaryOperation fw) {
      static_assert(is_invocable_r_v<double, UnaryOperation&, double>,
                    "discrete_distribution: fw must be callable with a double, returning a double");
      const size_t n = nw == 0 ? 1 : nw;
      const double delta = (xmax - xmin) / static_cast<double>(n);
      ycxx::detail::precondition(0 < delta, "discrete_distribution: requires 0 < (xmax - xmin) / n");
      if (nw != 0) {
        p_.reserve(n);
        for (size_t k = 0; k < n; ++k)
          p_.push_back(static_cast<double>(fw(xmin + static_cast<double>(k) * delta + delta / 2)));
      }
      init();
    }

    vector<double> probabilities() const { return p_; }
    friend bool operator==(const param_type& x, const param_type& y) { return x.p_ == y.p_; }

  private:
    friend discrete_distribution;
    struct exact_tag {};
    param_type(exact_tag, vector<double>&& p) : p_(std::move(p)), cdf_(ycxx::detail::rand_cumulative(p_)) {}

    void init() {
      if (p_.empty())
        p_.push_back(1.0);
      ycxx::detail::rand_check_weights(p_, "discrete_distribution: the weights must be non-negative and finite, "
                                           "with a positive sum");
      double sum = 0;
      for (double w : p_)
        sum += w;
      for (double& w : p_)
        w /= sum;
      cdf_ = ycxx::detail::rand_cumulative(p_);
    }

    vector<double> p_;
    vector<double> cdf_;
  };

  discrete_distribution() {}
  template <class InputIterator>
    requires ycxx::detail::rand_input_iter<InputIterator>
  discrete_distribution(InputIterator firstW, InputIterator lastW) : p_(firstW, lastW) {}
  discrete_distribution(initializer_list<double> wl) : p_(wl) {}
  template <class UnaryOperation>
  discrete_distribution(size_t nw, double xmin, double xmax, UnaryOperation fw) : p_(nw, xmin, xmax, std::move(fw)) {}
  explicit discrete_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const discrete_distribution& x, const discrete_distribution& y) { return x.p_ == y.p_; }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    return static_cast<IntType>(ycxx::detail::rand_find(parm.cdf_, ycxx::detail::rand_canonical<double>(g)));
  }

  vector<double> probabilities() const { return p_.probabilities(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return 0; }
  result_type max() const { return static_cast<IntType>(p_.p_.size() - 1); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const discrete_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.p_.p_.size());
    ycxx::detail::rand_put_seq(os, x.p_.p_);
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, discrete_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    size_t n = 0;
    if (!ycxx::detail::rand_get(is, n))
      return is;
    vector<double> p;
    double sum = 0;
    for (size_t k = 0; k < n; ++k) {
      double v = 0;
      if (!ycxx::detail::rand_get(is, v))
        return is;
      if (!(v >= 0 && v <= 1)) {
        is.setstate(basic_istream<charT, traits>::failbit);
        return is;
      }
      sum += v;
      p.push_back(v);
    }
    if (n == 0 || !(sum > 0)) {
      is.setstate(basic_istream<charT, traits>::failbit);
      return is;
    }
    x.p_ = param_type(typename param_type::exact_tag{}, std::move(p));
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.samp.pconst] ----------------------------------------------------------------------
template <class RealType = double>
class piecewise_constant_distribution {
  static_assert(ycxx::detail::rand_real_type<RealType>,
                "piecewise_constant_distribution: RealType must be float, double or long double "
                "([rand.req.genl]/1.5)");

public:
  using result_type = RealType;
  class param_type {
  public:
    using distribution_type = piecewise_constant_distribution;
    param_type() : b_{0, 1}, rho_{1}, cdf_{1.0} {}
    template <class InputIteratorB, class InputIteratorW>
      requires ycxx::detail::rand_input_iter<InputIteratorB>
    param_type(InputIteratorB firstB, InputIteratorB lastB, InputIteratorW firstW) {
      static_assert(is_convertible_v<typename iterator_traits<InputIteratorB>::value_type, double> &&
                        is_convertible_v<typename iterator_traits<InputIteratorW>::value_type, double>,
                    "piecewise_constant_distribution: the boundaries and weights must be convertible to double");
      for (; firstB != lastB; ++firstB)
        b_.push_back(static_cast<RealType>(*firstB));
      if (b_.size() < 2) {
        *this = param_type();
        return;
      }
      vector<double> w;
      for (size_t k = 0; k + 1 < b_.size(); ++k) {
        w.push_back(static_cast<double>(*firstW));
        if (k + 2 < b_.size()) // no increment past the last weight used (an input iterator reads ahead)
          ++firstW;
      }
      init(w);
    }
    template <class UnaryOperation>
    param_type(initializer_list<RealType> bl, UnaryOperation fw) {
      static_assert(is_invocable_r_v<double, UnaryOperation&, double>,
                    "piecewise_constant_distribution: fw must be callable with a double, returning a double");
      if (bl.size() < 2) {
        *this = param_type();
        return;
      }
      b_.assign(bl.begin(), bl.end());
      vector<double> w;
      for (size_t k = 0; k + 1 < b_.size(); ++k)
        w.push_back(static_cast<double>(fw((b_[k + 1] + b_[k]) / 2)));
      init(w);
    }
    template <class UnaryOperation>
    param_type(size_t nw, RealType xmin, RealType xmax, UnaryOperation fw) {
      static_assert(is_invocable_r_v<double, UnaryOperation&, double>,
                    "piecewise_constant_distribution: fw must be callable with a double, returning a double");
      const size_t n = nw == 0 ? 1 : nw;
      const RealType delta = (xmax - xmin) / static_cast<RealType>(n);
      ycxx::detail::precondition(0 < delta, "piecewise_constant_distribution: requires 0 < (xmax - xmin) / n");
      vector<double> w;
      for (size_t k = 0; k <= n; ++k)
        b_.push_back(k == n ? xmax : xmin + static_cast<RealType>(k) * delta);
      for (size_t k = 0; k < n; ++k)
        w.push_back(static_cast<double>(fw(b_[k] + delta / 2)));
      init(w);
    }

    vector<result_type> intervals() const { return b_; }
    vector<result_type> densities() const { return rho_; }
    friend bool operator==(const param_type& x, const param_type& y) { return x.b_ == y.b_ && x.rho_ == y.rho_; }

  private:
    friend piecewise_constant_distribution;
    struct exact_tag {};
    param_type(exact_tag, vector<RealType>&& b, vector<RealType>&& rho) : b_(std::move(b)), rho_(std::move(rho)) {
      build();
    }

    void init(const vector<double>& w) {
      for (size_t k = 0; k + 1 < b_.size(); ++k)
        ycxx::detail::precondition(b_[k] < b_[k + 1], "piecewise_constant_distribution: requires b_i < b_i+1");
      ycxx::detail::rand_check_weights(w, "piecewise_constant_distribution: the weights must be non-negative and "
                                          "finite, with a positive sum");
      double sum = 0;
      for (double x : w)
        sum += x;
      rho_.resize(w.size());
      for (size_t k = 0; k < w.size(); ++k)
        rho_[k] = static_cast<RealType>(w[k] / (sum * static_cast<double>(b_[k + 1] - b_[k])));
      build();
    }
    void build() {
      vector<double> mass(rho_.size());
      for (size_t k = 0; k < rho_.size(); ++k)
        mass[k] = static_cast<double>(rho_[k]) * static_cast<double>(b_[k + 1] - b_[k]);
      cdf_ = ycxx::detail::rand_cumulative(mass);
    }

    vector<RealType> b_;
    vector<RealType> rho_;
    vector<double> cdf_;
  };

  piecewise_constant_distribution() {}
  template <class InputIteratorB, class InputIteratorW>
    requires ycxx::detail::rand_input_iter<InputIteratorB>
  piecewise_constant_distribution(InputIteratorB firstB, InputIteratorB lastB, InputIteratorW firstW)
      : p_(firstB, lastB, firstW) {}
  template <class UnaryOperation>
  piecewise_constant_distribution(initializer_list<RealType> bl, UnaryOperation fw) : p_(bl, std::move(fw)) {}
  template <class UnaryOperation>
  piecewise_constant_distribution(size_t nw, RealType xmin, RealType xmax, UnaryOperation fw)
      : p_(nw, xmin, xmax, std::move(fw)) {}
  explicit piecewise_constant_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const piecewise_constant_distribution& x, const piecewise_constant_distribution& y) {
    return x.p_ == y.p_;
  }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    const size_t k = ycxx::detail::rand_find(parm.cdf_, ycxx::detail::rand_canonical<double>(g));
    const RealType lo = parm.b_[k], hi = parm.b_[k + 1];
    const RealType x = lo + (hi - lo) * ycxx::detail::rand_canonical<RealType>(g);
    return x < hi ? x : std::nextafter(hi, lo);
  }

  vector<result_type> intervals() const { return p_.intervals(); }
  vector<result_type> densities() const { return p_.densities(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return p_.b_.front(); }
  result_type max() const { return p_.b_.back(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os,
                                                  const piecewise_constant_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.p_.rho_.size());
    ycxx::detail::rand_put_seq(os, x.p_.b_);
    ycxx::detail::rand_put_seq(os, x.p_.rho_);
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is,
                                                  piecewise_constant_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    size_t n = 0;
    if (!ycxx::detail::rand_get(is, n))
      return is;
    vector<RealType> b, rho;
    for (size_t k = 0; k <= n; ++k) {
      RealType v{};
      if (!ycxx::detail::rand_get(is, v))
        return is;
      if (!b.empty() && !(b.back() < v)) {
        is.setstate(basic_istream<charT, traits>::failbit);
        return is;
      }
      b.push_back(v);
    }
    double total = 0;
    for (size_t k = 0; k < n; ++k) {
      RealType v{};
      if (!ycxx::detail::rand_get(is, v))
        return is;
      if (!(v >= 0) || !(v <= numeric_limits<RealType>::max())) {
        is.setstate(basic_istream<charT, traits>::failbit);
        return is;
      }
      total += static_cast<double>(v);
      rho.push_back(v);
    }
    if (n == 0 || !(total > 0)) {
      is.setstate(basic_istream<charT, traits>::failbit);
      return is;
    }
    x.p_ = param_type(typename param_type::exact_tag{}, std::move(b), std::move(rho));
    return is;
  }

private:
  param_type p_;
};

// ---- [rand.dist.samp.plinear] ---------------------------------------------------------------------
template <class RealType = double>
class piecewise_linear_distribution {
  static_assert(ycxx::detail::rand_real_type<RealType>,
                "piecewise_linear_distribution: RealType must be float, double or long double "
                "([rand.req.genl]/1.5)");

public:
  using result_type = RealType;
  class param_type {
  public:
    using distribution_type = piecewise_linear_distribution;
    param_type() : b_{0, 1}, rho_{1, 1}, cdf_{1.0} {}
    template <class InputIteratorB, class InputIteratorW>
      requires ycxx::detail::rand_input_iter<InputIteratorB>
    param_type(InputIteratorB firstB, InputIteratorB lastB, InputIteratorW firstW) {
      static_assert(is_convertible_v<typename iterator_traits<InputIteratorB>::value_type, double> &&
                        is_convertible_v<typename iterator_traits<InputIteratorW>::value_type, double>,
                    "piecewise_linear_distribution: the boundaries and weights must be convertible to double");
      for (; firstB != lastB; ++firstB)
        b_.push_back(static_cast<RealType>(*firstB));
      if (b_.size() < 2) {
        *this = param_type();
        return;
      }
      vector<double> w;
      for (size_t k = 0; k < b_.size(); ++k) {
        w.push_back(static_cast<double>(*firstW));
        if (k + 1 < b_.size()) // no increment past the last weight used
          ++firstW;
      }
      init(w);
    }
    template <class UnaryOperation>
    param_type(initializer_list<RealType> bl, UnaryOperation fw) {
      static_assert(is_invocable_r_v<double, UnaryOperation&, double>,
                    "piecewise_linear_distribution: fw must be callable with a double, returning a double");
      if (bl.size() < 2) {
        *this = param_type();
        return;
      }
      b_.assign(bl.begin(), bl.end());
      vector<double> w;
      for (size_t k = 0; k < b_.size(); ++k)
        w.push_back(static_cast<double>(fw(b_[k])));
      init(w);
    }
    template <class UnaryOperation>
    param_type(size_t nw, RealType xmin, RealType xmax, UnaryOperation fw) {
      static_assert(is_invocable_r_v<double, UnaryOperation&, double>,
                    "piecewise_linear_distribution: fw must be callable with a double, returning a double");
      const size_t n = nw == 0 ? 1 : nw;
      const RealType delta = (xmax - xmin) / static_cast<RealType>(n);
      ycxx::detail::precondition(0 < delta, "piecewise_linear_distribution: requires 0 < (xmax - xmin) / n");
      vector<double> w;
      for (size_t k = 0; k <= n; ++k) {
        b_.push_back(k == n ? xmax : xmin + static_cast<RealType>(k) * delta);
        w.push_back(static_cast<double>(fw(xmin + static_cast<RealType>(k) * delta)));
      }
      init(w);
    }

    vector<result_type> intervals() const { return b_; }
    vector<result_type> densities() const { return rho_; }
    friend bool operator==(const param_type& x, const param_type& y) { return x.b_ == y.b_ && x.rho_ == y.rho_; }

  private:
    friend piecewise_linear_distribution;
    struct exact_tag {};
    param_type(exact_tag, vector<RealType>&& b, vector<RealType>&& rho) : b_(std::move(b)), rho_(std::move(rho)) {
      build();
    }

    void init(const vector<double>& w) {
      for (size_t k = 0; k + 1 < b_.size(); ++k)
        ycxx::detail::precondition(b_[k] < b_[k + 1], "piecewise_linear_distribution: requires b_i < b_i+1");
      bool ok = true;
      double sum = 0;
      for (size_t k = 0; k < w.size(); ++k) {
        ok = ok && w[k] >= 0 && w[k] <= numeric_limits<double>::max();
        if (k + 1 < w.size())
          sum += (w[k] + w[k + 1]) * static_cast<double>(b_[k + 1] - b_[k]) / 2;
      }
      ycxx::detail::precondition(ok && sum > 0, "piecewise_linear_distribution: the weights must be non-negative "
                                                "and finite, with a positive integral");
      rho_.resize(w.size());
      for (size_t k = 0; k < w.size(); ++k)
        rho_[k] = static_cast<RealType>(w[k] / sum);
      build();
    }
    void build() {
      vector<double> mass(rho_.size() - 1);
      for (size_t k = 0; k + 1 < rho_.size(); ++k)
        mass[k] = (static_cast<double>(rho_[k]) + static_cast<double>(rho_[k + 1])) *
                  static_cast<double>(b_[k + 1] - b_[k]) / 2;
      cdf_ = ycxx::detail::rand_cumulative(mass);
    }

    vector<RealType> b_;
    vector<RealType> rho_;
    vector<double> cdf_;
  };

  piecewise_linear_distribution() {}
  template <class InputIteratorB, class InputIteratorW>
    requires ycxx::detail::rand_input_iter<InputIteratorB>
  piecewise_linear_distribution(InputIteratorB firstB, InputIteratorB lastB, InputIteratorW firstW)
      : p_(firstB, lastB, firstW) {}
  template <class UnaryOperation>
  piecewise_linear_distribution(initializer_list<RealType> bl, UnaryOperation fw) : p_(bl, std::move(fw)) {}
  template <class UnaryOperation>
  piecewise_linear_distribution(size_t nw, RealType xmin, RealType xmax, UnaryOperation fw)
      : p_(nw, xmin, xmax, std::move(fw)) {}
  explicit piecewise_linear_distribution(const param_type& parm) : p_(parm) {}
  void reset() {}

  friend bool operator==(const piecewise_linear_distribution& x, const piecewise_linear_distribution& y) {
    return x.p_ == y.p_;
  }

  template <class URBG>
  result_type operator()(URBG& g) {
    return (*this)(g, p_);
  }
  template <class URBG>
  result_type operator()(URBG& g, const param_type& parm) {
    const size_t k = ycxx::detail::rand_find(parm.cdf_, ycxx::detail::rand_canonical<double>(g));
    const RealType lo = parm.b_[k], hi = parm.b_[k + 1];
    const RealType r0 = parm.rho_[k], r1 = parm.rho_[k + 1];
    // Inverse of the linear density's distribution on [0, 1): F(t) = (r0 t + (r1 - r0) t^2 / 2)
    // / ((r0 + r1) / 2), solved in the cancellation-free form.
    const RealType u = ycxx::detail::rand_canonical<RealType>(g);
    RealType t;
    if (r0 == r1) {
      t = u;
    } else {
      const RealType num = u * (r0 + r1);
      const RealType den = r0 + std::sqrt(r0 * r0 + (r1 - r0) * num);
      t = den > 0 ? num / den : RealType(0);
    }
    const RealType x = lo + (hi - lo) * t;
    return x < hi ? x : std::nextafter(hi, lo);
  }

  vector<result_type> intervals() const { return p_.intervals(); }
  vector<result_type> densities() const { return p_.densities(); }
  param_type param() const { return p_; }
  void param(const param_type& parm) { p_ = parm; }
  result_type min() const { return p_.b_.front(); }
  result_type max() const { return p_.b_.back(); }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os,
                                                  const piecewise_linear_distribution& x) {
    auto st = ycxx::detail::rand_out_dist(os);
    ycxx::detail::rand_put(os, x.p_.b_.size() - 1);
    ycxx::detail::rand_put_seq(os, x.p_.b_);
    ycxx::detail::rand_put_seq(os, x.p_.rho_);
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, piecewise_linear_distribution& x) {
    auto st = ycxx::detail::rand_in(is);
    size_t n = 0;
    if (!ycxx::detail::rand_get(is, n))
      return is;
    vector<RealType> b, rho;
    for (size_t k = 0; k <= n; ++k) {
      RealType v{};
      if (!ycxx::detail::rand_get(is, v))
        return is;
      if (!b.empty() && !(b.back() < v)) {
        is.setstate(basic_istream<charT, traits>::failbit);
        return is;
      }
      b.push_back(v);
    }
    double total = 0;
    for (size_t k = 0; k <= n; ++k) {
      RealType v{};
      if (!ycxx::detail::rand_get(is, v))
        return is;
      if (!(v >= 0) || !(v <= numeric_limits<RealType>::max())) {
        is.setstate(basic_istream<charT, traits>::failbit);
        return is;
      }
      total += static_cast<double>(v);
      rho.push_back(v);
    }
    if (n == 0 || !(total > 0)) {
      is.setstate(basic_istream<charT, traits>::failbit);
      return is;
    }
    x.p_ = param_type(typename param_type::exact_tag{}, std::move(b), std::move(rho));
    return is;
  }

private:
  param_type p_;
};

} // namespace std
