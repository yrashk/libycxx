// libycxx core: the random number engines and engine adaptors of <random> ([rand.eng],
// [rand.adapt], [rand.predef]) and seed_seq ([rand.util.seedseq]).
//
// Every engine keeps its state so that the textual representation and equality follow the
// draft's sequence X directly: the lagged engines (mersenne_twister_engine,
// subtract_with_carry_engine) keep X in a ring buffer whose oldest element is at `pos_`, and
// compare and print it in logical order. All arithmetic is done in unsigned long long and
// reduced modulo 2^w (or m), so narrow UIntTypes never meet integral promotion.
#pragma once

#include <initializer_list>
#include <ycxx/core/array.hpp>
#include <ycxx/core/iterator_core.hpp>
#include <ycxx/core/move.hpp>
#include <ycxx/core/random_base.hpp>
#include <ycxx/core/vector.hpp>

namespace [[gnu::visibility("hidden")]] std {

// ---- [rand.util.seedseq] ------------------------------------------------------------------------
class seed_seq {
public:
  using result_type = uint_least32_t;

  seed_seq() noexcept {}
  template <class T>
    requires is_integral_v<T>
  seed_seq(initializer_list<T> il) : seed_seq(il.begin(), il.end()) {}
  template <class InputIterator>
  seed_seq(InputIterator begin, InputIterator end) {
    static_assert(is_integral_v<typename iterator_traits<InputIterator>::value_type>,
                  "seed_seq: the iterator's value_type must be an integer type");
    for (InputIterator s = begin; s != end; ++s)
      v_.push_back(static_cast<result_type>(static_cast<uint32_t>(*s)));
  }

  template <class RandomAccessIterator>
  void generate(RandomAccessIterator begin, RandomAccessIterator end) {
    using V = typename iterator_traits<RandomAccessIterator>::value_type;
    static_assert(is_unsigned_v<V> && !is_same_v<V, bool> && numeric_limits<V>::digits >= 32,
                  "seed_seq::generate: the iterator's value_type must be an unsigned integer type of at "
                  "least 32 bits");
    if (begin == end)
      return;
    using D = typename iterator_traits<RandomAccessIterator>::difference_type;
    const size_t s = v_.size(), n = static_cast<size_t>(end - begin);
    auto at = [&begin, n](size_t k) -> decltype(auto) { return begin[static_cast<D>(k % n)]; };
    auto get = [&at](size_t k) { return static_cast<uint32_t>(at(k)); };
    auto tf = [](uint32_t x) { return x ^ (x >> 27); };
    for (RandomAccessIterator i = begin; i != end; ++i)
      *i = V(0x8b8b8b8bu);
    const size_t t = n >= 623 ? 11 : n >= 68 ? 7 : n >= 39 ? 5 : n >= 7 ? 3 : (n - 1) / 2;
    const size_t p = (n - t) / 2, q = p + t;
    const size_t m = s + 1 > n ? s + 1 : n;
    for (size_t k = 0; k < m; ++k) {
      const uint32_t r1 = 1664525u * tf(get(k) ^ get(k + p) ^ get(k + n - 1));
      const uint32_t r2 =
          r1 + (k == 0 ? static_cast<uint32_t>(s) : static_cast<uint32_t>(k % n) + (k <= s ? v_[k - 1] : 0u));
      at(k + p) = V(static_cast<uint32_t>(get(k + p) + r1));
      at(k + q) = V(static_cast<uint32_t>(get(k + q) + r2));
      at(k) = V(r2);
    }
    for (size_t k = m; k < m + n; ++k) {
      const uint32_t r3 = 1566083941u * tf(get(k) + get(k + p) + get(k + n - 1));
      const uint32_t r4 = r3 - static_cast<uint32_t>(k % n);
      at(k + p) = V(get(k + p) ^ r3);
      at(k + q) = V(get(k + q) ^ r4);
      at(k) = V(r4);
    }
  }

  size_t size() const noexcept { return v_.size(); }
  template <class OutputIterator>
  void param(OutputIterator dest) const {
    static_assert(requires(OutputIterator& o, const result_type& r) { *o = r; },
                  "seed_seq::param: values of result_type must be writable to dest");
    for (result_type x : v_) {
      *dest = x;
      ++dest;
    }
  }

  seed_seq(const seed_seq&) = delete;
  void operator=(const seed_seq&) = delete;

private:
  vector<result_type> v_;
};

// ---- [rand.eng.lcong] -----------------------------------------------------------------------------
template <class UIntType, UIntType a, UIntType c, UIntType m>
class linear_congruential_engine {
  static_assert(ycxx::detail::rand_uint_type<UIntType>,
                "linear_congruential_engine: UIntType must be an unsigned integer type at least as wide as "
                "short ([rand.req.genl]/1.7)");
  static_assert(m == 0 || (a < m && c < m), "linear_congruential_engine: requires a < m and c < m");

  using u64 = ycxx::detail::rand_u64;
  static constexpr int digits = numeric_limits<UIntType>::digits;
  // m == 0 stands for 2^digits: arithmetic in u64 wraps and is then masked.
  static constexpr bool pow2 = m == 0;
  static constexpr u64 mod = static_cast<u64>(m); // when !pow2
  static constexpr u64 reduce(u64 x) noexcept { return pow2 ? x & ycxx::detail::rand_mask(digits) : x % mod; }
  static constexpr u64 mul(u64 x, u64 y) noexcept {
    if constexpr (pow2)
      return reduce(x * y);
    else
      return ycxx::detail::rand_mulmod(x, y, mod);
  }
  static constexpr u64 add(u64 x, u64 y) noexcept {
    if constexpr (pow2)
      return reduce(x + y);
    else
      return ycxx::detail::rand_addmod(x, y, mod);
  }

public:
  using result_type = UIntType;

  static constexpr result_type multiplier = a;
  static constexpr result_type increment = c;
  static constexpr result_type modulus = m;
  static constexpr result_type min() { return c == 0u ? 1u : 0u; }
  static constexpr result_type max() { return m - 1u; }
  static constexpr result_type default_seed = 1u;

  linear_congruential_engine() : linear_congruential_engine(default_seed) {}
  explicit linear_congruential_engine(result_type s) { seed(s); }
  template <class Sseq>
    requires ycxx::detail::rand_seed_seq<Sseq, result_type, linear_congruential_engine>
  explicit linear_congruential_engine(Sseq& q) {
    seed(q);
  }
  void seed(result_type s = default_seed) {
    const u64 sm = reduce(s);
    x_ = reduce(c) == 0 && sm == 0 ? 1 : sm;
  }
  template <class Sseq>
    requires ycxx::detail::rand_seed_seq<Sseq, result_type, linear_congruential_engine>
  void seed(Sseq& q) {
    // k = ceil(log2(m) / 32): the smallest k with 2^(32k) >= m.
    constexpr size_t k = pow2 ? (static_cast<size_t>(digits) + 31) / 32 : mod <= 1 ? 0 : mod - 1 <= 0xffffffffu ? 1 : 2;
    uint_least32_t arr[k + 3];
    q.generate(arr + 0, arr + k + 3);
    u64 s = 0;
    for (size_t j = 0; j < k; ++j)
      s |= static_cast<u64>(arr[j + 3] & 0xffffffffu) << (32 * j);
    s = reduce(s);
    x_ = reduce(c) == 0 && s == 0 ? 1 : s;
  }

  friend bool operator==(const linear_congruential_engine& x, const linear_congruential_engine& y) {
    return x.x_ == y.x_;
  }

  result_type operator()() {
    x_ = add(mul(a, x_), c);
    return static_cast<result_type>(x_);
  }
  void discard(unsigned long long z) {
    // x -> A x + C composed z times, by squaring: (A, C) o (A, C) = (A^2, A C + C).
    u64 ra = 1, rc = 0, pa = reduce(a), pc = reduce(c);
    for (; z != 0; z >>= 1) {
      if (z & 1) {
        ra = mul(ra, pa);
        rc = add(mul(rc, pa), pc);
      }
      pc = add(mul(pc, pa), pc);
      pa = mul(pa, pa);
    }
    x_ = add(mul(ra, x_), rc);
  }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os,
                                                  const linear_congruential_engine& x) {
    auto st = ycxx::detail::rand_out_engine(os);
    ycxx::detail::rand_put(os, static_cast<result_type>(x.x_));
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, linear_congruential_engine& x) {
    auto st = ycxx::detail::rand_in(is);
    result_type v{};
    if (ycxx::detail::rand_get(is, v)) {
      const u64 u = static_cast<u64>(v);
      if ((!pow2 && u >= mod) || (reduce(c) == 0 && u == 0)) // not a state the engine can be in
        is.setstate(basic_istream<charT, traits>::failbit);
      else
        x.x_ = u;
    }
    return is;
  }

private:
  u64 x_;
};

// ---- [rand.eng.mers] ------------------------------------------------------------------------------
template <class UIntType, size_t w, size_t n, size_t m, size_t r, UIntType a, size_t u, UIntType d, size_t s,
          UIntType b, size_t t, UIntType c, size_t l, UIntType f>
class mersenne_twister_engine {
  static_assert(ycxx::detail::rand_uint_type<UIntType>,
                "mersenne_twister_engine: UIntType must be an unsigned integer type at least as wide as "
                "short ([rand.req.genl]/1.7)");
  static_assert(0 < m && m <= n && 2 * u < w && r <= w && u <= w && s <= w && t <= w && l <= w &&
                    w <= static_cast<size_t>(numeric_limits<UIntType>::digits),
                "mersenne_twister_engine: invalid parameters ([rand.eng.mers]/4)");
  static constexpr ycxx::detail::rand_u64 mask = ycxx::detail::rand_mask(w);
  static_assert(a <= mask && b <= mask && c <= mask && d <= mask && f <= mask,
                "mersenne_twister_engine: a, b, c, d and f must be below 2^w");

  using u64 = ycxx::detail::rand_u64;

public:
  using result_type = UIntType;

  static constexpr size_t word_size = w;
  static constexpr size_t state_size = n;
  static constexpr size_t shift_size = m;
  static constexpr size_t mask_bits = r;
  static constexpr UIntType xor_mask = a;
  static constexpr size_t tempering_u = u;
  static constexpr UIntType tempering_d = d;
  static constexpr size_t tempering_s = s;
  static constexpr UIntType tempering_b = b;
  static constexpr size_t tempering_t = t;
  static constexpr UIntType tempering_c = c;
  static constexpr size_t tempering_l = l;
  static constexpr UIntType initialization_multiplier = f;
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return static_cast<result_type>(mask); }
  static constexpr result_type default_seed = 5489u;

  mersenne_twister_engine() : mersenne_twister_engine(default_seed) {}
  explicit mersenne_twister_engine(result_type value) { seed(value); }
  template <class Sseq>
    requires ycxx::detail::rand_seed_seq<Sseq, result_type, mersenne_twister_engine>
  explicit mersenne_twister_engine(Sseq& q) {
    seed(q);
  }
  void seed(result_type value = default_seed) {
    x_[0] = static_cast<u64>(value) & mask;
    for (size_t i = 1; i < n; ++i) {
      const u64 p = x_[i - 1];
      x_[i] = (static_cast<u64>(f) * (p ^ ycxx::detail::rand_shr(p, w - 2)) + i) & mask;
    }
    pos_ = 0;
  }
  template <class Sseq>
    requires ycxx::detail::rand_seed_seq<Sseq, result_type, mersenne_twister_engine>
  void seed(Sseq& q) {
    constexpr size_t k = (w + 31) / 32;
    uint_least32_t arr[n * k];
    q.generate(arr + 0, arr + n * k);
    for (size_t i = 0; i < n; ++i) {
      u64 v = 0;
      for (size_t j = 0; j < k; ++j)
        v |= static_cast<u64>(arr[k * i + j] & 0xffffffffu) << (32 * j);
      x_[i] = v & mask;
    }
    pos_ = 0;
    bool zero = (x_[0] & ~ycxx::detail::rand_mask(r) & mask) == 0;
    for (size_t i = 1; zero && i < n; ++i)
      zero = x_[i] == 0;
    if (zero)
      x_[0] = 1ull << (w - 1);
  }

  friend bool operator==(const mersenne_twister_engine& x, const mersenne_twister_engine& y) {
    for (size_t i = 0, px = x.pos_, py = y.pos_; i < n; ++i) {
      if (x.x_[px] != y.x_[py])
        return false;
      px = px + 1 == n ? 0 : px + 1;
      py = py + 1 == n ? 0 : py + 1;
    }
    return true;
  }

  result_type operator()() {
    u64 z = step();
    z ^= ycxx::detail::rand_shr(z, u) & d;
    z ^= ycxx::detail::rand_shl(z, s) & b;
    z ^= ycxx::detail::rand_shl(z, t) & c;
    z ^= ycxx::detail::rand_shr(z, l);
    return static_cast<result_type>(z);
  }
  void discard(unsigned long long z) {
    for (; z != 0; --z)
      step();
  }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const mersenne_twister_engine& x) {
    auto st = ycxx::detail::rand_out_engine(os);
    for (size_t i = 0, p = x.pos_; i < n; ++i, p = p + 1 == n ? 0 : p + 1) {
      if (i != 0)
        os.put(os.widen(' '));
      ycxx::detail::rand_put1(os, static_cast<result_type>(x.x_[p]));
    }
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, mersenne_twister_engine& x) {
    auto st = ycxx::detail::rand_in(is);
    u64 tmp[n];
    for (size_t i = 0; i < n; ++i) {
      result_type v{};
      if (!ycxx::detail::rand_get(is, v) || (static_cast<u64>(v) & ~mask) != 0) {
        is.setstate(basic_istream<charT, traits>::failbit);
        return is;
      }
      tmp[i] = static_cast<u64>(v);
    }
    for (size_t i = 0; i < n; ++i)
      x.x_[i] = tmp[i];
    x.pos_ = 0;
    return is;
  }

private:
  // One transition: X_i from X_{i-n} (at pos_), X_{i+1-n} and X_{i+m-n}; returns X_i.
  u64 step() {
    const size_t p1 = pos_ + 1 == n ? 0 : pos_ + 1;
    const size_t pm = pos_ + m >= n ? pos_ + m - n : pos_ + m;
    constexpr u64 lower = ycxx::detail::rand_mask(r);
    const u64 y = (x_[pos_] & ~lower & mask) | (x_[p1] & lower);
    const u64 v = x_[pm] ^ (y >> 1) ^ ((y & 1) ? static_cast<u64>(a) : 0);
    x_[pos_] = v;
    pos_ = p1;
    return v;
  }

  u64 x_[n];
  size_t pos_;
};

// ---- [rand.eng.sub] -------------------------------------------------------------------------------
template <class UIntType, size_t w, size_t s, size_t r>
class subtract_with_carry_engine {
  static_assert(ycxx::detail::rand_uint_type<UIntType>,
                "subtract_with_carry_engine: UIntType must be an unsigned integer type at least as wide as "
                "short ([rand.req.genl]/1.7)");
  static_assert(0u < s && s < r && 0 < w && w <= static_cast<size_t>(numeric_limits<UIntType>::digits),
                "subtract_with_carry_engine: invalid parameters ([rand.eng.sub]/5)");
  using u64 = ycxx::detail::rand_u64;
  static constexpr u64 mask = ycxx::detail::rand_mask(w);

public:
  using result_type = UIntType;

  static constexpr size_t word_size = w;
  static constexpr size_t short_lag = s;
  static constexpr size_t long_lag = r;
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return static_cast<result_type>(mask); }
  static constexpr uint_least32_t default_seed = 19780503u;

  subtract_with_carry_engine() : subtract_with_carry_engine(0u) {}
  explicit subtract_with_carry_engine(result_type value) { seed(value); }
  template <class Sseq>
    requires ycxx::detail::rand_seed_seq<Sseq, result_type, subtract_with_carry_engine>
  explicit subtract_with_carry_engine(Sseq& q) {
    seed(q);
  }
  void seed(result_type value = 0u) {
    linear_congruential_engine<uint_least32_t, 40014u, 0u, 2147483563u> e(
        value == 0u ? default_seed : static_cast<uint_least32_t>(value % 2147483563u));
    constexpr size_t k = (w + 31) / 32;
    for (size_t i = 0; i < r; ++i) {
      u64 v = 0;
      for (size_t j = 0; j < k; ++j)
        v |= ycxx::detail::rand_shl(static_cast<u64>(e()), 32 * j);
      x_[i] = v & mask;
    }
    pos_ = 0;
    carry_ = x_[r - 1] == 0;
  }
  template <class Sseq>
    requires ycxx::detail::rand_seed_seq<Sseq, result_type, subtract_with_carry_engine>
  void seed(Sseq& q) {
    constexpr size_t k = (w + 31) / 32;
    uint_least32_t arr[r * k];
    q.generate(arr + 0, arr + r * k);
    for (size_t i = 0; i < r; ++i) {
      u64 v = 0;
      for (size_t j = 0; j < k; ++j)
        v |= static_cast<u64>(arr[k * i + j] & 0xffffffffu) << (32 * j);
      x_[i] = v & mask;
    }
    pos_ = 0;
    carry_ = x_[r - 1] == 0;
  }

  friend bool operator==(const subtract_with_carry_engine& x, const subtract_with_carry_engine& y) {
    if (x.carry_ != y.carry_)
      return false;
    for (size_t i = 0, px = x.pos_, py = y.pos_; i < r; ++i) {
      if (x.x_[px] != y.x_[py])
        return false;
      px = px + 1 == r ? 0 : px + 1;
      py = py + 1 == r ? 0 : py + 1;
    }
    return true;
  }

  result_type operator()() {
    const size_t ps = pos_ >= s ? pos_ - s : pos_ + r - s; // X_{i-s}
    const u64 xs = x_[ps], xr = x_[pos_];
    const bool borrow = xs < xr || xs - xr < carry_;
    const u64 y = (xs - xr - carry_) & mask;
    carry_ = borrow;
    x_[pos_] = y;
    pos_ = pos_ + 1 == r ? 0 : pos_ + 1;
    return static_cast<result_type>(y);
  }
  void discard(unsigned long long z) {
    for (; z != 0; --z)
      (*this)();
  }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os,
                                                  const subtract_with_carry_engine& x) {
    auto st = ycxx::detail::rand_out_engine(os);
    for (size_t i = 0, p = x.pos_; i < r; ++i, p = p + 1 == r ? 0 : p + 1) {
      ycxx::detail::rand_put1(os, static_cast<result_type>(x.x_[p]));
      os.put(os.widen(' '));
    }
    ycxx::detail::rand_put1(os, static_cast<unsigned>(x.carry_));
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, subtract_with_carry_engine& x) {
    auto st = ycxx::detail::rand_in(is);
    u64 tmp[r];
    for (size_t i = 0; i < r; ++i) {
      result_type v{};
      if (!ycxx::detail::rand_get(is, v) || (static_cast<u64>(v) & ~mask) != 0) {
        is.setstate(basic_istream<charT, traits>::failbit);
        return is;
      }
      tmp[i] = static_cast<u64>(v);
    }
    unsigned cy = 0;
    if (!ycxx::detail::rand_get(is, cy) || cy > 1) {
      is.setstate(basic_istream<charT, traits>::failbit);
      return is;
    }
    for (size_t i = 0; i < r; ++i)
      x.x_[i] = tmp[i];
    x.pos_ = 0;
    x.carry_ = cy;
    return is;
  }

private:
  u64 x_[r];
  size_t pos_;
  u64 carry_;
};

// ---- [rand.eng.philox] ----------------------------------------------------------------------------
template <class UIntType, size_t w, size_t n, size_t r, UIntType... consts>
class philox_engine {
  static_assert(ycxx::detail::rand_uint_type<UIntType>,
                "philox_engine: UIntType must be an unsigned integer type at least as wide as short "
                "([rand.req.genl]/1.7)");
  static_assert(sizeof...(consts) == n, "philox_engine: sizeof...(consts) == n is required");
  static_assert(n == 2 || n == 4, "philox_engine: n must be 2 or 4");
  static_assert(0 < r, "philox_engine: 0 < r is required");
  static_assert(0 < w && w <= static_cast<size_t>(numeric_limits<UIntType>::digits),
                "philox_engine: 0 < w <= numeric_limits<UIntType>::digits is required");

  static constexpr size_t array_size = n / 2;
  using u64 = ycxx::detail::rand_u64;
  static constexpr u64 mask = ycxx::detail::rand_mask(w);
  static constexpr array<UIntType, n> all_consts{consts...};

  static constexpr array<UIntType, array_size> pick(size_t first) {
    array<UIntType, array_size> res{};
    for (size_t k = 0; k < array_size; ++k)
      res[k] = all_consts[2 * k + first];
    return res;
  }

public:
  using result_type = UIntType;

  static constexpr size_t word_size = w;
  static constexpr size_t word_count = n;
  static constexpr size_t round_count = r;
  static constexpr array<result_type, array_size> multipliers = pick(0);
  static constexpr array<result_type, array_size> round_consts = pick(1);
  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return static_cast<result_type>(mask); }
  static constexpr result_type default_seed = 20111115u;

  philox_engine() : philox_engine(default_seed) {}
  explicit philox_engine(result_type value) { seed(value); }
  template <class Sseq>
    requires ycxx::detail::rand_seed_seq<Sseq, result_type, philox_engine>
  explicit philox_engine(Sseq& q) {
    seed(q);
  }
  void seed(result_type value = default_seed) {
    for (size_t k = 0; k < array_size; ++k)
      k_[k] = 0;
    k_[0] = static_cast<u64>(value) & mask;
    reset_counter();
  }
  template <class Sseq>
    requires ycxx::detail::rand_seed_seq<Sseq, result_type, philox_engine>
  void seed(Sseq& q) {
    constexpr size_t p = (w + 31) / 32;
    uint_least32_t arr[array_size * p];
    q.generate(arr + 0, arr + array_size * p);
    for (size_t k = 0; k < array_size; ++k) {
      u64 v = 0;
      for (size_t j = 0; j < p; ++j)
        v |= static_cast<u64>(arr[k * p + j] & 0xffffffffu) << (32 * j);
      k_[k] = v & mask;
    }
    reset_counter();
  }
  void set_counter(const array<result_type, n>& counter) {
    for (size_t j = 0; j < n; ++j)
      x_[j] = static_cast<u64>(counter[n - 1 - j]) & mask;
    i_ = n - 1;
  }

  friend bool operator==(const philox_engine& x, const philox_engine& y) {
    // Y is a function of K and X whenever it is used (i < n - 1), so it need not be compared.
    for (size_t k = 0; k < array_size; ++k)
      if (x.k_[k] != y.k_[k])
        return false;
    for (size_t j = 0; j < n; ++j)
      if (x.x_[j] != y.x_[j])
        return false;
    return x.i_ == y.i_;
  }

  result_type operator()() {
    if (++i_ == n) {
      generate_block();
      increment(1);
      i_ = 0;
    }
    return static_cast<result_type>(y_[i_]);
  }
  void discard(unsigned long long z) {
    // Position i + z: (i + z) / n new blocks, ending at index (i + z) mod n.
    const unsigned long long zr = z % n;
    const unsigned long long blocks = z / n + (i_ + zr) / n;
    i_ = (i_ + zr) % n;
    if (blocks != 0) {
      increment(blocks - 1);
      generate_block();
      increment(1);
    }
  }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const philox_engine& x) {
    auto st = ycxx::detail::rand_out_engine(os);
    for (size_t k = 0; k < array_size; ++k) {
      ycxx::detail::rand_put1(os, static_cast<result_type>(x.k_[k]));
      os.put(os.widen(' '));
    }
    for (size_t j = 0; j < n; ++j) {
      ycxx::detail::rand_put1(os, static_cast<result_type>(x.x_[j]));
      os.put(os.widen(' '));
    }
    ycxx::detail::rand_put1(os, x.i_);
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, philox_engine& x) {
    auto st = ycxx::detail::rand_in(is);
    philox_engine tmp;
    for (size_t k = 0; k < array_size + n; ++k) {
      result_type v{};
      if (!ycxx::detail::rand_get(is, v) || (static_cast<u64>(v) & ~mask) != 0) {
        is.setstate(basic_istream<charT, traits>::failbit);
        return is;
      }
      (k < array_size ? tmp.k_[k] : tmp.x_[k - array_size]) = static_cast<u64>(v);
    }
    size_t i = 0;
    if (!ycxx::detail::rand_get(is, i) || i >= n) {
      is.setstate(basic_istream<charT, traits>::failbit);
      return is;
    }
    tmp.i_ = i;
    if (i != n - 1) { // rebuild Y = Philox(K, Z - 1)
      tmp.decrement();
      tmp.generate_block();
      tmp.increment(1);
    }
    x = tmp;
    return is;
  }

private:
  void reset_counter() {
    for (size_t j = 0; j < n; ++j) {
      x_[j] = 0;
      y_[j] = 0;
    }
    i_ = n - 1;
  }
  // Z += by, modulo 2^(n w).
  void increment(u64 by) {
    for (size_t j = 0; j < n && by != 0; ++j) {
      const u64 sum = (x_[j] + (by & mask)) ;
      u64 carry = ycxx::detail::rand_shr(by, w);
      if constexpr (w == 64)
        carry += sum < x_[j];
      else
        carry += sum >> w;
      x_[j] = sum & mask;
      by = carry;
    }
  }
  // Z -= 1, modulo 2^(n w).
  void decrement() {
    for (size_t j = 0; j < n; ++j) {
      const bool borrow = x_[j] == 0;
      x_[j] = (x_[j] - 1) & mask;
      if (!borrow)
        return;
    }
  }
  static void mulhilo(u64 a, u64 b, u64& hi, u64& lo) {
    if constexpr (w <= 32) {
      const u64 p = a * b;
      hi = p >> w;
      lo = p & mask;
    } else {
      const ycxx::detail::rand_u128 p = ycxx::detail::rand_mul_wide(a, b);
      if constexpr (w == 64) {
        hi = p.hi;
        lo = p.lo;
      } else {
        hi = (p.hi << (64 - w)) | (p.lo >> w);
        lo = p.lo & mask;
      }
    }
  }
  // Y = Philox(K, X) ([rand.eng.philox]/4).
  void generate_block() {
    u64 v[n];
    for (size_t j = 0; j < n; ++j)
      v[j] = x_[j];
    for (size_t q = 0; q < r; ++q) {
      u64 perm[n];
      if constexpr (n == 2) {
        perm[0] = v[0];
        perm[1] = v[1];
      } else {
        perm[0] = v[2];
        perm[1] = v[1];
        perm[2] = v[0];
        perm[3] = v[3];
      }
      for (size_t k = 0; k < array_size; ++k) {
        const u64 key = (k_[k] + static_cast<u64>(q) * static_cast<u64>(round_consts[k])) & mask;
        u64 hi, lo;
        mulhilo(perm[2 * k], static_cast<u64>(multipliers[k]), hi, lo);
        v[2 * k] = hi ^ key ^ perm[2 * k + 1];
        v[2 * k + 1] = lo;
      }
    }
    for (size_t j = 0; j < n; ++j)
      y_[j] = v[j];
  }

  u64 x_[n];          // the counter Z, least significant word first
  u64 k_[array_size]; // the keys
  u64 y_[n];          // the current block of output
  size_t i_;          // index of the last value returned from y_
};

// ---- [rand.adapt.disc] ----------------------------------------------------------------------------
template <class Engine, size_t p, size_t r>
class discard_block_engine {
  static_assert(!is_const_v<Engine> && !is_volatile_v<Engine>,
                "discard_block_engine: Engine must not be cv-qualified ([rand.req.genl]/1.1)");
  static_assert(0 < r && r <= p, "discard_block_engine: requires 0 < r <= p");

public:
  using result_type = Engine::result_type;

  static constexpr size_t block_size = p;
  static constexpr size_t used_block = r;
  static constexpr result_type min() { return Engine::min(); }
  static constexpr result_type max() { return Engine::max(); }

  discard_block_engine() : e_(), n_(0) {}
  explicit discard_block_engine(const Engine& e) : e_(e), n_(0) {}
  explicit discard_block_engine(Engine&& e) : e_(std::move(e)), n_(0) {}
  explicit discard_block_engine(result_type s) : e_(s), n_(0) {}
  template <class Sseq>
    requires ycxx::detail::rand_seed_seq<Sseq, result_type, discard_block_engine, Engine>
  explicit discard_block_engine(Sseq& q) : e_(q), n_(0) {}
  void seed() {
    e_.seed();
    n_ = 0;
  }
  void seed(result_type s) {
    e_.seed(s);
    n_ = 0;
  }
  template <class Sseq>
    requires ycxx::detail::rand_seed_seq<Sseq, result_type, discard_block_engine, Engine>
  void seed(Sseq& q) {
    e_.seed(q);
    n_ = 0;
  }

  friend bool operator==(const discard_block_engine& x, const discard_block_engine& y) {
    return x.n_ == y.n_ && x.e_ == y.e_;
  }

  result_type operator()() {
    if (n_ >= r) {
      e_.discard(static_cast<unsigned long long>(p - r));
      n_ = 0;
    }
    ++n_;
    return e_();
  }
  void discard(unsigned long long z) {
    for (; z != 0; --z)
      (*this)();
  }

  const Engine& base() const noexcept { return e_; }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const discard_block_engine& x) {
    os << x.e_;
    auto st = ycxx::detail::rand_out_engine(os);
    os.put(os.widen(' '));
    ycxx::detail::rand_put1(os, x.n_);
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, discard_block_engine& x) {
    Engine e = x.e_;
    is >> e;
    auto st = ycxx::detail::rand_in(is);
    size_t n = 0;
    if (!is.fail() && ycxx::detail::rand_get(is, n)) {
      if (n > r) {
        is.setstate(basic_istream<charT, traits>::failbit);
      } else {
        x.e_ = std::move(e);
        x.n_ = n;
      }
    }
    return is;
  }

private:
  Engine e_;
  size_t n_;
};

// ---- [rand.adapt.ibits] ---------------------------------------------------------------------------
template <class Engine, size_t w, class UIntType>
class independent_bits_engine {
  static_assert(!is_const_v<Engine> && !is_volatile_v<Engine>,
                "independent_bits_engine: Engine must not be cv-qualified ([rand.req.genl]/1.1)");
  static_assert(ycxx::detail::rand_uint_type<UIntType>,
                "independent_bits_engine: UIntType must be an unsigned integer type at least as wide as "
                "short ([rand.req.genl]/1.7)");
  static_assert(0 < w && w <= static_cast<size_t>(numeric_limits<UIntType>::digits),
                "independent_bits_engine: requires 0 < w <= numeric_limits<result_type>::digits");

  using u64 = ycxx::detail::rand_u64;
  using base_result = Engine::result_type;
  static constexpr u64 emin = static_cast<u64>(Engine::min());
  static constexpr u64 rm1 = static_cast<u64>(Engine::max()) - emin; // R - 1
  static constexpr bool full = rm1 == ~0ull;                           // R = 2^64
  // [rand.adapt.ibits]/2: m = floor(log2 R), n, w0, n0, y0 and y1. When R is a power of two
  // (always, if R = 2^64), no value is ever rejected.
  static constexpr size_t m_bits = full ? 64 : static_cast<size_t>(std::bit_width(rm1 + 1)) - 1;
  struct params {
    size_t n, w0, n0;
    u64 y0, y1;      // meaningful only when !pow2
    bool pow2;
  };
  static constexpr params compute() {
    params pr{};
    pr.pow2 = full || ((rm1 + 1) & rm1) == 0;
    const u64 R = rm1 + 1;
    auto with_n = [&](size_t nn) {
      pr.n = nn;
      pr.w0 = w / nn;
      pr.n0 = nn - w % nn;
      if (!pr.pow2) {
        pr.y0 = (R >> pr.w0) << pr.w0;
        pr.y1 = ycxx::detail::rand_shl(ycxx::detail::rand_shr(R, pr.w0 + 1), pr.w0 + 1);
      }
    };
    const size_t ceil_wm = (w + m_bits - 1) / m_bits;
    with_n(ceil_wm);
    if (!pr.pow2 && R - pr.y0 > pr.y0 / pr.n)
      with_n(ceil_wm + 1);
    return pr;
  }
  static constexpr params P = compute();

public:
  using result_type = UIntType;

  static constexpr result_type min() { return 0; }
  static constexpr result_type max() { return static_cast<result_type>(ycxx::detail::rand_mask(w)); }

  independent_bits_engine() : e_() {}
  explicit independent_bits_engine(const Engine& e) : e_(e) {}
  explicit independent_bits_engine(Engine&& e) : e_(std::move(e)) {}
  explicit independent_bits_engine(result_type s) : e_(s) {}
  template <class Sseq>
    requires ycxx::detail::rand_seed_seq<Sseq, result_type, independent_bits_engine, Engine>
  explicit independent_bits_engine(Sseq& q) : e_(q) {}
  void seed() { e_.seed(); }
  void seed(result_type s) { e_.seed(s); }
  template <class Sseq>
    requires ycxx::detail::rand_seed_seq<Sseq, result_type, independent_bits_engine, Engine>
  void seed(Sseq& q) {
    e_.seed(q);
  }

  friend bool operator==(const independent_bits_engine& x, const independent_bits_engine& y) { return x.e_ == y.e_; }

  result_type operator()() {
    u64 S = 0;
    for (size_t k = 0; k != P.n0; ++k) {
      u64 u;
      do
        u = static_cast<u64>(e_()) - emin;
      while (!P.pow2 && u >= P.y0);
      S = ycxx::detail::rand_shl(S, P.w0) + (u & ycxx::detail::rand_mask(P.w0));
    }
    for (size_t k = P.n0; k != P.n; ++k) {
      u64 u;
      do
        u = static_cast<u64>(e_()) - emin;
      while (!P.pow2 && u >= P.y1);
      S = ycxx::detail::rand_shl(S, P.w0 + 1) + (u & ycxx::detail::rand_mask(P.w0 + 1));
    }
    return static_cast<result_type>(S);
  }
  void discard(unsigned long long z) {
    for (; z != 0; --z)
      (*this)();
  }

  const Engine& base() const noexcept { return e_; }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os,
                                                  const independent_bits_engine& x) {
    return os << x.e_;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, independent_bits_engine& x) {
    return is >> x.e_;
  }

private:
  Engine e_;
};

// ---- [rand.adapt.shuf] ----------------------------------------------------------------------------
template <class Engine, size_t k>
class shuffle_order_engine {
  static_assert(!is_const_v<Engine> && !is_volatile_v<Engine>,
                "shuffle_order_engine: Engine must not be cv-qualified ([rand.req.genl]/1.1)");
  static_assert(0 < k, "shuffle_order_engine: requires 0 < k");
  using u64 = ycxx::detail::rand_u64;

public:
  using result_type = Engine::result_type;

  static constexpr size_t table_size = k;
  static constexpr result_type min() { return Engine::min(); }
  static constexpr result_type max() { return Engine::max(); }

  shuffle_order_engine() : e_() { init(); }
  explicit shuffle_order_engine(const Engine& e) : e_(e) { init(); }
  explicit shuffle_order_engine(Engine&& e) : e_(std::move(e)) { init(); }
  explicit shuffle_order_engine(result_type s) : e_(s) { init(); }
  template <class Sseq>
    requires ycxx::detail::rand_seed_seq<Sseq, result_type, shuffle_order_engine, Engine>
  explicit shuffle_order_engine(Sseq& q) : e_(q) {
    init();
  }
  void seed() {
    e_.seed();
    init();
  }
  void seed(result_type s) {
    e_.seed(s);
    init();
  }
  template <class Sseq>
    requires ycxx::detail::rand_seed_seq<Sseq, result_type, shuffle_order_engine, Engine>
  void seed(Sseq& q) {
    e_.seed(q);
    init();
  }

  friend bool operator==(const shuffle_order_engine& x, const shuffle_order_engine& y) {
    if (x.y_ != y.y_)
      return false;
    for (size_t i = 0; i < k; ++i)
      if (x.v_[i] != y.v_[i])
        return false;
    return x.e_ == y.e_;
  }

  result_type operator()() {
    // j = floor(k (Y - emin) / (emax - emin + 1)).
    constexpr u64 emin = static_cast<u64>(Engine::min());
    constexpr u64 rm1 = static_cast<u64>(Engine::max()) - emin;
    const u64 y = static_cast<u64>(y_) - emin;
    size_t j;
    if constexpr (rm1 == ~0ull) {
      j = static_cast<size_t>(ycxx::detail::rand_mul_wide(y, k).hi);
    } else if constexpr (k <= ~0ull / (rm1 + 1)) {
      j = static_cast<size_t>(k * y / (rm1 + 1));
    } else {
      // k (Y - emin) needs more than 64 bits: divide the 128-bit product.
      const ycxx::detail::rand_u128 pr = ycxx::detail::rand_mul_wide(y, k);
      ycxx::detail::rand_big num, den;
      num.w[0] = pr.lo;
      num.w[1] = pr.hi;
      den.w[0] = rm1 + 1;
      j = static_cast<size_t>(num.div(den).w[0]);
    }
    y_ = v_[j];
    v_[j] = e_();
    return y_;
  }
  void discard(unsigned long long z) {
    for (; z != 0; --z)
      (*this)();
  }

  const Engine& base() const noexcept { return e_; }

  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const shuffle_order_engine& x) {
    os << x.e_;
    auto st = ycxx::detail::rand_out_engine(os);
    for (size_t i = 0; i < k; ++i)
      ycxx::detail::rand_put_sep(os, x.v_[i]);
    ycxx::detail::rand_put_sep(os, x.y_);
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, shuffle_order_engine& x) {
    Engine e = x.e_;
    is >> e;
    if (is.fail())
      return is;
    auto st = ycxx::detail::rand_in(is);
    result_type v[k];
    result_type y{};
    for (size_t i = 0; i < k; ++i)
      if (!ycxx::detail::rand_get(is, v[i]))
        return is;
    if (!ycxx::detail::rand_get(is, y))
      return is;
    if (y < min() || y > max()) {
      is.setstate(basic_istream<charT, traits>::failbit);
      return is;
    }
    x.e_ = std::move(e);
    for (size_t i = 0; i < k; ++i)
      x.v_[i] = v[i];
    x.y_ = y;
    return is;
  }

private:
  void init() {
    for (size_t i = 0; i < k; ++i)
      v_[i] = e_();
    y_ = e_();
  }

  Engine e_;
  result_type v_[k];
  result_type y_;
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
