// libycxx core: <valarray> ([numarray]).
//
// valarray<T> owns a heap array obtained from std::allocator<T> (operator new); its iterators are
// plain pointers (contiguous, [valarray.range]). The operators and functions evaluate eagerly and
// return valarray<T> itself (no expression templates; [valarray.syn]/3 permits but does not
// require replacement types). The subset classes (slice_array, gslice_array, mask_array,
// indirect_array) refer to the elements of the valarray they were obtained from: a slice by its
// start, length and stride, the others by a precomputed list of element indices.
// Element access checks its hardened precondition (n < size()) when YCXX_HARDENED=1.
#pragma once

#include <initializer_list>
#include <ycxx/config.hpp>
#include <ycxx/core/cmath.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/uninitialized.hpp>

namespace [[gnu::visibility("hidden")]] std {
template <class T>
class valarray;
class slice;
template <class T>
class slice_array;
class gslice;
template <class T>
class gslice_array;
template <class T>
class mask_array;
template <class T>
class indirect_array;
} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// The operations, as function objects (the result is converted to T, or to bool for the
// comparisons, by the caller).
namespace va_op {
struct plus {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a + b) {
    return a + b;
  }
};
struct minus {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a - b) {
    return a - b;
  }
};
struct mul {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a * b) {
    return a * b;
  }
};
struct div {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a / b) {
    return a / b;
  }
};
struct mod {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a % b) {
    return a % b;
  }
};
struct bxor {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a ^ b) {
    return a ^ b;
  }
};
struct band {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a & b) {
    return a & b;
  }
};
struct bor {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a | b) {
    return a | b;
  }
};
struct shl {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a << b) {
    return a << b;
  }
};
struct shr {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a >> b) {
    return a >> b;
  }
};
struct land {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a && b) {
    return a && b;
  }
};
struct lor {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a || b) {
    return a || b;
  }
};
struct eq {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a == b) {
    return a == b;
  }
};
struct ne {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a != b) {
    return a != b;
  }
};
struct lt {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a < b) {
    return a < b;
  }
};
struct gt {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a > b) {
    return a > b;
  }
};
struct le {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a <= b) {
    return a <= b;
  }
};
struct ge {
  template <class A, class B>
  constexpr auto operator()(const A& a, const B& b) const -> decltype(a >= b) {
    return a >= b;
  }
};
// Compound assignments.
struct plus_assign {
  template <class A, class B>
  constexpr void operator()(A& a, const B& b) const {
    a += b;
  }
};
struct minus_assign {
  template <class A, class B>
  constexpr void operator()(A& a, const B& b) const {
    a -= b;
  }
};
struct mul_assign {
  template <class A, class B>
  constexpr void operator()(A& a, const B& b) const {
    a *= b;
  }
};
struct div_assign {
  template <class A, class B>
  constexpr void operator()(A& a, const B& b) const {
    a /= b;
  }
};
struct mod_assign {
  template <class A, class B>
  constexpr void operator()(A& a, const B& b) const {
    a %= b;
  }
};
struct xor_assign {
  template <class A, class B>
  constexpr void operator()(A& a, const B& b) const {
    a ^= b;
  }
};
struct and_assign {
  template <class A, class B>
  constexpr void operator()(A& a, const B& b) const {
    a &= b;
  }
};
struct or_assign {
  template <class A, class B>
  constexpr void operator()(A& a, const B& b) const {
    a |= b;
  }
};
struct shl_assign {
  template <class A, class B>
  constexpr void operator()(A& a, const B& b) const {
    a <<= b;
  }
};
struct shr_assign {
  template <class A, class B>
  constexpr void operator()(A& a, const B& b) const {
    a >>= b;
  }
};
struct assign {
  template <class A, class B>
  constexpr void operator()(A& a, const B& b) const {
    a = b;
  }
};
} // namespace va_op

// [valarray.transcend]/1: "A unique function with the indicated name can be applied
// (unqualified) to an operand of type T": the calls below are unqualified on purpose, with the
// std:: functions visible.
namespace va_math {
using std::abs;
using std::acos;
using std::asin;
using std::atan;
using std::atan2;
using std::cos;
using std::cosh;
using std::exp;
using std::log;
using std::log10;
using std::pow;
using std::sin;
using std::sinh;
using std::sqrt;
using std::tan;
using std::tanh;
struct f_abs {
  template <class X>
  auto operator()(const X& x) const -> decltype(abs(x)) {
    return abs(x);
  }
};
struct f_acos {
  template <class X>
  auto operator()(const X& x) const -> decltype(acos(x)) {
    return acos(x);
  }
};
struct f_asin {
  template <class X>
  auto operator()(const X& x) const -> decltype(asin(x)) {
    return asin(x);
  }
};
struct f_atan {
  template <class X>
  auto operator()(const X& x) const -> decltype(atan(x)) {
    return atan(x);
  }
};
struct f_atan2 {
  template <class X>
  auto operator()(const X& x, const X& y) const -> decltype(atan2(x, y)) {
    return atan2(x, y);
  }
};
struct f_cos {
  template <class X>
  auto operator()(const X& x) const -> decltype(cos(x)) {
    return cos(x);
  }
};
struct f_cosh {
  template <class X>
  auto operator()(const X& x) const -> decltype(cosh(x)) {
    return cosh(x);
  }
};
struct f_exp {
  template <class X>
  auto operator()(const X& x) const -> decltype(exp(x)) {
    return exp(x);
  }
};
struct f_log {
  template <class X>
  auto operator()(const X& x) const -> decltype(log(x)) {
    return log(x);
  }
};
struct f_log10 {
  template <class X>
  auto operator()(const X& x) const -> decltype(log10(x)) {
    return log10(x);
  }
};
struct f_pow {
  template <class X>
  auto operator()(const X& x, const X& y) const -> decltype(pow(x, y)) {
    return pow(x, y);
  }
};
struct f_sin {
  template <class X>
  auto operator()(const X& x) const -> decltype(sin(x)) {
    return sin(x);
  }
};
struct f_sinh {
  template <class X>
  auto operator()(const X& x) const -> decltype(sinh(x)) {
    return sinh(x);
  }
};
struct f_sqrt {
  template <class X>
  auto operator()(const X& x) const -> decltype(sqrt(x)) {
    return sqrt(x);
  }
};
struct f_tan {
  template <class X>
  auto operator()(const X& x) const -> decltype(tan(x)) {
    return tan(x);
  }
};
struct f_tanh {
  template <class X>
  auto operator()(const X& x) const -> decltype(tanh(x)) {
    return tanh(x);
  }
};
} // namespace va_math

// Builds valarray<R> of n elements, element i initialised with f(i).
struct va_access;

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// ---- slice ([class.slice]) ---------------------------------------------------------------------
class slice {
public:
  slice() : start_(0), size_(0), stride_(0) {}
  slice(size_t start, size_t length, size_t stride) : start_(start), size_(length), stride_(stride) {}
  slice(const slice&) = default;
  size_t start() const { return start_; }
  size_t size() const { return size_; }
  size_t stride() const { return stride_; }
  friend bool operator==(const slice& x, const slice& y) {
    return x.start() == y.start() && x.size() == y.size() && x.stride() == y.stride();
  }

private:
  size_t start_, size_, stride_;
};

// ---- valarray ([template.valarray]) --------------------------------------------------------------
template <class T>
class valarray {
public:
  using value_type = T;
  using iterator = T*;
  using const_iterator = const T*;

  // [valarray.cons]
  valarray() noexcept : data_(nullptr), size_(0) {}
  explicit valarray(size_t n) : data_(nullptr), size_(0) {
    build(n, [](T* p, size_t k) { std::uninitialized_value_construct_n(p, k); });
  }
  valarray(const T& v, size_t n) : data_(nullptr), size_(0) {
    build(n, [&v](T* p, size_t k) { std::uninitialized_fill_n(p, k, v); });
  }
  valarray(const T* p, size_t n) : data_(nullptr), size_(0) {
    build(n, [p](T* d, size_t k) { std::uninitialized_copy_n(p, k, d); });
  }
  valarray(const valarray& v) : valarray(v.data_, v.size_) {}
  valarray(valarray&& v) noexcept : data_(v.data_), size_(v.size_) {
    v.data_ = nullptr;
    v.size_ = 0;
  }
  valarray(const slice_array<T>& s);
  valarray(const gslice_array<T>& s);
  valarray(const mask_array<T>& s);
  valarray(const indirect_array<T>& s);
  valarray(initializer_list<T> il) : valarray(il.begin(), il.size()) {}
  ~valarray() { release(); }

  // [valarray.assign]
  valarray& operator=(const valarray& v) {
    if (this == __builtin_addressof(v)) return *this;
    if (size_ == v.size_) {
      for (size_t i = 0; i < size_; ++i) data_[i] = v.data_[i];
    } else {
      valarray tmp(v);
      swap(tmp);
    }
    return *this;
  }
  valarray& operator=(valarray&& v) noexcept {
    if (this != __builtin_addressof(v)) {
      release();
      data_ = v.data_;
      size_ = v.size_;
      v.data_ = nullptr;
      v.size_ = 0;
    }
    return *this;
  }
  valarray& operator=(initializer_list<T> il) { return *this = valarray(il); }
  valarray& operator=(const T& v) {
    for (size_t i = 0; i < size_; ++i) data_[i] = v;
    return *this;
  }
  valarray& operator=(const slice_array<T>& s);
  valarray& operator=(const gslice_array<T>& s);
  valarray& operator=(const mask_array<T>& s);
  valarray& operator=(const indirect_array<T>& s);

  // [valarray.access]
  const T& operator[](size_t n) const {
    ycxx::detail::precondition(n < size_, "std::valarray::operator[]: index out of range");
    return data_[n];
  }
  T& operator[](size_t n) {
    ycxx::detail::precondition(n < size_, "std::valarray::operator[]: index out of range");
    return data_[n];
  }

  // [valarray.sub]
  valarray operator[](slice s) const;
  slice_array<T> operator[](slice s);
  valarray operator[](const gslice& g) const;
  gslice_array<T> operator[](const gslice& g);
  valarray operator[](const valarray<bool>& mask) const;
  mask_array<T> operator[](const valarray<bool>& mask);
  valarray operator[](const valarray<size_t>& ind) const;
  indirect_array<T> operator[](const valarray<size_t>& ind);

  // [valarray.unary]
  valarray operator+() const {
    return map([](const T& x) -> T { return +x; });
  }
  valarray operator-() const {
    return map([](const T& x) -> T { return -x; });
  }
  valarray operator~() const {
    return map([](const T& x) -> T { return ~x; });
  }
  valarray<bool> operator!() const;

  // [valarray.cassign]
  valarray& operator*=(const T& v) { return update(v, ycxx::detail::va_op::mul_assign{}); }
  valarray& operator/=(const T& v) { return update(v, ycxx::detail::va_op::div_assign{}); }
  valarray& operator%=(const T& v) { return update(v, ycxx::detail::va_op::mod_assign{}); }
  valarray& operator+=(const T& v) { return update(v, ycxx::detail::va_op::plus_assign{}); }
  valarray& operator-=(const T& v) { return update(v, ycxx::detail::va_op::minus_assign{}); }
  valarray& operator^=(const T& v) { return update(v, ycxx::detail::va_op::xor_assign{}); }
  valarray& operator&=(const T& v) { return update(v, ycxx::detail::va_op::and_assign{}); }
  valarray& operator|=(const T& v) { return update(v, ycxx::detail::va_op::or_assign{}); }
  valarray& operator<<=(const T& v) { return update(v, ycxx::detail::va_op::shl_assign{}); }
  valarray& operator>>=(const T& v) { return update(v, ycxx::detail::va_op::shr_assign{}); }
  valarray& operator*=(const valarray& v) { return update(v, ycxx::detail::va_op::mul_assign{}); }
  valarray& operator/=(const valarray& v) { return update(v, ycxx::detail::va_op::div_assign{}); }
  valarray& operator%=(const valarray& v) { return update(v, ycxx::detail::va_op::mod_assign{}); }
  valarray& operator+=(const valarray& v) { return update(v, ycxx::detail::va_op::plus_assign{}); }
  valarray& operator-=(const valarray& v) { return update(v, ycxx::detail::va_op::minus_assign{}); }
  valarray& operator^=(const valarray& v) { return update(v, ycxx::detail::va_op::xor_assign{}); }
  valarray& operator|=(const valarray& v) { return update(v, ycxx::detail::va_op::or_assign{}); }
  valarray& operator&=(const valarray& v) { return update(v, ycxx::detail::va_op::and_assign{}); }
  valarray& operator<<=(const valarray& v) { return update(v, ycxx::detail::va_op::shl_assign{}); }
  valarray& operator>>=(const valarray& v) { return update(v, ycxx::detail::va_op::shr_assign{}); }

  // [valarray.range]
  iterator begin() noexcept { return data_; }
  iterator end() noexcept { return data_ + size_; }
  const_iterator begin() const noexcept { return data_; }
  const_iterator end() const noexcept { return data_ + size_; }

  // [valarray.members]
  void swap(valarray& v) noexcept {
    T* d = data_;
    data_ = v.data_;
    v.data_ = d;
    const size_t s = size_;
    size_ = v.size_;
    v.size_ = s;
  }
  size_t size() const noexcept { return size_; }
  T sum() const {
    ycxx::detail::precondition(size_ > 0, "std::valarray::sum: empty array");
    T r = data_[0];
    for (size_t i = 1; i < size_; ++i) r += data_[i];
    return r;
  }
  T min() const {
    ycxx::detail::precondition(size_ > 0, "std::valarray::min: empty array");
    size_t k = 0;
    for (size_t i = 1; i < size_; ++i)
      if (data_[i] < data_[k]) k = i;
    return data_[k];
  }
  T max() const {
    ycxx::detail::precondition(size_ > 0, "std::valarray::max: empty array");
    size_t k = 0;
    for (size_t i = 1; i < size_; ++i)
      if (data_[k] < data_[i]) k = i;
    return data_[k];
  }
  valarray shift(int n) const {
    valarray r(size_);
    for (size_t i = 0; i < size_; ++i) {
      const long long j = static_cast<long long>(i) + n;
      if (j >= 0 && static_cast<unsigned long long>(j) < size_) r.data_[i] = data_[j];
    }
    return r;
  }
  valarray cshift(int n) const {
    if (size_ == 0) return valarray();
    const long long sz = static_cast<long long>(size_);
    long long k = n % sz;
    if (k < 0) k += sz;
    const size_t first = static_cast<size_t>(k);
    return generate(size_, [this, first](size_t i) -> const T& {
      return data_[i < size_ - first ? first + i : i - (size_ - first)];
    });
  }
  valarray apply(T func(T)) const {
    return map([func](const T& x) -> T { return func(x); });
  }
  valarray apply(T func(const T&)) const {
    return map([func](const T& x) -> T { return func(x); });
  }
  void resize(size_t sz, T c = T()) {
    if (sz == size_) {
      for (size_t i = 0; i < size_; ++i) data_[i] = c;
      return;
    }
    valarray tmp(c, sz);
    swap(tmp);
  }

private:
  template <class U>
  friend class valarray;
  friend struct ycxx::detail::va_access;

  // A deallocation guard for a partially built array (exception safety without try).
  struct storage {
    T* p;
    size_t n;
    ~storage() {
      if (p) std::allocator<T>().deallocate(p, n);
    }
  };
  // Allocates n elements and lets init construct all of them (init either constructs every
  // element or destroys what it constructed and throws, as the uninitialized algorithms do).
  template <class Init>
  void build(size_t n, Init init) {
    if (n == 0) return;
    storage s{std::allocator<T>().allocate(n), n};
    init(s.p, n);
    data_ = s.p;
    size_ = n;
    s.p = nullptr;
  }
  void release() noexcept {
    if (data_) {
      std::destroy_n(data_, size_);
      std::allocator<T>().deallocate(data_, size_);
      data_ = nullptr;
      size_ = 0;
    }
  }
  // Builds an array of n elements, element i initialised with f(i), converted to T.
  template <class F>
  static valarray generate(size_t n, F f) {
    valarray r;
    r.build(n, [&f](T* p, size_t cnt) {
      size_t i = 0;
      struct undo {
        T* p;
        size_t* i;
        bool done;
        ~undo() {
          if (!done) std::destroy_n(p, *i);
        }
      } u{p, &i, false};
      for (; i < cnt; ++i) ::new (static_cast<void*>(p + i)) T(f(i));
      u.done = true;
    });
    return r;
  }
  template <class F>
  valarray map(F f) const {
    return generate(size_, [this, &f](size_t i) { return f(data_[i]); });
  }
  template <class Op>
  valarray& update(const T& v, Op op) {
    for (size_t i = 0; i < size_; ++i) op(data_[i], v);
    return *this;
  }
  template <class Op>
  valarray& update(const valarray& v, Op op) {
    ycxx::detail::precondition(size_ == v.size_, "std::valarray compound assignment: sizes differ");
    if (this == __builtin_addressof(v)) {
      const valarray copy(v);
      for (size_t i = 0; i < size_; ++i) op(data_[i], copy.data_[i]);
    } else {
      for (size_t i = 0; i < size_; ++i) op(data_[i], v.data_[i]);
    }
    return *this;
  }

  T* data_;
  size_t size_;
};

template <class T, size_t cnt>
valarray(const T (&)[cnt], size_t) -> valarray<T>;

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

struct va_access {
  template <class R, class F>
  static std::valarray<R> generate(std::size_t n, F f) {
    return std::valarray<R>::generate(n, f);
  }
  template <class T>
  static T* data(const std::valarray<T>& v) {
    return v.data_;
  }
};

// The element indices a gslice selects, the highest-ordered index turning fastest.
inline std::valarray<std::size_t> gslice_indices(std::size_t start, const std::valarray<std::size_t>& len,
                                                 const std::valarray<std::size_t>& str) {
  const std::size_t dims = len.size() < str.size() ? len.size() : str.size();
  std::size_t total = dims == 0 ? 0 : 1;
  for (std::size_t j = 0; j < dims; ++j) total *= len[j];
  std::valarray<std::size_t> idx(total);
  if (total == 0) return idx;
  std::valarray<std::size_t> pos(dims);
  for (std::size_t k = 0; k < total; ++k) {
    std::size_t off = start;
    for (std::size_t j = 0; j < dims; ++j) off += pos[j] * str[j];
    idx[k] = off;
    for (std::size_t j = dims; j-- > 0;) { // odometer: the last index turns fastest
      if (++pos[j] < len[j]) break;
      pos[j] = 0;
    }
  }
  return idx;
}

// The indices of the true elements of a mask.
inline std::valarray<std::size_t> va_mask_indices(const std::valarray<bool>& mask) {
  std::size_t n = 0;
  for (std::size_t i = 0; i < mask.size(); ++i) n += mask[i] ? 1 : 0;
  std::valarray<std::size_t> idx(n);
  for (std::size_t i = 0, k = 0; i < mask.size(); ++i)
    if (mask[i]) idx[k++] = i;
  return idx;
}

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// ---- gslice ([class.gslice]) ---------------------------------------------------------------------
class gslice {
public:
  gslice() : start_(0) {}
  gslice(size_t s, const valarray<size_t>& l, const valarray<size_t>& d) : start_(s), size_(l), stride_(d) {}
  size_t start() const { return start_; }
  valarray<size_t> size() const { return size_; }
  valarray<size_t> stride() const { return stride_; }

private:
  template <class T>
  friend class valarray;
  valarray<size_t> indices() const { return ycxx::detail::gslice_indices(start_, size_, stride_); }

  size_t start_;
  valarray<size_t> size_;
  valarray<size_t> stride_;
};

// ---- the subset classes --------------------------------------------------------------------------
// slice_array refers to base[start + i * stride]; the others to base[idx[i]].
template <class T>
class slice_array {
public:
  using value_type = T;
  void operator=(const valarray<T>& v) const { apply(v, ycxx::detail::va_op::assign{}); }
  void operator*=(const valarray<T>& v) const { apply(v, ycxx::detail::va_op::mul_assign{}); }
  void operator/=(const valarray<T>& v) const { apply(v, ycxx::detail::va_op::div_assign{}); }
  void operator%=(const valarray<T>& v) const { apply(v, ycxx::detail::va_op::mod_assign{}); }
  void operator+=(const valarray<T>& v) const { apply(v, ycxx::detail::va_op::plus_assign{}); }
  void operator-=(const valarray<T>& v) const { apply(v, ycxx::detail::va_op::minus_assign{}); }
  void operator^=(const valarray<T>& v) const { apply(v, ycxx::detail::va_op::xor_assign{}); }
  void operator&=(const valarray<T>& v) const { apply(v, ycxx::detail::va_op::and_assign{}); }
  void operator|=(const valarray<T>& v) const { apply(v, ycxx::detail::va_op::or_assign{}); }
  void operator<<=(const valarray<T>& v) const { apply(v, ycxx::detail::va_op::shl_assign{}); }
  void operator>>=(const valarray<T>& v) const { apply(v, ycxx::detail::va_op::shr_assign{}); }
  slice_array(const slice_array&) = default;
  ~slice_array() = default;
  const slice_array& operator=(const slice_array& s) const {
    for (size_t i = 0; i < size_; ++i) at(i) = s.at(i);
    return *this;
  }
  void operator=(const T& v) const {
    for (size_t i = 0; i < size_; ++i) at(i) = v;
  }
  slice_array() = delete;

private:
  template <class U>
  friend class valarray;
  slice_array(T* base, const slice& s) : base_(base), start_(s.start()), size_(s.size()), stride_(s.stride()) {}
  T& at(size_t i) const { return base_[start_ + i * stride_]; }
  template <class Op>
  void apply(const valarray<T>& v, Op op) const {
    for (size_t i = 0; i < size_; ++i) op(at(i), v[i]);
  }

  T* base_;
  size_t start_, size_, stride_;
};

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// The common part of gslice_array, mask_array and indirect_array: base[idx[i]].
template <class T>
class valarray_indexed {
public:
  using value_type = T;
  void operator*=(const std::valarray<T>& v) const { apply(v, ycxx::detail::va_op::mul_assign{}); }
  void operator/=(const std::valarray<T>& v) const { apply(v, ycxx::detail::va_op::div_assign{}); }
  void operator%=(const std::valarray<T>& v) const { apply(v, ycxx::detail::va_op::mod_assign{}); }
  void operator+=(const std::valarray<T>& v) const { apply(v, ycxx::detail::va_op::plus_assign{}); }
  void operator-=(const std::valarray<T>& v) const { apply(v, ycxx::detail::va_op::minus_assign{}); }
  void operator^=(const std::valarray<T>& v) const { apply(v, ycxx::detail::va_op::xor_assign{}); }
  void operator&=(const std::valarray<T>& v) const { apply(v, ycxx::detail::va_op::and_assign{}); }
  void operator|=(const std::valarray<T>& v) const { apply(v, ycxx::detail::va_op::or_assign{}); }
  void operator<<=(const std::valarray<T>& v) const { apply(v, ycxx::detail::va_op::shl_assign{}); }
  void operator>>=(const std::valarray<T>& v) const { apply(v, ycxx::detail::va_op::shr_assign{}); }

protected:
  valarray_indexed(T* base, std::valarray<std::size_t>&& idx)
      : base_(base), idx_(static_cast<std::valarray<std::size_t>&&>(idx)) {}
  valarray_indexed(const valarray_indexed&) = default;
  ~valarray_indexed() = default;
  valarray_indexed& operator=(const valarray_indexed&) = delete;

  template <class Op>
  void apply(const std::valarray<T>& v, Op op) const {
    for (std::size_t i = 0; i < idx_.size(); ++i) op(base_[idx_[i]], v[i]);
  }
  void fill(const T& v) const {
    for (std::size_t i = 0; i < idx_.size(); ++i) base_[idx_[i]] = v;
  }
  void copy_from(const valarray_indexed& s) const {
    for (std::size_t i = 0; i < idx_.size(); ++i) base_[idx_[i]] = s.base_[s.idx_[i]];
  }

  template <class U>
  friend class std::valarray;

  T* base_;
  std::valarray<std::size_t> idx_;
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {

template <class T>
class gslice_array : public ycxx::adl_free::valarray_indexed<T> {
  using base = ycxx::adl_free::valarray_indexed<T>;

public:
  using value_type = T;
  void operator=(const valarray<T>& v) const { this->apply(v, ycxx::detail::va_op::assign{}); }
  gslice_array(const gslice_array&) = default;
  ~gslice_array() = default;
  const gslice_array& operator=(const gslice_array& s) const {
    this->copy_from(s);
    return *this;
  }
  void operator=(const T& v) const { this->fill(v); }
  gslice_array() = delete;

private:
  template <class U>
  friend class valarray;
  gslice_array(T* p, valarray<size_t>&& idx) : base(p, static_cast<valarray<size_t>&&>(idx)) {}
};

template <class T>
class mask_array : public ycxx::adl_free::valarray_indexed<T> {
  using base = ycxx::adl_free::valarray_indexed<T>;

public:
  using value_type = T;
  void operator=(const valarray<T>& v) const { this->apply(v, ycxx::detail::va_op::assign{}); }
  mask_array(const mask_array&) = default;
  ~mask_array() = default;
  const mask_array& operator=(const mask_array& s) const {
    this->copy_from(s);
    return *this;
  }
  void operator=(const T& v) const { this->fill(v); }
  mask_array() = delete;

private:
  template <class U>
  friend class valarray;
  mask_array(T* p, valarray<size_t>&& idx) : base(p, static_cast<valarray<size_t>&&>(idx)) {}
};

template <class T>
class indirect_array : public ycxx::adl_free::valarray_indexed<T> {
  using base = ycxx::adl_free::valarray_indexed<T>;

public:
  using value_type = T;
  void operator=(const valarray<T>& v) const { this->apply(v, ycxx::detail::va_op::assign{}); }
  indirect_array(const indirect_array&) = default;
  ~indirect_array() = default;
  const indirect_array& operator=(const indirect_array& s) const {
    this->copy_from(s);
    return *this;
  }
  void operator=(const T& v) const { this->fill(v); }
  indirect_array() = delete;

private:
  template <class U>
  friend class valarray;
  indirect_array(T* p, valarray<size_t>&& idx) : base(p, static_cast<valarray<size_t>&&>(idx)) {}
};

// ---- valarray members that need the subset classes -----------------------------------------------
template <class T>
valarray<T>::valarray(const slice_array<T>& s) : data_(nullptr), size_(0) {
  *this = generate(s.size_, [&s](size_t i) -> const T& { return s.at(i); });
}
template <class T>
valarray<T>::valarray(const gslice_array<T>& s) : data_(nullptr), size_(0) {
  *this = generate(s.idx_.size(), [&s](size_t i) -> const T& { return s.base_[s.idx_[i]]; });
}
template <class T>
valarray<T>::valarray(const mask_array<T>& s) : data_(nullptr), size_(0) {
  *this = generate(s.idx_.size(), [&s](size_t i) -> const T& { return s.base_[s.idx_[i]]; });
}
template <class T>
valarray<T>::valarray(const indirect_array<T>& s) : data_(nullptr), size_(0) {
  *this = generate(s.idx_.size(), [&s](size_t i) -> const T& { return s.base_[s.idx_[i]]; });
}
template <class T>
valarray<T>& valarray<T>::operator=(const slice_array<T>& s) {
  ycxx::detail::precondition(s.size_ == size_, "std::valarray::operator=: the slice_array length differs");
  for (size_t i = 0; i < size_; ++i) data_[i] = s.at(i);
  return *this;
}
template <class T>
valarray<T>& valarray<T>::operator=(const gslice_array<T>& s) {
  ycxx::detail::precondition(s.idx_.size() == size_, "std::valarray::operator=: the gslice_array length differs");
  for (size_t i = 0; i < size_; ++i) data_[i] = s.base_[s.idx_[i]];
  return *this;
}
template <class T>
valarray<T>& valarray<T>::operator=(const mask_array<T>& s) {
  ycxx::detail::precondition(s.idx_.size() == size_, "std::valarray::operator=: the mask_array length differs");
  for (size_t i = 0; i < size_; ++i) data_[i] = s.base_[s.idx_[i]];
  return *this;
}
template <class T>
valarray<T>& valarray<T>::operator=(const indirect_array<T>& s) {
  ycxx::detail::precondition(s.idx_.size() == size_, "std::valarray::operator=: the indirect_array length differs");
  for (size_t i = 0; i < size_; ++i) data_[i] = s.base_[s.idx_[i]];
  return *this;
}

template <class T>
valarray<T> valarray<T>::operator[](slice s) const {
  return generate(s.size(), [this, &s](size_t i) -> const T& { return data_[s.start() + i * s.stride()]; });
}
template <class T>
slice_array<T> valarray<T>::operator[](slice s) {
  return slice_array<T>(data_, s);
}
template <class T>
valarray<T> valarray<T>::operator[](const gslice& g) const {
  const valarray<size_t> idx = g.indices();
  return generate(idx.size(), [this, &idx](size_t i) -> const T& { return data_[idx[i]]; });
}
template <class T>
gslice_array<T> valarray<T>::operator[](const gslice& g) {
  return gslice_array<T>(data_, g.indices());
}
template <class T>
valarray<T> valarray<T>::operator[](const valarray<bool>& mask) const {
  const valarray<size_t> idx = ycxx::detail::va_mask_indices(mask);
  return generate(idx.size(), [this, &idx](size_t i) -> const T& { return data_[idx[i]]; });
}
template <class T>
mask_array<T> valarray<T>::operator[](const valarray<bool>& mask) {
  return mask_array<T>(data_, ycxx::detail::va_mask_indices(mask));
}
template <class T>
valarray<T> valarray<T>::operator[](const valarray<size_t>& ind) const {
  return generate(ind.size(), [this, &ind](size_t i) -> const T& { return data_[ind[i]]; });
}
template <class T>
indirect_array<T> valarray<T>::operator[](const valarray<size_t>& ind) {
  return indirect_array<T>(data_, valarray<size_t>(ind));
}
template <class T>
valarray<bool> valarray<T>::operator!() const {
  return valarray<bool>::generate(size_, [this](size_t i) -> bool { return !data_[i]; });
}

// ---- [valarray.special] ----------------------------------------------------------------------------
template <class T>
void swap(valarray<T>& x, valarray<T>& y) noexcept {
  x.swap(y);
}

} // namespace std

// ---- [valarray.binary], [valarray.comparison], [valarray.transcend] ----------------------------------
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
template <class R, class T, class Op>
std::valarray<R> va_binary(const std::valarray<T>& x, const std::valarray<T>& y, Op op) {
  ycxx::detail::precondition(x.size() == y.size(), "std::valarray binary operator: sizes differ");
  const T* a = ycxx::detail::va_access::data(x);
  const T* b = ycxx::detail::va_access::data(y);
  return ycxx::detail::va_access::generate<R>(x.size(), [a, b, &op](std::size_t i) -> R { return op(a[i], b[i]); });
}
template <class R, class T, class Op>
std::valarray<R> va_binary_left(const std::valarray<T>& x, const T& y, Op op) {
  const T* a = ycxx::detail::va_access::data(x);
  return ycxx::detail::va_access::generate<R>(x.size(), [a, &y, &op](std::size_t i) -> R { return op(a[i], y); });
}
template <class R, class T, class Op>
std::valarray<R> va_binary_right(const T& x, const std::valarray<T>& y, Op op) {
  const T* b = ycxx::detail::va_access::data(y);
  return ycxx::detail::va_access::generate<R>(y.size(), [b, &x, &op](std::size_t i) -> R { return op(x, b[i]); });
}
template <class T, class F>
std::valarray<T> va_map(const std::valarray<T>& x, F f) {
  const T* a = ycxx::detail::va_access::data(x);
  return ycxx::detail::va_access::generate<T>(x.size(), [a, &f](std::size_t i) -> T { return f(a[i]); });
}
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

template <class T>
valarray<T> operator*(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<T>(x, y, ycxx::detail::va_op::mul{});
}
template <class T>
valarray<T> operator*(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<T>(x, y, ycxx::detail::va_op::mul{});
}
template <class T>
valarray<T> operator*(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<T>(x, y, ycxx::detail::va_op::mul{});
}
template <class T>
valarray<T> operator/(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<T>(x, y, ycxx::detail::va_op::div{});
}
template <class T>
valarray<T> operator/(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<T>(x, y, ycxx::detail::va_op::div{});
}
template <class T>
valarray<T> operator/(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<T>(x, y, ycxx::detail::va_op::div{});
}
template <class T>
valarray<T> operator%(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<T>(x, y, ycxx::detail::va_op::mod{});
}
template <class T>
valarray<T> operator%(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<T>(x, y, ycxx::detail::va_op::mod{});
}
template <class T>
valarray<T> operator%(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<T>(x, y, ycxx::detail::va_op::mod{});
}
template <class T>
valarray<T> operator+(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<T>(x, y, ycxx::detail::va_op::plus{});
}
template <class T>
valarray<T> operator+(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<T>(x, y, ycxx::detail::va_op::plus{});
}
template <class T>
valarray<T> operator+(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<T>(x, y, ycxx::detail::va_op::plus{});
}
template <class T>
valarray<T> operator-(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<T>(x, y, ycxx::detail::va_op::minus{});
}
template <class T>
valarray<T> operator-(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<T>(x, y, ycxx::detail::va_op::minus{});
}
template <class T>
valarray<T> operator-(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<T>(x, y, ycxx::detail::va_op::minus{});
}
template <class T>
valarray<T> operator^(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<T>(x, y, ycxx::detail::va_op::bxor{});
}
template <class T>
valarray<T> operator^(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<T>(x, y, ycxx::detail::va_op::bxor{});
}
template <class T>
valarray<T> operator^(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<T>(x, y, ycxx::detail::va_op::bxor{});
}
template <class T>
valarray<T> operator&(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<T>(x, y, ycxx::detail::va_op::band{});
}
template <class T>
valarray<T> operator&(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<T>(x, y, ycxx::detail::va_op::band{});
}
template <class T>
valarray<T> operator&(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<T>(x, y, ycxx::detail::va_op::band{});
}
template <class T>
valarray<T> operator|(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<T>(x, y, ycxx::detail::va_op::bor{});
}
template <class T>
valarray<T> operator|(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<T>(x, y, ycxx::detail::va_op::bor{});
}
template <class T>
valarray<T> operator|(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<T>(x, y, ycxx::detail::va_op::bor{});
}
template <class T>
valarray<T> operator<<(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<T>(x, y, ycxx::detail::va_op::shl{});
}
template <class T>
valarray<T> operator<<(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<T>(x, y, ycxx::detail::va_op::shl{});
}
template <class T>
valarray<T> operator<<(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<T>(x, y, ycxx::detail::va_op::shl{});
}
template <class T>
valarray<T> operator>>(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<T>(x, y, ycxx::detail::va_op::shr{});
}
template <class T>
valarray<T> operator>>(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<T>(x, y, ycxx::detail::va_op::shr{});
}
template <class T>
valarray<T> operator>>(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<T>(x, y, ycxx::detail::va_op::shr{});
}
template <class T>
valarray<bool> operator&&(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<bool>(x, y, ycxx::detail::va_op::land{});
}
template <class T>
valarray<bool> operator&&(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<bool>(x, y, ycxx::detail::va_op::land{});
}
template <class T>
valarray<bool> operator&&(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<bool>(x, y, ycxx::detail::va_op::land{});
}
template <class T>
valarray<bool> operator||(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<bool>(x, y, ycxx::detail::va_op::lor{});
}
template <class T>
valarray<bool> operator||(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<bool>(x, y, ycxx::detail::va_op::lor{});
}
template <class T>
valarray<bool> operator||(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<bool>(x, y, ycxx::detail::va_op::lor{});
}
template <class T>
valarray<bool> operator==(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<bool>(x, y, ycxx::detail::va_op::eq{});
}
template <class T>
valarray<bool> operator==(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<bool>(x, y, ycxx::detail::va_op::eq{});
}
template <class T>
valarray<bool> operator==(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<bool>(x, y, ycxx::detail::va_op::eq{});
}
template <class T>
valarray<bool> operator!=(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<bool>(x, y, ycxx::detail::va_op::ne{});
}
template <class T>
valarray<bool> operator!=(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<bool>(x, y, ycxx::detail::va_op::ne{});
}
template <class T>
valarray<bool> operator!=(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<bool>(x, y, ycxx::detail::va_op::ne{});
}
template <class T>
valarray<bool> operator<(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<bool>(x, y, ycxx::detail::va_op::lt{});
}
template <class T>
valarray<bool> operator<(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<bool>(x, y, ycxx::detail::va_op::lt{});
}
template <class T>
valarray<bool> operator<(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<bool>(x, y, ycxx::detail::va_op::lt{});
}
template <class T>
valarray<bool> operator>(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<bool>(x, y, ycxx::detail::va_op::gt{});
}
template <class T>
valarray<bool> operator>(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<bool>(x, y, ycxx::detail::va_op::gt{});
}
template <class T>
valarray<bool> operator>(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<bool>(x, y, ycxx::detail::va_op::gt{});
}
template <class T>
valarray<bool> operator<=(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<bool>(x, y, ycxx::detail::va_op::le{});
}
template <class T>
valarray<bool> operator<=(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<bool>(x, y, ycxx::detail::va_op::le{});
}
template <class T>
valarray<bool> operator<=(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<bool>(x, y, ycxx::detail::va_op::le{});
}
template <class T>
valarray<bool> operator>=(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<bool>(x, y, ycxx::detail::va_op::ge{});
}
template <class T>
valarray<bool> operator>=(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<bool>(x, y, ycxx::detail::va_op::ge{});
}
template <class T>
valarray<bool> operator>=(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<bool>(x, y, ycxx::detail::va_op::ge{});
}

// [valarray.transcend]
template <class T>
valarray<T> abs(const valarray<T>& x) {
  return ycxx::detail::va_map(x, ycxx::detail::va_math::f_abs{});
}
template <class T>
valarray<T> acos(const valarray<T>& x) {
  return ycxx::detail::va_map(x, ycxx::detail::va_math::f_acos{});
}
template <class T>
valarray<T> asin(const valarray<T>& x) {
  return ycxx::detail::va_map(x, ycxx::detail::va_math::f_asin{});
}
template <class T>
valarray<T> atan(const valarray<T>& x) {
  return ycxx::detail::va_map(x, ycxx::detail::va_math::f_atan{});
}
template <class T>
valarray<T> cos(const valarray<T>& x) {
  return ycxx::detail::va_map(x, ycxx::detail::va_math::f_cos{});
}
template <class T>
valarray<T> cosh(const valarray<T>& x) {
  return ycxx::detail::va_map(x, ycxx::detail::va_math::f_cosh{});
}
template <class T>
valarray<T> exp(const valarray<T>& x) {
  return ycxx::detail::va_map(x, ycxx::detail::va_math::f_exp{});
}
template <class T>
valarray<T> log(const valarray<T>& x) {
  return ycxx::detail::va_map(x, ycxx::detail::va_math::f_log{});
}
template <class T>
valarray<T> log10(const valarray<T>& x) {
  return ycxx::detail::va_map(x, ycxx::detail::va_math::f_log10{});
}
template <class T>
valarray<T> sin(const valarray<T>& x) {
  return ycxx::detail::va_map(x, ycxx::detail::va_math::f_sin{});
}
template <class T>
valarray<T> sinh(const valarray<T>& x) {
  return ycxx::detail::va_map(x, ycxx::detail::va_math::f_sinh{});
}
template <class T>
valarray<T> sqrt(const valarray<T>& x) {
  return ycxx::detail::va_map(x, ycxx::detail::va_math::f_sqrt{});
}
template <class T>
valarray<T> tan(const valarray<T>& x) {
  return ycxx::detail::va_map(x, ycxx::detail::va_math::f_tan{});
}
template <class T>
valarray<T> tanh(const valarray<T>& x) {
  return ycxx::detail::va_map(x, ycxx::detail::va_math::f_tanh{});
}
template <class T>
valarray<T> atan2(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<T>(x, y, ycxx::detail::va_math::f_atan2{});
}
template <class T>
valarray<T> atan2(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<T>(x, y, ycxx::detail::va_math::f_atan2{});
}
template <class T>
valarray<T> atan2(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<T>(x, y, ycxx::detail::va_math::f_atan2{});
}
template <class T>
valarray<T> pow(const valarray<T>& x, const valarray<T>& y) {
  return ycxx::detail::va_binary<T>(x, y, ycxx::detail::va_math::f_pow{});
}
template <class T>
valarray<T> pow(const valarray<T>& x, const typename valarray<T>::value_type& y) {
  return ycxx::detail::va_binary_left<T>(x, y, ycxx::detail::va_math::f_pow{});
}
template <class T>
valarray<T> pow(const typename valarray<T>::value_type& x, const valarray<T>& y) {
  return ycxx::detail::va_binary_right<T>(x, y, ycxx::detail::va_math::f_pow{});
}

} // namespace std
