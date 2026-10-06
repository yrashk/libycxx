// libycxx core: the random number engines and engine adaptors of <random> ([rand.eng],
// [rand.adapt], [rand.predef]) and seed_seq ([rand.util.seedseq]).
//
// Every engine keeps its state so that the textual representation and equality follow the
// draft's sequence X directly: the lagged engines (mersenne_twister_engine,
// subtract_with_carry_engine) keep X in a ring buffer whose oldest element is at `__pos_`, and
// compare and print it in logical order. All arithmetic is done in unsigned long long and
// reduced modulo 2^w (or m), so narrow UIntTypes never meet integral promotion.
#pragma once

#include <initializer_list>
#include <ycxx/core/array.hpp>
#include <ycxx/core/iterator_core.hpp>
#include <ycxx/core/move.hpp>
#include <ycxx/core/random_base.hpp>
#include <ycxx/core/vector.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

// ---- [rand.util.seedseq] ------------------------------------------------------------------------
class seed_seq {
public:
  using result_type = uint_least32_t;

  seed_seq() noexcept {}
  template <class _Tp>
    requires is_integral_v<_Tp>
  seed_seq(initializer_list<_Tp> il) : seed_seq(il.begin(), il.end()) {}
  template <class _InputIterator>
  seed_seq(_InputIterator begin, _InputIterator end) {
    static_assert(is_integral_v<typename iterator_traits<_InputIterator>::value_type>,
                  "seed_seq: the iterator's value_type must be an integer type");
    for (_InputIterator s = begin; s != end; ++s)
      __v_.push_back(static_cast<result_type>(static_cast<uint32_t>(*s)));
  }

  template <class _RandomAccessIterator>
  void generate(_RandomAccessIterator begin, _RandomAccessIterator end) {
    using _Vp = typename iterator_traits<_RandomAccessIterator>::value_type;
    static_assert(is_unsigned_v<_Vp> && !is_same_v<_Vp, bool> && numeric_limits<_Vp>::digits >= 32,
                  "seed_seq::generate: the iterator's value_type must be an unsigned integer type of at "
                  "least 32 bits");
    if (begin == end)
      return;
    using _Dp = typename iterator_traits<_RandomAccessIterator>::difference_type;
    const size_t s = __v_.size(), n = static_cast<size_t>(end - begin);
    auto at = [&begin, n](size_t k) -> decltype(auto) { return begin[static_cast<_Dp>(k % n)]; };
    auto get = [&at](size_t k) { return static_cast<uint32_t>(at(k)); };
    auto __tf = [](uint32_t __x) { return __x ^ (__x >> 27); };
    for (_RandomAccessIterator i = begin; i != end; ++i)
      *i = _Vp(0x8b8b8b8bu);
    const size_t t = n >= 623 ? 11 : n >= 68 ? 7 : n >= 39 ? 5 : n >= 7 ? 3 : (n - 1) / 2;
    const size_t p = (n - t) / 2, __q = p + t;
    const size_t m = s + 1 > n ? s + 1 : n;
    for (size_t k = 0; k < m; ++k) {
      const uint32_t __r1 = 1664525u * __tf(get(k) ^ get(k + p) ^ get(k + n - 1));
      const uint32_t __r2 =
          __r1 + (k == 0 ? static_cast<uint32_t>(s) : static_cast<uint32_t>(k % n) + (k <= s ? __v_[k - 1] : 0u));
      at(k + p) = _Vp(static_cast<uint32_t>(get(k + p) + __r1));
      at(k + __q) = _Vp(static_cast<uint32_t>(get(k + __q) + __r2));
      at(k) = _Vp(__r2);
    }
    for (size_t k = m; k < m + n; ++k) {
      const uint32_t __r3 = 1566083941u * __tf(get(k) + get(k + p) + get(k + n - 1));
      const uint32_t __r4 = __r3 - static_cast<uint32_t>(k % n);
      at(k + p) = _Vp(get(k + p) ^ __r3);
      at(k + __q) = _Vp(get(k + __q) ^ __r4);
      at(k) = _Vp(__r4);
    }
  }

  size_t size() const noexcept { return __v_.size(); }
  template <class _OutputIterator>
  void param(_OutputIterator __dest) const {
    static_assert(requires(_OutputIterator& __o, const result_type& r) { *__o = r; },
                  "seed_seq::param: values of result_type must be writable to dest");
    for (result_type __x : __v_) {
      *__dest = __x;
      ++__dest;
    }
  }

  seed_seq(const seed_seq&) = delete;
  void operator=(const seed_seq&) = delete;

private:
  vector<result_type> __v_;
};

// ---- [rand.eng.lcong] -----------------------------------------------------------------------------
template <class _UIntType, _UIntType a, _UIntType c, _UIntType m>
class linear_congruential_engine {
  static_assert(__ycxx::__detail::__rand_uint_type<_UIntType>,
                "linear_congruential_engine: UIntType must be an unsigned integer type at least as wide as "
                "short ([rand.req.genl]/1.7)");
  static_assert(m == 0 || (a < m && c < m), "linear_congruential_engine: requires a < m and c < m");

  using __y_u64 = __ycxx::__detail::__rand_u64;
  static constexpr int digits = numeric_limits<_UIntType>::digits;
  // m == 0 stands for 2^digits: arithmetic in u64 wraps and is then masked.
  static constexpr bool __pow2 = m == 0;
  static constexpr __y_u64 __mod = static_cast<__y_u64>(m); // when !pow2
  static constexpr __y_u64 reduce(__y_u64 __x) noexcept { return __pow2 ? __x & __ycxx::__detail::__rand_mask(digits) : __x % __mod; }
  static constexpr __y_u64 __mul(__y_u64 __x, __y_u64 y) noexcept {
    if constexpr (__pow2)
      return reduce(__x * y);
    else
      return __ycxx::__detail::__rand_mulmod(__x, y, __mod);
  }
  static constexpr __y_u64 add(__y_u64 __x, __y_u64 y) noexcept {
    if constexpr (__pow2)
      return reduce(__x + y);
    else
      return __ycxx::__detail::__rand_addmod(__x, y, __mod);
  }

public:
  using result_type = _UIntType;

  static constexpr result_type multiplier = a;
  static constexpr result_type increment = c;
  static constexpr result_type modulus = m;
  static constexpr result_type min() { return c == 0u ? 1u : 0u; }
  static constexpr result_type max() { return m - 1u; }
  static constexpr result_type default_seed = 1u;

  linear_congruential_engine() : linear_congruential_engine(default_seed) {}
  explicit linear_congruential_engine(result_type s) { seed(s); }
  template <class _Sseq>
    requires __ycxx::__detail::__rand_seed_seq<_Sseq, result_type, linear_congruential_engine>
  explicit linear_congruential_engine(_Sseq& __q) {
    seed(__q);
  }
  void seed(result_type s = default_seed) {
    const __y_u64 __sm = reduce(s);
    __x_ = reduce(c) == 0 && __sm == 0 ? 1 : __sm;
  }
  template <class _Sseq>
    requires __ycxx::__detail::__rand_seed_seq<_Sseq, result_type, linear_congruential_engine>
  void seed(_Sseq& __q) {
    // k = ceil(log2(m) / 32): the smallest k with 2^(32k) >= m.
    constexpr size_t k = __pow2 ? (static_cast<size_t>(digits) + 31) / 32 : __mod <= 1 ? 0 : __mod - 1 <= 0xffffffffu ? 1 : 2;
    uint_least32_t __arr[k + 3];
    __q.generate(__arr + 0, __arr + k + 3);
    __y_u64 s = 0;
    for (size_t __j = 0; __j < k; ++__j)
      s |= static_cast<__y_u64>(__arr[__j + 3] & 0xffffffffu) << (32 * __j);
    s = reduce(s);
    __x_ = reduce(c) == 0 && s == 0 ? 1 : s;
  }

  friend bool operator==(const linear_congruential_engine& __x, const linear_congruential_engine& y) {
    return __x.__x_ == y.__x_;
  }

  result_type operator()() {
    __x_ = add(__mul(a, __x_), c);
    return static_cast<result_type>(__x_);
  }
  void discard(unsigned long long __z) {
    // x -> A x + C composed z times, by squaring: (A, C) o (A, C) = (A^2, A C + C).
    __y_u64 __ra = 1, __rc = 0, __pa = reduce(a), __pc = reduce(c);
    for (; __z != 0; __z >>= 1) {
      if (__z & 1) {
        __ra = __mul(__ra, __pa);
        __rc = add(__mul(__rc, __pa), __pc);
      }
      __pc = add(__mul(__pc, __pa), __pc);
      __pa = __mul(__pa, __pa);
    }
    __x_ = add(__mul(__ra, __x_), __rc);
  }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os,
                                                  const linear_congruential_engine& __x) {
    auto __st = __ycxx::__detail::__rand_out_engine(__os);
    __ycxx::__detail::__rand_put(__os, static_cast<result_type>(__x.__x_));
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, linear_congruential_engine& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    result_type __v{};
    if (__ycxx::__detail::__rand_get(is, __v)) {
      const __y_u64 __u = static_cast<__y_u64>(__v);
      if ((!__pow2 && __u >= __mod) || (reduce(c) == 0 && __u == 0)) // not a state the engine can be in
        is.setstate(basic_istream<__charT, __traits>::failbit);
      else
        __x.__x_ = __u;
    }
    return is;
  }

private:
  __y_u64 __x_;
};

// ---- [rand.eng.mers] ------------------------------------------------------------------------------
template <class _UIntType, size_t __w, size_t n, size_t m, size_t r, _UIntType a, size_t __u, _UIntType d, size_t s,
          _UIntType b, size_t t, _UIntType c, size_t __l, _UIntType __f>
class mersenne_twister_engine {
  static_assert(__ycxx::__detail::__rand_uint_type<_UIntType>,
                "mersenne_twister_engine: UIntType must be an unsigned integer type at least as wide as "
                "short ([rand.req.genl]/1.7)");
  static_assert(0 < m && m <= n && 2 * __u < __w && r <= __w && __u <= __w && s <= __w && t <= __w && __l <= __w &&
                    __w <= static_cast<size_t>(numeric_limits<_UIntType>::digits),
                "mersenne_twister_engine: invalid parameters ([rand.eng.mers]/4)");
  static constexpr __ycxx::__detail::__rand_u64 mask = __ycxx::__detail::__rand_mask(__w);
  static_assert(a <= mask && b <= mask && c <= mask && d <= mask && __f <= mask,
                "mersenne_twister_engine: a, b, c, d and f must be below 2^w");

  using __y_u64 = __ycxx::__detail::__rand_u64;

public:
  using result_type = _UIntType;

  static constexpr size_t word_size = __w;
  static constexpr size_t state_size = n;
  static constexpr size_t shift_size = m;
  static constexpr size_t mask_bits = r;
  static constexpr _UIntType xor_mask = a;
  static constexpr size_t tempering_u = __u;
  static constexpr _UIntType tempering_d = d;
  static constexpr size_t tempering_s = s;
  static constexpr _UIntType tempering_b = b;
  static constexpr size_t tempering_t = t;
  static constexpr _UIntType tempering_c = c;
  static constexpr size_t tempering_l = __l;
  static constexpr _UIntType initialization_multiplier = __f;
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return static_cast<result_type>(mask); }
  static constexpr result_type default_seed = 5489u;

  mersenne_twister_engine() : mersenne_twister_engine(default_seed) {}
  explicit mersenne_twister_engine(result_type value) { seed(value); }
  template <class _Sseq>
    requires __ycxx::__detail::__rand_seed_seq<_Sseq, result_type, mersenne_twister_engine>
  explicit mersenne_twister_engine(_Sseq& __q) {
    seed(__q);
  }
  void seed(result_type value = default_seed) {
    __x_[0] = static_cast<__y_u64>(value) & mask;
    for (size_t i = 1; i < n; ++i) {
      const __y_u64 p = __x_[i - 1];
      __x_[i] = (static_cast<__y_u64>(__f) * (p ^ __ycxx::__detail::__rand_shr(p, __w - 2)) + i) & mask;
    }
    __pos_ = 0;
  }
  template <class _Sseq>
    requires __ycxx::__detail::__rand_seed_seq<_Sseq, result_type, mersenne_twister_engine>
  void seed(_Sseq& __q) {
    constexpr size_t k = (__w + 31) / 32;
    uint_least32_t __arr[n * k];
    __q.generate(__arr + 0, __arr + n * k);
    for (size_t i = 0; i < n; ++i) {
      __y_u64 __v = 0;
      for (size_t __j = 0; __j < k; ++__j)
        __v |= static_cast<__y_u64>(__arr[k * i + __j] & 0xffffffffu) << (32 * __j);
      __x_[i] = __v & mask;
    }
    __pos_ = 0;
    bool zero = (__x_[0] & ~__ycxx::__detail::__rand_mask(r) & mask) == 0;
    for (size_t i = 1; zero && i < n; ++i)
      zero = __x_[i] == 0;
    if (zero)
      __x_[0] = 1ull << (__w - 1);
  }

  friend bool operator==(const mersenne_twister_engine& __x, const mersenne_twister_engine& y) {
    for (size_t i = 0, __px = __x.__pos_, __py = y.__pos_; i < n; ++i) {
      if (__x.__x_[__px] != y.__x_[__py])
        return false;
      __px = __px + 1 == n ? 0 : __px + 1;
      __py = __py + 1 == n ? 0 : __py + 1;
    }
    return true;
  }

  result_type operator()() {
    __y_u64 __z = __step();
    __z ^= __ycxx::__detail::__rand_shr(__z, __u) & d;
    __z ^= __ycxx::__detail::__rand_shl(__z, s) & b;
    __z ^= __ycxx::__detail::__rand_shl(__z, t) & c;
    __z ^= __ycxx::__detail::__rand_shr(__z, __l);
    return static_cast<result_type>(__z);
  }
  void discard(unsigned long long __z) {
    for (; __z != 0; --__z)
      __step();
  }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const mersenne_twister_engine& __x) {
    auto __st = __ycxx::__detail::__rand_out_engine(__os);
    for (size_t i = 0, p = __x.__pos_; i < n; ++i, p = p + 1 == n ? 0 : p + 1) {
      if (i != 0)
        __os.put(__os.widen(' '));
      __ycxx::__detail::__rand_put1(__os, static_cast<result_type>(__x.__x_[p]));
    }
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, mersenne_twister_engine& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    __y_u64 __tmp[n];
    for (size_t i = 0; i < n; ++i) {
      result_type __v{};
      if (!__ycxx::__detail::__rand_get(is, __v) || (static_cast<__y_u64>(__v) & ~mask) != 0) {
        is.setstate(basic_istream<__charT, __traits>::failbit);
        return is;
      }
      __tmp[i] = static_cast<__y_u64>(__v);
    }
    for (size_t i = 0; i < n; ++i)
      __x.__x_[i] = __tmp[i];
    __x.__pos_ = 0;
    return is;
  }

private:
  // One transition: X_i from X_{i-n} (at pos_), X_{i+1-n} and X_{i+m-n}; returns X_i.
  __y_u64 __step() {
    const size_t __p1 = __pos_ + 1 == n ? 0 : __pos_ + 1;
    const size_t __pm = __pos_ + m >= n ? __pos_ + m - n : __pos_ + m;
    constexpr __y_u64 lower = __ycxx::__detail::__rand_mask(r);
    const __y_u64 y = (__x_[__pos_] & ~lower & mask) | (__x_[__p1] & lower);
    const __y_u64 __v = __x_[__pm] ^ (y >> 1) ^ ((y & 1) ? static_cast<__y_u64>(a) : 0);
    __x_[__pos_] = __v;
    __pos_ = __p1;
    return __v;
  }

  __y_u64 __x_[n];
  size_t __pos_;
};

// ---- [rand.eng.sub] -------------------------------------------------------------------------------
template <class _UIntType, size_t __w, size_t s, size_t r>
class subtract_with_carry_engine {
  static_assert(__ycxx::__detail::__rand_uint_type<_UIntType>,
                "subtract_with_carry_engine: UIntType must be an unsigned integer type at least as wide as "
                "short ([rand.req.genl]/1.7)");
  static_assert(0u < s && s < r && 0 < __w && __w <= static_cast<size_t>(numeric_limits<_UIntType>::digits),
                "subtract_with_carry_engine: invalid parameters ([rand.eng.sub]/5)");
  using __y_u64 = __ycxx::__detail::__rand_u64;
  static constexpr __y_u64 mask = __ycxx::__detail::__rand_mask(__w);

public:
  using result_type = _UIntType;

  static constexpr size_t word_size = __w;
  static constexpr size_t short_lag = s;
  static constexpr size_t long_lag = r;
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return static_cast<result_type>(mask); }
  static constexpr uint_least32_t default_seed = 19780503u;

  subtract_with_carry_engine() : subtract_with_carry_engine(0u) {}
  explicit subtract_with_carry_engine(result_type value) { seed(value); }
  template <class _Sseq>
    requires __ycxx::__detail::__rand_seed_seq<_Sseq, result_type, subtract_with_carry_engine>
  explicit subtract_with_carry_engine(_Sseq& __q) {
    seed(__q);
  }
  void seed(result_type value = 0u) {
    linear_congruential_engine<uint_least32_t, 40014u, 0u, 2147483563u> e(
        value == 0u ? default_seed : static_cast<uint_least32_t>(value % 2147483563u));
    constexpr size_t k = (__w + 31) / 32;
    for (size_t i = 0; i < r; ++i) {
      __y_u64 __v = 0;
      for (size_t __j = 0; __j < k; ++__j)
        __v |= __ycxx::__detail::__rand_shl(static_cast<__y_u64>(e()), 32 * __j);
      __x_[i] = __v & mask;
    }
    __pos_ = 0;
    __carry_ = __x_[r - 1] == 0;
  }
  template <class _Sseq>
    requires __ycxx::__detail::__rand_seed_seq<_Sseq, result_type, subtract_with_carry_engine>
  void seed(_Sseq& __q) {
    constexpr size_t k = (__w + 31) / 32;
    uint_least32_t __arr[r * k];
    __q.generate(__arr + 0, __arr + r * k);
    for (size_t i = 0; i < r; ++i) {
      __y_u64 __v = 0;
      for (size_t __j = 0; __j < k; ++__j)
        __v |= static_cast<__y_u64>(__arr[k * i + __j] & 0xffffffffu) << (32 * __j);
      __x_[i] = __v & mask;
    }
    __pos_ = 0;
    __carry_ = __x_[r - 1] == 0;
  }

  friend bool operator==(const subtract_with_carry_engine& __x, const subtract_with_carry_engine& y) {
    if (__x.__carry_ != y.__carry_)
      return false;
    for (size_t i = 0, __px = __x.__pos_, __py = y.__pos_; i < r; ++i) {
      if (__x.__x_[__px] != y.__x_[__py])
        return false;
      __px = __px + 1 == r ? 0 : __px + 1;
      __py = __py + 1 == r ? 0 : __py + 1;
    }
    return true;
  }

  result_type operator()() {
    const size_t __ps = __pos_ >= s ? __pos_ - s : __pos_ + r - s; // X_{i-s}
    const __y_u64 __xs = __x_[__ps], __xr = __x_[__pos_];
    const bool __borrow = __xs < __xr || __xs - __xr < __carry_;
    const __y_u64 y = (__xs - __xr - __carry_) & mask;
    __carry_ = __borrow;
    __x_[__pos_] = y;
    __pos_ = __pos_ + 1 == r ? 0 : __pos_ + 1;
    return static_cast<result_type>(y);
  }
  void discard(unsigned long long __z) {
    for (; __z != 0; --__z)
      (*this)();
  }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os,
                                                  const subtract_with_carry_engine& __x) {
    auto __st = __ycxx::__detail::__rand_out_engine(__os);
    for (size_t i = 0, p = __x.__pos_; i < r; ++i, p = p + 1 == r ? 0 : p + 1) {
      __ycxx::__detail::__rand_put1(__os, static_cast<result_type>(__x.__x_[p]));
      __os.put(__os.widen(' '));
    }
    __ycxx::__detail::__rand_put1(__os, static_cast<unsigned>(__x.__carry_));
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, subtract_with_carry_engine& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    __y_u64 __tmp[r];
    for (size_t i = 0; i < r; ++i) {
      result_type __v{};
      if (!__ycxx::__detail::__rand_get(is, __v) || (static_cast<__y_u64>(__v) & ~mask) != 0) {
        is.setstate(basic_istream<__charT, __traits>::failbit);
        return is;
      }
      __tmp[i] = static_cast<__y_u64>(__v);
    }
    unsigned __cy = 0;
    if (!__ycxx::__detail::__rand_get(is, __cy) || __cy > 1) {
      is.setstate(basic_istream<__charT, __traits>::failbit);
      return is;
    }
    for (size_t i = 0; i < r; ++i)
      __x.__x_[i] = __tmp[i];
    __x.__pos_ = 0;
    __x.__carry_ = __cy;
    return is;
  }

private:
  __y_u64 __x_[r];
  size_t __pos_;
  __y_u64 __carry_;
};

// ---- [rand.eng.philox] ----------------------------------------------------------------------------
template <class _UIntType, size_t __w, size_t n, size_t r, _UIntType... __consts>
class philox_engine {
  static_assert(__ycxx::__detail::__rand_uint_type<_UIntType>,
                "philox_engine: UIntType must be an unsigned integer type at least as wide as short "
                "([rand.req.genl]/1.7)");
  static_assert(sizeof...(__consts) == n, "philox_engine: sizeof...(consts) == n is required");
  static_assert(n == 2 || n == 4, "philox_engine: n must be 2 or 4");
  static_assert(0 < r, "philox_engine: 0 < r is required");
  static_assert(0 < __w && __w <= static_cast<size_t>(numeric_limits<_UIntType>::digits),
                "philox_engine: 0 < w <= numeric_limits<UIntType>::digits is required");

  static constexpr size_t __array_size = n / 2;
  using __y_u64 = __ycxx::__detail::__rand_u64;
  static constexpr __y_u64 mask = __ycxx::__detail::__rand_mask(__w);
  static constexpr array<_UIntType, n> __all_consts{__consts...};

  static constexpr array<_UIntType, __array_size> __pick(size_t first) {
    array<_UIntType, __array_size> __res{};
    for (size_t k = 0; k < __array_size; ++k)
      __res[k] = __all_consts[2 * k + first];
    return __res;
  }

public:
  using result_type = _UIntType;

  static constexpr size_t word_size = __w;
  static constexpr size_t word_count = n;
  static constexpr size_t round_count = r;
  static constexpr array<result_type, __array_size> multipliers = __pick(0);
  static constexpr array<result_type, __array_size> round_consts = __pick(1);
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return static_cast<result_type>(mask); }
  static constexpr result_type default_seed = 20111115u;

  philox_engine() : philox_engine(default_seed) {}
  explicit philox_engine(result_type value) { seed(value); }
  template <class _Sseq>
    requires __ycxx::__detail::__rand_seed_seq<_Sseq, result_type, philox_engine>
  explicit philox_engine(_Sseq& __q) {
    seed(__q);
  }
  void seed(result_type value = default_seed) {
    for (size_t k = 0; k < __array_size; ++k)
      __k_[k] = 0;
    __k_[0] = static_cast<__y_u64>(value) & mask;
    __reset_counter();
  }
  template <class _Sseq>
    requires __ycxx::__detail::__rand_seed_seq<_Sseq, result_type, philox_engine>
  void seed(_Sseq& __q) {
    constexpr size_t p = (__w + 31) / 32;
    uint_least32_t __arr[__array_size * p];
    __q.generate(__arr + 0, __arr + __array_size * p);
    for (size_t k = 0; k < __array_size; ++k) {
      __y_u64 __v = 0;
      for (size_t __j = 0; __j < p; ++__j)
        __v |= static_cast<__y_u64>(__arr[k * p + __j] & 0xffffffffu) << (32 * __j);
      __k_[k] = __v & mask;
    }
    __reset_counter();
  }
  void set_counter(const array<result_type, n>& __counter) {
    for (size_t __j = 0; __j < n; ++__j)
      __x_[__j] = static_cast<__y_u64>(__counter[n - 1 - __j]) & mask;
    __i_ = n - 1;
  }

  friend bool operator==(const philox_engine& __x, const philox_engine& y) {
    // Y is a function of K and X whenever it is used (i < n - 1), so it need not be compared.
    for (size_t k = 0; k < __array_size; ++k)
      if (__x.__k_[k] != y.__k_[k])
        return false;
    for (size_t __j = 0; __j < n; ++__j)
      if (__x.__x_[__j] != y.__x_[__j])
        return false;
    return __x.__i_ == y.__i_;
  }

  result_type operator()() {
    if (++__i_ == n) {
      __generate_block();
      increment(1);
      __i_ = 0;
    }
    return static_cast<result_type>(__y_[__i_]);
  }
  void discard(unsigned long long __z) {
    // Position i + z: (i + z) / n new blocks, ending at index (i + z) mod n.
    const unsigned long long __zr = __z % n;
    const unsigned long long __blocks = __z / n + (__i_ + __zr) / n;
    __i_ = (__i_ + __zr) % n;
    if (__blocks != 0) {
      increment(__blocks - 1);
      __generate_block();
      increment(1);
    }
  }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const philox_engine& __x) {
    auto __st = __ycxx::__detail::__rand_out_engine(__os);
    for (size_t k = 0; k < __array_size; ++k) {
      __ycxx::__detail::__rand_put1(__os, static_cast<result_type>(__x.__k_[k]));
      __os.put(__os.widen(' '));
    }
    for (size_t __j = 0; __j < n; ++__j) {
      __ycxx::__detail::__rand_put1(__os, static_cast<result_type>(__x.__x_[__j]));
      __os.put(__os.widen(' '));
    }
    __ycxx::__detail::__rand_put1(__os, __x.__i_);
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, philox_engine& __x) {
    auto __st = __ycxx::__detail::__rand_in(is);
    philox_engine __tmp;
    for (size_t k = 0; k < __array_size + n; ++k) {
      result_type __v{};
      if (!__ycxx::__detail::__rand_get(is, __v) || (static_cast<__y_u64>(__v) & ~mask) != 0) {
        is.setstate(basic_istream<__charT, __traits>::failbit);
        return is;
      }
      (k < __array_size ? __tmp.__k_[k] : __tmp.__x_[k - __array_size]) = static_cast<__y_u64>(__v);
    }
    size_t i = 0;
    if (!__ycxx::__detail::__rand_get(is, i) || i >= n) {
      is.setstate(basic_istream<__charT, __traits>::failbit);
      return is;
    }
    __tmp.__i_ = i;
    if (i != n - 1) { // rebuild Y = Philox(K, Z - 1)
      __tmp.__decrement();
      __tmp.__generate_block();
      __tmp.increment(1);
    }
    __x = __tmp;
    return is;
  }

private:
  void __reset_counter() {
    for (size_t __j = 0; __j < n; ++__j) {
      __x_[__j] = 0;
      __y_[__j] = 0;
    }
    __i_ = n - 1;
  }
  // Z += by, modulo 2^(n w).
  void increment(__y_u64 __by) {
    for (size_t __j = 0; __j < n && __by != 0; ++__j) {
      const __y_u64 sum = (__x_[__j] + (__by & mask)) ;
      __y_u64 __carry = __ycxx::__detail::__rand_shr(__by, __w);
      if constexpr (__w == 64)
        __carry += sum < __x_[__j];
      else
        __carry += sum >> __w;
      __x_[__j] = sum & mask;
      __by = __carry;
    }
  }
  // Z -= 1, modulo 2^(n w).
  void __decrement() {
    for (size_t __j = 0; __j < n; ++__j) {
      const bool __borrow = __x_[__j] == 0;
      __x_[__j] = (__x_[__j] - 1) & mask;
      if (!__borrow)
        return;
    }
  }
  static void __mulhilo(__y_u64 a, __y_u64 b, __y_u64& __hi, __y_u64& __lo) {
    if constexpr (__w <= 32) {
      const __y_u64 p = a * b;
      __hi = p >> __w;
      __lo = p & mask;
    } else {
      const __ycxx::__detail::__rand_u128 p = __ycxx::__detail::__rand_mul_wide(a, b);
      if constexpr (__w == 64) {
        __hi = p.__hi;
        __lo = p.__lo;
      } else {
        __hi = (p.__hi << (64 - __w)) | (p.__lo >> __w);
        __lo = p.__lo & mask;
      }
    }
  }
  // Y = Philox(K, X) ([rand.eng.philox]/4).
  void __generate_block() {
    __y_u64 __v[n];
    for (size_t __j = 0; __j < n; ++__j)
      __v[__j] = __x_[__j];
    for (size_t __q = 0; __q < r; ++__q) {
      __y_u64 __perm[n];
      if constexpr (n == 2) {
        __perm[0] = __v[0];
        __perm[1] = __v[1];
      } else {
        __perm[0] = __v[2];
        __perm[1] = __v[1];
        __perm[2] = __v[0];
        __perm[3] = __v[3];
      }
      for (size_t k = 0; k < __array_size; ++k) {
        const __y_u64 key = (__k_[k] + static_cast<__y_u64>(__q) * static_cast<__y_u64>(round_consts[k])) & mask;
        __y_u64 __hi, __lo;
        __mulhilo(__perm[2 * k], static_cast<__y_u64>(multipliers[k]), __hi, __lo);
        __v[2 * k] = __hi ^ key ^ __perm[2 * k + 1];
        __v[2 * k + 1] = __lo;
      }
    }
    for (size_t __j = 0; __j < n; ++__j)
      __y_[__j] = __v[__j];
  }

  __y_u64 __x_[n];          // the counter Z, least significant word first
  __y_u64 __k_[__array_size]; // the keys
  __y_u64 __y_[n];          // the current block of output
  size_t __i_;          // index of the last value returned from y_
};

// ---- [rand.adapt.disc] ----------------------------------------------------------------------------
template <class _Engine, size_t p, size_t r>
class discard_block_engine {
  static_assert(!is_const_v<_Engine> && !is_volatile_v<_Engine>,
                "discard_block_engine: Engine must not be cv-qualified ([rand.req.genl]/1.1)");
  static_assert(0 < r && r <= p, "discard_block_engine: requires 0 < r <= p");

public:
  using result_type = _Engine::result_type;

  static constexpr size_t block_size = p;
  static constexpr size_t used_block = r;
  static constexpr result_type min() { return _Engine::min(); }
  static constexpr result_type max() { return _Engine::max(); }

  discard_block_engine() : __e_(), __n_(0) {}
  explicit discard_block_engine(const _Engine& e) : __e_(e), __n_(0) {}
  explicit discard_block_engine(_Engine&& e) : __e_(std::move(e)), __n_(0) {}
  explicit discard_block_engine(result_type s) : __e_(s), __n_(0) {}
  template <class _Sseq>
    requires __ycxx::__detail::__rand_seed_seq<_Sseq, result_type, discard_block_engine, _Engine>
  explicit discard_block_engine(_Sseq& __q) : __e_(__q), __n_(0) {}
  void seed() {
    __e_.seed();
    __n_ = 0;
  }
  void seed(result_type s) {
    __e_.seed(s);
    __n_ = 0;
  }
  template <class _Sseq>
    requires __ycxx::__detail::__rand_seed_seq<_Sseq, result_type, discard_block_engine, _Engine>
  void seed(_Sseq& __q) {
    __e_.seed(__q);
    __n_ = 0;
  }

  friend bool operator==(const discard_block_engine& __x, const discard_block_engine& y) {
    return __x.__n_ == y.__n_ && __x.__e_ == y.__e_;
  }

  result_type operator()() {
    if (__n_ >= r) {
      __e_.discard(static_cast<unsigned long long>(p - r));
      __n_ = 0;
    }
    ++__n_;
    return __e_();
  }
  void discard(unsigned long long __z) {
    for (; __z != 0; --__z)
      (*this)();
  }

  const _Engine& base() const noexcept { return __e_; }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const discard_block_engine& __x) {
    __os << __x.__e_;
    auto __st = __ycxx::__detail::__rand_out_engine(__os);
    __os.put(__os.widen(' '));
    __ycxx::__detail::__rand_put1(__os, __x.__n_);
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, discard_block_engine& __x) {
    _Engine e = __x.__e_;
    is >> e;
    auto __st = __ycxx::__detail::__rand_in(is);
    size_t n = 0;
    if (!is.fail() && __ycxx::__detail::__rand_get(is, n)) {
      if (n > r) {
        is.setstate(basic_istream<__charT, __traits>::failbit);
      } else {
        __x.__e_ = std::move(e);
        __x.__n_ = n;
      }
    }
    return is;
  }

private:
  _Engine __e_;
  size_t __n_;
};

// ---- [rand.adapt.ibits] ---------------------------------------------------------------------------
template <class _Engine, size_t __w, class _UIntType>
class independent_bits_engine {
  static_assert(!is_const_v<_Engine> && !is_volatile_v<_Engine>,
                "independent_bits_engine: Engine must not be cv-qualified ([rand.req.genl]/1.1)");
  static_assert(__ycxx::__detail::__rand_uint_type<_UIntType>,
                "independent_bits_engine: UIntType must be an unsigned integer type at least as wide as "
                "short ([rand.req.genl]/1.7)");
  static_assert(0 < __w && __w <= static_cast<size_t>(numeric_limits<_UIntType>::digits),
                "independent_bits_engine: requires 0 < w <= numeric_limits<result_type>::digits");

  using __y_u64 = __ycxx::__detail::__rand_u64;
  using __base_result = _Engine::result_type;
  static constexpr __y_u64 __emin = static_cast<__y_u64>(_Engine::min());
  static constexpr __y_u64 __rm1 = static_cast<__y_u64>(_Engine::max()) - __emin; // R - 1
  static constexpr bool __full = __rm1 == ~0ull;                           // R = 2^64
  // [rand.adapt.ibits]/2: m = floor(log2 R), n, w0, n0, y0 and y1. When R is a power of two
  // (always, if R = 2^64), no value is ever rejected.
  static constexpr size_t __m_bits = __full ? 64 : static_cast<size_t>(std::bit_width(__rm1 + 1)) - 1;
  struct __params {
    size_t n, __w0, __n0;
    __y_u64 __y0, __y1;      // meaningful only when !pow2
    bool __pow2;
  };
  static constexpr __params __compute() {
    __params __pr{};
    __pr.__pow2 = __full || ((__rm1 + 1) & __rm1) == 0;
    const __y_u64 _Rp = __rm1 + 1;
    auto __with_n = [&](size_t __nn) {
      __pr.n = __nn;
      __pr.__w0 = __w / __nn;
      __pr.__n0 = __nn - __w % __nn;
      if (!__pr.__pow2) {
        __pr.__y0 = (_Rp >> __pr.__w0) << __pr.__w0;
        __pr.__y1 = __ycxx::__detail::__rand_shl(__ycxx::__detail::__rand_shr(_Rp, __pr.__w0 + 1), __pr.__w0 + 1);
      }
    };
    const size_t __ceil_wm = (__w + __m_bits - 1) / __m_bits;
    __with_n(__ceil_wm);
    if (!__pr.__pow2 && _Rp - __pr.__y0 > __pr.__y0 / __pr.n)
      __with_n(__ceil_wm + 1);
    return __pr;
  }
  static constexpr __params _Pp = __compute();

public:
  using result_type = _UIntType;

  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return static_cast<result_type>(__ycxx::__detail::__rand_mask(__w)); }

  independent_bits_engine() : __e_() {}
  explicit independent_bits_engine(const _Engine& e) : __e_(e) {}
  explicit independent_bits_engine(_Engine&& e) : __e_(std::move(e)) {}
  explicit independent_bits_engine(result_type s) : __e_(s) {}
  template <class _Sseq>
    requires __ycxx::__detail::__rand_seed_seq<_Sseq, result_type, independent_bits_engine, _Engine>
  explicit independent_bits_engine(_Sseq& __q) : __e_(__q) {}
  void seed() { __e_.seed(); }
  void seed(result_type s) { __e_.seed(s); }
  template <class _Sseq>
    requires __ycxx::__detail::__rand_seed_seq<_Sseq, result_type, independent_bits_engine, _Engine>
  void seed(_Sseq& __q) {
    __e_.seed(__q);
  }

  friend bool operator==(const independent_bits_engine& __x, const independent_bits_engine& y) { return __x.__e_ == y.__e_; }

  result_type operator()() {
    __y_u64 _Sp = 0;
    for (size_t k = 0; k != _Pp.__n0; ++k) {
      __y_u64 __u;
      do
        __u = static_cast<__y_u64>(__e_()) - __emin;
      while (!_Pp.__pow2 && __u >= _Pp.__y0);
      _Sp = __ycxx::__detail::__rand_shl(_Sp, _Pp.__w0) + (__u & __ycxx::__detail::__rand_mask(_Pp.__w0));
    }
    for (size_t k = _Pp.__n0; k != _Pp.n; ++k) {
      __y_u64 __u;
      do
        __u = static_cast<__y_u64>(__e_()) - __emin;
      while (!_Pp.__pow2 && __u >= _Pp.__y1);
      _Sp = __ycxx::__detail::__rand_shl(_Sp, _Pp.__w0 + 1) + (__u & __ycxx::__detail::__rand_mask(_Pp.__w0 + 1));
    }
    return static_cast<result_type>(_Sp);
  }
  void discard(unsigned long long __z) {
    for (; __z != 0; --__z)
      (*this)();
  }

  const _Engine& base() const noexcept { return __e_; }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os,
                                                  const independent_bits_engine& __x) {
    return __os << __x.__e_;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, independent_bits_engine& __x) {
    return is >> __x.__e_;
  }

private:
  _Engine __e_;
};

// ---- [rand.adapt.shuf] ----------------------------------------------------------------------------
template <class _Engine, size_t k>
class shuffle_order_engine {
  static_assert(!is_const_v<_Engine> && !is_volatile_v<_Engine>,
                "shuffle_order_engine: Engine must not be cv-qualified ([rand.req.genl]/1.1)");
  static_assert(0 < k, "shuffle_order_engine: requires 0 < k");
  using __y_u64 = __ycxx::__detail::__rand_u64;

public:
  using result_type = _Engine::result_type;

  static constexpr size_t table_size = k;
  static constexpr result_type min() { return _Engine::min(); }
  static constexpr result_type max() { return _Engine::max(); }

  shuffle_order_engine() : __e_() { init(); }
  explicit shuffle_order_engine(const _Engine& e) : __e_(e) { init(); }
  explicit shuffle_order_engine(_Engine&& e) : __e_(std::move(e)) { init(); }
  explicit shuffle_order_engine(result_type s) : __e_(s) { init(); }
  template <class _Sseq>
    requires __ycxx::__detail::__rand_seed_seq<_Sseq, result_type, shuffle_order_engine, _Engine>
  explicit shuffle_order_engine(_Sseq& __q) : __e_(__q) {
    init();
  }
  void seed() {
    __e_.seed();
    init();
  }
  void seed(result_type s) {
    __e_.seed(s);
    init();
  }
  template <class _Sseq>
    requires __ycxx::__detail::__rand_seed_seq<_Sseq, result_type, shuffle_order_engine, _Engine>
  void seed(_Sseq& __q) {
    __e_.seed(__q);
    init();
  }

  friend bool operator==(const shuffle_order_engine& __x, const shuffle_order_engine& y) {
    if (__x.__y_ != y.__y_)
      return false;
    for (size_t i = 0; i < k; ++i)
      if (__x.__v_[i] != y.__v_[i])
        return false;
    return __x.__e_ == y.__e_;
  }

  result_type operator()() {
    // j = floor(k (Y - emin) / (emax - emin + 1)).
    constexpr __y_u64 __emin = static_cast<__y_u64>(_Engine::min());
    constexpr __y_u64 __rm1 = static_cast<__y_u64>(_Engine::max()) - __emin;
    const __y_u64 y = static_cast<__y_u64>(__y_) - __emin;
    size_t __j;
    if constexpr (__rm1 == ~0ull) {
      __j = static_cast<size_t>(__ycxx::__detail::__rand_mul_wide(y, k).__hi);
    } else if constexpr (k <= ~0ull / (__rm1 + 1)) {
      __j = static_cast<size_t>(k * y / (__rm1 + 1));
    } else {
      // k (Y - emin) needs more than 64 bits: divide the 128-bit product.
      const __ycxx::__detail::__rand_u128 __pr = __ycxx::__detail::__rand_mul_wide(y, k);
      __ycxx::__detail::__rand_big num, den;
      num.__w[0] = __pr.__lo;
      num.__w[1] = __pr.__hi;
      den.__w[0] = __rm1 + 1;
      __j = static_cast<size_t>(num.div(den).__w[0]);
    }
    __y_ = __v_[__j];
    __v_[__j] = __e_();
    return __y_;
  }
  void discard(unsigned long long __z) {
    for (; __z != 0; --__z)
      (*this)();
  }

  const _Engine& base() const noexcept { return __e_; }

  template <class __charT, class __traits>
  friend basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, const shuffle_order_engine& __x) {
    __os << __x.__e_;
    auto __st = __ycxx::__detail::__rand_out_engine(__os);
    for (size_t i = 0; i < k; ++i)
      __ycxx::__detail::__rand_put_sep(__os, __x.__v_[i]);
    __ycxx::__detail::__rand_put_sep(__os, __x.__y_);
    return __os;
  }
  template <class __charT, class __traits>
  friend basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, shuffle_order_engine& __x) {
    _Engine e = __x.__e_;
    is >> e;
    if (is.fail())
      return is;
    auto __st = __ycxx::__detail::__rand_in(is);
    result_type __v[k];
    result_type y{};
    for (size_t i = 0; i < k; ++i)
      if (!__ycxx::__detail::__rand_get(is, __v[i]))
        return is;
    if (!__ycxx::__detail::__rand_get(is, y))
      return is;
    if (y < min() || y > max()) {
      is.setstate(basic_istream<__charT, __traits>::failbit);
      return is;
    }
    __x.__e_ = std::move(e);
    for (size_t i = 0; i < k; ++i)
      __x.__v_[i] = __v[i];
    __x.__y_ = y;
    return is;
  }

private:
  void init() {
    for (size_t i = 0; i < k; ++i)
      __v_[i] = __e_();
    __y_ = __e_();
  }

  _Engine __e_;
  result_type __v_[k];
  result_type __y_;
};

// ---- [rand.predef] --------------------------------------------------------------------------------
using minstd_rand0 = linear_congruential_engine<uint_fast32_t, 16'807, 0, 2'147'483'647>;
using minstd_rand = linear_congruential_engine<uint_fast32_t, 48'271, 0, 2'147'483'647>;
using mt19937 = mersenne_twister_engine<uint_fast32_t, 32, 624, 397, 31, 0x9908'b0df, 11, 0xffff'ffff, 7, 0x9d2c'5680,
                                        15, 0xefc6'0000, 18, 1'812'433'253>;
using mt19937_64 =
    mersenne_twister_engine<uint_fast64_t, 64, 312, 156, 31, 0xb502'6f5a'a966'19e9, 29, 0x5555'5555'5555'5555, 17,
                            0x71d6'7fff'eda6'0000, 37, 0xfff7'eee0'0000'0000, 43, 6'364'136'223'846'793'005>;
using ranlux24_base = subtract_with_carry_engine<uint_fast32_t, 24, 10, 24>;
using ranlux48_base = subtract_with_carry_engine<uint_fast64_t, 48, 5, 12>;
using ranlux24 = discard_block_engine<ranlux24_base, 223, 23>;
using ranlux48 = discard_block_engine<ranlux48_base, 389, 11>;
using knuth_b = shuffle_order_engine<minstd_rand0, 256>;
using philox4x32 = philox_engine<uint_fast32_t, 32, 4, 10, 0xCD9E8D57, 0x9E3779B9, 0xD2511F53, 0xBB67AE85>;
using philox4x64 = philox_engine<uint_fast64_t, 64, 4, 10, 0xCA5A826395121157, 0x9E3779B97F4A7C15, 0xD2E7470EE14C6C93,
                                 0xBB67AE8584CAA73B>;
// Implementation-defined ([rand.predef]/10): the 32-bit Mersenne twister, a good general-purpose
// choice.
using default_random_engine = mt19937;

} // namespace std
