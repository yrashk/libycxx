// libycxx core: <simd> part 2 ([simd.class], [simd.mask.class] and their non-member operations).
// The representation is described in simd_base.hpp; the mathematical functions are in
// simd_math.hpp, included at the end.
#pragma once

#include <ycxx/core/bitset.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/pair.hpp>
#include <ycxx/core/range_access.hpp>
#include <ycxx/core/simd_base.hpp>
#include <ycxx/core/tuple.hpp>

namespace ycxx::adl_free {
// real-type of a basic_vec whose value_type is not complex ([simd.overview]/3).
struct simd_not_complex {};
} // namespace ycxx::adl_free

namespace std::simd {

template <class T, class Abi = ycxx::detail::simd_native_abi<T>>
class basic_vec;

template <class T, ycxx::detail::simd_size_t N = ycxx::detail::simd_size_v<T, ycxx::detail::simd_native_abi<T>>>
using vec = basic_vec<T, ycxx::detail::simd_deduce_abi_t<T, N>>;
template <class T, ycxx::detail::simd_size_t N = ycxx::detail::simd_size_v<T, ycxx::detail::simd_native_abi<T>>>
using mask = typename vec<T, N>::mask_type;

// [simd.traits]
template <class T, class U = typename T::value_type>
struct alignment {};
template <class T, class Abi, class U>
  requires(ycxx::detail::simd_size_v<T, Abi> != 0 && ycxx::detail::simd_vectorizable<U>)
struct alignment<basic_vec<T, Abi>, U>
    : integral_constant<size_t, alignof(ycxx::detail::simd_storage<U, ycxx::detail::simd_size_v<T, Abi>>)> {};
template <class T, class U = typename T::value_type>
inline constexpr size_t alignment_v = alignment<T, U>::value;

template <class T, class V>
struct rebind {};
template <class T, class U, int N>
  requires(ycxx::detail::simd_vectorizable<T> && ycxx::detail::simd_size_v<U, ycxx::adl_free::simd_abi<N>> != 0)
struct rebind<T, basic_vec<U, ycxx::adl_free::simd_abi<N>>> {
  using type = basic_vec<T, ycxx::adl_free::simd_abi<N>>;
};
template <class T, size_t Bytes, int N>
  requires(ycxx::detail::simd_vectorizable<T> && ycxx::detail::simd_mask_size_v<Bytes, ycxx::adl_free::simd_abi<N>> != 0)
struct rebind<T, basic_mask<Bytes, ycxx::adl_free::simd_abi<N>>> {
  using type = basic_mask<sizeof(T), ycxx::adl_free::simd_abi<N>>;
};
template <class T, class V>
using rebind_t = typename rebind<T, V>::type;

template <ycxx::detail::simd_size_t N, class V>
struct resize {};
template <ycxx::detail::simd_size_t N, class T, int M>
  requires(ycxx::detail::simd_size_v<T, ycxx::adl_free::simd_abi<M>> != 0 &&
           ycxx::detail::simd_size_v<T, ycxx::adl_free::simd_abi<N>> != 0)
struct resize<N, basic_vec<T, ycxx::adl_free::simd_abi<M>>> {
  using type = basic_vec<T, ycxx::adl_free::simd_abi<N>>;
};
template <ycxx::detail::simd_size_t N, size_t Bytes, int M>
  requires(ycxx::detail::simd_mask_size_v<Bytes, ycxx::adl_free::simd_abi<M>> != 0 &&
           ycxx::detail::simd_mask_size_v<Bytes, ycxx::adl_free::simd_abi<N>> != 0)
struct resize<N, basic_mask<Bytes, ycxx::adl_free::simd_abi<M>>> {
  using type = basic_mask<Bytes, ycxx::adl_free::simd_abi<N>>;
};
template <ycxx::detail::simd_size_t N, class V>
using resize_t = typename resize<N, V>::type;

} // namespace std::simd

namespace ycxx::detail {

// [simd.expos]
template <class V>
concept simd_vec_type = std::same_as<V, std::simd::basic_vec<typename V::value_type, typename V::abi_type>> &&
                        std::is_default_constructible_v<V>;
template <class V>
concept simd_mask_type = std::same_as<V, std::simd::basic_mask<simd_mask_element_size<V>, typename V::abi_type>> &&
                         std::is_default_constructible_v<V>;
template <class V>
concept simd_floating_point = simd_vec_type<V> && std::floating_point<typename V::value_type>;
template <class V>
concept simd_integral = simd_vec_type<V> && std::integral<typename V::value_type>;
template <class V>
concept simd_complex = simd_vec_type<V> && simd_is_complex<typename V::value_type>;
template <class T>
using deduced_vec_t = decltype(std::declval<const T&>() + std::declval<const T&>());
template <class T>
concept math_floating_point = simd_floating_point<deduced_vec_t<T>>;
template <class BinaryOperation, class T>
concept reduction_binary_operation =
    requires(const BinaryOperation binary_op, const std::simd::vec<T, 1> v) {
      { binary_op(v, v) } -> std::same_as<std::simd::vec<T, 1>>;
    };

template <class T, int N>
struct simd_real_type {
  using type = ycxx::adl_free::simd_not_complex;
};
template <class T, int N>
struct simd_real_type<std::complex<T>, N> {
  using type = std::simd::basic_vec<T, ycxx::adl_free::simd_abi<N>>;
};

template <std::size_t Bytes, int N>
using simd_mask_storage = simd_storage<simd_mask_element<Bytes>, N>;

// [simd.ctor]/2: the broadcast constructor's constraints.
template <class U, class T, class From = std::remove_cvref_t<U>>
concept simd_broadcast_from =
    (std::convertible_to<U, T> && !is_arithmetic_v<From> && !constexpr_wrapper_like<From>) ||
    (is_arithmetic_v<From> && ycxx::detail::simd_value_preserving<From, T>()) ||
    (constexpr_wrapper_like<From> && is_arithmetic_v<std::remove_cvref_t<decltype(From::value)>> &&
     ycxx::detail::simd_representable<From::value, T>());

// [simd.ctor]/8: the generator constructor's constraints, for one index.
template <class From, class T>
concept simd_generated_value =
    std::convertible_to<From, T> &&
    (!is_arithmetic_v<std::remove_cvref_t<From>> || ycxx::detail::simd_value_preserving<std::remove_cvref_t<From>, T>());
template <class G, class T, int I>
concept simd_generator_element = requires(G& g) {
  { g(std::integral_constant<simd_size_t, I>()) } -> simd_generated_value<T>;
};
template <class G, class T, int... I>
consteval bool simd_is_generator(std::integer_sequence<int, I...>) {
  return (simd_generator_element<G, T, I> && ...);
}
template <class G, class T, int N>
concept simd_generator = ycxx::detail::simd_is_generator<G, T>(std::make_integer_sequence<int, N>());

// [simd.mask.ctor]/4
template <class G, int I>
concept simd_mask_generator_element =
    requires(G& g) { requires std::same_as<decltype(g(std::integral_constant<simd_size_t, I>())), bool>; };
template <class G, int... I>
consteval bool simd_is_mask_generator(std::integer_sequence<int, I...>) {
  return (simd_mask_generator_element<G, I> && ...);
}
template <class G, int N>
concept simd_mask_generator = ycxx::detail::simd_is_mask_generator<G>(std::make_integer_sequence<int, N>());

// [simd.ctor]/12: the range constructor's constraints.
template <class R, class T, int N>
concept simd_range_init = std::ranges::contiguous_range<R> && std::ranges::sized_range<R> &&
                          simd_static_size<R> == static_cast<std::size_t>(N) &&
                          simd_vectorizable<std::ranges::range_value_t<R>> &&
                          simd_explicitly_convertible_to<std::ranges::range_value_t<R>, T>;

// Element-wise comparison of two storages giving a mask storage: chunk by chunk when the
// layouts match (the vector comparison gives the all-ones/zero lanes of the mask), else element
// by element. f is a generic comparison.
template <class MS, class S, class F>
constexpr MS simd_compare(const S& a, const S& b, F f) noexcept {
  using M = typename MS::element_type;
  if constexpr (S::is_vector && MS::is_vector && S::lanes == MS::lanes) {
    MS r;
    for (int k = 0; k < S::chunks; ++k)
      r.c[k] = __builtin_bit_cast(typename MS::chunk_type, f(a.c[k], b.c[k]));
    return r;
  } else {
    return MS::generate([&](int i) { return f(a.get(i), b.get(i)) ? M(-1) : M(0); });
  }
}

// m[i] ? a[i] : b[i]
template <class S, class MS>
constexpr S simd_blend(const MS& m, const S& a, const S& b) noexcept {
  if constexpr (S::is_vector && MS::is_vector && S::lanes == MS::lanes) {
    using MC = typename MS::chunk_type;
    using C = typename S::chunk_type;
    S r;
    for (int k = 0; k < S::chunks; ++k) {
      MC x = __builtin_bit_cast(MC, a.c[k]), y = __builtin_bit_cast(MC, b.c[k]);
      r.c[k] = __builtin_bit_cast(C, static_cast<MC>((m.c[k] & x) | (~m.c[k] & y)));
    }
    return r;
  } else {
    return S::generate([&](int i) { return m.get(i) ? a.get(i) : b.get(i); });
  }
}

// Element-wise static_cast from storage SU to storage S.
template <class S, class SU>
constexpr S simd_convert(const SU& u) noexcept {
  using T = typename S::element_type;
  if constexpr (std::is_same_v<S, SU>) {
    return u;
  } else if constexpr (S::is_vector && SU::is_vector && S::lanes == SU::lanes) {
    S r;
    for (int k = 0; k < S::chunks; ++k)
      r.c[k] = __builtin_convertvector(u.c[k], typename S::chunk_type);
    return r;
  } else {
    return S::generate([&](int i) { return static_cast<T>(u.get(i)); });
  }
}

// Shifts: in the promoted type, as for the scalars (a vector of 8- or 16-bit lanes is widened
// to int lanes, so a shift count up to 31 has the scalar meaning).
template <bool Left, class S>
constexpr S simd_shift(const S& a, const S& b) noexcept {
  using E = typename S::element_type;
  if constexpr (S::is_vector && sizeof(E) < sizeof(int)) {
    using W = simd_vector<int, S::lanes>;
    using C = typename S::chunk_type;
    return S::map2(a, b, [](C x, C y) {
      W wx = __builtin_convertvector(x, W), wy = __builtin_convertvector(y, W);
      if constexpr (Left)
        return __builtin_convertvector(wx << wy, C);
      else
        return __builtin_convertvector(wx >> wy, C);
    });
  } else {
    return S::map2(a, b, [](auto x, auto y) {
      if constexpr (Left)
        return x << y;
      else
        return x >> y;
    });
  }
}
template <bool Left, class S>
constexpr S simd_shift(const S& a, simd_size_t n) noexcept {
  using E = typename S::element_type;
  if constexpr (S::is_vector && sizeof(E) < sizeof(int)) {
    using W = simd_vector<int, S::lanes>;
    using C = typename S::chunk_type;
    return S::map(a, [n](C x) {
      W wx = __builtin_convertvector(x, W);
      if constexpr (Left)
        return __builtin_convertvector(wx << n, C);
      else
        return __builtin_convertvector(wx >> n, C);
    });
  } else {
    return S::map(a, [n](auto x) {
      if constexpr (Left)
        return x << n;
      else
        return x >> n;
    });
  }
}

// The mask reductions work on the object representation as unsigned words: every element is all
// ones or all zeros, so counting set bits counts elements (little-endian targets; elsewhere the
// elements are visited one by one).
template <class S>
consteval std::size_t simd_word_size() {
  constexpr std::size_t bytes = sizeof(S);
  return bytes % 8 == 0 ? 8 : bytes % 4 == 0 ? 4 : bytes % 2 == 0 ? 2 : 1;
}
template <class S>
using simd_word =
    std::conditional_t<simd_word_size<S>() == 8, unsigned long long,
                       std::conditional_t<simd_word_size<S>() == 4, unsigned,
                                          std::conditional_t<simd_word_size<S>() == 2, unsigned short, unsigned char>>>;
template <class S>
inline constexpr bool simd_words_ok = std::endian::native == std::endian::little;
template <class S>
constexpr auto simd_words(const S& s) noexcept {
  using W = simd_array<simd_word<S>, sizeof(S) / sizeof(simd_word<S>)>;
  return __builtin_bit_cast(W, s);
}

// The identities of [simd.reductions]/9.
template <class Op>
inline constexpr bool simd_known_identity =
    std::is_same_v<Op, std::plus<>> || std::is_same_v<Op, std::multiplies<>> || std::is_same_v<Op, std::bit_and<>> ||
    std::is_same_v<Op, std::bit_or<>> || std::is_same_v<Op, std::bit_xor<>>;
template <class T, class Op>
constexpr T simd_identity() noexcept {
  if constexpr (std::is_same_v<Op, std::multiplies<>>)
    return T(1);
  else if constexpr (std::is_same_v<Op, std::bit_and<>>)
    return T(~T());
  else
    return T();
}

} // namespace ycxx::detail

namespace std::simd {

// [simd.overview]/1: a disabled basic_vec.
template <class T, class Abi>
class basic_vec {
public:
  using value_type = T;
  using mask_type = basic_mask<sizeof(T), Abi>;
  using abi_type = Abi;
  basic_vec() = delete;
  ~basic_vec() = delete;
  basic_vec(const basic_vec&) = delete;
  basic_vec& operator=(const basic_vec&) = delete;
};

// [simd.mask.overview]/1: a disabled basic_mask.
template <size_t Bytes, class Abi>
class basic_mask {
public:
  using value_type = bool;
  using abi_type = Abi;
  basic_mask() = delete;
  ~basic_mask() = delete;
  basic_mask(const basic_mask&) = delete;
  basic_mask& operator=(const basic_mask&) = delete;
};

// [simd.mask.class]
template <size_t Bytes, int N>
  requires(ycxx::detail::simd_mask_size_v<Bytes, ycxx::adl_free::simd_abi<N>> != 0)
class basic_mask<Bytes, ycxx::adl_free::simd_abi<N>> {
  using element = ycxx::detail::simd_mask_element<Bytes>;
  using storage = ycxx::detail::simd_mask_storage<Bytes, N>;
  storage data_;

  friend ycxx::detail::simd_access;
  template <class, class>
  friend class basic_vec;
  template <size_t, class>
  friend class basic_mask;
  constexpr basic_mask(ycxx::detail::simd_access, const storage& s) noexcept : data_(s) {}
  static constexpr basic_mask make(const storage& s) noexcept { return basic_mask(ycxx::detail::simd_access{}, s); }
  // The vec of +k, -k, ~k: integer-from<Bytes> elements.
  using int_vec = basic_vec<ycxx::detail::simd_integer_from<(Bytes > 8 ? 8 : Bytes)>, ycxx::adl_free::simd_abi<N>>;

public:
  using value_type = bool;
  using abi_type = ycxx::adl_free::simd_abi<N>;
  using iterator = ycxx::adl_free::simd_iterator<basic_mask>;
  using const_iterator = ycxx::adl_free::simd_iterator<const basic_mask>;

  constexpr iterator begin() noexcept { return {*this, 0}; }
  constexpr const_iterator begin() const noexcept { return {*this, 0}; }
  constexpr const_iterator cbegin() const noexcept { return {*this, 0}; }
  constexpr default_sentinel_t end() const noexcept { return {}; }
  constexpr default_sentinel_t cend() const noexcept { return {}; }

  static constexpr integral_constant<ycxx::detail::simd_size_t, N> size{};

  constexpr basic_mask() noexcept = default;

  // [simd.mask.ctor]
  constexpr explicit basic_mask(same_as<value_type> auto x) noexcept
      : data_(storage::broadcast(x ? element(-1) : element(0))) {}
  template <size_t UBytes, class UAbi>
    requires(ycxx::detail::simd_mask_size_v<UBytes, UAbi> == N)
  constexpr explicit basic_mask(const basic_mask<UBytes, UAbi>& x) noexcept
      : data_(ycxx::detail::simd_convert<storage>(x.data_)) {}
  template <class G>
    requires ycxx::detail::simd_mask_generator<G, N>
  constexpr explicit basic_mask(G&& gen) {
    ycxx::detail::simd_array<element, N> a;
    [&]<int... I>(integer_sequence<int, I...>) {
      ((a.v[I] = gen(integral_constant<ycxx::detail::simd_size_t, I>()) ? element(-1) : element(0)), ...);
    }(make_integer_sequence<int, N>());
    data_ = storage::from_array(a);
  }
  template <same_as<bitset<static_cast<size_t>(N)>> T>
  constexpr basic_mask(const T& b) noexcept
      : data_(storage::generate([&](int i) { return b[static_cast<size_t>(i)] ? element(-1) : element(0); })) {}
  template <unsigned_integral T>
    requires(!same_as<T, value_type>)
  constexpr explicit basic_mask(T val) noexcept
      : data_(storage::generate([&](int i) {
          return i < numeric_limits<T>::digits && ((val >> i) & 1) != 0 ? element(-1) : element(0);
        })) {}

  // [simd.mask.subscr]
  constexpr value_type operator[](ycxx::detail::simd_size_t i) const {
    ycxx::detail::precondition(i >= 0 && i < N, "std::simd::basic_mask::operator[]: index out of range");
    return data_.get(i) != 0;
  }
  template <ycxx::detail::simd_integral I>
  constexpr resize_t<I::size(), basic_mask> operator[](const I& indices) const {
    return permute(*this, indices);
  }

  // [simd.mask.unary]
  constexpr basic_mask operator!() const noexcept {
    return make(storage::map(data_, [](auto x) { return ~x; }));
  }
  constexpr int_vec operator+() const noexcept
    requires(Bytes <= 8)
  {
    return ycxx::detail::simd_access::make<int_vec>(storage::map(data_, [](auto x) { return -x; }));
  }
  constexpr int_vec operator-() const noexcept
    requires(Bytes <= 8)
  {
    return ycxx::detail::simd_access::make<int_vec>(data_);
  }
  constexpr int_vec operator~() const noexcept
    requires(Bytes <= 8)
  {
    return ycxx::detail::simd_access::make<int_vec>(storage::map(data_, [](auto x) { return x - element(1); }));
  }
  void operator+() const noexcept
    requires(Bytes > 8)
  = delete;
  void operator-() const noexcept
    requires(Bytes > 8)
  = delete;
  void operator~() const noexcept
    requires(Bytes > 8)
  = delete;

  // [simd.mask.conv]
  template <class U, class A>
    requires(ycxx::detail::simd_size_v<U, A> == N)
  constexpr explicit(sizeof(U) != Bytes) operator basic_vec<U, A>() const noexcept {
    using VS = ycxx::detail::simd_storage<U, N>;
    if constexpr (ycxx::detail::simd_arithmetic<U>)
      return ycxx::detail::simd_access::make<basic_vec<U, A>>(
          ycxx::detail::simd_convert<VS>(storage::map(data_, [](auto x) { return -x; })));
    else
      return ycxx::detail::simd_access::make<basic_vec<U, A>>(
          VS::generate([&](int i) { return static_cast<U>(data_.get(i) != 0); }));
  }
  constexpr bitset<static_cast<size_t>(N)> to_bitset() const noexcept {
    bitset<static_cast<size_t>(N)> b;
    for (int i = 0; i < N; ++i)
      if (data_.get(i) != 0)
        b.set(static_cast<size_t>(i));
    return b;
  }
  constexpr unsigned long long to_ullong() const {
    unsigned long long r = 0;
    for (int i = 0; i < N; ++i)
      if (data_.get(i) != 0) {
        ycxx::detail::precondition(i < numeric_limits<unsigned long long>::digits,
                                   "std::simd::basic_mask::to_ullong: a set element does not fit");
        r |= 1ull << i;
      }
    return r;
  }

  // [simd.mask.binary]
  friend constexpr basic_mask operator&&(const basic_mask& a, const basic_mask& b) noexcept {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x & y; }));
  }
  friend constexpr basic_mask operator||(const basic_mask& a, const basic_mask& b) noexcept {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x | y; }));
  }
  friend constexpr basic_mask operator&(const basic_mask& a, const basic_mask& b) noexcept {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x & y; }));
  }
  friend constexpr basic_mask operator|(const basic_mask& a, const basic_mask& b) noexcept {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x | y; }));
  }
  friend constexpr basic_mask operator^(const basic_mask& a, const basic_mask& b) noexcept {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x ^ y; }));
  }
  // [simd.mask.cassign]
  friend constexpr basic_mask& operator&=(basic_mask& a, const basic_mask& b) noexcept { return a = a & b; }
  friend constexpr basic_mask& operator|=(basic_mask& a, const basic_mask& b) noexcept { return a = a | b; }
  friend constexpr basic_mask& operator^=(basic_mask& a, const basic_mask& b) noexcept { return a = a ^ b; }
  // [simd.mask.comparison] (false < true)
  friend constexpr basic_mask operator==(const basic_mask& a, const basic_mask& b) noexcept {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return ~(x ^ y); }));
  }
  friend constexpr basic_mask operator!=(const basic_mask& a, const basic_mask& b) noexcept {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x ^ y; }));
  }
  friend constexpr basic_mask operator>=(const basic_mask& a, const basic_mask& b) noexcept {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x | ~y; }));
  }
  friend constexpr basic_mask operator<=(const basic_mask& a, const basic_mask& b) noexcept {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return ~x | y; }));
  }
  friend constexpr basic_mask operator>(const basic_mask& a, const basic_mask& b) noexcept {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x & ~y; }));
  }
  friend constexpr basic_mask operator<(const basic_mask& a, const basic_mask& b) noexcept {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return ~x & y; }));
  }

  // [simd.mask.cond]
  friend constexpr basic_mask simd_select_impl(const basic_mask& k, const basic_mask& a, const basic_mask& b) noexcept {
    return make(ycxx::detail::simd_blend(k.data_, a.data_, b.data_));
  }
  friend constexpr basic_mask simd_select_impl(const basic_mask& k, same_as<bool> auto a, same_as<bool> auto b) noexcept {
    return make(ycxx::detail::simd_blend(k.data_, storage::broadcast(a ? element(-1) : element(0)),
                                         storage::broadcast(b ? element(-1) : element(0))));
  }
  template <class T0, class T1>
    requires(same_as<T0, T1> && ycxx::detail::simd_vectorizable<T0> && sizeof(T0) == Bytes)
  friend constexpr basic_vec<T0, ycxx::adl_free::simd_abi<N>> simd_select_impl(const basic_mask& k, const T0& a,
                                                                               const T1& b) noexcept {
    using VS = ycxx::detail::simd_storage<T0, N>;
    return ycxx::detail::simd_access::make<basic_vec<T0, ycxx::adl_free::simd_abi<N>>>(
        ycxx::detail::simd_blend(k.data_, VS::broadcast(a), VS::broadcast(b)));
  }
};

// [simd.class]
template <class T, int N>
  requires(ycxx::detail::simd_size_v<T, ycxx::adl_free::simd_abi<N>> != 0)
class basic_vec<T, ycxx::adl_free::simd_abi<N>> {
  using storage = ycxx::detail::simd_storage<T, N>;
  using mask_storage = ycxx::detail::simd_mask_storage<sizeof(T), N>;
  using real_type = typename ycxx::detail::simd_real_type<T, N>::type;
  storage data_;

  friend ycxx::detail::simd_access;
  template <class, class>
  friend class basic_vec;
  template <size_t, class>
  friend class basic_mask;
  constexpr basic_vec(ycxx::detail::simd_access, const storage& s) noexcept : data_(s) {}
  static constexpr basic_vec make(const storage& s) noexcept { return basic_vec(ycxx::detail::simd_access{}, s); }

  template <class U, class... Flags>
  static constexpr storage from_range(const U* p, const mask_storage* k) {
    if constexpr (!ycxx::detail::simd_has_convert<Flags...>)
      static_assert(ycxx::detail::simd_value_preserving<U, T>(),
                    "std::simd::basic_vec: the conversion from the range's value type is not value-preserving "
                    "(pass flag_convert)");
    return storage::generate([&](int i) { return k == nullptr || k->get(i) != 0 ? static_cast<T>(p[i]) : T(); });
  }

public:
  using value_type = T;
  using mask_type = basic_mask<sizeof(T), ycxx::adl_free::simd_abi<N>>;
  using abi_type = ycxx::adl_free::simd_abi<N>;
  using iterator = ycxx::adl_free::simd_iterator<basic_vec>;
  using const_iterator = ycxx::adl_free::simd_iterator<const basic_vec>;

  constexpr iterator begin() noexcept { return {*this, 0}; }
  constexpr const_iterator begin() const noexcept { return {*this, 0}; }
  constexpr const_iterator cbegin() const noexcept { return {*this, 0}; }
  constexpr default_sentinel_t end() const noexcept { return {}; }
  constexpr default_sentinel_t cend() const noexcept { return {}; }

  static constexpr integral_constant<ycxx::detail::simd_size_t, N> size{};

  constexpr basic_vec() noexcept = default;

  // [simd.ctor]
  template <class U>
    requires ycxx::detail::simd_broadcast_from<U, T>
  constexpr basic_vec(U&& value) noexcept : data_(storage::broadcast(static_cast<T>(static_cast<U&&>(value)))) {}
  template <class U, class UAbi>
    requires(ycxx::detail::simd_size_v<U, UAbi> == N && ycxx::detail::simd_explicitly_convertible_to<U, T>)
  constexpr explicit(!ycxx::detail::simd_implicit_conversion<U, T>()) basic_vec(const basic_vec<U, UAbi>& x) noexcept
      : data_(ycxx::detail::simd_convert<storage>(x.data_)) {}
  template <class G>
    requires ycxx::detail::simd_generator<G, T, N>
  constexpr explicit basic_vec(G&& gen) {
    ycxx::detail::simd_array<T, N> a;
    [&]<int... I>(integer_sequence<int, I...>) {
      ((a.v[I] = static_cast<T>(gen(integral_constant<ycxx::detail::simd_size_t, I>()))), ...);
    }(make_integer_sequence<int, N>());
    data_ = storage::from_array(a);
  }
  template <class R, class... Flags>
    requires ycxx::detail::simd_range_init<R, T, N>
  constexpr basic_vec(R&& r, flags<Flags...> = {})
      : data_(from_range<ranges::range_value_t<R>, Flags...>(ranges::data(r), nullptr)) {}
  template <class R, class... Flags>
    requires ycxx::detail::simd_range_init<R, T, N>
  constexpr basic_vec(R&& r, const mask_type& mask, flags<Flags...> = {})
      : data_(from_range<ranges::range_value_t<R>, Flags...>(ranges::data(r), __builtin_addressof(mask.data_))) {}
  constexpr basic_vec(const real_type& reals, const real_type& imags = {}) noexcept
    requires ycxx::detail::simd_is_complex<T>
      : data_(storage::generate([&](int i) { return T(reals[i], imags[i]); })) {}

  // [simd.subscr]
  constexpr value_type operator[](ycxx::detail::simd_size_t i) const {
    ycxx::detail::precondition(i >= 0 && i < N, "std::simd::basic_vec::operator[]: index out of range");
    return data_.get(i);
  }
  template <ycxx::detail::simd_integral I>
  constexpr resize_t<I::size(), basic_vec> operator[](const I& indices) const {
    return permute(*this, indices);
  }

  // [simd.complex.access]
  constexpr real_type real() const noexcept
    requires ycxx::detail::simd_is_complex<T>
  {
    return real_type([&](int i) { return data_.get(i).real(); });
  }
  constexpr real_type imag() const noexcept
    requires ycxx::detail::simd_is_complex<T>
  {
    return real_type([&](int i) { return data_.get(i).imag(); });
  }
  constexpr void real(const real_type& v) noexcept
    requires ycxx::detail::simd_is_complex<T>
  {
    data_ = storage::generate([&](int i) { return T(v[i], data_.get(i).imag()); });
  }
  constexpr void imag(const real_type& v) noexcept
    requires ycxx::detail::simd_is_complex<T>
  {
    data_ = storage::generate([&](int i) { return T(data_.get(i).real(), v[i]); });
  }

  // [simd.unary]
  constexpr basic_vec& operator++() noexcept
    requires requires(value_type a) { ++a; }
  {
    data_ = storage::map(data_, [](auto x) { return x + T(1); });
    return *this;
  }
  constexpr basic_vec operator++(int) noexcept
    requires requires(value_type a) { a++; }
  {
    basic_vec tmp = *this;
    ++*this;
    return tmp;
  }
  constexpr basic_vec& operator--() noexcept
    requires requires(value_type a) { --a; }
  {
    data_ = storage::map(data_, [](auto x) { return x - T(1); });
    return *this;
  }
  constexpr basic_vec operator--(int) noexcept
    requires requires(value_type a) { a--; }
  {
    basic_vec tmp = *this;
    --*this;
    return tmp;
  }
  constexpr mask_type operator!() const noexcept
    requires requires(const value_type a) { !a; }
  {
    return ycxx::detail::simd_access::make<mask_type>(ycxx::detail::simd_compare<mask_storage>(
        data_, storage::broadcast(T()), [](auto x, auto y) { return x == y; }));
  }
  constexpr basic_vec operator~() const noexcept
    requires requires(const value_type a) { ~a; }
  {
    return make(storage::map(data_, [](auto x) { return ~x; }));
  }
  constexpr basic_vec operator+() const noexcept
    requires requires(const value_type a) { +a; }
  {
    return *this;
  }
  constexpr basic_vec operator-() const noexcept
    requires requires(const value_type a) { -a; }
  {
    return make(storage::map(data_, [](auto x) { return -x; }));
  }

  // [simd.binary]
  friend constexpr basic_vec operator+(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x + y; }
  {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x + y; }));
  }
  friend constexpr basic_vec operator-(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x - y; }
  {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x - y; }));
  }
  friend constexpr basic_vec operator*(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x * y; }
  {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x * y; }));
  }
  friend constexpr basic_vec operator/(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x / y; }
  {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x / y; }));
  }
  friend constexpr basic_vec operator%(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x % y; }
  {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x % y; }));
  }
  friend constexpr basic_vec operator&(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x & y; }
  {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x & y; }));
  }
  friend constexpr basic_vec operator|(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x | y; }
  {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x | y; }));
  }
  friend constexpr basic_vec operator^(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x ^ y; }
  {
    return make(storage::map2(a.data_, b.data_, [](auto x, auto y) { return x ^ y; }));
  }
  friend constexpr basic_vec operator<<(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x << y; }
  {
    return make(ycxx::detail::simd_shift<true>(a.data_, b.data_));
  }
  friend constexpr basic_vec operator>>(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x >> y; }
  {
    return make(ycxx::detail::simd_shift<false>(a.data_, b.data_));
  }
  friend constexpr basic_vec operator<<(const basic_vec& v, ycxx::detail::simd_size_t n) noexcept
    requires requires(value_type x, ycxx::detail::simd_size_t y) { x << y; }
  {
    return make(ycxx::detail::simd_shift<true>(v.data_, n));
  }
  friend constexpr basic_vec operator>>(const basic_vec& v, ycxx::detail::simd_size_t n) noexcept
    requires requires(value_type x, ycxx::detail::simd_size_t y) { x >> y; }
  {
    return make(ycxx::detail::simd_shift<false>(v.data_, n));
  }

  // [simd.cassign]
  friend constexpr basic_vec& operator+=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x + y; }
  {
    return a = a + b;
  }
  friend constexpr basic_vec& operator-=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x - y; }
  {
    return a = a - b;
  }
  friend constexpr basic_vec& operator*=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x * y; }
  {
    return a = a * b;
  }
  friend constexpr basic_vec& operator/=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x / y; }
  {
    return a = a / b;
  }
  friend constexpr basic_vec& operator%=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x % y; }
  {
    return a = a % b;
  }
  friend constexpr basic_vec& operator&=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x & y; }
  {
    return a = a & b;
  }
  friend constexpr basic_vec& operator|=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x | y; }
  {
    return a = a | b;
  }
  friend constexpr basic_vec& operator^=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x ^ y; }
  {
    return a = a ^ b;
  }
  friend constexpr basic_vec& operator<<=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x << y; }
  {
    return a = a << b;
  }
  friend constexpr basic_vec& operator>>=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x >> y; }
  {
    return a = a >> b;
  }
  friend constexpr basic_vec& operator<<=(basic_vec& a, ycxx::detail::simd_size_t n) noexcept
    requires requires(value_type x, ycxx::detail::simd_size_t y) { x << y; }
  {
    return a = a << n;
  }
  friend constexpr basic_vec& operator>>=(basic_vec& a, ycxx::detail::simd_size_t n) noexcept
    requires requires(value_type x, ycxx::detail::simd_size_t y) { x >> y; }
  {
    return a = a >> n;
  }

  // [simd.comparison]
  friend constexpr mask_type operator==(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x == y; }
  {
    return ycxx::detail::simd_access::make<mask_type>(
        ycxx::detail::simd_compare<mask_storage>(a.data_, b.data_, [](auto x, auto y) { return x == y; }));
  }
  friend constexpr mask_type operator!=(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x != y; }
  {
    return ycxx::detail::simd_access::make<mask_type>(
        ycxx::detail::simd_compare<mask_storage>(a.data_, b.data_, [](auto x, auto y) { return x != y; }));
  }
  friend constexpr mask_type operator>=(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x >= y; }
  {
    return ycxx::detail::simd_access::make<mask_type>(
        ycxx::detail::simd_compare<mask_storage>(a.data_, b.data_, [](auto x, auto y) { return x >= y; }));
  }
  friend constexpr mask_type operator<=(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x <= y; }
  {
    return ycxx::detail::simd_access::make<mask_type>(
        ycxx::detail::simd_compare<mask_storage>(a.data_, b.data_, [](auto x, auto y) { return x <= y; }));
  }
  friend constexpr mask_type operator>(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x > y; }
  {
    return ycxx::detail::simd_access::make<mask_type>(
        ycxx::detail::simd_compare<mask_storage>(a.data_, b.data_, [](auto x, auto y) { return x > y; }));
  }
  friend constexpr mask_type operator<(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type x, value_type y) { x < y; }
  {
    return ycxx::detail::simd_access::make<mask_type>(
        ycxx::detail::simd_compare<mask_storage>(a.data_, b.data_, [](auto x, auto y) { return x < y; }));
  }

  // [simd.cond]
  friend constexpr basic_vec simd_select_impl(const mask_type& k, const basic_vec& a, const basic_vec& b) noexcept {
    return make(ycxx::detail::simd_blend(ycxx::detail::simd_access::data(k), a.data_, b.data_));
  }
};

template <class R, class... Ts>
  requires(ranges::contiguous_range<R> && ranges::sized_range<R> &&
           ycxx::detail::simd_static_size<R> != dynamic_extent)
basic_vec(R&& r, Ts...)
    -> basic_vec<ranges::range_value_t<R>,
                 ycxx::detail::simd_deduce_abi_t<ranges::range_value_t<R>,
                                                 static_cast<ycxx::detail::simd_size_t>(ycxx::detail::simd_static_size<R>)>>;
template <size_t Bytes, class Abi>
  requires(ycxx::detail::simd_mask_size_v<Bytes, Abi> != 0 && Bytes <= 8)
basic_vec(basic_mask<Bytes, Abi>) -> basic_vec<ycxx::detail::simd_integer_from<Bytes>, Abi>;

// [simd.creation]
template <class T, class Abi>
  requires(ycxx::detail::simd_vec_type<T> &&
           (basic_vec<typename T::value_type, Abi>::size() % T::size() == 0 ||
            requires { typename resize_t<basic_vec<typename T::value_type, Abi>::size() % T::size(), T>; }))
constexpr auto chunk(const basic_vec<typename T::value_type, Abi>& x) noexcept {
  constexpr int size = basic_vec<typename T::value_type, Abi>::size();
  constexpr int w = T::size(), n = size / w, rem = size % w;
  auto part = [&]<class P>(int base) { return P([&](int i) { return x[base + i]; }); };
  return [&]<size_t... J>(index_sequence<J...>) {
    if constexpr (rem == 0)
      return array<T, n>{part.template operator()<T>(static_cast<int>(J) * w)...};
    else
      return tuple<conditional_t<true, T, integral_constant<size_t, J>>..., resize_t<rem, T>>(
          part.template operator()<T>(static_cast<int>(J) * w)..., part.template operator()<resize_t<rem, T>>(n * w));
  }(make_index_sequence<static_cast<size_t>(n)>());
}
template <class T, class Abi>
  requires(ycxx::detail::simd_mask_type<T> &&
           (basic_mask<ycxx::detail::simd_mask_element_size<T>, Abi>::size() % T::size() == 0 ||
            requires {
              typename resize_t<basic_mask<ycxx::detail::simd_mask_element_size<T>, Abi>::size() % T::size(), T>;
            }))
constexpr auto chunk(const basic_mask<ycxx::detail::simd_mask_element_size<T>, Abi>& x) noexcept {
  constexpr int size = basic_mask<ycxx::detail::simd_mask_element_size<T>, Abi>::size();
  constexpr int w = T::size(), n = size / w, rem = size % w;
  auto part = [&]<class P>(int base) { return P([&](int i) { return bool(x[base + i]); }); };
  return [&]<size_t... J>(index_sequence<J...>) {
    if constexpr (rem == 0)
      return array<T, n>{part.template operator()<T>(static_cast<int>(J) * w)...};
    else
      return tuple<conditional_t<true, T, integral_constant<size_t, J>>..., resize_t<rem, T>>(
          part.template operator()<T>(static_cast<int>(J) * w)..., part.template operator()<resize_t<rem, T>>(n * w));
  }(make_index_sequence<static_cast<size_t>(n)>());
}
template <ycxx::detail::simd_size_t N, class T, class Abi>
constexpr auto chunk(const basic_vec<T, Abi>& x) noexcept {
  return std::simd::chunk<resize_t<N, basic_vec<T, Abi>>>(x);
}
template <ycxx::detail::simd_size_t N, size_t Bytes, class Abi>
constexpr auto chunk(const basic_mask<Bytes, Abi>& x) noexcept {
  return std::simd::chunk<resize_t<N, basic_mask<Bytes, Abi>>>(x);
}

template <class T, class... Abis>
constexpr resize_t<(basic_vec<T, Abis>::size() + ...), basic_vec<T, Abis...[0]>>
cat(const basic_vec<T, Abis>&... xs) noexcept {
  using R = resize_t<(basic_vec<T, Abis>::size() + ...), basic_vec<T, Abis...[0]>>;
  ycxx::detail::simd_array<T, R::size()> a;
  int at = 0;
  ((void)[&] {
     for (int i = 0; i < xs.size(); ++i)
       a.v[at + i] = xs[i];
     at += xs.size();
   }(),
   ...);
  return ycxx::detail::simd_access::make<R>(ycxx::detail::simd_storage<T, R::size()>::from_array(a));
}
template <size_t Bytes, class... Abis>
constexpr resize_t<(basic_mask<Bytes, Abis>::size() + ...), basic_mask<Bytes, Abis...[0]>>
cat(const basic_mask<Bytes, Abis>&... xs) noexcept {
  using R = resize_t<(basic_mask<Bytes, Abis>::size() + ...), basic_mask<Bytes, Abis...[0]>>;
  using E = ycxx::detail::simd_mask_element<Bytes>;
  ycxx::detail::simd_array<E, R::size()> a;
  int at = 0;
  ((void)[&] {
     for (int i = 0; i < xs.size(); ++i)
       a.v[at + i] = xs[i] ? E(-1) : E(0);
     at += xs.size();
   }(),
   ...);
  return ycxx::detail::simd_access::make<R>(ycxx::detail::simd_mask_storage<Bytes, R::size()>::from_array(a));
}

} // namespace std::simd

namespace ycxx::detail {
template <class T>
consteval T simd_make_iota() {
  if constexpr (is_arithmetic_v<T>) {
    static_assert(simd_vectorizable<T>, "std::simd::iota<T>: T must be vectorizable");
    return T();
  } else {
    static_assert(simd_vec_type<T> && is_arithmetic_v<typename T::value_type>,
                  "std::simd::iota<T>: T must be an enabled basic_vec of an arithmetic type");
    using U = typename T::value_type;
    static_assert(T::size() - 1 <= std::numeric_limits<U>::max(), "std::simd::iota<T>: T::size() - 1 does not fit");
    return ycxx::detail::simd_access::make<T>(
        simd_storage<U, T::size()>::generate([](int i) { return static_cast<U>(i); }));
  }
}
} // namespace ycxx::detail

namespace std::simd {

template <class T>
inline constexpr T iota = ycxx::detail::simd_make_iota<T>();

// [simd.mask.reductions]
template <size_t Bytes, class Abi>
constexpr bool all_of(const basic_mask<Bytes, Abi>& k) noexcept {
  const auto& s = ycxx::detail::simd_access::data(k);
  for (auto w : ycxx::detail::simd_words(s).v)
    if (static_cast<decltype(w)>(~w) != 0)
      return false;
  return true;
}
template <size_t Bytes, class Abi>
constexpr bool any_of(const basic_mask<Bytes, Abi>& k) noexcept {
  const auto& s = ycxx::detail::simd_access::data(k);
  for (auto w : ycxx::detail::simd_words(s).v)
    if (w != 0)
      return true;
  return false;
}
template <size_t Bytes, class Abi>
constexpr bool none_of(const basic_mask<Bytes, Abi>& k) noexcept {
  return !std::simd::any_of(k);
}
template <size_t Bytes, class Abi>
constexpr ycxx::detail::simd_size_t reduce_count(const basic_mask<Bytes, Abi>& k) noexcept {
  const auto& s = ycxx::detail::simd_access::data(k);
  using S = remove_cvref_t<decltype(s)>;
  int bits = 0;
  for (auto w : ycxx::detail::simd_words(s).v)
    bits += std::popcount(w);
  return bits / static_cast<int>(sizeof(typename S::element_type) * __CHAR_BIT__);
}
template <size_t Bytes, class Abi>
constexpr ycxx::detail::simd_size_t reduce_min_index(const basic_mask<Bytes, Abi>& k) {
  ycxx::detail::precondition(std::simd::any_of(k), "std::simd::reduce_min_index: no element is set");
  const auto& s = ycxx::detail::simd_access::data(k);
  using S = remove_cvref_t<decltype(s)>;
  if constexpr (ycxx::detail::simd_words_ok<S>) {
    constexpr int ebits = static_cast<int>(sizeof(typename S::element_type) * __CHAR_BIT__);
    auto words = ycxx::detail::simd_words(s);
    constexpr int wbits = static_cast<int>(sizeof(words.v[0]) * __CHAR_BIT__);
    for (int j = 0; j < static_cast<int>(sizeof(words.v) / sizeof(words.v[0])); ++j)
      if (words.v[j] != 0)
        return (j * wbits + std::countr_zero(words.v[j])) / ebits;
    return 0;
  } else {
    for (int i = 0; i < k.size(); ++i)
      if (k[i])
        return i;
    return 0;
  }
}
template <size_t Bytes, class Abi>
constexpr ycxx::detail::simd_size_t reduce_max_index(const basic_mask<Bytes, Abi>& k) {
  ycxx::detail::precondition(std::simd::any_of(k), "std::simd::reduce_max_index: no element is set");
  const auto& s = ycxx::detail::simd_access::data(k);
  using S = remove_cvref_t<decltype(s)>;
  if constexpr (ycxx::detail::simd_words_ok<S>) {
    constexpr int ebits = static_cast<int>(sizeof(typename S::element_type) * __CHAR_BIT__);
    auto words = ycxx::detail::simd_words(s);
    constexpr int wbits = static_cast<int>(sizeof(words.v[0]) * __CHAR_BIT__);
    for (int j = static_cast<int>(sizeof(words.v) / sizeof(words.v[0])) - 1; j >= 0; --j)
      if (words.v[j] != 0)
        return (j * wbits + std::bit_width(words.v[j]) - 1) / ebits;
    return 0;
  } else {
    for (int i = k.size() - 1; i >= 0; --i)
      if (k[i])
        return i;
    return 0;
  }
}
constexpr bool all_of(same_as<bool> auto x) noexcept {
  return x;
}
constexpr bool any_of(same_as<bool> auto x) noexcept {
  return x;
}
constexpr bool none_of(same_as<bool> auto x) noexcept {
  return !x;
}
constexpr ycxx::detail::simd_size_t reduce_count(same_as<bool> auto x) noexcept {
  return x;
}
constexpr ycxx::detail::simd_size_t reduce_min_index(same_as<bool> auto x) {
  ycxx::detail::precondition(x, "std::simd::reduce_min_index: the argument is false");
  return 0;
}
constexpr ycxx::detail::simd_size_t reduce_max_index(same_as<bool> auto x) {
  ycxx::detail::precondition(x, "std::simd::reduce_max_index: the argument is false");
  return 0;
}

// [simd.alg]
template <class T, class Abi>
  requires totally_ordered<T>
constexpr basic_vec<T, Abi> min(const basic_vec<T, Abi>& a, const basic_vec<T, Abi>& b) noexcept {
  return simd_select_impl(b < a, b, a);
}
template <class T, class Abi>
  requires totally_ordered<T>
constexpr basic_vec<T, Abi> max(const basic_vec<T, Abi>& a, const basic_vec<T, Abi>& b) noexcept {
  return simd_select_impl(a < b, b, a);
}
template <class T, class Abi>
  requires totally_ordered<T>
constexpr pair<basic_vec<T, Abi>, basic_vec<T, Abi>> minmax(const basic_vec<T, Abi>& a,
                                                            const basic_vec<T, Abi>& b) noexcept {
  return pair{std::simd::min(a, b), std::simd::max(a, b)};
}
template <class T, class Abi>
  requires totally_ordered<T>
constexpr basic_vec<T, Abi> clamp(const basic_vec<T, Abi>& v, const basic_vec<T, Abi>& lo,
                                  const basic_vec<T, Abi>& hi) {
  ycxx::detail::precondition(std::simd::none_of(hi < lo), "std::simd::clamp: lo is greater than hi");
  return simd_select_impl(v < lo, lo, simd_select_impl(hi < v, hi, v));
}
template <class T, class U>
constexpr auto select(bool c, const T& a, const U& b) -> remove_cvref_t<decltype(c ? a : b)> {
  return c ? a : b;
}
template <size_t Bytes, class Abi, class T, class U>
constexpr auto select(const basic_mask<Bytes, Abi>& c, const T& a, const U& b) noexcept
    -> decltype(simd_select_impl(c, a, b)) {
  return simd_select_impl(c, a, b);
}

} // namespace std::simd

namespace ycxx::detail {

// GENERALIZED_SUM over the elements of x: halves combined with op until one element is left.
template <class V, class Op>
constexpr typename V::value_type simd_reduce_tree(const V& x, const Op& op) {
  using T = typename V::value_type;
  constexpr int n = V::size();
  if constexpr (n == 1) {
    return x[0];
  } else if constexpr (simd_known_identity<Op> && is_arithmetic_v<T> &&
                       std::remove_cvref_t<decltype(simd_access::data(std::declval<V&>()))>::is_vector) {
    // The std function objects on vector chunks: fold the chunks, then the lanes of the result.
    const auto& s = simd_access::data(x);
    auto acc = s.c[0];
    for (int k = 1; k < s.chunks; ++k)
      acc = op(acc, s.c[k]);
    T r = acc[0];
    for (int i = 1; i < s.lanes; ++i)
      r = static_cast<T>(op(r, static_cast<T>(acc[i])));
    return r;
  } else if constexpr (n % 2 == 0) {
    auto parts = std::simd::chunk<n / 2>(x);
    return ycxx::detail::simd_reduce_tree(op(parts[0], parts[1]), op);
  } else {
    auto parts = std::simd::chunk<n - 1>(x);
    return op(std::simd::vec<T, 1>(ycxx::detail::simd_reduce_tree(std::get<0>(parts), op)), std::get<1>(parts))[0];
  }
}

// Masked GENERALIZED_SUM: the selected elements, left to right (identity if none).
template <class V, class Op>
constexpr typename V::value_type simd_reduce_masked(const V& x, const typename V::mask_type& k, const Op& op,
                                                    typename V::value_type identity) {
  using T = typename V::value_type;
  if (std::simd::none_of(k))
    return identity;
  if constexpr (simd_known_identity<Op> && is_integral_v<T>) {
    // The identities are exact for integers: reduce everything with the others replaced.
    return ycxx::detail::simd_reduce_tree(std::simd::select(k, x, V(identity)), op);
  } else {
    int i = std::simd::reduce_min_index(k);
    std::simd::vec<T, 1> acc(x[i]);
    for (++i; i < V::size(); ++i)
      if (k[i])
        acc = op(acc, std::simd::vec<T, 1>(x[i]));
    return acc[0];
  }
}

// The fill value for a masked reduce_min/reduce_max: no element compares below/above it.
template <class T, bool Min>
constexpr T simd_minmax_fill() noexcept {
  if constexpr (std::numeric_limits<T>::has_infinity)
    return Min ? std::numeric_limits<T>::infinity() : -std::numeric_limits<T>::infinity();
  else
    return Min ? std::numeric_limits<T>::max() : std::numeric_limits<T>::lowest();
}

template <class V, bool Min>
constexpr typename V::value_type simd_reduce_minmax(const V& x) noexcept {
  constexpr int n = V::size();
  if constexpr (n == 1) {
    return x[0];
  } else if constexpr (n % 2 == 0) {
    auto parts = std::simd::chunk<n / 2>(x);
    if constexpr (Min)
      return ycxx::detail::simd_reduce_minmax<std::remove_cvref_t<decltype(parts[0])>, Min>(
          std::simd::min(parts[0], parts[1]));
    else
      return ycxx::detail::simd_reduce_minmax<std::remove_cvref_t<decltype(parts[0])>, Min>(
          std::simd::max(parts[0], parts[1]));
  } else {
    auto parts = std::simd::chunk<n - 1>(x);
    auto r = ycxx::detail::simd_reduce_minmax<std::remove_cvref_t<decltype(std::get<0>(parts))>, Min>(std::get<0>(parts));
    auto last = std::get<1>(parts)[0];
    if constexpr (Min)
      return last < r ? last : r;
    else
      return r < last ? last : r;
  }
}

} // namespace ycxx::detail

namespace std::simd {

// [simd.reductions]
template <class T, class Abi, class BinaryOperation = plus<>>
  requires ycxx::detail::reduction_binary_operation<BinaryOperation, T>
constexpr T reduce(const basic_vec<T, Abi>& x, BinaryOperation binary_op = {}) {
  return ycxx::detail::simd_reduce_tree(x, binary_op);
}
template <class T, class Abi, class BinaryOperation = plus<>>
  requires(ycxx::detail::reduction_binary_operation<BinaryOperation, T> &&
           ycxx::detail::simd_known_identity<BinaryOperation>)
constexpr T reduce(const basic_vec<T, Abi>& x, const typename basic_vec<T, Abi>::mask_type& mask,
                   BinaryOperation binary_op = {}) {
  return ycxx::detail::simd_reduce_masked(x, mask, binary_op, ycxx::detail::simd_identity<T, BinaryOperation>());
}
template <class T, class Abi, class BinaryOperation>
  requires ycxx::detail::reduction_binary_operation<BinaryOperation, T>
constexpr T reduce(const basic_vec<T, Abi>& x, const typename basic_vec<T, Abi>::mask_type& mask,
                   BinaryOperation binary_op, type_identity_t<T> identity_element) {
  return ycxx::detail::simd_reduce_masked(x, mask, binary_op, identity_element);
}
template <class T, class BinaryOperation = plus<>>
  requires(ycxx::detail::simd_vectorizable<T> && ycxx::detail::reduction_binary_operation<BinaryOperation, T>)
constexpr T reduce(const T& x, BinaryOperation = {}) {
  return x;
}
template <class T, class BinaryOperation = plus<>>
  requires(ycxx::detail::simd_vectorizable<T> && ycxx::detail::reduction_binary_operation<BinaryOperation, T> &&
           ycxx::detail::simd_known_identity<BinaryOperation>)
constexpr T reduce(const T& x, same_as<bool> auto mask, BinaryOperation = {}) {
  return mask ? x : ycxx::detail::simd_identity<T, BinaryOperation>();
}
template <class T, class BinaryOperation>
  requires(ycxx::detail::simd_vectorizable<T> && ycxx::detail::reduction_binary_operation<BinaryOperation, T>)
constexpr T reduce(const T& x, same_as<bool> auto mask, BinaryOperation, type_identity_t<T> identity_element) {
  return mask ? x : identity_element;
}

template <class T, class Abi>
  requires totally_ordered<T>
constexpr T reduce_min(const basic_vec<T, Abi>& x) noexcept {
  return ycxx::detail::simd_reduce_minmax<basic_vec<T, Abi>, true>(x);
}
template <class T, class Abi>
  requires totally_ordered<T>
constexpr T reduce_min(const basic_vec<T, Abi>& x, const typename basic_vec<T, Abi>::mask_type& mask) noexcept {
  if (std::simd::none_of(mask))
    return numeric_limits<T>::max();
  return ycxx::detail::simd_reduce_minmax<basic_vec<T, Abi>, true>(
      simd_select_impl(mask, x, basic_vec<T, Abi>(ycxx::detail::simd_minmax_fill<T, true>())));
}
template <class T, class Abi>
  requires totally_ordered<T>
constexpr T reduce_max(const basic_vec<T, Abi>& x) noexcept {
  return ycxx::detail::simd_reduce_minmax<basic_vec<T, Abi>, false>(x);
}
template <class T, class Abi>
  requires totally_ordered<T>
constexpr T reduce_max(const basic_vec<T, Abi>& x, const typename basic_vec<T, Abi>::mask_type& mask) noexcept {
  if (std::simd::none_of(mask))
    return numeric_limits<T>::lowest();
  return ycxx::detail::simd_reduce_minmax<basic_vec<T, Abi>, false>(
      simd_select_impl(mask, x, basic_vec<T, Abi>(ycxx::detail::simd_minmax_fill<T, false>())));
}
template <class T>
  requires(ycxx::detail::simd_vectorizable<T> && totally_ordered<T>)
constexpr T reduce_min(const T& x) noexcept {
  return x;
}
template <class T>
  requires(ycxx::detail::simd_vectorizable<T> && totally_ordered<T>)
constexpr T reduce_min(const T& x, same_as<bool> auto mask) noexcept {
  return mask ? x : numeric_limits<T>::max();
}
template <class T>
  requires(ycxx::detail::simd_vectorizable<T> && totally_ordered<T>)
constexpr T reduce_max(const T& x) noexcept {
  return x;
}
template <class T>
  requires(ycxx::detail::simd_vectorizable<T> && totally_ordered<T>)
constexpr T reduce_max(const T& x, same_as<bool> auto mask) noexcept {
  return mask ? x : numeric_limits<T>::lowest();
}

} // namespace std::simd

namespace ycxx::detail {

// The default V of the loads and gathers.
template <class V, class U>
using simd_load_vec_t = std::conditional_t<std::is_void_v<V>, std::simd::basic_vec<U>, V>;
template <class V, class U, class I>
using simd_gather_vec_t = std::conditional_t<std::is_void_v<V>, std::simd::vec<U, I::size()>, V>;

// [simd.loadstore]/7, [simd.permute.memory]/7: the Mandates of the loads.
template <class V, class U, class... Flags>
consteval bool simd_check_load() {
  static_assert(simd_vectorizable<U>, "std::simd: the range's value type is not vectorizable");
  static_assert(std::same_as<std::remove_cvref_t<V>, V> && simd_vec_type<V>,
                "std::simd: V is not an enabled specialization of basic_vec");
  if constexpr (simd_vec_type<V>) {
    static_assert(simd_explicitly_convertible_to<U, typename V::value_type>,
                  "std::simd: the range's value type does not convert to V::value_type");
    if constexpr (!simd_has_convert<Flags...>)
      static_assert(simd_value_preserving<U, typename V::value_type>(),
                    "std::simd: the conversion from the range's value type to V::value_type is not value-preserving "
                    "(pass flag_convert)");
  }
  return true;
}
// [simd.loadstore]/17, [simd.permute.memory]/16: the Mandates of the stores.
template <class T, class U, class... Flags>
consteval bool simd_check_store() {
  static_assert(simd_vectorizable<U>, "std::simd: the range's value type is not vectorizable");
  if constexpr (!simd_has_convert<Flags...>)
    static_assert(simd_value_preserving<T, U>(),
                  "std::simd: the conversion from the element type to the range's value type is not value-preserving "
                  "(pass flag_convert)");
  return true;
}

// The alignment the flags promise for a pointer to U loaded into or stored from V (0: none).
template <class V, class U, class... Flags>
consteval std::size_t simd_flags_alignment() {
  std::size_t a = simd_overalignment<Flags...>();
  if constexpr (simd_has_aligned<Flags...>)
    a = a > std::simd::alignment_v<V, U> ? a : std::simd::alignment_v<V, U>;
  return a;
}
template <std::size_t A, class P>
constexpr P* simd_assume_aligned(P* p) noexcept {
  if constexpr (A > 1) {
    if !consteval {
      ycxx::detail::precondition(reinterpret_cast<std::uintptr_t>(p) % A == 0,
                                 "std::simd: the pointer is not aligned as the flags promise");
      return static_cast<P*>(__builtin_assume_aligned(p, A));
    }
  }
  return p;
}

// partial_load: the elements i < n (selected by k, if given) of p, converted.
template <class V, class... Flags, class U>
constexpr V simd_load(U* p, std::size_t n, const typename V::mask_type* k) {
  using T = typename V::value_type;
  constexpr int N = V::size();
  using S = std::remove_cvref_t<decltype(simd_access::data(std::declval<V&>()))>;
  p = ycxx::detail::simd_assume_aligned<simd_flags_alignment<V, std::remove_cv_t<U>, Flags...>()>(p);
  if !consteval {
    if constexpr (std::is_same_v<std::remove_cv_t<U>, T> && S::is_vector) {
      if (k == nullptr && n >= static_cast<std::size_t>(N)) {
        S s;
        __builtin_memcpy(__builtin_addressof(s), p, sizeof(s));
        return simd_access::make<V>(s);
      }
    }
  }
  const auto* ks = k == nullptr ? nullptr : __builtin_addressof(simd_access::data(*k));
  return simd_access::make<V>(S::generate([&](int i) {
    return (ks == nullptr || ks->get(i) != 0) && static_cast<std::size_t>(i) < n ? static_cast<T>(p[i]) : T();
  }));
}
// partial_store
template <class... Flags, class T, class Abi, class U>
constexpr void simd_store(const std::simd::basic_vec<T, Abi>& v, U* p, std::size_t n,
                          const typename std::simd::basic_vec<T, Abi>::mask_type* k) {
  using V = std::simd::basic_vec<T, Abi>;
  constexpr int N = V::size();
  using S = std::remove_cvref_t<decltype(simd_access::data(std::declval<V&>()))>;
  p = ycxx::detail::simd_assume_aligned<simd_flags_alignment<V, U, Flags...>()>(p);
  if !consteval {
    if constexpr (std::is_same_v<U, T> && S::is_vector) {
      if (k == nullptr && n >= static_cast<std::size_t>(N)) {
        const S& s = simd_access::data(v);
        __builtin_memcpy(p, __builtin_addressof(s), sizeof(s));
        return;
      }
    }
  }
  for (int i = 0; i < N && static_cast<std::size_t>(i) < n; ++i)
    if (k == nullptr || (*k)[i])
      p[i] = static_cast<U>(v[i]);
}

template <class R, class V>
constexpr void simd_check_unchecked_size(R& r) {
  if constexpr (simd_static_size<R> != std::dynamic_extent)
    static_assert(simd_static_size<R> >= static_cast<std::size_t>(V::size()),
                  "std::simd: unchecked load/store: the range is smaller than V::size()");
  ycxx::detail::precondition(std::ranges::size(r) >= static_cast<std::size_t>(V::size()),
                             "std::simd: unchecked load/store: the range is smaller than V::size()");
}

template <class R, class T>
concept simd_store_range =
    std::indirectly_writable<std::ranges::iterator_t<R>, std::ranges::range_value_t<R>> &&
    simd_explicitly_convertible_to<T, std::ranges::range_value_t<R>>;

} // namespace ycxx::detail

namespace std::simd {

// [simd.loadstore]
template <class V = void, ranges::contiguous_range R, class... Flags>
  requires ranges::sized_range<R>
constexpr ycxx::detail::simd_load_vec_t<V, ranges::range_value_t<R>> unchecked_load(R&& r, flags<Flags...> = {}) {
  using VV = ycxx::detail::simd_load_vec_t<V, ranges::range_value_t<R>>;
  static_assert(ycxx::detail::simd_check_load<VV, ranges::range_value_t<R>, Flags...>());
  ycxx::detail::simd_check_unchecked_size<R, VV>(r);
  return ycxx::detail::simd_load<VV, Flags...>(ranges::data(r), ranges::size(r), nullptr);
}
template <class V = void, ranges::contiguous_range R, class... Flags>
  requires ranges::sized_range<R>
constexpr ycxx::detail::simd_load_vec_t<V, ranges::range_value_t<R>>
unchecked_load(R&& r, const typename ycxx::detail::simd_load_vec_t<V, ranges::range_value_t<R>>::mask_type& k,
               flags<Flags...> = {}) {
  using VV = ycxx::detail::simd_load_vec_t<V, ranges::range_value_t<R>>;
  static_assert(ycxx::detail::simd_check_load<VV, ranges::range_value_t<R>, Flags...>());
  ycxx::detail::simd_check_unchecked_size<R, VV>(r);
  return ycxx::detail::simd_load<VV, Flags...>(ranges::data(r), ranges::size(r), __builtin_addressof(k));
}
template <class V = void, contiguous_iterator I, class... Flags>
constexpr ycxx::detail::simd_load_vec_t<V, iter_value_t<I>> unchecked_load(I first, iter_difference_t<I> n,
                                                                          flags<Flags...> f = {}) {
  return std::simd::unchecked_load<V>(span<const iter_value_t<I>>(first, static_cast<size_t>(n)), f);
}
template <class V = void, contiguous_iterator I, class... Flags>
constexpr ycxx::detail::simd_load_vec_t<V, iter_value_t<I>>
unchecked_load(I first, iter_difference_t<I> n,
               const typename ycxx::detail::simd_load_vec_t<V, iter_value_t<I>>::mask_type& k, flags<Flags...> f = {}) {
  return std::simd::unchecked_load<V>(span<const iter_value_t<I>>(first, static_cast<size_t>(n)), k, f);
}
template <class V = void, contiguous_iterator I, sized_sentinel_for<I> S, class... Flags>
constexpr ycxx::detail::simd_load_vec_t<V, iter_value_t<I>> unchecked_load(I first, S last, flags<Flags...> f = {}) {
  return std::simd::unchecked_load<V>(span<const iter_value_t<I>>(first, last), f);
}
template <class V = void, contiguous_iterator I, sized_sentinel_for<I> S, class... Flags>
constexpr ycxx::detail::simd_load_vec_t<V, iter_value_t<I>>
unchecked_load(I first, S last, const typename ycxx::detail::simd_load_vec_t<V, iter_value_t<I>>::mask_type& k,
               flags<Flags...> f = {}) {
  return std::simd::unchecked_load<V>(span<const iter_value_t<I>>(first, last), k, f);
}

template <class V = void, ranges::contiguous_range R, class... Flags>
  requires ranges::sized_range<R>
constexpr ycxx::detail::simd_load_vec_t<V, ranges::range_value_t<R>> partial_load(R&& r, flags<Flags...> = {}) {
  using VV = ycxx::detail::simd_load_vec_t<V, ranges::range_value_t<R>>;
  static_assert(ycxx::detail::simd_check_load<VV, ranges::range_value_t<R>, Flags...>());
  return ycxx::detail::simd_load<VV, Flags...>(ranges::data(r), ranges::size(r), nullptr);
}
template <class V = void, ranges::contiguous_range R, class... Flags>
  requires ranges::sized_range<R>
constexpr ycxx::detail::simd_load_vec_t<V, ranges::range_value_t<R>>
partial_load(R&& r, const typename ycxx::detail::simd_load_vec_t<V, ranges::range_value_t<R>>::mask_type& k,
             flags<Flags...> = {}) {
  using VV = ycxx::detail::simd_load_vec_t<V, ranges::range_value_t<R>>;
  static_assert(ycxx::detail::simd_check_load<VV, ranges::range_value_t<R>, Flags...>());
  return ycxx::detail::simd_load<VV, Flags...>(ranges::data(r), ranges::size(r), __builtin_addressof(k));
}
template <class V = void, contiguous_iterator I, class... Flags>
constexpr ycxx::detail::simd_load_vec_t<V, iter_value_t<I>> partial_load(I first, iter_difference_t<I> n,
                                                                        flags<Flags...> f = {}) {
  return std::simd::partial_load<V>(span<const iter_value_t<I>>(first, static_cast<size_t>(n)), f);
}
template <class V = void, contiguous_iterator I, class... Flags>
constexpr ycxx::detail::simd_load_vec_t<V, iter_value_t<I>>
partial_load(I first, iter_difference_t<I> n,
             const typename ycxx::detail::simd_load_vec_t<V, iter_value_t<I>>::mask_type& k, flags<Flags...> f = {}) {
  return std::simd::partial_load<V>(span<const iter_value_t<I>>(first, static_cast<size_t>(n)), k, f);
}
template <class V = void, contiguous_iterator I, sized_sentinel_for<I> S, class... Flags>
constexpr ycxx::detail::simd_load_vec_t<V, iter_value_t<I>> partial_load(I first, S last, flags<Flags...> f = {}) {
  return std::simd::partial_load<V>(span<const iter_value_t<I>>(first, last), f);
}
template <class V = void, contiguous_iterator I, sized_sentinel_for<I> S, class... Flags>
constexpr ycxx::detail::simd_load_vec_t<V, iter_value_t<I>>
partial_load(I first, S last, const typename ycxx::detail::simd_load_vec_t<V, iter_value_t<I>>::mask_type& k,
             flags<Flags...> f = {}) {
  return std::simd::partial_load<V>(span<const iter_value_t<I>>(first, last), k, f);
}

template <class T, class Abi, ranges::contiguous_range R, class... Flags>
  requires(ranges::sized_range<R> && ycxx::detail::simd_store_range<R, T>)
constexpr void unchecked_store(const basic_vec<T, Abi>& v, R&& r, flags<Flags...> = {}) {
  static_assert(ycxx::detail::simd_check_store<T, ranges::range_value_t<R>, Flags...>());
  ycxx::detail::simd_check_unchecked_size<R, basic_vec<T, Abi>>(r);
  ycxx::detail::simd_store<Flags...>(v, ranges::data(r), ranges::size(r), nullptr);
}
template <class T, class Abi, ranges::contiguous_range R, class... Flags>
  requires(ranges::sized_range<R> && ycxx::detail::simd_store_range<R, T>)
constexpr void unchecked_store(const basic_vec<T, Abi>& v, R&& r, const typename basic_vec<T, Abi>::mask_type& mask,
                               flags<Flags...> = {}) {
  static_assert(ycxx::detail::simd_check_store<T, ranges::range_value_t<R>, Flags...>());
  ycxx::detail::simd_check_unchecked_size<R, basic_vec<T, Abi>>(r);
  ycxx::detail::simd_store<Flags...>(v, ranges::data(r), ranges::size(r), __builtin_addressof(mask));
}
template <class T, class Abi, contiguous_iterator I, class... Flags>
  requires ycxx::detail::simd_store_range<span<iter_value_t<I>>, T>
constexpr void unchecked_store(const basic_vec<T, Abi>& v, I first, iter_difference_t<I> n, flags<Flags...> f = {}) {
  std::simd::unchecked_store(v, span<iter_value_t<I>>(first, static_cast<size_t>(n)), f);
}
template <class T, class Abi, contiguous_iterator I, class... Flags>
  requires ycxx::detail::simd_store_range<span<iter_value_t<I>>, T>
constexpr void unchecked_store(const basic_vec<T, Abi>& v, I first, iter_difference_t<I> n,
                               const typename basic_vec<T, Abi>::mask_type& mask, flags<Flags...> f = {}) {
  std::simd::unchecked_store(v, span<iter_value_t<I>>(first, static_cast<size_t>(n)), mask, f);
}
template <class T, class Abi, contiguous_iterator I, sized_sentinel_for<I> S, class... Flags>
  requires ycxx::detail::simd_store_range<span<iter_value_t<I>>, T>
constexpr void unchecked_store(const basic_vec<T, Abi>& v, I first, S last, flags<Flags...> f = {}) {
  std::simd::unchecked_store(v, span<iter_value_t<I>>(first, last), f);
}
template <class T, class Abi, contiguous_iterator I, sized_sentinel_for<I> S, class... Flags>
  requires ycxx::detail::simd_store_range<span<iter_value_t<I>>, T>
constexpr void unchecked_store(const basic_vec<T, Abi>& v, I first, S last,
                               const typename basic_vec<T, Abi>::mask_type& mask, flags<Flags...> f = {}) {
  std::simd::unchecked_store(v, span<iter_value_t<I>>(first, last), mask, f);
}

template <class T, class Abi, ranges::contiguous_range R, class... Flags>
  requires(ranges::sized_range<R> && ycxx::detail::simd_store_range<R, T>)
constexpr void partial_store(const basic_vec<T, Abi>& v, R&& r, flags<Flags...> = {}) {
  static_assert(ycxx::detail::simd_check_store<T, ranges::range_value_t<R>, Flags...>());
  ycxx::detail::simd_store<Flags...>(v, ranges::data(r), ranges::size(r), nullptr);
}
template <class T, class Abi, ranges::contiguous_range R, class... Flags>
  requires(ranges::sized_range<R> && ycxx::detail::simd_store_range<R, T>)
constexpr void partial_store(const basic_vec<T, Abi>& v, R&& r, const typename basic_vec<T, Abi>::mask_type& mask,
                             flags<Flags...> = {}) {
  static_assert(ycxx::detail::simd_check_store<T, ranges::range_value_t<R>, Flags...>());
  ycxx::detail::simd_store<Flags...>(v, ranges::data(r), ranges::size(r), __builtin_addressof(mask));
}
template <class T, class Abi, contiguous_iterator I, class... Flags>
  requires ycxx::detail::simd_store_range<span<iter_value_t<I>>, T>
constexpr void partial_store(const basic_vec<T, Abi>& v, I first, iter_difference_t<I> n, flags<Flags...> f = {}) {
  std::simd::partial_store(v, span<iter_value_t<I>>(first, static_cast<size_t>(n)), f);
}
template <class T, class Abi, contiguous_iterator I, class... Flags>
  requires ycxx::detail::simd_store_range<span<iter_value_t<I>>, T>
constexpr void partial_store(const basic_vec<T, Abi>& v, I first, iter_difference_t<I> n,
                             const typename basic_vec<T, Abi>::mask_type& mask, flags<Flags...> f = {}) {
  std::simd::partial_store(v, span<iter_value_t<I>>(first, static_cast<size_t>(n)), mask, f);
}
template <class T, class Abi, contiguous_iterator I, sized_sentinel_for<I> S, class... Flags>
  requires ycxx::detail::simd_store_range<span<iter_value_t<I>>, T>
constexpr void partial_store(const basic_vec<T, Abi>& v, I first, S last, flags<Flags...> f = {}) {
  std::simd::partial_store(v, span<iter_value_t<I>>(first, last), f);
}
template <class T, class Abi, contiguous_iterator I, sized_sentinel_for<I> S, class... Flags>
  requires ycxx::detail::simd_store_range<span<iter_value_t<I>>, T>
constexpr void partial_store(const basic_vec<T, Abi>& v, I first, S last,
                             const typename basic_vec<T, Abi>::mask_type& mask, flags<Flags...> f = {}) {
  std::simd::partial_store(v, span<iter_value_t<I>>(first, last), mask, f);
}

// [simd.permute.static]
inline constexpr ycxx::detail::simd_size_t zero_element = numeric_limits<ycxx::detail::simd_size_t>::min();
inline constexpr ycxx::detail::simd_size_t uninit_element = numeric_limits<ycxx::detail::simd_size_t>::min() + 1;

} // namespace std::simd

namespace ycxx::detail {

// The default N of the static permute (any value that is not a width).
inline constexpr simd_size_t simd_permute_default = std::numeric_limits<simd_size_t>::min();
template <simd_size_t N, class V>
inline constexpr simd_size_t simd_permute_size = N == simd_permute_default ? V::size() : N;

template <class IdxMap>
concept simd_index_map = std::integral<std::invoke_result_t<IdxMap&, simd_size_t>> ||
                         std::integral<std::invoke_result_t<IdxMap&, simd_size_t, simd_size_t>>;

// perm-fn<I>() ([simd.permute.static]/1.2)
template <simd_size_t Src, class V>
constexpr typename V::value_type simd_permute_pick(const V& v) {
  static_assert(Src == std::simd::zero_element || Src == std::simd::uninit_element || (Src >= 0 && Src < V::size()),
                "std::simd::permute: the index map gives an index out of range");
  if constexpr (Src == std::simd::zero_element || Src == std::simd::uninit_element)
    return typename V::value_type();
  else
    return v[Src];
}
template <simd_size_t I, class V, class IdxMap>
constexpr typename V::value_type simd_permute_element(const V& v, IdxMap& idxmap) {
  if constexpr (requires { idxmap(I, V::size()); }) {
    constexpr auto src = idxmap(simd_size_t(I), simd_size_t(V::size()));
    return ycxx::detail::simd_permute_pick<static_cast<simd_size_t>(src)>(v);
  } else {
    constexpr auto src = idxmap(simd_size_t(I));
    return ycxx::detail::simd_permute_pick<static_cast<simd_size_t>(src)>(v);
  }
}

} // namespace ycxx::detail

namespace std::simd {

template <ycxx::detail::simd_size_t N = ycxx::detail::simd_permute_default, class V, class IdxMap>
  requires((ycxx::detail::simd_vec_type<V> || ycxx::detail::simd_mask_type<V>) && ycxx::detail::simd_index_map<IdxMap>)
constexpr resize_t<ycxx::detail::simd_permute_size<N, V>, V> permute(const V& v, IdxMap&& idxmap) {
  using R = resize_t<ycxx::detail::simd_permute_size<N, V>, V>;
  return R([&](auto i) { return ycxx::detail::simd_permute_element<decltype(i)::value>(v, idxmap); });
}

// [simd.permute.dynamic]
template <class V, ycxx::detail::simd_integral I>
  requires(ycxx::detail::simd_vec_type<V> || ycxx::detail::simd_mask_type<V>)
constexpr resize_t<I::size(), V> permute(const V& v, const I& indices) {
  using R = resize_t<I::size(), V>;
  using T = typename V::value_type;
  if constexpr (ycxx::detail::simd_vec_type<V>) {
    using S = remove_cvref_t<decltype(ycxx::detail::simd_access::data(declval<R&>()))>;
    return ycxx::detail::simd_access::make<R>(S::generate([&](int i) -> T {
      auto j = indices[i];
      ycxx::detail::precondition(j >= 0 && j < V::size(), "std::simd::permute: index out of range");
      return v[static_cast<ycxx::detail::simd_size_t>(j)];
    }));
  } else {
    using S = remove_cvref_t<decltype(ycxx::detail::simd_access::data(declval<R&>()))>;
    using E = typename S::element_type;
    return ycxx::detail::simd_access::make<R>(S::generate([&](int i) {
      auto j = indices[i];
      ycxx::detail::precondition(j >= 0 && j < V::size(), "std::simd::permute: index out of range");
      return v[static_cast<ycxx::detail::simd_size_t>(j)] ? E(-1) : E(0);
    }));
  }
}

} // namespace std::simd

namespace ycxx::detail {
// Builds a V from values of its elements (bool for a mask).
template <class V, class F>
constexpr V simd_build(F&& f) {
  using S = std::remove_cvref_t<decltype(simd_access::data(std::declval<V&>()))>;
  if constexpr (simd_mask_type<V>) {
    using E = typename S::element_type;
    return simd_access::make<V>(S::generate([&](int i) { return f(i) ? E(-1) : E(0); }));
  } else {
    return simd_access::make<V>(S::generate(f));
  }
}
template <class V, class M>
constexpr V simd_compress(const V& v, const M& selector, const typename V::value_type& fill) {
  simd_array<typename V::value_type, V::size()> a;
  int n = 0;
  for (int i = 0; i < V::size(); ++i)
    if (selector[i])
      a.v[n++] = v[i];
  for (int i = n; i < V::size(); ++i)
    a.v[i] = fill;
  return ycxx::detail::simd_build<V>([&](int i) { return a.v[i]; });
}
template <class V, class M>
constexpr V simd_expand(const V& v, const M& selector, const V& original) {
  simd_array<typename V::value_type, V::size()> a;
  int n = 0;
  for (int i = 0; i < V::size(); ++i)
    a.v[i] = selector[i] ? v[n++] : original[i];
  return ycxx::detail::simd_build<V>([&](int i) { return a.v[i]; });
}
} // namespace ycxx::detail

namespace std::simd {

// [simd.permute.mask]
template <ycxx::detail::simd_vec_type V>
constexpr V compress(const V& v, const typename V::mask_type& selector) {
  return ycxx::detail::simd_compress(v, selector, typename V::value_type());
}
template <ycxx::detail::simd_mask_type V>
constexpr V compress(const V& v, const type_identity_t<V>& selector) {
  return ycxx::detail::simd_compress(v, selector, false);
}
template <ycxx::detail::simd_vec_type V>
constexpr V compress(const V& v, const typename V::mask_type& selector, const typename V::value_type& fill_value) {
  return ycxx::detail::simd_compress(v, selector, fill_value);
}
template <ycxx::detail::simd_mask_type V>
constexpr V compress(const V& v, const type_identity_t<V>& selector, const typename V::value_type& fill_value) {
  return ycxx::detail::simd_compress(v, selector, fill_value);
}
template <ycxx::detail::simd_vec_type V>
constexpr V expand(const V& v, const typename V::mask_type& selector, const V& original = {}) {
  return ycxx::detail::simd_expand(v, selector, original);
}
template <ycxx::detail::simd_mask_type V>
constexpr V expand(const V& v, const type_identity_t<V>& selector, const V& original = {}) {
  return ycxx::detail::simd_expand(v, selector, original);
}

} // namespace std::simd

namespace ycxx::detail {

template <class R, class V>
concept simd_gather_range = simd_vectorizable<std::ranges::range_value_t<R>> &&
                            simd_explicitly_convertible_to<std::ranges::range_value_t<R>, typename V::value_type>;

template <class V, class I, class... Flags, class U>
constexpr V simd_gather(U* p, std::size_t n, const typename I::mask_type* k, const I& indices) {
  static_assert(V::size() == I::size(), "std::simd: gather: V::size() != I::size()");
  using T = typename V::value_type;
  p = ycxx::detail::simd_assume_aligned<simd_flags_alignment<V, std::remove_cv_t<U>, Flags...>()>(p);
  using S = std::remove_cvref_t<decltype(simd_access::data(std::declval<V&>()))>;
  return simd_access::make<V>(S::generate([&](int i) {
    auto j = indices[i];
    return (k == nullptr || (*k)[i]) && j >= 0 && static_cast<std::size_t>(j) < n ? static_cast<T>(p[j]) : T();
  }));
}
template <class... Flags, class V, class I, class U>
constexpr void simd_scatter(const V& v, U* p, std::size_t n, const typename I::mask_type* k, const I& indices) {
  p = ycxx::detail::simd_assume_aligned<simd_flags_alignment<V, U, Flags...>()>(p);
  for (int i = 0; i < V::size(); ++i) {
    auto j = indices[i];
    if ((k == nullptr || (*k)[i]) && j >= 0 && static_cast<std::size_t>(j) < n)
      p[j] = static_cast<U>(v[i]);
  }
}
template <class I>
constexpr void simd_check_indices(const I& indices, const typename I::mask_type& k, std::size_t n) {
  if consteval {
  } else {
    if constexpr (!cfg::hardened)
      return;
  }
  for (int i = 0; i < I::size(); ++i)
    if (k[i])
      ycxx::detail::precondition(indices[i] >= 0 && static_cast<std::size_t>(indices[i]) < n,
                                 "std::simd: unchecked gather/scatter: index out of range");
}

} // namespace ycxx::detail

namespace std::simd {

// [simd.permute.memory]
template <class V = void, ranges::contiguous_range R, ycxx::detail::simd_integral I, class... Flags>
  requires(ranges::sized_range<R> && ycxx::detail::simd_gather_range<R, ycxx::detail::simd_gather_vec_t<V, ranges::range_value_t<R>, I>>)
constexpr ycxx::detail::simd_gather_vec_t<V, ranges::range_value_t<R>, I> partial_gather_from(R&& in, const I& indices,
                                                                                              flags<Flags...> = {}) {
  using VV = ycxx::detail::simd_gather_vec_t<V, ranges::range_value_t<R>, I>;
  static_assert(ycxx::detail::simd_check_load<VV, ranges::range_value_t<R>, Flags...>());
  return ycxx::detail::simd_gather<VV, I, Flags...>(ranges::data(in), ranges::size(in), nullptr, indices);
}
template <class V = void, ranges::contiguous_range R, ycxx::detail::simd_integral I, class... Flags>
  requires(ranges::sized_range<R> && ycxx::detail::simd_gather_range<R, ycxx::detail::simd_gather_vec_t<V, ranges::range_value_t<R>, I>>)
constexpr ycxx::detail::simd_gather_vec_t<V, ranges::range_value_t<R>, I>
partial_gather_from(R&& in, const typename I::mask_type& mask, const I& indices, flags<Flags...> = {}) {
  using VV = ycxx::detail::simd_gather_vec_t<V, ranges::range_value_t<R>, I>;
  static_assert(ycxx::detail::simd_check_load<VV, ranges::range_value_t<R>, Flags...>());
  return ycxx::detail::simd_gather<VV, I, Flags...>(ranges::data(in), ranges::size(in), __builtin_addressof(mask),
                                                    indices);
}
template <class V = void, ranges::contiguous_range R, ycxx::detail::simd_integral I, class... Flags>
  requires(ranges::sized_range<R> && ycxx::detail::simd_gather_range<R, ycxx::detail::simd_gather_vec_t<V, ranges::range_value_t<R>, I>>)
constexpr ycxx::detail::simd_gather_vec_t<V, ranges::range_value_t<R>, I> unchecked_gather_from(R&& in, const I& indices,
                                                                                                flags<Flags...> f = {}) {
  ycxx::detail::simd_check_indices(indices, typename I::mask_type(true), ranges::size(in));
  return std::simd::partial_gather_from<V>(in, indices, f);
}
template <class V = void, ranges::contiguous_range R, ycxx::detail::simd_integral I, class... Flags>
  requires(ranges::sized_range<R> && ycxx::detail::simd_gather_range<R, ycxx::detail::simd_gather_vec_t<V, ranges::range_value_t<R>, I>>)
constexpr ycxx::detail::simd_gather_vec_t<V, ranges::range_value_t<R>, I>
unchecked_gather_from(R&& in, const typename I::mask_type& mask, const I& indices, flags<Flags...> f = {}) {
  ycxx::detail::simd_check_indices(indices, mask, ranges::size(in));
  return std::simd::partial_gather_from<V>(in, mask, indices, f);
}

template <ycxx::detail::simd_vec_type V, ranges::contiguous_range R, ycxx::detail::simd_integral I, class... Flags>
  requires(ranges::sized_range<R> && V::size() == I::size() &&
           ycxx::detail::simd_store_range<R, typename V::value_type>)
constexpr void partial_scatter_to(const V& v, R&& out, const I& indices, flags<Flags...> = {}) {
  static_assert(ycxx::detail::simd_check_store<typename V::value_type, ranges::range_value_t<R>, Flags...>());
  ycxx::detail::simd_scatter<Flags...>(v, ranges::data(out), ranges::size(out), nullptr, indices);
}
template <ycxx::detail::simd_vec_type V, ranges::contiguous_range R, ycxx::detail::simd_integral I, class... Flags>
  requires(ranges::sized_range<R> && V::size() == I::size() &&
           ycxx::detail::simd_store_range<R, typename V::value_type>)
constexpr void partial_scatter_to(const V& v, R&& out, const typename I::mask_type& mask, const I& indices,
                                  flags<Flags...> = {}) {
  static_assert(ycxx::detail::simd_check_store<typename V::value_type, ranges::range_value_t<R>, Flags...>());
  ycxx::detail::simd_scatter<Flags...>(v, ranges::data(out), ranges::size(out), __builtin_addressof(mask), indices);
}
template <ycxx::detail::simd_vec_type V, ranges::contiguous_range R, ycxx::detail::simd_integral I, class... Flags>
  requires(ranges::sized_range<R> && V::size() == I::size() &&
           ycxx::detail::simd_store_range<R, typename V::value_type>)
constexpr void unchecked_scatter_to(const V& v, R&& out, const I& indices, flags<Flags...> f = {}) {
  ycxx::detail::simd_check_indices(indices, typename I::mask_type(true), ranges::size(out));
  std::simd::partial_scatter_to(v, out, indices, f);
}
template <ycxx::detail::simd_vec_type V, ranges::contiguous_range R, ycxx::detail::simd_integral I, class... Flags>
  requires(ranges::sized_range<R> && V::size() == I::size() &&
           ycxx::detail::simd_store_range<R, typename V::value_type>)
constexpr void unchecked_scatter_to(const V& v, R&& out, const typename I::mask_type& mask, const I& indices,
                                    flags<Flags...> f = {}) {
  ycxx::detail::simd_check_indices(indices, mask, ranges::size(out));
  std::simd::partial_scatter_to(v, out, mask, indices, f);
}

} // namespace std::simd

namespace ycxx::detail {
template <class T>
concept simd_unsigned_element = bit_unsigned<T>;
template <class T>
concept simd_integer_element = bit_integer<T>;
} // namespace ycxx::detail

namespace std::simd {

// [simd.bit]: the <bit> functions element-wise.
template <ycxx::detail::simd_vec_type V>
  requires integral<typename V::value_type>
constexpr V byteswap(const V& v) noexcept {
  return ycxx::detail::simd_build<V>([&](int i) { return std::byteswap(v[i]); });
}
template <ycxx::detail::simd_integral V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr V bit_reverse(const V& v) noexcept {
  return ycxx::detail::simd_build<V>([&](int i) { return std::bit_reverse(v[i]); });
}
template <ycxx::detail::simd_vec_type V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr V bit_ceil(const V& v) {
  return ycxx::detail::simd_build<V>([&](int i) { return std::bit_ceil(v[i]); });
}
template <ycxx::detail::simd_vec_type V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr V bit_floor(const V& v) noexcept {
  return ycxx::detail::simd_build<V>([&](int i) { return std::bit_floor(v[i]); });
}
template <ycxx::detail::simd_vec_type V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr typename V::mask_type has_single_bit(const V& v) noexcept {
  return ycxx::detail::simd_build<typename V::mask_type>([&](int i) { return std::has_single_bit(v[i]); });
}
template <ycxx::detail::simd_integral VX, ycxx::detail::simd_integral VS>
  requires(ycxx::detail::simd_integer_element<typename VX::value_type> &&
           ycxx::detail::simd_integer_element<typename VS::value_type> && VX::size() == VS::size() &&
           sizeof(typename VX::value_type) == sizeof(typename VS::value_type))
constexpr VX shl(const VX& x, const VS& s) noexcept {
  return ycxx::detail::simd_build<VX>([&](int i) { return std::shl(x[i], s[i]); });
}
template <ycxx::detail::simd_integral V, class S>
  requires(ycxx::detail::simd_integer_element<typename V::value_type> && ycxx::detail::simd_integer_element<S>)
constexpr V shl(const V& x, S s) noexcept {
  return ycxx::detail::simd_build<V>([&](int i) { return std::shl(x[i], s); });
}
template <ycxx::detail::simd_integral VX, ycxx::detail::simd_integral VS>
  requires(ycxx::detail::simd_integer_element<typename VX::value_type> &&
           ycxx::detail::simd_integer_element<typename VS::value_type> && VX::size() == VS::size() &&
           sizeof(typename VX::value_type) == sizeof(typename VS::value_type))
constexpr VX shr(const VX& x, const VS& s) noexcept {
  return ycxx::detail::simd_build<VX>([&](int i) { return std::shr(x[i], s[i]); });
}
template <ycxx::detail::simd_integral V, class S>
  requires(ycxx::detail::simd_integer_element<typename V::value_type> && ycxx::detail::simd_integer_element<S>)
constexpr V shr(const V& x, S s) noexcept {
  return ycxx::detail::simd_build<V>([&](int i) { return std::shr(x[i], s); });
}
template <ycxx::detail::simd_vec_type V0, ycxx::detail::simd_vec_type V1>
  requires(ycxx::detail::simd_unsigned_element<typename V0::value_type> && integral<typename V1::value_type> &&
           V0::size() == V1::size() && sizeof(typename V0::value_type) == sizeof(typename V1::value_type))
constexpr V0 rotl(const V0& v, const V1& s) noexcept {
  return ycxx::detail::simd_build<V0>([&](int i) { return std::rotl(v[i], static_cast<int>(s[i])); });
}
template <ycxx::detail::simd_vec_type V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr V rotl(const V& v, int s) noexcept {
  return ycxx::detail::simd_build<V>([&](int i) { return std::rotl(v[i], s); });
}
template <ycxx::detail::simd_vec_type V0, ycxx::detail::simd_vec_type V1>
  requires(ycxx::detail::simd_unsigned_element<typename V0::value_type> && integral<typename V1::value_type> &&
           V0::size() == V1::size() && sizeof(typename V0::value_type) == sizeof(typename V1::value_type))
constexpr V0 rotr(const V0& v, const V1& s) noexcept {
  return ycxx::detail::simd_build<V0>([&](int i) { return std::rotr(v[i], static_cast<int>(s[i])); });
}
template <ycxx::detail::simd_vec_type V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr V rotr(const V& v, int s) noexcept {
  return ycxx::detail::simd_build<V>([&](int i) { return std::rotr(v[i], s); });
}
template <ycxx::detail::simd_integral V0, ycxx::detail::simd_integral V1>
  requires(ycxx::detail::simd_unsigned_element<typename V0::value_type> && V0::size() == V1::size() &&
           sizeof(typename V0::value_type) == sizeof(typename V1::value_type))
constexpr V0 bit_repeat(const V0& v, const V1& l) {
  return ycxx::detail::simd_build<V0>([&](int i) { return std::bit_repeat(v[i], static_cast<int>(l[i])); });
}
template <ycxx::detail::simd_integral V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr V bit_repeat(const V& v, int l) {
  return ycxx::detail::simd_build<V>([&](int i) { return std::bit_repeat(v[i], l); });
}
template <ycxx::detail::simd_vec_type V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr rebind_t<make_signed_t<typename V::value_type>, V> bit_width(const V& v) noexcept {
  return ycxx::detail::simd_build<rebind_t<make_signed_t<typename V::value_type>, V>>(
      [&](int i) { return static_cast<make_signed_t<typename V::value_type>>(std::bit_width(v[i])); });
}
template <ycxx::detail::simd_vec_type V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr rebind_t<make_signed_t<typename V::value_type>, V> countl_zero(const V& v) noexcept {
  return ycxx::detail::simd_build<rebind_t<make_signed_t<typename V::value_type>, V>>(
      [&](int i) { return static_cast<make_signed_t<typename V::value_type>>(std::countl_zero(v[i])); });
}
template <ycxx::detail::simd_vec_type V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr rebind_t<make_signed_t<typename V::value_type>, V> countl_one(const V& v) noexcept {
  return ycxx::detail::simd_build<rebind_t<make_signed_t<typename V::value_type>, V>>(
      [&](int i) { return static_cast<make_signed_t<typename V::value_type>>(std::countl_one(v[i])); });
}
template <ycxx::detail::simd_vec_type V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr rebind_t<make_signed_t<typename V::value_type>, V> countr_zero(const V& v) noexcept {
  return ycxx::detail::simd_build<rebind_t<make_signed_t<typename V::value_type>, V>>(
      [&](int i) { return static_cast<make_signed_t<typename V::value_type>>(std::countr_zero(v[i])); });
}
template <ycxx::detail::simd_vec_type V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr rebind_t<make_signed_t<typename V::value_type>, V> countr_one(const V& v) noexcept {
  return ycxx::detail::simd_build<rebind_t<make_signed_t<typename V::value_type>, V>>(
      [&](int i) { return static_cast<make_signed_t<typename V::value_type>>(std::countr_one(v[i])); });
}
template <ycxx::detail::simd_vec_type V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr rebind_t<make_signed_t<typename V::value_type>, V> popcount(const V& v) noexcept {
  return ycxx::detail::simd_build<rebind_t<make_signed_t<typename V::value_type>, V>>(
      [&](int i) { return static_cast<make_signed_t<typename V::value_type>>(std::popcount(v[i])); });
}
template <ycxx::detail::simd_integral V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr V bit_compress(const V& v, const V& m) noexcept {
  return ycxx::detail::simd_build<V>([&](int i) { return std::bit_compress(v[i], m[i]); });
}
template <ycxx::detail::simd_integral V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr V bit_expand(const V& v, const V& m) noexcept {
  return ycxx::detail::simd_build<V>([&](int i) { return std::bit_expand(v[i], m[i]); });
}
template <ycxx::detail::simd_integral V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr V bit_compress(const V& v, typename V::value_type m) noexcept {
  return ycxx::detail::simd_build<V>([&](int i) { return std::bit_compress(v[i], m); });
}
template <ycxx::detail::simd_integral V>
  requires ycxx::detail::simd_unsigned_element<typename V::value_type>
constexpr V bit_expand(const V& v, typename V::value_type m) noexcept {
  return ycxx::detail::simd_build<V>([&](int i) { return std::bit_expand(v[i], m); });
}

} // namespace std::simd

namespace ycxx::detail {
// fabs on vector chunks: clear the sign bits.
template <class V>
constexpr V simd_fabs(const V& x) {
  using S = std::remove_cvref_t<decltype(simd_access::data(std::declval<V&>()))>;
  using T = typename V::value_type;
  if constexpr (S::is_vector) {
    using I = simd_integer_from<sizeof(T)>;
    using MC = simd_vector<I, S::lanes>;
    using C = typename S::chunk_type;
    return simd_access::make<V>(S::map(simd_access::data(x), [](C c) {
      return __builtin_bit_cast(C, static_cast<MC>(__builtin_bit_cast(MC, c) & (MC{} + std::numeric_limits<I>::max())));
    }));
  } else {
    return ycxx::detail::simd_build<V>([&](int i) { return std::fabs(x[i]); });
  }
}
} // namespace ycxx::detail

#include <ycxx/core/simd_math.hpp>
