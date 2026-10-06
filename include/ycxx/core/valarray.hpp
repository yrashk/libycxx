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

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Tp>
class valarray;
class slice;
template <class _Tp>
class slice_array;
class gslice;
template <class _Tp>
class gslice_array;
template <class _Tp>
class mask_array;
template <class _Tp>
class indirect_array;
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// The operations, as function objects (the result is converted to T, or to bool for the
// comparisons, by the caller).
namespace __va_op {
struct plus {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a + b) {
    return a + b;
  }
};
struct minus {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a - b) {
    return a - b;
  }
};
struct __mul {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a * b) {
    return a * b;
  }
};
struct div {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a / b) {
    return a / b;
  }
};
struct __mod {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a % b) {
    return a % b;
  }
};
struct __bxor {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a ^ b) {
    return a ^ b;
  }
};
struct __band {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a & b) {
    return a & b;
  }
};
struct __bor {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a | b) {
    return a | b;
  }
};
struct shl {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a << b) {
    return a << b;
  }
};
struct shr {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a >> b) {
    return a >> b;
  }
};
struct __land {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a && b) {
    return a && b;
  }
};
struct __lor {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a || b) {
    return a || b;
  }
};
struct eq {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a == b) {
    return a == b;
  }
};
struct __ne {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a != b) {
    return a != b;
  }
};
struct lt {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a < b) {
    return a < b;
  }
};
struct __gt {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a > b) {
    return a > b;
  }
};
struct __le {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a <= b) {
    return a <= b;
  }
};
struct __ge {
  template <class _Ap, class _Bp>
  constexpr auto operator()(const _Ap& a, const _Bp& b) const -> decltype(a >= b) {
    return a >= b;
  }
};
// Compound assignments.
struct __plus_assign {
  template <class _Ap, class _Bp>
  constexpr void operator()(_Ap& a, const _Bp& b) const {
    a += b;
  }
};
struct __minus_assign {
  template <class _Ap, class _Bp>
  constexpr void operator()(_Ap& a, const _Bp& b) const {
    a -= b;
  }
};
struct __mul_assign {
  template <class _Ap, class _Bp>
  constexpr void operator()(_Ap& a, const _Bp& b) const {
    a *= b;
  }
};
struct __div_assign {
  template <class _Ap, class _Bp>
  constexpr void operator()(_Ap& a, const _Bp& b) const {
    a /= b;
  }
};
struct __mod_assign {
  template <class _Ap, class _Bp>
  constexpr void operator()(_Ap& a, const _Bp& b) const {
    a %= b;
  }
};
struct __xor_assign {
  template <class _Ap, class _Bp>
  constexpr void operator()(_Ap& a, const _Bp& b) const {
    a ^= b;
  }
};
struct __and_assign {
  template <class _Ap, class _Bp>
  constexpr void operator()(_Ap& a, const _Bp& b) const {
    a &= b;
  }
};
struct __or_assign {
  template <class _Ap, class _Bp>
  constexpr void operator()(_Ap& a, const _Bp& b) const {
    a |= b;
  }
};
struct __shl_assign {
  template <class _Ap, class _Bp>
  constexpr void operator()(_Ap& a, const _Bp& b) const {
    a <<= b;
  }
};
struct __shr_assign {
  template <class _Ap, class _Bp>
  constexpr void operator()(_Ap& a, const _Bp& b) const {
    a >>= b;
  }
};
struct assign {
  template <class _Ap, class _Bp>
  constexpr void operator()(_Ap& a, const _Bp& b) const {
    a = b;
  }
};
} // namespace va_op

// [valarray.transcend]/1: "A unique function with the indicated name can be applied
// (unqualified) to an operand of type T": the calls below are unqualified on purpose, with the
// std:: functions visible.
namespace __va_math {
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
struct __f_abs {
  template <class _Xp>
  auto operator()(const _Xp& __x) const -> decltype(abs(__x)) {
    return abs(__x);
  }
};
struct __f_acos {
  template <class _Xp>
  auto operator()(const _Xp& __x) const -> decltype(acos(__x)) {
    return acos(__x);
  }
};
struct __f_asin {
  template <class _Xp>
  auto operator()(const _Xp& __x) const -> decltype(asin(__x)) {
    return asin(__x);
  }
};
struct __f_atan {
  template <class _Xp>
  auto operator()(const _Xp& __x) const -> decltype(atan(__x)) {
    return atan(__x);
  }
};
struct __f_atan2 {
  template <class _Xp>
  auto operator()(const _Xp& __x, const _Xp& y) const -> decltype(atan2(__x, y)) {
    return atan2(__x, y);
  }
};
struct __f_cos {
  template <class _Xp>
  auto operator()(const _Xp& __x) const -> decltype(cos(__x)) {
    return cos(__x);
  }
};
struct __f_cosh {
  template <class _Xp>
  auto operator()(const _Xp& __x) const -> decltype(cosh(__x)) {
    return cosh(__x);
  }
};
struct __f_exp {
  template <class _Xp>
  auto operator()(const _Xp& __x) const -> decltype(exp(__x)) {
    return exp(__x);
  }
};
struct __f_log {
  template <class _Xp>
  auto operator()(const _Xp& __x) const -> decltype(log(__x)) {
    return log(__x);
  }
};
struct __f_log10 {
  template <class _Xp>
  auto operator()(const _Xp& __x) const -> decltype(log10(__x)) {
    return log10(__x);
  }
};
struct __f_pow {
  template <class _Xp>
  auto operator()(const _Xp& __x, const _Xp& y) const -> decltype(pow(__x, y)) {
    return pow(__x, y);
  }
};
struct __f_sin {
  template <class _Xp>
  auto operator()(const _Xp& __x) const -> decltype(sin(__x)) {
    return sin(__x);
  }
};
struct __f_sinh {
  template <class _Xp>
  auto operator()(const _Xp& __x) const -> decltype(sinh(__x)) {
    return sinh(__x);
  }
};
struct __f_sqrt {
  template <class _Xp>
  auto operator()(const _Xp& __x) const -> decltype(sqrt(__x)) {
    return sqrt(__x);
  }
};
struct __f_tan {
  template <class _Xp>
  auto operator()(const _Xp& __x) const -> decltype(tan(__x)) {
    return tan(__x);
  }
};
struct __f_tanh {
  template <class _Xp>
  auto operator()(const _Xp& __x) const -> decltype(tanh(__x)) {
    return tanh(__x);
  }
};
} // namespace va_math

// Builds valarray<R> of n elements, element i initialised with f(i).
struct __va_access;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// ---- slice ([class.slice]) ---------------------------------------------------------------------
class slice {
public:
  slice() : __start_(0), __size_(0), __stride_(0) {}
  slice(size_t start, size_t length, size_t stride) : __start_(start), __size_(length), __stride_(stride) {}
  slice(const slice&) = default;
  size_t start() const { return __start_; }
  size_t size() const { return __size_; }
  size_t stride() const { return __stride_; }
  friend bool operator==(const slice& __x, const slice& y) {
    return __x.start() == y.start() && __x.size() == y.size() && __x.stride() == y.stride();
  }

private:
  size_t __start_, __size_, __stride_;
};

// ---- valarray ([template.valarray]) --------------------------------------------------------------
template <class _Tp>
class valarray {
public:
  using value_type = _Tp;
  using iterator = _Tp*;
  using const_iterator = const _Tp*;

  // [valarray.cons]
  valarray() noexcept : __data_(nullptr), __size_(0) {}
  explicit valarray(size_t n) : __data_(nullptr), __size_(0) {
    __build(n, [](_Tp* p, size_t k) { std::uninitialized_value_construct_n(p, k); });
  }
  valarray(const _Tp& __v, size_t n) : __data_(nullptr), __size_(0) {
    __build(n, [&__v](_Tp* p, size_t k) { std::uninitialized_fill_n(p, k, __v); });
  }
  valarray(const _Tp* p, size_t n) : __data_(nullptr), __size_(0) {
    __build(n, [p](_Tp* d, size_t k) { std::uninitialized_copy_n(p, k, d); });
  }
  valarray(const valarray& __v) : valarray(__v.__data_, __v.__size_) {}
  valarray(valarray&& __v) noexcept : __data_(__v.__data_), __size_(__v.__size_) {
    __v.__data_ = nullptr;
    __v.__size_ = 0;
  }
  valarray(const slice_array<_Tp>& s);
  valarray(const gslice_array<_Tp>& s);
  valarray(const mask_array<_Tp>& s);
  valarray(const indirect_array<_Tp>& s);
  valarray(initializer_list<_Tp> il) : valarray(il.begin(), il.size()) {}
  ~valarray() { release(); }

  // [valarray.assign]
  valarray& operator=(const valarray& __v) {
    if (this == __builtin_addressof(__v)) return *this;
    if (__size_ == __v.__size_) {
      for (size_t i = 0; i < __size_; ++i) __data_[i] = __v.__data_[i];
    } else {
      valarray __tmp(__v);
      swap(__tmp);
    }
    return *this;
  }
  valarray& operator=(valarray&& __v) noexcept {
    if (this != __builtin_addressof(__v)) {
      release();
      __data_ = __v.__data_;
      __size_ = __v.__size_;
      __v.__data_ = nullptr;
      __v.__size_ = 0;
    }
    return *this;
  }
  valarray& operator=(initializer_list<_Tp> il) { return *this = valarray(il); }
  valarray& operator=(const _Tp& __v) {
    for (size_t i = 0; i < __size_; ++i) __data_[i] = __v;
    return *this;
  }
  valarray& operator=(const slice_array<_Tp>& s);
  valarray& operator=(const gslice_array<_Tp>& s);
  valarray& operator=(const mask_array<_Tp>& s);
  valarray& operator=(const indirect_array<_Tp>& s);

  // [valarray.access]
  const _Tp& operator[](size_t n) const {
    __ycxx::__detail::__precondition(n < __size_, "std::valarray::operator[]: index out of range");
    return __data_[n];
  }
  _Tp& operator[](size_t n) {
    __ycxx::__detail::__precondition(n < __size_, "std::valarray::operator[]: index out of range");
    return __data_[n];
  }

  // [valarray.sub]
  valarray operator[](slice s) const;
  slice_array<_Tp> operator[](slice s);
  valarray operator[](const gslice& __g) const;
  gslice_array<_Tp> operator[](const gslice& __g);
  valarray operator[](const valarray<bool>& mask) const;
  mask_array<_Tp> operator[](const valarray<bool>& mask);
  valarray operator[](const valarray<size_t>& __ind) const;
  indirect_array<_Tp> operator[](const valarray<size_t>& __ind);

  // [valarray.unary]
  valarray operator+() const {
    return map([](const _Tp& __x) -> _Tp { return +__x; });
  }
  valarray operator-() const {
    return map([](const _Tp& __x) -> _Tp { return -__x; });
  }
  valarray operator~() const {
    return map([](const _Tp& __x) -> _Tp { return ~__x; });
  }
  valarray<bool> operator!() const;

  // [valarray.cassign]
  valarray& operator*=(const _Tp& __v) { return __update(__v, __ycxx::__detail::__va_op::__mul_assign{}); }
  valarray& operator/=(const _Tp& __v) { return __update(__v, __ycxx::__detail::__va_op::__div_assign{}); }
  valarray& operator%=(const _Tp& __v) { return __update(__v, __ycxx::__detail::__va_op::__mod_assign{}); }
  valarray& operator+=(const _Tp& __v) { return __update(__v, __ycxx::__detail::__va_op::__plus_assign{}); }
  valarray& operator-=(const _Tp& __v) { return __update(__v, __ycxx::__detail::__va_op::__minus_assign{}); }
  valarray& operator^=(const _Tp& __v) { return __update(__v, __ycxx::__detail::__va_op::__xor_assign{}); }
  valarray& operator&=(const _Tp& __v) { return __update(__v, __ycxx::__detail::__va_op::__and_assign{}); }
  valarray& operator|=(const _Tp& __v) { return __update(__v, __ycxx::__detail::__va_op::__or_assign{}); }
  valarray& operator<<=(const _Tp& __v) { return __update(__v, __ycxx::__detail::__va_op::__shl_assign{}); }
  valarray& operator>>=(const _Tp& __v) { return __update(__v, __ycxx::__detail::__va_op::__shr_assign{}); }
  valarray& operator*=(const valarray& __v) { return __update(__v, __ycxx::__detail::__va_op::__mul_assign{}); }
  valarray& operator/=(const valarray& __v) { return __update(__v, __ycxx::__detail::__va_op::__div_assign{}); }
  valarray& operator%=(const valarray& __v) { return __update(__v, __ycxx::__detail::__va_op::__mod_assign{}); }
  valarray& operator+=(const valarray& __v) { return __update(__v, __ycxx::__detail::__va_op::__plus_assign{}); }
  valarray& operator-=(const valarray& __v) { return __update(__v, __ycxx::__detail::__va_op::__minus_assign{}); }
  valarray& operator^=(const valarray& __v) { return __update(__v, __ycxx::__detail::__va_op::__xor_assign{}); }
  valarray& operator|=(const valarray& __v) { return __update(__v, __ycxx::__detail::__va_op::__or_assign{}); }
  valarray& operator&=(const valarray& __v) { return __update(__v, __ycxx::__detail::__va_op::__and_assign{}); }
  valarray& operator<<=(const valarray& __v) { return __update(__v, __ycxx::__detail::__va_op::__shl_assign{}); }
  valarray& operator>>=(const valarray& __v) { return __update(__v, __ycxx::__detail::__va_op::__shr_assign{}); }

  // [valarray.range]
  iterator begin() noexcept { return __data_; }
  iterator end() noexcept { return __data_ + __size_; }
  const_iterator begin() const noexcept { return __data_; }
  const_iterator end() const noexcept { return __data_ + __size_; }

  // [valarray.members]
  void swap(valarray& __v) noexcept {
    _Tp* d = __data_;
    __data_ = __v.__data_;
    __v.__data_ = d;
    const size_t s = __size_;
    __size_ = __v.__size_;
    __v.__size_ = s;
  }
  size_t size() const noexcept { return __size_; }
  _Tp sum() const {
    __ycxx::__detail::__precondition(__size_ > 0, "std::valarray::sum: empty array");
    _Tp r = __data_[0];
    for (size_t i = 1; i < __size_; ++i) r += __data_[i];
    return r;
  }
  _Tp min() const {
    __ycxx::__detail::__precondition(__size_ > 0, "std::valarray::min: empty array");
    size_t k = 0;
    for (size_t i = 1; i < __size_; ++i)
      if (__data_[i] < __data_[k]) k = i;
    return __data_[k];
  }
  _Tp max() const {
    __ycxx::__detail::__precondition(__size_ > 0, "std::valarray::max: empty array");
    size_t k = 0;
    for (size_t i = 1; i < __size_; ++i)
      if (__data_[k] < __data_[i]) k = i;
    return __data_[k];
  }
  valarray shift(int n) const {
    valarray r(__size_);
    for (size_t i = 0; i < __size_; ++i) {
      const long long __j = static_cast<long long>(i) + n;
      if (__j >= 0 && static_cast<unsigned long long>(__j) < __size_) r.__data_[i] = __data_[__j];
    }
    return r;
  }
  valarray cshift(int n) const {
    if (__size_ == 0) return valarray();
    const long long __sz = static_cast<long long>(__size_);
    long long k = n % __sz;
    if (k < 0) k += __sz;
    const size_t first = static_cast<size_t>(k);
    return generate(__size_, [this, first](size_t i) -> const _Tp& {
      return __data_[i < __size_ - first ? first + i : i - (__size_ - first)];
    });
  }
  valarray apply(_Tp __func(_Tp)) const {
    return map([__func](const _Tp& __x) -> _Tp { return __func(__x); });
  }
  valarray apply(_Tp __func(const _Tp&)) const {
    return map([__func](const _Tp& __x) -> _Tp { return __func(__x); });
  }
  void resize(size_t __sz, _Tp c = _Tp()) {
    if (__sz == __size_) {
      for (size_t i = 0; i < __size_; ++i) __data_[i] = c;
      return;
    }
    valarray __tmp(c, __sz);
    swap(__tmp);
  }

private:
  template <class _Up>
  friend class valarray;
  friend struct __ycxx::__detail::__va_access;

  // A deallocation guard for a partially built array (exception safety without try).
  struct __storage {
    _Tp* p;
    size_t n;
    ~__storage() {
      if (p) std::allocator<_Tp>().deallocate(p, n);
    }
  };
  // Allocates n elements and lets init construct all of them (init either constructs every
  // element or destroys what it constructed and throws, as the uninitialized algorithms do).
  template <class Init>
  void __build(size_t n, Init init) {
    if (n == 0) return;
    __storage s{std::allocator<_Tp>().allocate(n), n};
    init(s.p, n);
    __data_ = s.p;
    __size_ = n;
    s.p = nullptr;
  }
  void release() noexcept {
    if (__data_) {
      std::destroy_n(__data_, __size_);
      std::allocator<_Tp>().deallocate(__data_, __size_);
      __data_ = nullptr;
      __size_ = 0;
    }
  }
  // Builds an array of n elements, element i initialised with f(i), converted to T.
  template <class _Fp>
  static valarray generate(size_t n, _Fp __f) {
    valarray r;
    r.__build(n, [&__f](_Tp* p, size_t __cnt) {
      size_t i = 0;
      struct __undo {
        _Tp* p;
        size_t* i;
        bool done;
        ~__undo() {
          if (!done) std::destroy_n(p, *i);
        }
      } __u{p, &i, false};
      for (; i < __cnt; ++i) ::new (static_cast<void*>(p + i)) _Tp(__f(i));
      __u.done = true;
    });
    return r;
  }
  template <class _Fp>
  valarray map(_Fp __f) const {
    return generate(__size_, [this, &__f](size_t i) { return __f(__data_[i]); });
  }
  template <class _Op_>
  valarray& __update(const _Tp& __v, _Op_ op) {
    for (size_t i = 0; i < __size_; ++i) op(__data_[i], __v);
    return *this;
  }
  template <class _Op_>
  valarray& __update(const valarray& __v, _Op_ op) {
    __ycxx::__detail::__precondition(__size_ == __v.__size_, "std::valarray compound assignment: sizes differ");
    if (this == __builtin_addressof(__v)) {
      const valarray copy(__v);
      for (size_t i = 0; i < __size_; ++i) op(__data_[i], copy.__data_[i]);
    } else {
      for (size_t i = 0; i < __size_; ++i) op(__data_[i], __v.__data_[i]);
    }
    return *this;
  }

  _Tp* __data_;
  size_t __size_;
};

template <class _Tp, size_t __cnt>
valarray(const _Tp (&)[__cnt], size_t) -> valarray<_Tp>;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

struct __va_access {
  template <class _Rp, class _Fp>
  static std::valarray<_Rp> generate(std::size_t n, _Fp __f) {
    return std::valarray<_Rp>::generate(n, __f);
  }
  template <class _Tp>
  static _Tp* data(const std::valarray<_Tp>& __v) {
    return __v.__data_;
  }
};

// The element indices a gslice selects, the highest-ordered index turning fastest.
inline std::valarray<std::size_t> __gslice_indices(std::size_t start, const std::valarray<std::size_t>& __len,
                                                 const std::valarray<std::size_t>& str) {
  const std::size_t dims = __len.size() < str.size() ? __len.size() : str.size();
  std::size_t __total = dims == 0 ? 0 : 1;
  for (std::size_t __j = 0; __j < dims; ++__j) __total *= __len[__j];
  std::valarray<std::size_t> __idx(__total);
  if (__total == 0) return __idx;
  std::valarray<std::size_t> __pos(dims);
  for (std::size_t k = 0; k < __total; ++k) {
    std::size_t __off = start;
    for (std::size_t __j = 0; __j < dims; ++__j) __off += __pos[__j] * str[__j];
    __idx[k] = __off;
    for (std::size_t __j = dims; __j-- > 0;) { // odometer: the last index turns fastest
      if (++__pos[__j] < __len[__j]) break;
      __pos[__j] = 0;
    }
  }
  return __idx;
}

// The indices of the true elements of a mask.
inline std::valarray<std::size_t> __va_mask_indices(const std::valarray<bool>& mask) {
  std::size_t n = 0;
  for (std::size_t i = 0; i < mask.size(); ++i) n += mask[i] ? 1 : 0;
  std::valarray<std::size_t> __idx(n);
  for (std::size_t i = 0, k = 0; i < mask.size(); ++i)
    if (mask[i]) __idx[k++] = i;
  return __idx;
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// ---- gslice ([class.gslice]) ---------------------------------------------------------------------
class gslice {
public:
  gslice() : __start_(0) {}
  gslice(size_t s, const valarray<size_t>& __l, const valarray<size_t>& d) : __start_(s), __size_(__l), __stride_(d) {}
  size_t start() const { return __start_; }
  valarray<size_t> size() const { return __size_; }
  valarray<size_t> stride() const { return __stride_; }

private:
  template <class _Tp>
  friend class valarray;
  valarray<size_t> indices() const { return __ycxx::__detail::__gslice_indices(__start_, __size_, __stride_); }

  size_t __start_;
  valarray<size_t> __size_;
  valarray<size_t> __stride_;
};

// ---- the subset classes --------------------------------------------------------------------------
// slice_array refers to base[start + i * stride]; the others to base[idx[i]].
template <class _Tp>
class slice_array {
public:
  using value_type = _Tp;
  void operator=(const valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::assign{}); }
  void operator*=(const valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__mul_assign{}); }
  void operator/=(const valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__div_assign{}); }
  void operator%=(const valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__mod_assign{}); }
  void operator+=(const valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__plus_assign{}); }
  void operator-=(const valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__minus_assign{}); }
  void operator^=(const valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__xor_assign{}); }
  void operator&=(const valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__and_assign{}); }
  void operator|=(const valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__or_assign{}); }
  void operator<<=(const valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__shl_assign{}); }
  void operator>>=(const valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__shr_assign{}); }
  slice_array(const slice_array&) = default;
  ~slice_array() = default;
  const slice_array& operator=(const slice_array& s) const {
    for (size_t i = 0; i < __size_; ++i) at(i) = s.at(i);
    return *this;
  }
  void operator=(const _Tp& __v) const {
    for (size_t i = 0; i < __size_; ++i) at(i) = __v;
  }
  slice_array() = delete;

private:
  template <class _Up>
  friend class valarray;
  slice_array(_Tp* base, const slice& s) : __base_(base), __start_(s.start()), __size_(s.size()), __stride_(s.stride()) {}
  _Tp& at(size_t i) const { return __base_[__start_ + i * __stride_]; }
  template <class _Op_>
  void apply(const valarray<_Tp>& __v, _Op_ op) const {
    for (size_t i = 0; i < __size_; ++i) op(at(i), __v[i]);
  }

  _Tp* __base_;
  size_t __start_, __size_, __stride_;
};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// The common part of gslice_array, mask_array and indirect_array: base[idx[i]].
template <class _Tp>
class __valarray_indexed {
public:
  using value_type = _Tp;
  void operator*=(const std::valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__mul_assign{}); }
  void operator/=(const std::valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__div_assign{}); }
  void operator%=(const std::valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__mod_assign{}); }
  void operator+=(const std::valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__plus_assign{}); }
  void operator-=(const std::valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__minus_assign{}); }
  void operator^=(const std::valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__xor_assign{}); }
  void operator&=(const std::valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__and_assign{}); }
  void operator|=(const std::valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__or_assign{}); }
  void operator<<=(const std::valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__shl_assign{}); }
  void operator>>=(const std::valarray<_Tp>& __v) const { apply(__v, __ycxx::__detail::__va_op::__shr_assign{}); }

protected:
  __valarray_indexed(_Tp* base, std::valarray<std::size_t>&& __idx)
      : __base_(base), __idx_(static_cast<std::valarray<std::size_t>&&>(__idx)) {}
  __valarray_indexed(const __valarray_indexed&) = default;
  ~__valarray_indexed() = default;
  __valarray_indexed& operator=(const __valarray_indexed&) = delete;

  template <class _Op_>
  void apply(const std::valarray<_Tp>& __v, _Op_ op) const {
    for (std::size_t i = 0; i < __idx_.size(); ++i) op(__base_[__idx_[i]], __v[i]);
  }
  void fill(const _Tp& __v) const {
    for (std::size_t i = 0; i < __idx_.size(); ++i) __base_[__idx_[i]] = __v;
  }
  void __copy_from(const __valarray_indexed& s) const {
    for (std::size_t i = 0; i < __idx_.size(); ++i) __base_[__idx_[i]] = s.__base_[s.__idx_[i]];
  }

  template <class _Up>
  friend class std::valarray;

  _Tp* __base_;
  std::valarray<std::size_t> __idx_;
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp>
class gslice_array : public __ycxx::__adl_free::__valarray_indexed<_Tp> {
  using base = __ycxx::__adl_free::__valarray_indexed<_Tp>;

public:
  using value_type = _Tp;
  void operator=(const valarray<_Tp>& __v) const { this->apply(__v, __ycxx::__detail::__va_op::assign{}); }
  gslice_array(const gslice_array&) = default;
  ~gslice_array() = default;
  const gslice_array& operator=(const gslice_array& s) const {
    this->__copy_from(s);
    return *this;
  }
  void operator=(const _Tp& __v) const { this->fill(__v); }
  gslice_array() = delete;

private:
  template <class _Up>
  friend class valarray;
  gslice_array(_Tp* p, valarray<size_t>&& __idx) : base(p, static_cast<valarray<size_t>&&>(__idx)) {}
};

template <class _Tp>
class mask_array : public __ycxx::__adl_free::__valarray_indexed<_Tp> {
  using base = __ycxx::__adl_free::__valarray_indexed<_Tp>;

public:
  using value_type = _Tp;
  void operator=(const valarray<_Tp>& __v) const { this->apply(__v, __ycxx::__detail::__va_op::assign{}); }
  mask_array(const mask_array&) = default;
  ~mask_array() = default;
  const mask_array& operator=(const mask_array& s) const {
    this->__copy_from(s);
    return *this;
  }
  void operator=(const _Tp& __v) const { this->fill(__v); }
  mask_array() = delete;

private:
  template <class _Up>
  friend class valarray;
  mask_array(_Tp* p, valarray<size_t>&& __idx) : base(p, static_cast<valarray<size_t>&&>(__idx)) {}
};

template <class _Tp>
class indirect_array : public __ycxx::__adl_free::__valarray_indexed<_Tp> {
  using base = __ycxx::__adl_free::__valarray_indexed<_Tp>;

public:
  using value_type = _Tp;
  void operator=(const valarray<_Tp>& __v) const { this->apply(__v, __ycxx::__detail::__va_op::assign{}); }
  indirect_array(const indirect_array&) = default;
  ~indirect_array() = default;
  const indirect_array& operator=(const indirect_array& s) const {
    this->__copy_from(s);
    return *this;
  }
  void operator=(const _Tp& __v) const { this->fill(__v); }
  indirect_array() = delete;

private:
  template <class _Up>
  friend class valarray;
  indirect_array(_Tp* p, valarray<size_t>&& __idx) : base(p, static_cast<valarray<size_t>&&>(__idx)) {}
};

// ---- valarray members that need the subset classes -----------------------------------------------
template <class _Tp>
valarray<_Tp>::valarray(const slice_array<_Tp>& s) : __data_(nullptr), __size_(0) {
  *this = generate(s.__size_, [&s](size_t i) -> const _Tp& { return s.at(i); });
}
template <class _Tp>
valarray<_Tp>::valarray(const gslice_array<_Tp>& s) : __data_(nullptr), __size_(0) {
  *this = generate(s.__idx_.size(), [&s](size_t i) -> const _Tp& { return s.__base_[s.__idx_[i]]; });
}
template <class _Tp>
valarray<_Tp>::valarray(const mask_array<_Tp>& s) : __data_(nullptr), __size_(0) {
  *this = generate(s.__idx_.size(), [&s](size_t i) -> const _Tp& { return s.__base_[s.__idx_[i]]; });
}
template <class _Tp>
valarray<_Tp>::valarray(const indirect_array<_Tp>& s) : __data_(nullptr), __size_(0) {
  *this = generate(s.__idx_.size(), [&s](size_t i) -> const _Tp& { return s.__base_[s.__idx_[i]]; });
}
template <class _Tp>
valarray<_Tp>& valarray<_Tp>::operator=(const slice_array<_Tp>& s) {
  __ycxx::__detail::__precondition(s.__size_ == __size_, "std::valarray::operator=: the slice_array length differs");
  for (size_t i = 0; i < __size_; ++i) __data_[i] = s.at(i);
  return *this;
}
template <class _Tp>
valarray<_Tp>& valarray<_Tp>::operator=(const gslice_array<_Tp>& s) {
  __ycxx::__detail::__precondition(s.__idx_.size() == __size_, "std::valarray::operator=: the gslice_array length differs");
  for (size_t i = 0; i < __size_; ++i) __data_[i] = s.__base_[s.__idx_[i]];
  return *this;
}
template <class _Tp>
valarray<_Tp>& valarray<_Tp>::operator=(const mask_array<_Tp>& s) {
  __ycxx::__detail::__precondition(s.__idx_.size() == __size_, "std::valarray::operator=: the mask_array length differs");
  for (size_t i = 0; i < __size_; ++i) __data_[i] = s.__base_[s.__idx_[i]];
  return *this;
}
template <class _Tp>
valarray<_Tp>& valarray<_Tp>::operator=(const indirect_array<_Tp>& s) {
  __ycxx::__detail::__precondition(s.__idx_.size() == __size_, "std::valarray::operator=: the indirect_array length differs");
  for (size_t i = 0; i < __size_; ++i) __data_[i] = s.__base_[s.__idx_[i]];
  return *this;
}

template <class _Tp>
valarray<_Tp> valarray<_Tp>::operator[](slice s) const {
  return generate(s.size(), [this, &s](size_t i) -> const _Tp& { return __data_[s.start() + i * s.stride()]; });
}
template <class _Tp>
slice_array<_Tp> valarray<_Tp>::operator[](slice s) {
  return slice_array<_Tp>(__data_, s);
}
template <class _Tp>
valarray<_Tp> valarray<_Tp>::operator[](const gslice& __g) const {
  const valarray<size_t> __idx = __g.indices();
  return generate(__idx.size(), [this, &__idx](size_t i) -> const _Tp& { return __data_[__idx[i]]; });
}
template <class _Tp>
gslice_array<_Tp> valarray<_Tp>::operator[](const gslice& __g) {
  return gslice_array<_Tp>(__data_, __g.indices());
}
template <class _Tp>
valarray<_Tp> valarray<_Tp>::operator[](const valarray<bool>& mask) const {
  const valarray<size_t> __idx = __ycxx::__detail::__va_mask_indices(mask);
  return generate(__idx.size(), [this, &__idx](size_t i) -> const _Tp& { return __data_[__idx[i]]; });
}
template <class _Tp>
mask_array<_Tp> valarray<_Tp>::operator[](const valarray<bool>& mask) {
  return mask_array<_Tp>(__data_, __ycxx::__detail::__va_mask_indices(mask));
}
template <class _Tp>
valarray<_Tp> valarray<_Tp>::operator[](const valarray<size_t>& __ind) const {
  return generate(__ind.size(), [this, &__ind](size_t i) -> const _Tp& { return __data_[__ind[i]]; });
}
template <class _Tp>
indirect_array<_Tp> valarray<_Tp>::operator[](const valarray<size_t>& __ind) {
  return indirect_array<_Tp>(__data_, valarray<size_t>(__ind));
}
template <class _Tp>
valarray<bool> valarray<_Tp>::operator!() const {
  return valarray<bool>::generate(__size_, [this](size_t i) -> bool { return !__data_[i]; });
}

// ---- [valarray.special] ----------------------------------------------------------------------------
template <class _Tp>
void swap(valarray<_Tp>& __x, valarray<_Tp>& y) noexcept {
  __x.swap(y);
}

} // namespace std

// ---- [valarray.binary], [valarray.comparison], [valarray.transcend] ----------------------------------
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Rp, class _Tp, class _Op_>
std::valarray<_Rp> __va_binary(const std::valarray<_Tp>& __x, const std::valarray<_Tp>& y, _Op_ op) {
  __ycxx::__detail::__precondition(__x.size() == y.size(), "std::valarray binary operator: sizes differ");
  const _Tp* a = __ycxx::__detail::__va_access::data(__x);
  const _Tp* b = __ycxx::__detail::__va_access::data(y);
  return __ycxx::__detail::__va_access::generate<_Rp>(__x.size(), [a, b, &op](std::size_t i) -> _Rp { return op(a[i], b[i]); });
}
template <class _Rp, class _Tp, class _Op_>
std::valarray<_Rp> __va_binary_left(const std::valarray<_Tp>& __x, const _Tp& y, _Op_ op) {
  const _Tp* a = __ycxx::__detail::__va_access::data(__x);
  return __ycxx::__detail::__va_access::generate<_Rp>(__x.size(), [a, &y, &op](std::size_t i) -> _Rp { return op(a[i], y); });
}
template <class _Rp, class _Tp, class _Op_>
std::valarray<_Rp> __va_binary_right(const _Tp& __x, const std::valarray<_Tp>& y, _Op_ op) {
  const _Tp* b = __ycxx::__detail::__va_access::data(y);
  return __ycxx::__detail::__va_access::generate<_Rp>(y.size(), [b, &__x, &op](std::size_t i) -> _Rp { return op(__x, b[i]); });
}
template <class _Tp, class _Fp>
std::valarray<_Tp> __va_map(const std::valarray<_Tp>& __x, _Fp __f) {
  const _Tp* a = __ycxx::__detail::__va_access::data(__x);
  return __ycxx::__detail::__va_access::generate<_Tp>(__x.size(), [a, &__f](std::size_t i) -> _Tp { return __f(a[i]); });
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp>
valarray<_Tp> operator*(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<_Tp>(__x, y, __ycxx::__detail::__va_op::__mul{});
}
template <class _Tp>
valarray<_Tp> operator*(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<_Tp>(__x, y, __ycxx::__detail::__va_op::__mul{});
}
template <class _Tp>
valarray<_Tp> operator*(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<_Tp>(__x, y, __ycxx::__detail::__va_op::__mul{});
}
template <class _Tp>
valarray<_Tp> operator/(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<_Tp>(__x, y, __ycxx::__detail::__va_op::div{});
}
template <class _Tp>
valarray<_Tp> operator/(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<_Tp>(__x, y, __ycxx::__detail::__va_op::div{});
}
template <class _Tp>
valarray<_Tp> operator/(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<_Tp>(__x, y, __ycxx::__detail::__va_op::div{});
}
template <class _Tp>
valarray<_Tp> operator%(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<_Tp>(__x, y, __ycxx::__detail::__va_op::__mod{});
}
template <class _Tp>
valarray<_Tp> operator%(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<_Tp>(__x, y, __ycxx::__detail::__va_op::__mod{});
}
template <class _Tp>
valarray<_Tp> operator%(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<_Tp>(__x, y, __ycxx::__detail::__va_op::__mod{});
}
template <class _Tp>
valarray<_Tp> operator+(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<_Tp>(__x, y, __ycxx::__detail::__va_op::plus{});
}
template <class _Tp>
valarray<_Tp> operator+(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<_Tp>(__x, y, __ycxx::__detail::__va_op::plus{});
}
template <class _Tp>
valarray<_Tp> operator+(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<_Tp>(__x, y, __ycxx::__detail::__va_op::plus{});
}
template <class _Tp>
valarray<_Tp> operator-(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<_Tp>(__x, y, __ycxx::__detail::__va_op::minus{});
}
template <class _Tp>
valarray<_Tp> operator-(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<_Tp>(__x, y, __ycxx::__detail::__va_op::minus{});
}
template <class _Tp>
valarray<_Tp> operator-(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<_Tp>(__x, y, __ycxx::__detail::__va_op::minus{});
}
template <class _Tp>
valarray<_Tp> operator^(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<_Tp>(__x, y, __ycxx::__detail::__va_op::__bxor{});
}
template <class _Tp>
valarray<_Tp> operator^(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<_Tp>(__x, y, __ycxx::__detail::__va_op::__bxor{});
}
template <class _Tp>
valarray<_Tp> operator^(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<_Tp>(__x, y, __ycxx::__detail::__va_op::__bxor{});
}
template <class _Tp>
valarray<_Tp> operator&(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<_Tp>(__x, y, __ycxx::__detail::__va_op::__band{});
}
template <class _Tp>
valarray<_Tp> operator&(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<_Tp>(__x, y, __ycxx::__detail::__va_op::__band{});
}
template <class _Tp>
valarray<_Tp> operator&(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<_Tp>(__x, y, __ycxx::__detail::__va_op::__band{});
}
template <class _Tp>
valarray<_Tp> operator|(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<_Tp>(__x, y, __ycxx::__detail::__va_op::__bor{});
}
template <class _Tp>
valarray<_Tp> operator|(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<_Tp>(__x, y, __ycxx::__detail::__va_op::__bor{});
}
template <class _Tp>
valarray<_Tp> operator|(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<_Tp>(__x, y, __ycxx::__detail::__va_op::__bor{});
}
template <class _Tp>
valarray<_Tp> operator<<(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<_Tp>(__x, y, __ycxx::__detail::__va_op::shl{});
}
template <class _Tp>
valarray<_Tp> operator<<(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<_Tp>(__x, y, __ycxx::__detail::__va_op::shl{});
}
template <class _Tp>
valarray<_Tp> operator<<(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<_Tp>(__x, y, __ycxx::__detail::__va_op::shl{});
}
template <class _Tp>
valarray<_Tp> operator>>(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<_Tp>(__x, y, __ycxx::__detail::__va_op::shr{});
}
template <class _Tp>
valarray<_Tp> operator>>(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<_Tp>(__x, y, __ycxx::__detail::__va_op::shr{});
}
template <class _Tp>
valarray<_Tp> operator>>(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<_Tp>(__x, y, __ycxx::__detail::__va_op::shr{});
}
template <class _Tp>
valarray<bool> operator&&(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<bool>(__x, y, __ycxx::__detail::__va_op::__land{});
}
template <class _Tp>
valarray<bool> operator&&(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<bool>(__x, y, __ycxx::__detail::__va_op::__land{});
}
template <class _Tp>
valarray<bool> operator&&(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<bool>(__x, y, __ycxx::__detail::__va_op::__land{});
}
template <class _Tp>
valarray<bool> operator||(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<bool>(__x, y, __ycxx::__detail::__va_op::__lor{});
}
template <class _Tp>
valarray<bool> operator||(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<bool>(__x, y, __ycxx::__detail::__va_op::__lor{});
}
template <class _Tp>
valarray<bool> operator||(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<bool>(__x, y, __ycxx::__detail::__va_op::__lor{});
}
template <class _Tp>
valarray<bool> operator==(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<bool>(__x, y, __ycxx::__detail::__va_op::eq{});
}
template <class _Tp>
valarray<bool> operator==(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<bool>(__x, y, __ycxx::__detail::__va_op::eq{});
}
template <class _Tp>
valarray<bool> operator==(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<bool>(__x, y, __ycxx::__detail::__va_op::eq{});
}
template <class _Tp>
valarray<bool> operator!=(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<bool>(__x, y, __ycxx::__detail::__va_op::__ne{});
}
template <class _Tp>
valarray<bool> operator!=(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<bool>(__x, y, __ycxx::__detail::__va_op::__ne{});
}
template <class _Tp>
valarray<bool> operator!=(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<bool>(__x, y, __ycxx::__detail::__va_op::__ne{});
}
template <class _Tp>
valarray<bool> operator<(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<bool>(__x, y, __ycxx::__detail::__va_op::lt{});
}
template <class _Tp>
valarray<bool> operator<(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<bool>(__x, y, __ycxx::__detail::__va_op::lt{});
}
template <class _Tp>
valarray<bool> operator<(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<bool>(__x, y, __ycxx::__detail::__va_op::lt{});
}
template <class _Tp>
valarray<bool> operator>(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<bool>(__x, y, __ycxx::__detail::__va_op::__gt{});
}
template <class _Tp>
valarray<bool> operator>(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<bool>(__x, y, __ycxx::__detail::__va_op::__gt{});
}
template <class _Tp>
valarray<bool> operator>(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<bool>(__x, y, __ycxx::__detail::__va_op::__gt{});
}
template <class _Tp>
valarray<bool> operator<=(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<bool>(__x, y, __ycxx::__detail::__va_op::__le{});
}
template <class _Tp>
valarray<bool> operator<=(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<bool>(__x, y, __ycxx::__detail::__va_op::__le{});
}
template <class _Tp>
valarray<bool> operator<=(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<bool>(__x, y, __ycxx::__detail::__va_op::__le{});
}
template <class _Tp>
valarray<bool> operator>=(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<bool>(__x, y, __ycxx::__detail::__va_op::__ge{});
}
template <class _Tp>
valarray<bool> operator>=(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<bool>(__x, y, __ycxx::__detail::__va_op::__ge{});
}
template <class _Tp>
valarray<bool> operator>=(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<bool>(__x, y, __ycxx::__detail::__va_op::__ge{});
}

// [valarray.transcend]
template <class _Tp>
valarray<_Tp> abs(const valarray<_Tp>& __x) {
  return __ycxx::__detail::__va_map(__x, __ycxx::__detail::__va_math::__f_abs{});
}
template <class _Tp>
valarray<_Tp> acos(const valarray<_Tp>& __x) {
  return __ycxx::__detail::__va_map(__x, __ycxx::__detail::__va_math::__f_acos{});
}
template <class _Tp>
valarray<_Tp> asin(const valarray<_Tp>& __x) {
  return __ycxx::__detail::__va_map(__x, __ycxx::__detail::__va_math::__f_asin{});
}
template <class _Tp>
valarray<_Tp> atan(const valarray<_Tp>& __x) {
  return __ycxx::__detail::__va_map(__x, __ycxx::__detail::__va_math::__f_atan{});
}
template <class _Tp>
valarray<_Tp> cos(const valarray<_Tp>& __x) {
  return __ycxx::__detail::__va_map(__x, __ycxx::__detail::__va_math::__f_cos{});
}
template <class _Tp>
valarray<_Tp> cosh(const valarray<_Tp>& __x) {
  return __ycxx::__detail::__va_map(__x, __ycxx::__detail::__va_math::__f_cosh{});
}
template <class _Tp>
valarray<_Tp> exp(const valarray<_Tp>& __x) {
  return __ycxx::__detail::__va_map(__x, __ycxx::__detail::__va_math::__f_exp{});
}
template <class _Tp>
valarray<_Tp> log(const valarray<_Tp>& __x) {
  return __ycxx::__detail::__va_map(__x, __ycxx::__detail::__va_math::__f_log{});
}
template <class _Tp>
valarray<_Tp> log10(const valarray<_Tp>& __x) {
  return __ycxx::__detail::__va_map(__x, __ycxx::__detail::__va_math::__f_log10{});
}
template <class _Tp>
valarray<_Tp> sin(const valarray<_Tp>& __x) {
  return __ycxx::__detail::__va_map(__x, __ycxx::__detail::__va_math::__f_sin{});
}
template <class _Tp>
valarray<_Tp> sinh(const valarray<_Tp>& __x) {
  return __ycxx::__detail::__va_map(__x, __ycxx::__detail::__va_math::__f_sinh{});
}
template <class _Tp>
valarray<_Tp> sqrt(const valarray<_Tp>& __x) {
  return __ycxx::__detail::__va_map(__x, __ycxx::__detail::__va_math::__f_sqrt{});
}
template <class _Tp>
valarray<_Tp> tan(const valarray<_Tp>& __x) {
  return __ycxx::__detail::__va_map(__x, __ycxx::__detail::__va_math::__f_tan{});
}
template <class _Tp>
valarray<_Tp> tanh(const valarray<_Tp>& __x) {
  return __ycxx::__detail::__va_map(__x, __ycxx::__detail::__va_math::__f_tanh{});
}
template <class _Tp>
valarray<_Tp> atan2(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<_Tp>(__x, y, __ycxx::__detail::__va_math::__f_atan2{});
}
template <class _Tp>
valarray<_Tp> atan2(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<_Tp>(__x, y, __ycxx::__detail::__va_math::__f_atan2{});
}
template <class _Tp>
valarray<_Tp> atan2(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<_Tp>(__x, y, __ycxx::__detail::__va_math::__f_atan2{});
}
template <class _Tp>
valarray<_Tp> pow(const valarray<_Tp>& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary<_Tp>(__x, y, __ycxx::__detail::__va_math::__f_pow{});
}
template <class _Tp>
valarray<_Tp> pow(const valarray<_Tp>& __x, const typename valarray<_Tp>::value_type& y) {
  return __ycxx::__detail::__va_binary_left<_Tp>(__x, y, __ycxx::__detail::__va_math::__f_pow{});
}
template <class _Tp>
valarray<_Tp> pow(const typename valarray<_Tp>::value_type& __x, const valarray<_Tp>& y) {
  return __ycxx::__detail::__va_binary_right<_Tp>(__x, y, __ycxx::__detail::__va_math::__f_pow{});
}

} // namespace std
