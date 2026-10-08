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

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {
// real-type of a basic_vec whose value_type is not complex ([simd.overview]/3).
struct __simd_not_complex {};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace simd {

template <class _Tp, class _Abi = __ycxx::__detail::__simd_native_abi<_Tp>>
class basic_vec;

template <class _Tp, __ycxx::__detail::__simd_size_t _Np = __ycxx::__detail::__simd_size_v<_Tp, __ycxx::__detail::__simd_native_abi<_Tp>>>
using vec = basic_vec<_Tp, __ycxx::__detail::__simd_deduce_abi_t<_Tp, _Np>>;
template <class _Tp, __ycxx::__detail::__simd_size_t _Np = __ycxx::__detail::__simd_size_v<_Tp, __ycxx::__detail::__simd_native_abi<_Tp>>>
using mask = typename vec<_Tp, _Np>::mask_type;

// [simd.traits]
template <class _Tp, class _Up = typename _Tp::value_type>
struct alignment {};
template <class _Tp, class _Abi, class _Up>
  requires(__ycxx::__detail::__simd_size_v<_Tp, _Abi> != 0 && __ycxx::__detail::__simd_vectorizable<_Up>)
struct alignment<basic_vec<_Tp, _Abi>, _Up>
    : integral_constant<size_t, alignof(__ycxx::__detail::__simd_storage<_Up, __ycxx::__detail::__simd_size_v<_Tp, _Abi>>)> {};
template <class _Tp, class _Up = typename _Tp::value_type>
inline constexpr size_t alignment_v = alignment<_Tp, _Up>::value;

template <class _Tp, class _Vp>
struct rebind {};
template <class _Tp, class _Up, int _Np>
  requires(__ycxx::__detail::__simd_vectorizable<_Tp> && __ycxx::__detail::__simd_size_v<_Up, __ycxx::__adl_free::__simd_abi<_Np>> != 0)
struct rebind<_Tp, basic_vec<_Up, __ycxx::__adl_free::__simd_abi<_Np>>> {
  using type = basic_vec<_Tp, __ycxx::__adl_free::__simd_abi<_Np>>;
};
template <class _Tp, size_t _Bytes, int _Np>
  requires(__ycxx::__detail::__simd_vectorizable<_Tp> &&
           __ycxx::__detail::__simd_mask_size_v<_Bytes, __ycxx::__adl_free::__simd_abi<_Np>> != 0)
struct rebind<_Tp, basic_mask<_Bytes, __ycxx::__adl_free::__simd_abi<_Np>>> {
  using type = basic_mask<sizeof(_Tp), __ycxx::__adl_free::__simd_abi<_Np>>;
};
template <class _Tp, class _Vp>
using rebind_t = typename rebind<_Tp, _Vp>::type;

template <__ycxx::__detail::__simd_size_t _Np, class _Vp>
struct resize {};
template <__ycxx::__detail::__simd_size_t _Np, class _Tp, int _Mp>
  requires(__ycxx::__detail::__simd_size_v<_Tp, __ycxx::__adl_free::__simd_abi<_Mp>> != 0 &&
           __ycxx::__detail::__simd_size_v<_Tp, __ycxx::__adl_free::__simd_abi<_Np>> != 0)
struct resize<_Np, basic_vec<_Tp, __ycxx::__adl_free::__simd_abi<_Mp>>> {
  using type = basic_vec<_Tp, __ycxx::__adl_free::__simd_abi<_Np>>;
};
template <__ycxx::__detail::__simd_size_t _Np, size_t _Bytes, int _Mp>
  requires(__ycxx::__detail::__simd_mask_size_v<_Bytes, __ycxx::__adl_free::__simd_abi<_Mp>> != 0 &&
           __ycxx::__detail::__simd_mask_size_v<_Bytes, __ycxx::__adl_free::__simd_abi<_Np>> != 0)
struct resize<_Np, basic_mask<_Bytes, __ycxx::__adl_free::__simd_abi<_Mp>>> {
  using type = basic_mask<_Bytes, __ycxx::__adl_free::__simd_abi<_Np>>;
};
template <__ycxx::__detail::__simd_size_t _Np, class _Vp>
using resize_t = typename resize<_Np, _Vp>::type;

}}} // namespace std::simd

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// [simd.expos]
template <class _Vp>
concept __simd_vec_type = std::same_as<_Vp, std::simd::basic_vec<typename _Vp::value_type, typename _Vp::abi_type>> &&
                        std::is_default_constructible_v<_Vp>;
template <class _Vp>
concept __simd_mask_type = std::same_as<_Vp, std::simd::basic_mask<__simd_mask_element_size<_Vp>, typename _Vp::abi_type>> &&
                         std::is_default_constructible_v<_Vp>;
template <class _Vp>
concept __simd_floating_point = __simd_vec_type<_Vp> && std::floating_point<typename _Vp::value_type>;
template <class _Vp>
concept __simd_integral = __simd_vec_type<_Vp> && std::integral<typename _Vp::value_type>;
template <class _Vp>
concept __simd_complex = __simd_vec_type<_Vp> && __simd_is_complex<typename _Vp::value_type>;
template <class _Tp>
using __deduced_vec_t = decltype(std::declval<const _Tp&>() + std::declval<const _Tp&>());
template <class _Tp>
concept __math_floating_point = __simd_floating_point<__deduced_vec_t<_Tp>>;
template <class _BinaryOperation, class _Tp>
concept __reduction_binary_operation =
    requires(const _BinaryOperation __binary_op, const std::simd::vec<_Tp, 1> __v) {
      { __binary_op(__v, __v) } -> std::same_as<std::simd::vec<_Tp, 1>>;
    };

template <class _Tp, int _Np>
struct __simd_real_type {
  using type = __ycxx::__adl_free::__simd_not_complex;
};
template <class _Tp, int _Np>
struct __simd_real_type<std::complex<_Tp>, _Np> {
  using type = std::simd::basic_vec<_Tp, __ycxx::__adl_free::__simd_abi<_Np>>;
};

template <std::size_t _Bytes, int _Np>
using __simd_mask_storage = __simd_storage<__simd_mask_element<_Bytes>, _Np>;

// [simd.ctor]/2: the broadcast constructor's constraints.
template <class _Up, class _Tp, class _From = std::remove_cvref_t<_Up>>
concept __simd_broadcast_from =
    (std::convertible_to<_Up, _Tp> && !is_arithmetic_v<_From> && !__constexpr_wrapper_like<_From>) ||
    (is_arithmetic_v<_From> && __ycxx::__detail::__simd_value_preserving<_From, _Tp>()) ||
    (__constexpr_wrapper_like<_From> && is_arithmetic_v<std::remove_cvref_t<decltype(_From::value)>> &&
     __ycxx::__detail::__simd_representable<_From::value, _Tp>());

// [simd.ctor]/8: the generator constructor's constraints, for one index.
template <class _From, class _Tp>
concept __simd_generated_value =
    std::convertible_to<_From, _Tp> &&
    (!is_arithmetic_v<std::remove_cvref_t<_From>> ||
     __ycxx::__detail::__simd_value_preserving<std::remove_cvref_t<_From>, _Tp>());
template <class _Gp, class _Tp, int _Ip>
concept __simd_generator_element = requires(_Gp& __g) {
  { __g(std::integral_constant<__simd_size_t, _Ip>()) } -> __simd_generated_value<_Tp>;
};
template <class _Gp, class _Tp, int... _Ip>
consteval bool __simd_is_generator(std::integer_sequence<int, _Ip...>) {
  return (__simd_generator_element<_Gp, _Tp, _Ip> && ...);
}
template <class _Gp, class _Tp, int _Np>
concept __simd_generator = __ycxx::__detail::__simd_is_generator<_Gp, _Tp>(std::make_integer_sequence<int, _Np>());

// [simd.mask.ctor]/4
template <class _Gp, int _Ip>
concept __simd_mask_generator_element =
    requires(_Gp& __g) { requires std::same_as<decltype(__g(std::integral_constant<__simd_size_t, _Ip>())), bool>; };
template <class _Gp, int... _Ip>
consteval bool __simd_is_mask_generator(std::integer_sequence<int, _Ip...>) {
  return (__simd_mask_generator_element<_Gp, _Ip> && ...);
}
template <class _Gp, int _Np>
concept __simd_mask_generator = __ycxx::__detail::__simd_is_mask_generator<_Gp>(std::make_integer_sequence<int, _Np>());

// [simd.ctor]/12: the range constructor's constraints.
template <class _Rp>
inline constexpr __simd_size_t __simd_static_width = static_cast<__simd_size_t>(__simd_static_size<_Rp>);
template <class _Rp, class _Tp, int _Np>
concept __simd_range_init = std::ranges::contiguous_range<_Rp> && std::ranges::sized_range<_Rp> &&
                          __simd_static_size<_Rp> == static_cast<std::size_t>(_Np) &&
                          __simd_vectorizable<std::ranges::range_value_t<_Rp>> &&
                          __simd_explicitly_convertible_to<std::ranges::range_value_t<_Rp>, _Tp>;

// Element-wise comparison of two storages giving a mask storage: chunk by chunk when the
// layouts match (the vector comparison gives the all-ones/zero lanes of the mask), else element
// by element. f is a generic comparison.
template <class _MS, class _Sp, class _Fp>
constexpr _MS __simd_compare(const _Sp& a, const _Sp& b, _Fp __f) noexcept {
  using _Mp = typename _MS::element_type;
  if constexpr (_Sp::__is_vector && _MS::__is_vector && _Sp::__lanes == _MS::__lanes) {
    _MS r;
    for (int k = 0; k < _Sp::__chunks; ++k)
      r.c[k] = __builtin_bit_cast(typename _MS::__chunk_type, __f(a.c[k], b.c[k]));
    return r;
  } else {
    return _MS::generate([&](int i) { return __f(a.get(i), b.get(i)) ? _Mp(-1) : _Mp(0); });
  }
}

// m[i] ? a[i] : b[i]
template <class _Sp, class _MS>
constexpr _Sp __simd_blend(const _MS& m, const _Sp& a, const _Sp& b) noexcept {
  if constexpr (_Sp::__is_vector && _MS::__is_vector && _Sp::__lanes == _MS::__lanes) {
    using _MC = typename _MS::__chunk_type;
    using _Cp = typename _Sp::__chunk_type;
    _Sp r;
    for (int k = 0; k < _Sp::__chunks; ++k) {
      _MC __x = __builtin_bit_cast(_MC, a.c[k]), y = __builtin_bit_cast(_MC, b.c[k]);
      r.c[k] = __builtin_bit_cast(_Cp, static_cast<_MC>((m.c[k] & __x) | (~m.c[k] & y)));
    }
    return r;
  } else {
    return _Sp::generate([&](int i) { return m.get(i) ? a.get(i) : b.get(i); });
  }
}

// Element-wise static_cast from storage SU to storage S.
template <class _Sp, class _SU>
constexpr _Sp __simd_convert(const _SU& __u) noexcept {
  using _Tp = typename _Sp::element_type;
  if constexpr (std::is_same_v<_Sp, _SU>) {
    return __u;
  } else if constexpr (_Sp::__is_vector && _SU::__is_vector && _Sp::__lanes == _SU::__lanes) {
    _Sp r;
    for (int k = 0; k < _Sp::__chunks; ++k)
      r.c[k] = __builtin_convertvector(__u.c[k], typename _Sp::__chunk_type);
    return r;
  } else {
    return _Sp::generate([&](int i) { return static_cast<_Tp>(__u.get(i)); });
  }
}

// Shifts: in the promoted type, as for the scalars (a vector of 8- or 16-bit lanes is widened
// to int lanes, so a shift count up to 31 has the scalar meaning).
template <bool _Left, class _Sp>
constexpr _Sp __simd_shift(const _Sp& a, const _Sp& b) noexcept {
  using _Ep = typename _Sp::element_type;
  if constexpr (_Sp::__is_vector && sizeof(_Ep) < sizeof(int)) {
    using _Wp = __simd_vector<int, _Sp::__lanes>;
    using _Cp = typename _Sp::__chunk_type;
    return _Sp::__map2(a, b, [](_Cp __x, _Cp y) {
      _Wp __wx = __builtin_convertvector(__x, _Wp), __wy = __builtin_convertvector(y, _Wp);
      if constexpr (_Left)
        return __builtin_convertvector(__wx << __wy, _Cp);
      else
        return __builtin_convertvector(__wx >> __wy, _Cp);
    });
  } else {
    return _Sp::__map2(a, b, [](auto __x, auto y) {
      if constexpr (_Left)
        return __x << y;
      else
        return __x >> y;
    });
  }
}
template <bool _Left, class _Sp>
constexpr _Sp __simd_shift(const _Sp& a, __simd_size_t n) noexcept {
  using _Ep = typename _Sp::element_type;
  if constexpr (_Sp::__is_vector && sizeof(_Ep) < sizeof(int)) {
    using _Wp = __simd_vector<int, _Sp::__lanes>;
    using _Cp = typename _Sp::__chunk_type;
    return _Sp::map(a, [n](_Cp __x) {
      _Wp __wx = __builtin_convertvector(__x, _Wp);
      if constexpr (_Left)
        return __builtin_convertvector(__wx << n, _Cp);
      else
        return __builtin_convertvector(__wx >> n, _Cp);
    });
  } else {
    return _Sp::map(a, [n](auto __x) {
      if constexpr (_Left)
        return __x << n;
      else
        return __x >> n;
    });
  }
}

// The mask reductions work on the object representation as unsigned words: every element is all
// ones or all zeros, so counting set bits counts elements (little-endian targets; elsewhere the
// elements are visited one by one).
template <class _Sp>
consteval std::size_t __simd_word_size() {
  constexpr std::size_t bytes = sizeof(_Sp);
  return bytes % 8 == 0 ? 8 : bytes % 4 == 0 ? 4 : bytes % 2 == 0 ? 2 : 1;
}
template <class _Sp>
using __simd_word =
    std::conditional_t<__simd_word_size<_Sp>() == 8, unsigned long long,
                       std::conditional_t<__simd_word_size<_Sp>() == 4, unsigned,
                                          std::conditional_t<__simd_word_size<_Sp>() == 2, unsigned short, unsigned char>>>;
template <class _Sp>
inline constexpr bool __simd_words_ok = std::endian::native == std::endian::little;
template <class _Sp>
constexpr auto __simd_words(const _Sp& s) noexcept {
  using _Wp = __simd_array<__simd_word<_Sp>, sizeof(_Sp) / sizeof(__simd_word<_Sp>)>;
  return __builtin_bit_cast(_Wp, s);
}

// The identities of [simd.reductions]/9.
template <class _Op_>
inline constexpr bool __simd_known_identity =
    std::is_same_v<_Op_, std::plus<>> || std::is_same_v<_Op_, std::multiplies<>> || std::is_same_v<_Op_, std::bit_and<>> ||
    std::is_same_v<_Op_, std::bit_or<>> || std::is_same_v<_Op_, std::bit_xor<>>;
template <class _Tp, class _Op_>
constexpr _Tp __simd_identity() noexcept {
  if constexpr (std::is_same_v<_Op_, std::multiplies<>>)
    return _Tp(1);
  else if constexpr (std::is_same_v<_Op_, std::bit_and<>>)
    return _Tp(~_Tp());
  else
    return _Tp();
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace simd {

// [simd.overview]/1: a disabled basic_vec.
template <class _Tp, class _Abi>
class basic_vec {
public:
  using value_type = _Tp;
  using mask_type = basic_mask<sizeof(_Tp), _Abi>;
  using abi_type = _Abi;
  basic_vec() = delete;
  ~basic_vec() = delete;
  basic_vec(const basic_vec&) = delete;
  basic_vec& operator=(const basic_vec&) = delete;
};

// [simd.mask.overview]/1: a disabled basic_mask.
template <size_t _Bytes, class _Abi>
class basic_mask {
public:
  using value_type = bool;
  using abi_type = _Abi;
  basic_mask() = delete;
  ~basic_mask() = delete;
  basic_mask(const basic_mask&) = delete;
  basic_mask& operator=(const basic_mask&) = delete;
};

// [simd.mask.class]
template <size_t _Bytes, int _Np>
  requires(__ycxx::__detail::__simd_mask_size_v<_Bytes, __ycxx::__adl_free::__simd_abi<_Np>> != 0)
class basic_mask<_Bytes, __ycxx::__adl_free::__simd_abi<_Np>> {
  using __element = __ycxx::__detail::__simd_mask_element<_Bytes>;
  using __storage = __ycxx::__detail::__simd_mask_storage<_Bytes, _Np>;
  __storage __data_;

  friend __ycxx::__detail::__simd_access;
  template <class, class>
  friend class basic_vec;
  template <size_t, class>
  friend class basic_mask;
  constexpr basic_mask(__ycxx::__detail::__simd_access, const __storage& s) noexcept : __data_(s) {}
  static constexpr basic_mask __make(const __storage& s) noexcept { return basic_mask(__ycxx::__detail::__simd_access{}, s); }
  // The vec of +k, -k, ~k: integer-from<Bytes> elements.
  using __int_vec = basic_vec<__ycxx::__detail::__simd_integer_from<(_Bytes > 8 ? 8 : _Bytes)>, __ycxx::__adl_free::__simd_abi<_Np>>;

public:
  using value_type = bool;
  using abi_type = __ycxx::__adl_free::__simd_abi<_Np>;
  using iterator = __ycxx::__adl_free::__simd_iterator<basic_mask>;
  using const_iterator = __ycxx::__adl_free::__simd_iterator<const basic_mask>;

  constexpr iterator begin() noexcept { return {*this, 0}; }
  constexpr const_iterator begin() const noexcept { return {*this, 0}; }
  constexpr const_iterator cbegin() const noexcept { return {*this, 0}; }
  constexpr default_sentinel_t end() const noexcept { return {}; }
  constexpr default_sentinel_t cend() const noexcept { return {}; }

  static constexpr integral_constant<__ycxx::__detail::__simd_size_t, _Np> size{};

  constexpr basic_mask() noexcept = default;

  // [simd.mask.ctor]
  constexpr explicit basic_mask(same_as<value_type> auto __x) noexcept
      : __data_(__storage::__broadcast(__x ? __element(-1) : __element(0))) {}
  template <size_t _UBytes, class _UAbi>
    requires(__ycxx::__detail::__simd_mask_size_v<_UBytes, _UAbi> == _Np)
  constexpr explicit basic_mask(const basic_mask<_UBytes, _UAbi>& __x) noexcept
      : __data_(__ycxx::__detail::__simd_convert<__storage>(__x.__data_)) {}
  template <class _Gp>
    requires __ycxx::__detail::__simd_mask_generator<_Gp, _Np>
  constexpr explicit basic_mask(_Gp&& __gen) {
    __ycxx::__detail::__simd_array<__element, _Np> a;
    [&]<int... _Ip>(integer_sequence<int, _Ip...>) {
      ((a.__v[_Ip] = __gen(integral_constant<__ycxx::__detail::__simd_size_t, _Ip>()) ? __element(-1) : __element(0)), ...);
    }(make_integer_sequence<int, _Np>());
    __data_ = __storage::__from_array(a);
  }
  template <same_as<bitset<static_cast<size_t>(_Np)>> _Tp>
  constexpr basic_mask(const _Tp& b) noexcept
      : __data_(__storage::generate([&](int i) { return b[static_cast<size_t>(i)] ? __element(-1) : __element(0); })) {}
  template <unsigned_integral _Tp>
    requires(!same_as<_Tp, value_type>)
  constexpr explicit basic_mask(_Tp __val) noexcept
      : __data_(__storage::generate([&](int i) {
          return i < numeric_limits<_Tp>::digits && ((__val >> i) & 1) != 0 ? __element(-1) : __element(0);
        })) {}

  // [simd.mask.subscr]
  constexpr value_type operator[](__ycxx::__detail::__simd_size_t i) const {
    __ycxx::__detail::__precondition(i >= 0 && i < _Np, "std::simd::basic_mask::operator[]: index out of range");
    return __data_.get(i) != 0;
  }
  template <__ycxx::__detail::__simd_integral _Ip>
  constexpr resize_t<_Ip::size(), basic_mask> operator[](const _Ip& indices) const {
    return permute(*this, indices);
  }

  // [simd.mask.unary]
  constexpr basic_mask operator!() const noexcept {
    return __make(__storage::map(__data_, [](auto __x) { return ~__x; }));
  }
  constexpr __int_vec operator+() const noexcept
    requires(_Bytes <= 8)
  {
    return __ycxx::__detail::__simd_access::__make<__int_vec>(__storage::map(__data_, [](auto __x) { return -__x; }));
  }
  constexpr __int_vec operator-() const noexcept
    requires(_Bytes <= 8)
  {
    return __ycxx::__detail::__simd_access::__make<__int_vec>(__data_);
  }
  constexpr __int_vec operator~() const noexcept
    requires(_Bytes <= 8)
  {
    return __ycxx::__detail::__simd_access::__make<__int_vec>(__storage::map(__data_, [](auto __x) { return __x - __element(1); }));
  }
  void operator+() const noexcept
    requires(_Bytes > 8)
  = delete;
  void operator-() const noexcept
    requires(_Bytes > 8)
  = delete;
  void operator~() const noexcept
    requires(_Bytes > 8)
  = delete;

  // [simd.mask.conv]
  template <class _Up, class _Ap>
    requires(__ycxx::__detail::__simd_size_v<_Up, _Ap> == _Np)
  constexpr explicit(sizeof(_Up) != _Bytes) operator basic_vec<_Up, _Ap>() const noexcept {
    using _VS = __ycxx::__detail::__simd_storage<_Up, _Np>;
    if constexpr (__ycxx::__detail::__simd_arithmetic<_Up>)
      return __ycxx::__detail::__simd_access::__make<basic_vec<_Up, _Ap>>(
          __ycxx::__detail::__simd_convert<_VS>(__storage::map(__data_, [](auto __x) { return -__x; })));
    else
      return __ycxx::__detail::__simd_access::__make<basic_vec<_Up, _Ap>>(
          _VS::generate([&](int i) { return static_cast<_Up>(__data_.get(i) != 0); }));
  }
  constexpr bitset<static_cast<size_t>(_Np)> to_bitset() const noexcept {
    bitset<static_cast<size_t>(_Np)> b;
    for (int i = 0; i < _Np; ++i)
      if (__data_.get(i) != 0)
        b.set(static_cast<size_t>(i));
    return b;
  }
  constexpr unsigned long long to_ullong() const {
    unsigned long long r = 0;
    for (int i = 0; i < _Np; ++i)
      if (__data_.get(i) != 0) {
        __ycxx::__detail::__precondition(i < numeric_limits<unsigned long long>::digits,
                                   "std::simd::basic_mask::to_ullong: a set element does not fit");
        r |= 1ull << i;
      }
    return r;
  }

  // [simd.mask.binary]
  friend constexpr basic_mask operator&&(const basic_mask& a, const basic_mask& b) noexcept {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x & y; }));
  }
  friend constexpr basic_mask operator||(const basic_mask& a, const basic_mask& b) noexcept {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x | y; }));
  }
  friend constexpr basic_mask operator&(const basic_mask& a, const basic_mask& b) noexcept {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x & y; }));
  }
  friend constexpr basic_mask operator|(const basic_mask& a, const basic_mask& b) noexcept {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x | y; }));
  }
  friend constexpr basic_mask operator^(const basic_mask& a, const basic_mask& b) noexcept {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x ^ y; }));
  }
  // [simd.mask.cassign]
  friend constexpr basic_mask& operator&=(basic_mask& a, const basic_mask& b) noexcept { return a = a & b; }
  friend constexpr basic_mask& operator|=(basic_mask& a, const basic_mask& b) noexcept { return a = a | b; }
  friend constexpr basic_mask& operator^=(basic_mask& a, const basic_mask& b) noexcept { return a = a ^ b; }
  // [simd.mask.comparison] (false < true)
  friend constexpr basic_mask operator==(const basic_mask& a, const basic_mask& b) noexcept {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return ~(__x ^ y); }));
  }
  friend constexpr basic_mask operator!=(const basic_mask& a, const basic_mask& b) noexcept {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x ^ y; }));
  }
  friend constexpr basic_mask operator>=(const basic_mask& a, const basic_mask& b) noexcept {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x | ~y; }));
  }
  friend constexpr basic_mask operator<=(const basic_mask& a, const basic_mask& b) noexcept {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return ~__x | y; }));
  }
  friend constexpr basic_mask operator>(const basic_mask& a, const basic_mask& b) noexcept {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x & ~y; }));
  }
  friend constexpr basic_mask operator<(const basic_mask& a, const basic_mask& b) noexcept {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return ~__x & y; }));
  }

  // [simd.mask.cond]
  friend constexpr basic_mask __simd_select_impl(const basic_mask& k, const basic_mask& a, const basic_mask& b) noexcept {
    return __make(__ycxx::__detail::__simd_blend(k.__data_, a.__data_, b.__data_));
  }
  friend constexpr basic_mask __simd_select_impl(const basic_mask& k, same_as<bool> auto a,
                                               same_as<bool> auto b) noexcept {
    return __make(__ycxx::__detail::__simd_blend(k.__data_, __storage::__broadcast(a ? __element(-1) : __element(0)),
                                         __storage::__broadcast(b ? __element(-1) : __element(0))));
  }
  template <class _T0, class _T1>
    requires(same_as<_T0, _T1> && __ycxx::__detail::__simd_vectorizable<_T0> && sizeof(_T0) == _Bytes)
  friend constexpr basic_vec<_T0, __ycxx::__adl_free::__simd_abi<_Np>> __simd_select_impl(const basic_mask& k, const _T0& a,
                                                                               const _T1& b) noexcept {
    using _VS = __ycxx::__detail::__simd_storage<_T0, _Np>;
    return __ycxx::__detail::__simd_access::__make<basic_vec<_T0, __ycxx::__adl_free::__simd_abi<_Np>>>(
        __ycxx::__detail::__simd_blend(k.__data_, _VS::__broadcast(a), _VS::__broadcast(b)));
  }
};

// [simd.class]
template <class _Tp, int _Np>
  requires(__ycxx::__detail::__simd_size_v<_Tp, __ycxx::__adl_free::__simd_abi<_Np>> != 0)
class basic_vec<_Tp, __ycxx::__adl_free::__simd_abi<_Np>> {
  using __storage = __ycxx::__detail::__simd_storage<_Tp, _Np>;
  using __mask_storage = __ycxx::__detail::__simd_mask_storage<sizeof(_Tp), _Np>;
  using __real_type = typename __ycxx::__detail::__simd_real_type<_Tp, _Np>::type;
  __storage __data_;

  friend __ycxx::__detail::__simd_access;
  template <class, class>
  friend class basic_vec;
  template <size_t, class>
  friend class basic_mask;
  constexpr basic_vec(__ycxx::__detail::__simd_access, const __storage& s) noexcept : __data_(s) {}
  static constexpr basic_vec __make(const __storage& s) noexcept { return basic_vec(__ycxx::__detail::__simd_access{}, s); }

  template <class _Up, class... _Flags>
  static constexpr __storage from_range(const _Up* p, const __mask_storage* k) {
    if constexpr (!__ycxx::__detail::__simd_has_convert<_Flags...>)
      static_assert(__ycxx::__detail::__simd_value_preserving<_Up, _Tp>(),
                    "std::simd::basic_vec: the conversion from the range's value type is not value-preserving "
                    "(pass flag_convert)");
    return __storage::generate([&](int i) { return k == nullptr || k->get(i) != 0 ? static_cast<_Tp>(p[i]) : _Tp(); });
  }

public:
  using value_type = _Tp;
  using mask_type = basic_mask<sizeof(_Tp), __ycxx::__adl_free::__simd_abi<_Np>>;
  using abi_type = __ycxx::__adl_free::__simd_abi<_Np>;
  using iterator = __ycxx::__adl_free::__simd_iterator<basic_vec>;
  using const_iterator = __ycxx::__adl_free::__simd_iterator<const basic_vec>;

  constexpr iterator begin() noexcept { return {*this, 0}; }
  constexpr const_iterator begin() const noexcept { return {*this, 0}; }
  constexpr const_iterator cbegin() const noexcept { return {*this, 0}; }
  constexpr default_sentinel_t end() const noexcept { return {}; }
  constexpr default_sentinel_t cend() const noexcept { return {}; }

  static constexpr integral_constant<__ycxx::__detail::__simd_size_t, _Np> size{};

  constexpr basic_vec() noexcept = default;

  // [simd.ctor]
  template <class _Up>
    requires __ycxx::__detail::__simd_broadcast_from<_Up, _Tp>
  constexpr basic_vec(_Up&& value) noexcept : __data_(__storage::__broadcast(static_cast<_Tp>(static_cast<_Up&&>(value)))) {}
  template <class _Up, class _UAbi>
    requires(__ycxx::__detail::__simd_size_v<_Up, _UAbi> == _Np && __ycxx::__detail::__simd_explicitly_convertible_to<_Up, _Tp>)
  constexpr explicit(!__ycxx::__detail::__simd_implicit_conversion<_Up, _Tp>()) basic_vec(const basic_vec<_Up, _UAbi>& __x) noexcept
      : __data_(__ycxx::__detail::__simd_convert<__storage>(__x.__data_)) {}
  template <class _Gp>
    requires __ycxx::__detail::__simd_generator<_Gp, _Tp, _Np>
  constexpr explicit basic_vec(_Gp&& __gen) {
    __ycxx::__detail::__simd_array<_Tp, _Np> a;
    [&]<int... _Ip>(integer_sequence<int, _Ip...>) {
      ((a.__v[_Ip] = static_cast<_Tp>(__gen(integral_constant<__ycxx::__detail::__simd_size_t, _Ip>()))), ...);
    }(make_integer_sequence<int, _Np>());
    __data_ = __storage::__from_array(a);
  }
  template <class _Rp, class... _Flags>
    requires __ycxx::__detail::__simd_range_init<_Rp, _Tp, _Np>
  constexpr basic_vec(_Rp&& r, flags<_Flags...> = {})
      : __data_(from_range<ranges::range_value_t<_Rp>, _Flags...>(ranges::data(r), nullptr)) {}
  template <class _Rp, class... _Flags>
    requires __ycxx::__detail::__simd_range_init<_Rp, _Tp, _Np>
  constexpr basic_vec(_Rp&& r, const mask_type& mask, flags<_Flags...> = {})
      : __data_(from_range<ranges::range_value_t<_Rp>, _Flags...>(ranges::data(r), __builtin_addressof(mask.__data_))) {}
  constexpr basic_vec(const __real_type& __reals, const __real_type& __imags = {}) noexcept
    requires __ycxx::__detail::__simd_is_complex<_Tp>
      : __data_(__storage::generate([&](int i) { return _Tp(__reals[i], __imags[i]); })) {}

  // [simd.subscr]
  constexpr value_type operator[](__ycxx::__detail::__simd_size_t i) const {
    __ycxx::__detail::__precondition(i >= 0 && i < _Np, "std::simd::basic_vec::operator[]: index out of range");
    return __data_.get(i);
  }
  template <__ycxx::__detail::__simd_integral _Ip>
  constexpr resize_t<_Ip::size(), basic_vec> operator[](const _Ip& indices) const {
    return permute(*this, indices);
  }

  // [simd.complex.access]
  constexpr __real_type real() const noexcept
    requires __ycxx::__detail::__simd_is_complex<_Tp>
  {
    return __real_type([&](int i) { return __data_.get(i).real(); });
  }
  constexpr __real_type imag() const noexcept
    requires __ycxx::__detail::__simd_is_complex<_Tp>
  {
    return __real_type([&](int i) { return __data_.get(i).imag(); });
  }
  constexpr void real(const __real_type& __v) noexcept
    requires __ycxx::__detail::__simd_is_complex<_Tp>
  {
    __data_ = __storage::generate([&](int i) { return _Tp(__v[i], __data_.get(i).imag()); });
  }
  constexpr void imag(const __real_type& __v) noexcept
    requires __ycxx::__detail::__simd_is_complex<_Tp>
  {
    __data_ = __storage::generate([&](int i) { return _Tp(__data_.get(i).real(), __v[i]); });
  }

  // [simd.unary]
  constexpr basic_vec& operator++() noexcept
    requires requires(value_type a) { ++a; }
  {
    __data_ = __storage::map(__data_, [](auto __x) { return __x + _Tp(1); });
    return *this;
  }
  constexpr basic_vec operator++(int) noexcept
    requires requires(value_type a) { a++; }
  {
    basic_vec __tmp = *this;
    ++*this;
    return __tmp;
  }
  constexpr basic_vec& operator--() noexcept
    requires requires(value_type a) { --a; }
  {
    __data_ = __storage::map(__data_, [](auto __x) { return __x - _Tp(1); });
    return *this;
  }
  constexpr basic_vec operator--(int) noexcept
    requires requires(value_type a) { a--; }
  {
    basic_vec __tmp = *this;
    --*this;
    return __tmp;
  }
  constexpr mask_type operator!() const noexcept
    requires requires(const value_type a) { !a; }
  {
    return __ycxx::__detail::__simd_access::__make<mask_type>(__ycxx::__detail::__simd_compare<__mask_storage>(
        __data_, __storage::__broadcast(_Tp()), [](auto __x, auto y) { return __x == y; }));
  }
  constexpr basic_vec operator~() const noexcept
    requires requires(const value_type a) { ~a; }
  {
    return __make(__storage::map(__data_, [](auto __x) { return ~__x; }));
  }
  constexpr basic_vec operator+() const noexcept
    requires requires(const value_type a) { +a; }
  {
    return *this;
  }
  constexpr basic_vec operator-() const noexcept
    requires requires(const value_type a) { -a; }
  {
    return __make(__storage::map(__data_, [](auto __x) { return -__x; }));
  }

  // [simd.binary]
  friend constexpr basic_vec operator+(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x + y; }
  {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x + y; }));
  }
  friend constexpr basic_vec operator-(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x - y; }
  {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x - y; }));
  }
  friend constexpr basic_vec operator*(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x * y; }
  {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x * y; }));
  }
  friend constexpr basic_vec operator/(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x / y; }
  {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x / y; }));
  }
  friend constexpr basic_vec operator%(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x % y; }
  {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x % y; }));
  }
  friend constexpr basic_vec operator&(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x & y; }
  {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x & y; }));
  }
  friend constexpr basic_vec operator|(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x | y; }
  {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x | y; }));
  }
  friend constexpr basic_vec operator^(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x ^ y; }
  {
    return __make(__storage::__map2(a.__data_, b.__data_, [](auto __x, auto y) { return __x ^ y; }));
  }
  friend constexpr basic_vec operator<<(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x << y; }
  {
    return __make(__ycxx::__detail::__simd_shift<true>(a.__data_, b.__data_));
  }
  friend constexpr basic_vec operator>>(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x >> y; }
  {
    return __make(__ycxx::__detail::__simd_shift<false>(a.__data_, b.__data_));
  }
  friend constexpr basic_vec operator<<(const basic_vec& __v, __ycxx::__detail::__simd_size_t n) noexcept
    requires requires(value_type __x, __ycxx::__detail::__simd_size_t y) { __x << y; }
  {
    return __make(__ycxx::__detail::__simd_shift<true>(__v.__data_, n));
  }
  friend constexpr basic_vec operator>>(const basic_vec& __v, __ycxx::__detail::__simd_size_t n) noexcept
    requires requires(value_type __x, __ycxx::__detail::__simd_size_t y) { __x >> y; }
  {
    return __make(__ycxx::__detail::__simd_shift<false>(__v.__data_, n));
  }

  // [simd.cassign]
  friend constexpr basic_vec& operator+=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x + y; }
  {
    return a = a + b;
  }
  friend constexpr basic_vec& operator-=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x - y; }
  {
    return a = a - b;
  }
  friend constexpr basic_vec& operator*=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x * y; }
  {
    return a = a * b;
  }
  friend constexpr basic_vec& operator/=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x / y; }
  {
    return a = a / b;
  }
  friend constexpr basic_vec& operator%=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x % y; }
  {
    return a = a % b;
  }
  friend constexpr basic_vec& operator&=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x & y; }
  {
    return a = a & b;
  }
  friend constexpr basic_vec& operator|=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x | y; }
  {
    return a = a | b;
  }
  friend constexpr basic_vec& operator^=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x ^ y; }
  {
    return a = a ^ b;
  }
  friend constexpr basic_vec& operator<<=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x << y; }
  {
    return a = a << b;
  }
  friend constexpr basic_vec& operator>>=(basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x >> y; }
  {
    return a = a >> b;
  }
  friend constexpr basic_vec& operator<<=(basic_vec& a, __ycxx::__detail::__simd_size_t n) noexcept
    requires requires(value_type __x, __ycxx::__detail::__simd_size_t y) { __x << y; }
  {
    return a = a << n;
  }
  friend constexpr basic_vec& operator>>=(basic_vec& a, __ycxx::__detail::__simd_size_t n) noexcept
    requires requires(value_type __x, __ycxx::__detail::__simd_size_t y) { __x >> y; }
  {
    return a = a >> n;
  }

  // [simd.comparison]
  friend constexpr mask_type operator==(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x == y; }
  {
    return __ycxx::__detail::__simd_access::__make<mask_type>(
        __ycxx::__detail::__simd_compare<__mask_storage>(a.__data_, b.__data_, [](auto __x, auto y) { return __x == y; }));
  }
  friend constexpr mask_type operator!=(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x != y; }
  {
    return __ycxx::__detail::__simd_access::__make<mask_type>(
        __ycxx::__detail::__simd_compare<__mask_storage>(a.__data_, b.__data_, [](auto __x, auto y) { return __x != y; }));
  }
  friend constexpr mask_type operator>=(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x >= y; }
  {
    return __ycxx::__detail::__simd_access::__make<mask_type>(
        __ycxx::__detail::__simd_compare<__mask_storage>(a.__data_, b.__data_, [](auto __x, auto y) { return __x >= y; }));
  }
  friend constexpr mask_type operator<=(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x <= y; }
  {
    return __ycxx::__detail::__simd_access::__make<mask_type>(
        __ycxx::__detail::__simd_compare<__mask_storage>(a.__data_, b.__data_, [](auto __x, auto y) { return __x <= y; }));
  }
  friend constexpr mask_type operator>(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x > y; }
  {
    return __ycxx::__detail::__simd_access::__make<mask_type>(
        __ycxx::__detail::__simd_compare<__mask_storage>(a.__data_, b.__data_, [](auto __x, auto y) { return __x > y; }));
  }
  friend constexpr mask_type operator<(const basic_vec& a, const basic_vec& b) noexcept
    requires requires(value_type __x, value_type y) { __x < y; }
  {
    return __ycxx::__detail::__simd_access::__make<mask_type>(
        __ycxx::__detail::__simd_compare<__mask_storage>(a.__data_, b.__data_, [](auto __x, auto y) { return __x < y; }));
  }

  // [simd.cond]
  friend constexpr basic_vec __simd_select_impl(const mask_type& k, const basic_vec& a, const basic_vec& b) noexcept {
    return __make(__ycxx::__detail::__simd_blend(__ycxx::__detail::__simd_access::data(k), a.__data_, b.__data_));
  }
};

template <class _Rp, class... _Ts>
  requires(ranges::contiguous_range<_Rp> && ranges::sized_range<_Rp> &&
           __ycxx::__detail::__simd_static_size<_Rp> != dynamic_extent)
basic_vec(_Rp&& r, _Ts...) -> basic_vec<ranges::range_value_t<_Rp>,
                                     __ycxx::__detail::__simd_deduce_abi_t<ranges::range_value_t<_Rp>,
                                                                     __ycxx::__detail::__simd_static_width<_Rp>>>;
template <size_t _Bytes, class _Abi>
  requires(__ycxx::__detail::__simd_mask_size_v<_Bytes, _Abi> != 0 && _Bytes <= 8)
basic_vec(basic_mask<_Bytes, _Abi>) -> basic_vec<__ycxx::__detail::__simd_integer_from<_Bytes>, _Abi>;

// [simd.creation]
template <class _Tp, class _Abi>
  requires(__ycxx::__detail::__simd_vec_type<_Tp> &&
           (basic_vec<typename _Tp::value_type, _Abi>::size() % _Tp::size() == 0 ||
            requires { typename resize_t<basic_vec<typename _Tp::value_type, _Abi>::size() % _Tp::size(), _Tp>; }))
constexpr auto chunk(const basic_vec<typename _Tp::value_type, _Abi>& __x) noexcept {
  constexpr int size = basic_vec<typename _Tp::value_type, _Abi>::size();
  constexpr int __w = _Tp::size(), n = size / __w, rem = size % __w;
  auto part = [&]<class _Pp>(int base) { return _Pp([&](int i) { return __x[base + i]; }); };
  return [&]<size_t... _Jp>(index_sequence<_Jp...>) {
    if constexpr (rem == 0)
      return array<_Tp, n>{part.template operator()<_Tp>(static_cast<int>(_Jp) * __w)...};
    else
      return tuple<conditional_t<true, _Tp, integral_constant<size_t, _Jp>>..., resize_t<rem, _Tp>>(
          part.template operator()<_Tp>(static_cast<int>(_Jp) * __w)..., part.template operator()<resize_t<rem, _Tp>>(n * __w));
  }(make_index_sequence<static_cast<size_t>(n)>());
}
template <class _Tp, class _Abi>
  requires(__ycxx::__detail::__simd_mask_type<_Tp> &&
           (basic_mask<__ycxx::__detail::__simd_mask_element_size<_Tp>, _Abi>::size() % _Tp::size() == 0 ||
            requires {
              typename resize_t<basic_mask<__ycxx::__detail::__simd_mask_element_size<_Tp>, _Abi>::size() % _Tp::size(), _Tp>;
            }))
constexpr auto chunk(const basic_mask<__ycxx::__detail::__simd_mask_element_size<_Tp>, _Abi>& __x) noexcept {
  constexpr int size = basic_mask<__ycxx::__detail::__simd_mask_element_size<_Tp>, _Abi>::size();
  constexpr int __w = _Tp::size(), n = size / __w, rem = size % __w;
  auto part = [&]<class _Pp>(int base) { return _Pp([&](int i) { return bool(__x[base + i]); }); };
  return [&]<size_t... _Jp>(index_sequence<_Jp...>) {
    if constexpr (rem == 0)
      return array<_Tp, n>{part.template operator()<_Tp>(static_cast<int>(_Jp) * __w)...};
    else
      return tuple<conditional_t<true, _Tp, integral_constant<size_t, _Jp>>..., resize_t<rem, _Tp>>(
          part.template operator()<_Tp>(static_cast<int>(_Jp) * __w)..., part.template operator()<resize_t<rem, _Tp>>(n * __w));
  }(make_index_sequence<static_cast<size_t>(n)>());
}
template <__ycxx::__detail::__simd_size_t _Np, class _Tp, class _Abi>
constexpr auto chunk(const basic_vec<_Tp, _Abi>& __x) noexcept {
  return std::simd::chunk<resize_t<_Np, basic_vec<_Tp, _Abi>>>(__x);
}
template <__ycxx::__detail::__simd_size_t _Np, size_t _Bytes, class _Abi>
constexpr auto chunk(const basic_mask<_Bytes, _Abi>& __x) noexcept {
  return std::simd::chunk<resize_t<_Np, basic_mask<_Bytes, _Abi>>>(__x);
}

template <class _Tp, class... _Abis>
constexpr resize_t<(basic_vec<_Tp, _Abis>::size() + ...), basic_vec<_Tp, _Abis...[0]>>
cat(const basic_vec<_Tp, _Abis>&... __xs) noexcept {
  using _Rp = resize_t<(basic_vec<_Tp, _Abis>::size() + ...), basic_vec<_Tp, _Abis...[0]>>;
  __ycxx::__detail::__simd_array<_Tp, _Rp::size()> a;
  int at = 0;
  ((void)[&] {
     for (int i = 0; i < __xs.size(); ++i)
       a.__v[at + i] = __xs[i];
     at += __xs.size();
   }(),
   ...);
  return __ycxx::__detail::__simd_access::__make<_Rp>(__ycxx::__detail::__simd_storage<_Tp, _Rp::size()>::__from_array(a));
}
template <size_t _Bytes, class... _Abis>
constexpr resize_t<(basic_mask<_Bytes, _Abis>::size() + ...), basic_mask<_Bytes, _Abis...[0]>>
cat(const basic_mask<_Bytes, _Abis>&... __xs) noexcept {
  using _Rp = resize_t<(basic_mask<_Bytes, _Abis>::size() + ...), basic_mask<_Bytes, _Abis...[0]>>;
  using _Ep = __ycxx::__detail::__simd_mask_element<_Bytes>;
  __ycxx::__detail::__simd_array<_Ep, _Rp::size()> a;
  int at = 0;
  ((void)[&] {
     for (int i = 0; i < __xs.size(); ++i)
       a.__v[at + i] = __xs[i] ? _Ep(-1) : _Ep(0);
     at += __xs.size();
   }(),
   ...);
  return __ycxx::__detail::__simd_access::__make<_Rp>(__ycxx::__detail::__simd_mask_storage<_Bytes, _Rp::size()>::__from_array(a));
}

}}} // namespace std::simd

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Tp>
consteval _Tp __simd_make_iota() {
  if constexpr (is_arithmetic_v<_Tp>) {
    static_assert(__simd_vectorizable<_Tp>, "std::simd::iota<T>: T must be vectorizable");
    return _Tp();
  } else {
    static_assert(__simd_vec_type<_Tp> && is_arithmetic_v<typename _Tp::value_type>,
                  "std::simd::iota<T>: T must be an enabled basic_vec of an arithmetic type");
    using _Up = typename _Tp::value_type;
    static_assert(_Tp::size() - 1 <= std::numeric_limits<_Up>::max(), "std::simd::iota<T>: T::size() - 1 does not fit");
    return __ycxx::__detail::__simd_access::__make<_Tp>(
        __simd_storage<_Up, _Tp::size()>::generate([](int i) { return static_cast<_Up>(i); }));
  }
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace simd {

template <class _Tp>
inline constexpr _Tp iota = __ycxx::__detail::__simd_make_iota<_Tp>();

// [simd.mask.reductions]
template <size_t _Bytes, class _Abi>
constexpr bool all_of(const basic_mask<_Bytes, _Abi>& k) noexcept {
  const auto& s = __ycxx::__detail::__simd_access::data(k);
  for (auto __w : __ycxx::__detail::__simd_words(s).__v)
    if (static_cast<decltype(__w)>(~__w) != 0)
      return false;
  return true;
}
template <size_t _Bytes, class _Abi>
constexpr bool any_of(const basic_mask<_Bytes, _Abi>& k) noexcept {
  const auto& s = __ycxx::__detail::__simd_access::data(k);
  for (auto __w : __ycxx::__detail::__simd_words(s).__v)
    if (__w != 0)
      return true;
  return false;
}
template <size_t _Bytes, class _Abi>
constexpr bool none_of(const basic_mask<_Bytes, _Abi>& k) noexcept {
  return !std::simd::any_of(k);
}
template <size_t _Bytes, class _Abi>
constexpr __ycxx::__detail::__simd_size_t reduce_count(const basic_mask<_Bytes, _Abi>& k) noexcept {
  const auto& s = __ycxx::__detail::__simd_access::data(k);
  using _Sp = remove_cvref_t<decltype(s)>;
  int bits = 0;
  for (auto __w : __ycxx::__detail::__simd_words(s).__v)
    bits += std::popcount(__w);
  return bits / static_cast<int>(sizeof(typename _Sp::element_type) * __CHAR_BIT__);
}
template <size_t _Bytes, class _Abi>
constexpr __ycxx::__detail::__simd_size_t reduce_min_index(const basic_mask<_Bytes, _Abi>& k) {
  __ycxx::__detail::__precondition(std::simd::any_of(k), "std::simd::reduce_min_index: no element is set");
  const auto& s = __ycxx::__detail::__simd_access::data(k);
  using _Sp = remove_cvref_t<decltype(s)>;
  if constexpr (__ycxx::__detail::__simd_words_ok<_Sp>) {
    constexpr int __ebits = static_cast<int>(sizeof(typename _Sp::element_type) * __CHAR_BIT__);
    auto __words = __ycxx::__detail::__simd_words(s);
    constexpr int __wbits = static_cast<int>(sizeof(__words.__v[0]) * __CHAR_BIT__);
    for (int __j = 0; __j < static_cast<int>(sizeof(__words.__v) / sizeof(__words.__v[0])); ++__j)
      if (__words.__v[__j] != 0)
        return (__j * __wbits + std::countr_zero(__words.__v[__j])) / __ebits;
    return 0;
  } else {
    for (int i = 0; i < k.size(); ++i)
      if (k[i])
        return i;
    return 0;
  }
}
template <size_t _Bytes, class _Abi>
constexpr __ycxx::__detail::__simd_size_t reduce_max_index(const basic_mask<_Bytes, _Abi>& k) {
  __ycxx::__detail::__precondition(std::simd::any_of(k), "std::simd::reduce_max_index: no element is set");
  const auto& s = __ycxx::__detail::__simd_access::data(k);
  using _Sp = remove_cvref_t<decltype(s)>;
  if constexpr (__ycxx::__detail::__simd_words_ok<_Sp>) {
    constexpr int __ebits = static_cast<int>(sizeof(typename _Sp::element_type) * __CHAR_BIT__);
    auto __words = __ycxx::__detail::__simd_words(s);
    constexpr int __wbits = static_cast<int>(sizeof(__words.__v[0]) * __CHAR_BIT__);
    for (int __j = static_cast<int>(sizeof(__words.__v) / sizeof(__words.__v[0])) - 1; __j >= 0; --__j)
      if (__words.__v[__j] != 0)
        return (__j * __wbits + std::bit_width(__words.__v[__j]) - 1) / __ebits;
    return 0;
  } else {
    for (int i = k.size() - 1; i >= 0; --i)
      if (k[i])
        return i;
    return 0;
  }
}
constexpr bool all_of(same_as<bool> auto __x) noexcept {
  return __x;
}
constexpr bool any_of(same_as<bool> auto __x) noexcept {
  return __x;
}
constexpr bool none_of(same_as<bool> auto __x) noexcept {
  return !__x;
}
constexpr __ycxx::__detail::__simd_size_t reduce_count(same_as<bool> auto __x) noexcept {
  return __x;
}
constexpr __ycxx::__detail::__simd_size_t reduce_min_index(same_as<bool> auto __x) {
  __ycxx::__detail::__precondition(__x, "std::simd::reduce_min_index: the argument is false");
  return 0;
}
constexpr __ycxx::__detail::__simd_size_t reduce_max_index(same_as<bool> auto __x) {
  __ycxx::__detail::__precondition(__x, "std::simd::reduce_max_index: the argument is false");
  return 0;
}

// [simd.alg]
template <class _Tp, class _Abi>
  requires totally_ordered<_Tp>
constexpr basic_vec<_Tp, _Abi> min(const basic_vec<_Tp, _Abi>& a, const basic_vec<_Tp, _Abi>& b) noexcept {
  return __simd_select_impl(b < a, b, a);
}
template <class _Tp, class _Abi>
  requires totally_ordered<_Tp>
constexpr basic_vec<_Tp, _Abi> max(const basic_vec<_Tp, _Abi>& a, const basic_vec<_Tp, _Abi>& b) noexcept {
  return __simd_select_impl(a < b, b, a);
}
template <class _Tp, class _Abi>
  requires totally_ordered<_Tp>
constexpr pair<basic_vec<_Tp, _Abi>, basic_vec<_Tp, _Abi>> minmax(const basic_vec<_Tp, _Abi>& a,
                                                            const basic_vec<_Tp, _Abi>& b) noexcept {
  return pair{std::simd::min(a, b), std::simd::max(a, b)};
}
template <class _Tp, class _Abi>
  requires totally_ordered<_Tp>
constexpr basic_vec<_Tp, _Abi> clamp(const basic_vec<_Tp, _Abi>& __v, const basic_vec<_Tp, _Abi>& __lo,
                                  const basic_vec<_Tp, _Abi>& __hi) {
  __ycxx::__detail::__precondition(std::simd::none_of(__hi < __lo), "std::simd::clamp: lo is greater than hi");
  return __simd_select_impl(__v < __lo, __lo, __simd_select_impl(__hi < __v, __hi, __v));
}
template <class _Tp, class _Up>
constexpr auto select(bool c, const _Tp& a, const _Up& b) -> remove_cvref_t<decltype(c ? a : b)> {
  return c ? a : b;
}
template <size_t _Bytes, class _Abi, class _Tp, class _Up>
constexpr auto select(const basic_mask<_Bytes, _Abi>& c, const _Tp& a, const _Up& b) noexcept
    -> decltype(__simd_select_impl(c, a, b)) {
  return __simd_select_impl(c, a, b);
}

}}} // namespace std::simd

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// GENERALIZED_SUM over the elements of x: halves combined with op until one element is left.
template <class _Vp, class _Op_>
constexpr typename _Vp::value_type __simd_reduce_tree(const _Vp& __x, const _Op_& op) {
  using _Tp = typename _Vp::value_type;
  constexpr int n = _Vp::size();
  if constexpr (n == 1) {
    return __x[0];
  } else if constexpr (__simd_known_identity<_Op_> && is_arithmetic_v<_Tp> &&
                       std::remove_cvref_t<decltype(__simd_access::data(std::declval<_Vp&>()))>::__is_vector) {
    // The std function objects on vector chunks: fold the chunks, then the lanes of the result.
    const auto& s = __simd_access::data(__x);
    auto __acc = s.c[0];
    for (int k = 1; k < s.__chunks; ++k)
      __acc = op(__acc, s.c[k]);
    _Tp r = __acc[0];
    for (int i = 1; i < s.__lanes; ++i)
      r = static_cast<_Tp>(op(r, static_cast<_Tp>(__acc[i])));
    return r;
  } else if constexpr (n % 2 == 0) {
    auto __parts = std::simd::chunk<n / 2>(__x);
    return __ycxx::__detail::__simd_reduce_tree(op(__parts[0], __parts[1]), op);
  } else {
    auto __parts = std::simd::chunk<n - 1>(__x);
    return op(std::simd::vec<_Tp, 1>(__ycxx::__detail::__simd_reduce_tree(std::get<0>(__parts), op)), std::get<1>(__parts))[0];
  }
}

// Masked GENERALIZED_SUM: the selected elements, left to right (identity if none).
template <class _Vp, class _Op_>
constexpr typename _Vp::value_type __simd_reduce_masked(const _Vp& __x, const typename _Vp::mask_type& k, const _Op_& op,
                                                    typename _Vp::value_type identity) {
  using _Tp = typename _Vp::value_type;
  if (std::simd::none_of(k))
    return identity;
  if constexpr (__simd_known_identity<_Op_> && is_integral_v<_Tp>) {
    // The identities are exact for integers: reduce everything with the others replaced.
    return __ycxx::__detail::__simd_reduce_tree(std::simd::select(k, __x, _Vp(identity)), op);
  } else {
    int i = std::simd::reduce_min_index(k);
    std::simd::vec<_Tp, 1> __acc(__x[i]);
    for (++i; i < _Vp::size(); ++i)
      if (k[i])
        __acc = op(__acc, std::simd::vec<_Tp, 1>(__x[i]));
    return __acc[0];
  }
}

// The fill value for a masked reduce_min/reduce_max: no element compares below/above it.
template <class _Tp, bool _Min>
constexpr _Tp __simd_minmax_fill() noexcept {
  if constexpr (std::numeric_limits<_Tp>::has_infinity)
    return _Min ? std::numeric_limits<_Tp>::infinity() : -std::numeric_limits<_Tp>::infinity();
  else
    return _Min ? std::numeric_limits<_Tp>::max() : std::numeric_limits<_Tp>::lowest();
}

template <class _Vp, bool _Min>
constexpr typename _Vp::value_type __simd_reduce_minmax(const _Vp& __x) noexcept {
  constexpr int n = _Vp::size();
  if constexpr (n == 1) {
    return __x[0];
  } else if constexpr (n % 2 == 0) {
    auto __parts = std::simd::chunk<n / 2>(__x);
    if constexpr (_Min)
      return __ycxx::__detail::__simd_reduce_minmax<std::remove_cvref_t<decltype(__parts[0])>, _Min>(
          std::simd::min(__parts[0], __parts[1]));
    else
      return __ycxx::__detail::__simd_reduce_minmax<std::remove_cvref_t<decltype(__parts[0])>, _Min>(
          std::simd::max(__parts[0], __parts[1]));
  } else {
    auto __parts = std::simd::chunk<n - 1>(__x);
    using _Hp = std::remove_cvref_t<decltype(std::get<0>(__parts))>;
    auto r = __ycxx::__detail::__simd_reduce_minmax<_Hp, _Min>(std::get<0>(__parts));
    auto last = std::get<1>(__parts)[0];
    if constexpr (_Min)
      return last < r ? last : r;
    else
      return r < last ? last : r;
  }
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace simd {

// [simd.reductions]
template <class _Tp, class _Abi, class _BinaryOperation = plus<>>
  requires __ycxx::__detail::__reduction_binary_operation<_BinaryOperation, _Tp>
constexpr _Tp reduce(const basic_vec<_Tp, _Abi>& __x, _BinaryOperation __binary_op = {}) {
  return __ycxx::__detail::__simd_reduce_tree(__x, __binary_op);
}
template <class _Tp, class _Abi, class _BinaryOperation = plus<>>
  requires(__ycxx::__detail::__reduction_binary_operation<_BinaryOperation, _Tp> &&
           __ycxx::__detail::__simd_known_identity<_BinaryOperation>)
constexpr _Tp reduce(const basic_vec<_Tp, _Abi>& __x, const typename basic_vec<_Tp, _Abi>::mask_type& mask,
                   _BinaryOperation __binary_op = {}) {
  return __ycxx::__detail::__simd_reduce_masked(__x, mask, __binary_op, __ycxx::__detail::__simd_identity<_Tp, _BinaryOperation>());
}
template <class _Tp, class _Abi, class _BinaryOperation>
  requires __ycxx::__detail::__reduction_binary_operation<_BinaryOperation, _Tp>
constexpr _Tp reduce(const basic_vec<_Tp, _Abi>& __x, const typename basic_vec<_Tp, _Abi>::mask_type& mask,
                   _BinaryOperation __binary_op, type_identity_t<_Tp> __identity_element) {
  return __ycxx::__detail::__simd_reduce_masked(__x, mask, __binary_op, __identity_element);
}
template <class _Tp, class _BinaryOperation = plus<>>
  requires(__ycxx::__detail::__simd_vectorizable<_Tp> && __ycxx::__detail::__reduction_binary_operation<_BinaryOperation, _Tp>)
constexpr _Tp reduce(const _Tp& __x, _BinaryOperation = {}) {
  return __x;
}
template <class _Tp, class _BinaryOperation = plus<>>
  requires(__ycxx::__detail::__simd_vectorizable<_Tp> && __ycxx::__detail::__reduction_binary_operation<_BinaryOperation, _Tp> &&
           __ycxx::__detail::__simd_known_identity<_BinaryOperation>)
constexpr _Tp reduce(const _Tp& __x, same_as<bool> auto mask, _BinaryOperation = {}) {
  return mask ? __x : __ycxx::__detail::__simd_identity<_Tp, _BinaryOperation>();
}
template <class _Tp, class _BinaryOperation>
  requires(__ycxx::__detail::__simd_vectorizable<_Tp> && __ycxx::__detail::__reduction_binary_operation<_BinaryOperation, _Tp>)
constexpr _Tp reduce(const _Tp& __x, same_as<bool> auto mask, _BinaryOperation, type_identity_t<_Tp> __identity_element) {
  return mask ? __x : __identity_element;
}

template <class _Tp, class _Abi>
  requires totally_ordered<_Tp>
constexpr _Tp reduce_min(const basic_vec<_Tp, _Abi>& __x) noexcept {
  return __ycxx::__detail::__simd_reduce_minmax<basic_vec<_Tp, _Abi>, true>(__x);
}
template <class _Tp, class _Abi>
  requires totally_ordered<_Tp>
constexpr _Tp reduce_min(const basic_vec<_Tp, _Abi>& __x, const typename basic_vec<_Tp, _Abi>::mask_type& mask) noexcept {
  if (std::simd::none_of(mask))
    return numeric_limits<_Tp>::max();
  return __ycxx::__detail::__simd_reduce_minmax<basic_vec<_Tp, _Abi>, true>(
      __simd_select_impl(mask, __x, basic_vec<_Tp, _Abi>(__ycxx::__detail::__simd_minmax_fill<_Tp, true>())));
}
template <class _Tp, class _Abi>
  requires totally_ordered<_Tp>
constexpr _Tp reduce_max(const basic_vec<_Tp, _Abi>& __x) noexcept {
  return __ycxx::__detail::__simd_reduce_minmax<basic_vec<_Tp, _Abi>, false>(__x);
}
template <class _Tp, class _Abi>
  requires totally_ordered<_Tp>
constexpr _Tp reduce_max(const basic_vec<_Tp, _Abi>& __x, const typename basic_vec<_Tp, _Abi>::mask_type& mask) noexcept {
  if (std::simd::none_of(mask))
    return numeric_limits<_Tp>::lowest();
  return __ycxx::__detail::__simd_reduce_minmax<basic_vec<_Tp, _Abi>, false>(
      __simd_select_impl(mask, __x, basic_vec<_Tp, _Abi>(__ycxx::__detail::__simd_minmax_fill<_Tp, false>())));
}
template <class _Tp>
  requires(__ycxx::__detail::__simd_vectorizable<_Tp> && totally_ordered<_Tp>)
constexpr _Tp reduce_min(const _Tp& __x) noexcept {
  return __x;
}
template <class _Tp>
  requires(__ycxx::__detail::__simd_vectorizable<_Tp> && totally_ordered<_Tp>)
constexpr _Tp reduce_min(const _Tp& __x, same_as<bool> auto mask) noexcept {
  return mask ? __x : numeric_limits<_Tp>::max();
}
template <class _Tp>
  requires(__ycxx::__detail::__simd_vectorizable<_Tp> && totally_ordered<_Tp>)
constexpr _Tp reduce_max(const _Tp& __x) noexcept {
  return __x;
}
template <class _Tp>
  requires(__ycxx::__detail::__simd_vectorizable<_Tp> && totally_ordered<_Tp>)
constexpr _Tp reduce_max(const _Tp& __x, same_as<bool> auto mask) noexcept {
  return mask ? __x : numeric_limits<_Tp>::lowest();
}

}}} // namespace std::simd

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// The default V of the loads and gathers.
template <class _Vp, class _Up>
using __simd_load_vec_t = std::conditional_t<std::is_void_v<_Vp>, std::simd::basic_vec<_Up>, _Vp>;
template <class _Vp, class _Up, class _Ip>
using __simd_gather_vec_t = std::conditional_t<std::is_void_v<_Vp>, std::simd::vec<_Up, _Ip::size()>, _Vp>;

// [simd.loadstore]/7, [simd.permute.memory]/7: the Mandates of the loads.
template <class _Vp, class _Up, class... _Flags>
consteval bool __simd_check_load() {
  static_assert(__simd_vectorizable<_Up>, "std::simd: the range's value type is not vectorizable");
  static_assert(std::same_as<std::remove_cvref_t<_Vp>, _Vp> && __simd_vec_type<_Vp>,
                "std::simd: V is not an enabled specialization of basic_vec");
  if constexpr (__simd_vec_type<_Vp>) {
    static_assert(__simd_explicitly_convertible_to<_Up, typename _Vp::value_type>,
                  "std::simd: the range's value type does not convert to V::value_type");
    if constexpr (!__simd_has_convert<_Flags...>)
      static_assert(__simd_value_preserving<_Up, typename _Vp::value_type>(),
                    "std::simd: the conversion from the range's value type to V::value_type is not value-preserving "
                    "(pass flag_convert)");
  }
  return true;
}
// [simd.loadstore]/17, [simd.permute.memory]/16: the Mandates of the stores.
template <class _Tp, class _Up, class... _Flags>
consteval bool __simd_check_store() {
  static_assert(__simd_vectorizable<_Up>, "std::simd: the range's value type is not vectorizable");
  if constexpr (!__simd_has_convert<_Flags...>)
    static_assert(__simd_value_preserving<_Tp, _Up>(),
                  "std::simd: the conversion from the element type to the range's value type is not value-preserving "
                  "(pass flag_convert)");
  return true;
}

// The alignment the flags promise for a pointer to U loaded into or stored from V (0: none).
template <class _Vp, class _Up, class... _Flags>
consteval std::size_t __simd_flags_alignment() {
  std::size_t a = __simd_overalignment<_Flags...>();
  if constexpr (__simd_has_aligned<_Flags...>)
    a = a > std::simd::alignment_v<_Vp, _Up> ? a : std::simd::alignment_v<_Vp, _Up>;
  return a;
}
template <std::size_t _Ap, class _Pp>
constexpr _Pp* __simd_assume_aligned(_Pp* p) noexcept {
  if constexpr (_Ap > 1) {
    if !consteval {
      __ycxx::__detail::__precondition(reinterpret_cast<std::uintptr_t>(p) % _Ap == 0,
                                 "std::simd: the pointer is not aligned as the flags promise");
      return static_cast<_Pp*>(__builtin_assume_aligned(p, _Ap));
    }
  }
  return p;
}

// partial_load: the elements i < n (selected by k, if given) of p, converted.
template <class _Vp, class... _Flags, class _Up>
constexpr _Vp __simd_load(_Up* p, std::size_t n, const typename _Vp::mask_type* k) {
  using _Tp = typename _Vp::value_type;
  constexpr int _Np = _Vp::size();
  using _Sp = std::remove_cvref_t<decltype(__simd_access::data(std::declval<_Vp&>()))>;
  p = __ycxx::__detail::__simd_assume_aligned<__simd_flags_alignment<_Vp, std::remove_cv_t<_Up>, _Flags...>()>(p);
  if !consteval {
    if constexpr (std::is_same_v<std::remove_cv_t<_Up>, _Tp> && _Sp::__is_vector) {
      if (k == nullptr && n >= static_cast<std::size_t>(_Np)) {
        _Sp s;
        __builtin_memcpy(__builtin_addressof(s), p, sizeof(s));
        return __simd_access::__make<_Vp>(s);
      }
    }
  }
  const auto* __ks = k == nullptr ? nullptr : __builtin_addressof(__simd_access::data(*k));
  return __simd_access::__make<_Vp>(_Sp::generate([&](int i) {
    return (__ks == nullptr || __ks->get(i) != 0) && static_cast<std::size_t>(i) < n ? static_cast<_Tp>(p[i]) : _Tp();
  }));
}
// partial_store
template <class... _Flags, class _Tp, class _Abi, class _Up>
constexpr void __simd_store(const std::simd::basic_vec<_Tp, _Abi>& __v, _Up* p, std::size_t n,
                          const typename std::simd::basic_vec<_Tp, _Abi>::mask_type* k) {
  using _Vp = std::simd::basic_vec<_Tp, _Abi>;
  constexpr int _Np = _Vp::size();
  using _Sp = std::remove_cvref_t<decltype(__simd_access::data(std::declval<_Vp&>()))>;
  p = __ycxx::__detail::__simd_assume_aligned<__simd_flags_alignment<_Vp, _Up, _Flags...>()>(p);
  if !consteval {
    if constexpr (std::is_same_v<_Up, _Tp> && _Sp::__is_vector) {
      if (k == nullptr && n >= static_cast<std::size_t>(_Np)) {
        const _Sp& s = __simd_access::data(__v);
        __builtin_memcpy(p, __builtin_addressof(s), sizeof(s));
        return;
      }
    }
  }
  for (int i = 0; i < _Np && static_cast<std::size_t>(i) < n; ++i)
    if (k == nullptr || (*k)[i])
      p[i] = static_cast<_Up>(__v[i]);
}

template <class _Rp, class _Vp>
constexpr void __simd_check_unchecked_size(_Rp& r) {
  if constexpr (__simd_static_size<_Rp> != std::dynamic_extent)
    static_assert(__simd_static_size<_Rp> >= static_cast<std::size_t>(_Vp::size()),
                  "std::simd: unchecked load/store: the range is smaller than V::size()");
  __ycxx::__detail::__precondition(std::ranges::size(r) >= static_cast<std::size_t>(_Vp::size()),
                             "std::simd: unchecked load/store: the range is smaller than V::size()");
}

template <class _Rp, class _Tp>
concept __simd_store_range =
    std::indirectly_writable<std::ranges::iterator_t<_Rp>, std::ranges::range_value_t<_Rp>> &&
    __simd_explicitly_convertible_to<_Tp, std::ranges::range_value_t<_Rp>>;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace simd {

// [simd.loadstore]
template <class _Vp = void, ranges::contiguous_range _Rp, class... _Flags>
  requires ranges::sized_range<_Rp>
constexpr __ycxx::__detail::__simd_load_vec_t<_Vp, ranges::range_value_t<_Rp>> unchecked_load(_Rp&& r, flags<_Flags...> = {}) {
  using _VV = __ycxx::__detail::__simd_load_vec_t<_Vp, ranges::range_value_t<_Rp>>;
  static_assert(__ycxx::__detail::__simd_check_load<_VV, ranges::range_value_t<_Rp>, _Flags...>());
  __ycxx::__detail::__simd_check_unchecked_size<_Rp, _VV>(r);
  return __ycxx::__detail::__simd_load<_VV, _Flags...>(ranges::data(r), ranges::size(r), nullptr);
}
template <class _Vp = void, ranges::contiguous_range _Rp, class... _Flags>
  requires ranges::sized_range<_Rp>
constexpr __ycxx::__detail::__simd_load_vec_t<_Vp, ranges::range_value_t<_Rp>>
unchecked_load(_Rp&& r, const typename __ycxx::__detail::__simd_load_vec_t<_Vp, ranges::range_value_t<_Rp>>::mask_type& k,
               flags<_Flags...> = {}) {
  using _VV = __ycxx::__detail::__simd_load_vec_t<_Vp, ranges::range_value_t<_Rp>>;
  static_assert(__ycxx::__detail::__simd_check_load<_VV, ranges::range_value_t<_Rp>, _Flags...>());
  __ycxx::__detail::__simd_check_unchecked_size<_Rp, _VV>(r);
  return __ycxx::__detail::__simd_load<_VV, _Flags...>(ranges::data(r), ranges::size(r), __builtin_addressof(k));
}
template <class _Vp = void, contiguous_iterator _Ip, class... _Flags>
constexpr __ycxx::__detail::__simd_load_vec_t<_Vp, iter_value_t<_Ip>> unchecked_load(_Ip first, iter_difference_t<_Ip> n,
                                                                          flags<_Flags...> __f = {}) {
  return std::simd::unchecked_load<_Vp>(span<const iter_value_t<_Ip>>(first, static_cast<size_t>(n)), __f);
}
template <class _Vp = void, contiguous_iterator _Ip, class... _Flags>
constexpr __ycxx::__detail::__simd_load_vec_t<_Vp, iter_value_t<_Ip>>
unchecked_load(_Ip first, iter_difference_t<_Ip> n,
               const typename __ycxx::__detail::__simd_load_vec_t<_Vp, iter_value_t<_Ip>>::mask_type& k, flags<_Flags...> __f = {}) {
  return std::simd::unchecked_load<_Vp>(span<const iter_value_t<_Ip>>(first, static_cast<size_t>(n)), k, __f);
}
template <class _Vp = void, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
constexpr __ycxx::__detail::__simd_load_vec_t<_Vp, iter_value_t<_Ip>> unchecked_load(_Ip first, _Sp last, flags<_Flags...> __f = {}) {
  return std::simd::unchecked_load<_Vp>(span<const iter_value_t<_Ip>>(first, last), __f);
}
template <class _Vp = void, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
constexpr __ycxx::__detail::__simd_load_vec_t<_Vp, iter_value_t<_Ip>>
unchecked_load(_Ip first, _Sp last, const typename __ycxx::__detail::__simd_load_vec_t<_Vp, iter_value_t<_Ip>>::mask_type& k,
               flags<_Flags...> __f = {}) {
  return std::simd::unchecked_load<_Vp>(span<const iter_value_t<_Ip>>(first, last), k, __f);
}

template <class _Vp = void, ranges::contiguous_range _Rp, class... _Flags>
  requires ranges::sized_range<_Rp>
constexpr __ycxx::__detail::__simd_load_vec_t<_Vp, ranges::range_value_t<_Rp>> partial_load(_Rp&& r, flags<_Flags...> = {}) {
  using _VV = __ycxx::__detail::__simd_load_vec_t<_Vp, ranges::range_value_t<_Rp>>;
  static_assert(__ycxx::__detail::__simd_check_load<_VV, ranges::range_value_t<_Rp>, _Flags...>());
  return __ycxx::__detail::__simd_load<_VV, _Flags...>(ranges::data(r), ranges::size(r), nullptr);
}
template <class _Vp = void, ranges::contiguous_range _Rp, class... _Flags>
  requires ranges::sized_range<_Rp>
constexpr __ycxx::__detail::__simd_load_vec_t<_Vp, ranges::range_value_t<_Rp>>
partial_load(_Rp&& r, const typename __ycxx::__detail::__simd_load_vec_t<_Vp, ranges::range_value_t<_Rp>>::mask_type& k,
             flags<_Flags...> = {}) {
  using _VV = __ycxx::__detail::__simd_load_vec_t<_Vp, ranges::range_value_t<_Rp>>;
  static_assert(__ycxx::__detail::__simd_check_load<_VV, ranges::range_value_t<_Rp>, _Flags...>());
  return __ycxx::__detail::__simd_load<_VV, _Flags...>(ranges::data(r), ranges::size(r), __builtin_addressof(k));
}
template <class _Vp = void, contiguous_iterator _Ip, class... _Flags>
constexpr __ycxx::__detail::__simd_load_vec_t<_Vp, iter_value_t<_Ip>> partial_load(_Ip first, iter_difference_t<_Ip> n,
                                                                        flags<_Flags...> __f = {}) {
  return std::simd::partial_load<_Vp>(span<const iter_value_t<_Ip>>(first, static_cast<size_t>(n)), __f);
}
template <class _Vp = void, contiguous_iterator _Ip, class... _Flags>
constexpr __ycxx::__detail::__simd_load_vec_t<_Vp, iter_value_t<_Ip>>
partial_load(_Ip first, iter_difference_t<_Ip> n,
             const typename __ycxx::__detail::__simd_load_vec_t<_Vp, iter_value_t<_Ip>>::mask_type& k, flags<_Flags...> __f = {}) {
  return std::simd::partial_load<_Vp>(span<const iter_value_t<_Ip>>(first, static_cast<size_t>(n)), k, __f);
}
template <class _Vp = void, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
constexpr __ycxx::__detail::__simd_load_vec_t<_Vp, iter_value_t<_Ip>> partial_load(_Ip first, _Sp last, flags<_Flags...> __f = {}) {
  return std::simd::partial_load<_Vp>(span<const iter_value_t<_Ip>>(first, last), __f);
}
template <class _Vp = void, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
constexpr __ycxx::__detail::__simd_load_vec_t<_Vp, iter_value_t<_Ip>>
partial_load(_Ip first, _Sp last, const typename __ycxx::__detail::__simd_load_vec_t<_Vp, iter_value_t<_Ip>>::mask_type& k,
             flags<_Flags...> __f = {}) {
  return std::simd::partial_load<_Vp>(span<const iter_value_t<_Ip>>(first, last), k, __f);
}

template <class _Tp, class _Abi, ranges::contiguous_range _Rp, class... _Flags>
  requires(ranges::sized_range<_Rp> && __ycxx::__detail::__simd_store_range<_Rp, _Tp>)
constexpr void unchecked_store(const basic_vec<_Tp, _Abi>& __v, _Rp&& r, flags<_Flags...> = {}) {
  static_assert(__ycxx::__detail::__simd_check_store<_Tp, ranges::range_value_t<_Rp>, _Flags...>());
  __ycxx::__detail::__simd_check_unchecked_size<_Rp, basic_vec<_Tp, _Abi>>(r);
  __ycxx::__detail::__simd_store<_Flags...>(__v, ranges::data(r), ranges::size(r), nullptr);
}
template <class _Tp, class _Abi, ranges::contiguous_range _Rp, class... _Flags>
  requires(ranges::sized_range<_Rp> && __ycxx::__detail::__simd_store_range<_Rp, _Tp>)
constexpr void unchecked_store(const basic_vec<_Tp, _Abi>& __v, _Rp&& r, const typename basic_vec<_Tp, _Abi>::mask_type& mask,
                               flags<_Flags...> = {}) {
  static_assert(__ycxx::__detail::__simd_check_store<_Tp, ranges::range_value_t<_Rp>, _Flags...>());
  __ycxx::__detail::__simd_check_unchecked_size<_Rp, basic_vec<_Tp, _Abi>>(r);
  __ycxx::__detail::__simd_store<_Flags...>(__v, ranges::data(r), ranges::size(r), __builtin_addressof(mask));
}
template <class _Tp, class _Abi, contiguous_iterator _Ip, class... _Flags>
  requires __ycxx::__detail::__simd_store_range<span<iter_value_t<_Ip>>, _Tp>
constexpr void unchecked_store(const basic_vec<_Tp, _Abi>& __v, _Ip first, iter_difference_t<_Ip> n, flags<_Flags...> __f = {}) {
  std::simd::unchecked_store(__v, span<iter_value_t<_Ip>>(first, static_cast<size_t>(n)), __f);
}
template <class _Tp, class _Abi, contiguous_iterator _Ip, class... _Flags>
  requires __ycxx::__detail::__simd_store_range<span<iter_value_t<_Ip>>, _Tp>
constexpr void unchecked_store(const basic_vec<_Tp, _Abi>& __v, _Ip first, iter_difference_t<_Ip> n,
                               const typename basic_vec<_Tp, _Abi>::mask_type& mask, flags<_Flags...> __f = {}) {
  std::simd::unchecked_store(__v, span<iter_value_t<_Ip>>(first, static_cast<size_t>(n)), mask, __f);
}
template <class _Tp, class _Abi, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
  requires __ycxx::__detail::__simd_store_range<span<iter_value_t<_Ip>>, _Tp>
constexpr void unchecked_store(const basic_vec<_Tp, _Abi>& __v, _Ip first, _Sp last, flags<_Flags...> __f = {}) {
  std::simd::unchecked_store(__v, span<iter_value_t<_Ip>>(first, last), __f);
}
template <class _Tp, class _Abi, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
  requires __ycxx::__detail::__simd_store_range<span<iter_value_t<_Ip>>, _Tp>
constexpr void unchecked_store(const basic_vec<_Tp, _Abi>& __v, _Ip first, _Sp last,
                               const typename basic_vec<_Tp, _Abi>::mask_type& mask, flags<_Flags...> __f = {}) {
  std::simd::unchecked_store(__v, span<iter_value_t<_Ip>>(first, last), mask, __f);
}

template <class _Tp, class _Abi, ranges::contiguous_range _Rp, class... _Flags>
  requires(ranges::sized_range<_Rp> && __ycxx::__detail::__simd_store_range<_Rp, _Tp>)
constexpr void partial_store(const basic_vec<_Tp, _Abi>& __v, _Rp&& r, flags<_Flags...> = {}) {
  static_assert(__ycxx::__detail::__simd_check_store<_Tp, ranges::range_value_t<_Rp>, _Flags...>());
  __ycxx::__detail::__simd_store<_Flags...>(__v, ranges::data(r), ranges::size(r), nullptr);
}
template <class _Tp, class _Abi, ranges::contiguous_range _Rp, class... _Flags>
  requires(ranges::sized_range<_Rp> && __ycxx::__detail::__simd_store_range<_Rp, _Tp>)
constexpr void partial_store(const basic_vec<_Tp, _Abi>& __v, _Rp&& r, const typename basic_vec<_Tp, _Abi>::mask_type& mask,
                             flags<_Flags...> = {}) {
  static_assert(__ycxx::__detail::__simd_check_store<_Tp, ranges::range_value_t<_Rp>, _Flags...>());
  __ycxx::__detail::__simd_store<_Flags...>(__v, ranges::data(r), ranges::size(r), __builtin_addressof(mask));
}
template <class _Tp, class _Abi, contiguous_iterator _Ip, class... _Flags>
  requires __ycxx::__detail::__simd_store_range<span<iter_value_t<_Ip>>, _Tp>
constexpr void partial_store(const basic_vec<_Tp, _Abi>& __v, _Ip first, iter_difference_t<_Ip> n, flags<_Flags...> __f = {}) {
  std::simd::partial_store(__v, span<iter_value_t<_Ip>>(first, static_cast<size_t>(n)), __f);
}
template <class _Tp, class _Abi, contiguous_iterator _Ip, class... _Flags>
  requires __ycxx::__detail::__simd_store_range<span<iter_value_t<_Ip>>, _Tp>
constexpr void partial_store(const basic_vec<_Tp, _Abi>& __v, _Ip first, iter_difference_t<_Ip> n,
                             const typename basic_vec<_Tp, _Abi>::mask_type& mask, flags<_Flags...> __f = {}) {
  std::simd::partial_store(__v, span<iter_value_t<_Ip>>(first, static_cast<size_t>(n)), mask, __f);
}
template <class _Tp, class _Abi, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
  requires __ycxx::__detail::__simd_store_range<span<iter_value_t<_Ip>>, _Tp>
constexpr void partial_store(const basic_vec<_Tp, _Abi>& __v, _Ip first, _Sp last, flags<_Flags...> __f = {}) {
  std::simd::partial_store(__v, span<iter_value_t<_Ip>>(first, last), __f);
}
template <class _Tp, class _Abi, contiguous_iterator _Ip, sized_sentinel_for<_Ip> _Sp, class... _Flags>
  requires __ycxx::__detail::__simd_store_range<span<iter_value_t<_Ip>>, _Tp>
constexpr void partial_store(const basic_vec<_Tp, _Abi>& __v, _Ip first, _Sp last,
                             const typename basic_vec<_Tp, _Abi>::mask_type& mask, flags<_Flags...> __f = {}) {
  std::simd::partial_store(__v, span<iter_value_t<_Ip>>(first, last), mask, __f);
}

// [simd.permute.static]
inline constexpr __ycxx::__detail::__simd_size_t zero_element = numeric_limits<__ycxx::__detail::__simd_size_t>::min();
inline constexpr __ycxx::__detail::__simd_size_t uninit_element = numeric_limits<__ycxx::__detail::__simd_size_t>::min() + 1;

}}} // namespace std::simd

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// The default N of the static permute (any value that is not a width).
inline constexpr __simd_size_t __simd_permute_default = std::numeric_limits<__simd_size_t>::min();
template <__simd_size_t _Np, class _Vp>
inline constexpr __simd_size_t __simd_permute_size = _Np == __simd_permute_default ? _Vp::size() : _Np;

template <class _IdxMap>
concept __simd_index_map = std::integral<std::invoke_result_t<_IdxMap&, __simd_size_t>> ||
                         std::integral<std::invoke_result_t<_IdxMap&, __simd_size_t, __simd_size_t>>;

// perm-fn<I>() ([simd.permute.static]/1.2)
template <__simd_size_t _Src, class _Vp>
constexpr typename _Vp::value_type __simd_permute_pick(const _Vp& __v) {
  static_assert(_Src == std::simd::zero_element || _Src == std::simd::uninit_element || (_Src >= 0 && _Src < _Vp::size()),
                "std::simd::permute: the index map gives an index out of range");
  if constexpr (_Src == std::simd::zero_element || _Src == std::simd::uninit_element)
    return typename _Vp::value_type();
  else
    return __v[_Src];
}
template <__simd_size_t _Ip, class _Vp, class _IdxMap>
constexpr typename _Vp::value_type __simd_permute_element(const _Vp& __v, _IdxMap& __idxmap) {
  if constexpr (requires { __idxmap(_Ip, _Vp::size()); }) {
    constexpr auto __src = __idxmap(__simd_size_t(_Ip), __simd_size_t(_Vp::size()));
    return __ycxx::__detail::__simd_permute_pick<static_cast<__simd_size_t>(__src)>(__v);
  } else {
    constexpr auto __src = __idxmap(__simd_size_t(_Ip));
    return __ycxx::__detail::__simd_permute_pick<static_cast<__simd_size_t>(__src)>(__v);
  }
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace simd {

template <__ycxx::__detail::__simd_size_t _Np = __ycxx::__detail::__simd_permute_default, class _Vp, class _IdxMap>
  requires((__ycxx::__detail::__simd_vec_type<_Vp> || __ycxx::__detail::__simd_mask_type<_Vp>) && __ycxx::__detail::__simd_index_map<_IdxMap>)
constexpr resize_t<__ycxx::__detail::__simd_permute_size<_Np, _Vp>, _Vp> permute(const _Vp& __v, _IdxMap&& __idxmap) {
  using _Rp = resize_t<__ycxx::__detail::__simd_permute_size<_Np, _Vp>, _Vp>;
  return _Rp([&](auto i) { return __ycxx::__detail::__simd_permute_element<decltype(i)::value>(__v, __idxmap); });
}

// [simd.permute.dynamic]
template <class _Vp, __ycxx::__detail::__simd_integral _Ip>
  requires(__ycxx::__detail::__simd_vec_type<_Vp> || __ycxx::__detail::__simd_mask_type<_Vp>)
constexpr resize_t<_Ip::size(), _Vp> permute(const _Vp& __v, const _Ip& indices) {
  using _Rp = resize_t<_Ip::size(), _Vp>;
  using _Tp = typename _Vp::value_type;
  if constexpr (__ycxx::__detail::__simd_vec_type<_Vp>) {
    using _Sp = remove_cvref_t<decltype(__ycxx::__detail::__simd_access::data(declval<_Rp&>()))>;
    return __ycxx::__detail::__simd_access::__make<_Rp>(_Sp::generate([&](int i) -> _Tp {
      auto __j = indices[i];
      __ycxx::__detail::__precondition(__j >= 0 && __j < _Vp::size(), "std::simd::permute: index out of range");
      return __v[static_cast<__ycxx::__detail::__simd_size_t>(__j)];
    }));
  } else {
    using _Sp = remove_cvref_t<decltype(__ycxx::__detail::__simd_access::data(declval<_Rp&>()))>;
    using _Ep = typename _Sp::element_type;
    return __ycxx::__detail::__simd_access::__make<_Rp>(_Sp::generate([&](int i) {
      auto __j = indices[i];
      __ycxx::__detail::__precondition(__j >= 0 && __j < _Vp::size(), "std::simd::permute: index out of range");
      return __v[static_cast<__ycxx::__detail::__simd_size_t>(__j)] ? _Ep(-1) : _Ep(0);
    }));
  }
}

}}} // namespace std::simd

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// Builds a V from values of its elements (bool for a mask).
template <class _Vp, class _Fp>
constexpr _Vp __simd_build(_Fp&& __f) {
  using _Sp = std::remove_cvref_t<decltype(__simd_access::data(std::declval<_Vp&>()))>;
  if constexpr (__simd_mask_type<_Vp>) {
    using _Ep = typename _Sp::element_type;
    return __simd_access::__make<_Vp>(_Sp::generate([&](int i) { return __f(i) ? _Ep(-1) : _Ep(0); }));
  } else {
    return __simd_access::__make<_Vp>(_Sp::generate(__f));
  }
}
template <class _Vp, class _Mp>
constexpr _Vp __simd_compress(const _Vp& __v, const _Mp& __selector, const typename _Vp::value_type& fill) {
  __simd_array<typename _Vp::value_type, _Vp::size()> a;
  int n = 0;
  for (int i = 0; i < _Vp::size(); ++i)
    if (__selector[i])
      a.__v[n++] = __v[i];
  for (int i = n; i < _Vp::size(); ++i)
    a.__v[i] = fill;
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return a.__v[i]; });
}
template <class _Vp, class _Mp>
constexpr _Vp __simd_expand(const _Vp& __v, const _Mp& __selector, const _Vp& __original) {
  __simd_array<typename _Vp::value_type, _Vp::size()> a;
  int n = 0;
  for (int i = 0; i < _Vp::size(); ++i)
    a.__v[i] = __selector[i] ? __v[n++] : __original[i];
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return a.__v[i]; });
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace simd {

// [simd.permute.mask]
template <__ycxx::__detail::__simd_vec_type _Vp>
constexpr _Vp compress(const _Vp& __v, const typename _Vp::mask_type& __selector) {
  return __ycxx::__detail::__simd_compress(__v, __selector, typename _Vp::value_type());
}
template <__ycxx::__detail::__simd_mask_type _Vp>
constexpr _Vp compress(const _Vp& __v, const type_identity_t<_Vp>& __selector) {
  return __ycxx::__detail::__simd_compress(__v, __selector, false);
}
template <__ycxx::__detail::__simd_vec_type _Vp>
constexpr _Vp compress(const _Vp& __v, const typename _Vp::mask_type& __selector, const typename _Vp::value_type& __fill_value) {
  return __ycxx::__detail::__simd_compress(__v, __selector, __fill_value);
}
template <__ycxx::__detail::__simd_mask_type _Vp>
constexpr _Vp compress(const _Vp& __v, const type_identity_t<_Vp>& __selector, const typename _Vp::value_type& __fill_value) {
  return __ycxx::__detail::__simd_compress(__v, __selector, __fill_value);
}
template <__ycxx::__detail::__simd_vec_type _Vp>
constexpr _Vp expand(const _Vp& __v, const typename _Vp::mask_type& __selector, const _Vp& __original = {}) {
  return __ycxx::__detail::__simd_expand(__v, __selector, __original);
}
template <__ycxx::__detail::__simd_mask_type _Vp>
constexpr _Vp expand(const _Vp& __v, const type_identity_t<_Vp>& __selector, const _Vp& __original = {}) {
  return __ycxx::__detail::__simd_expand(__v, __selector, __original);
}

}}} // namespace std::simd

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <class _Rp, class _Vp>
concept __simd_gather_range = __simd_vectorizable<std::ranges::range_value_t<_Rp>> &&
                            __simd_explicitly_convertible_to<std::ranges::range_value_t<_Rp>, typename _Vp::value_type>;
// [simd.permute.memory]/6 for the default or given V.
template <class _Vp, class _Rp, class _Ip>
using __simd_gather_t = __simd_gather_vec_t<_Vp, std::ranges::range_value_t<_Rp>, _Ip>;
template <class _Vp, class _Rp, class _Ip>
concept __simd_gather_source = std::ranges::sized_range<_Rp> && __simd_gather_range<_Rp, __simd_gather_t<_Vp, _Rp, _Ip>>;

template <class _Vp, class _Ip, class... _Flags, class _Up>
constexpr _Vp __simd_gather(_Up* p, std::size_t n, const typename _Ip::mask_type* k, const _Ip& indices) {
  static_assert(_Vp::size() == _Ip::size(), "std::simd: gather: V::size() != I::size()");
  using _Tp = typename _Vp::value_type;
  p = __ycxx::__detail::__simd_assume_aligned<__simd_flags_alignment<_Vp, std::remove_cv_t<_Up>, _Flags...>()>(p);
  using _Sp = std::remove_cvref_t<decltype(__simd_access::data(std::declval<_Vp&>()))>;
  return __simd_access::__make<_Vp>(_Sp::generate([&](int i) {
    auto __j = indices[i];
    return (k == nullptr || (*k)[i]) && __j >= 0 && static_cast<std::size_t>(__j) < n ? static_cast<_Tp>(p[__j]) : _Tp();
  }));
}
template <class... _Flags, class _Vp, class _Ip, class _Up>
constexpr void __simd_scatter(const _Vp& __v, _Up* p, std::size_t n, const typename _Ip::mask_type* k, const _Ip& indices) {
  p = __ycxx::__detail::__simd_assume_aligned<__simd_flags_alignment<_Vp, _Up, _Flags...>()>(p);
  for (int i = 0; i < _Vp::size(); ++i) {
    auto __j = indices[i];
    if ((k == nullptr || (*k)[i]) && __j >= 0 && static_cast<std::size_t>(__j) < n)
      p[__j] = static_cast<_Up>(__v[i]);
  }
}
template <class _Ip>
constexpr void __simd_check_indices(const _Ip& indices, const typename _Ip::mask_type& k, std::size_t n) {
  if !consteval {
    if constexpr (!__cfg::__hardened)
      return;
  }
  for (int i = 0; i < _Ip::size(); ++i)
    if (k[i])
      __ycxx::__detail::__precondition(indices[i] >= 0 && static_cast<std::size_t>(indices[i]) < n,
                                 "std::simd: unchecked gather/scatter: index out of range");
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace simd {

// [simd.permute.memory]
template <class _Vp = void, ranges::contiguous_range _Rp, __ycxx::__detail::__simd_integral _Ip, class... _Flags>
  requires __ycxx::__detail::__simd_gather_source<_Vp, _Rp, _Ip>
constexpr __ycxx::__detail::__simd_gather_t<_Vp, _Rp, _Ip> partial_gather_from(_Rp&& in, const _Ip& indices, flags<_Flags...> = {}) {
  using _VV = __ycxx::__detail::__simd_gather_t<_Vp, _Rp, _Ip>;
  static_assert(__ycxx::__detail::__simd_check_load<_VV, ranges::range_value_t<_Rp>, _Flags...>());
  return __ycxx::__detail::__simd_gather<_VV, _Ip, _Flags...>(ranges::data(in), ranges::size(in), nullptr, indices);
}
template <class _Vp = void, ranges::contiguous_range _Rp, __ycxx::__detail::__simd_integral _Ip, class... _Flags>
  requires __ycxx::__detail::__simd_gather_source<_Vp, _Rp, _Ip>
constexpr __ycxx::__detail::__simd_gather_t<_Vp, _Rp, _Ip>
partial_gather_from(_Rp&& in, const typename _Ip::mask_type& mask, const _Ip& indices, flags<_Flags...> = {}) {
  using _VV = __ycxx::__detail::__simd_gather_t<_Vp, _Rp, _Ip>;
  static_assert(__ycxx::__detail::__simd_check_load<_VV, ranges::range_value_t<_Rp>, _Flags...>());
  return __ycxx::__detail::__simd_gather<_VV, _Ip, _Flags...>(ranges::data(in), ranges::size(in), __builtin_addressof(mask),
                                                    indices);
}
template <class _Vp = void, ranges::contiguous_range _Rp, __ycxx::__detail::__simd_integral _Ip, class... _Flags>
  requires __ycxx::__detail::__simd_gather_source<_Vp, _Rp, _Ip>
constexpr __ycxx::__detail::__simd_gather_t<_Vp, _Rp, _Ip> unchecked_gather_from(_Rp&& in, const _Ip& indices,
                                                                    flags<_Flags...> __f = {}) {
  __ycxx::__detail::__simd_check_indices(indices, typename _Ip::mask_type(true), ranges::size(in));
  return std::simd::partial_gather_from<_Vp>(in, indices, __f);
}
template <class _Vp = void, ranges::contiguous_range _Rp, __ycxx::__detail::__simd_integral _Ip, class... _Flags>
  requires __ycxx::__detail::__simd_gather_source<_Vp, _Rp, _Ip>
constexpr __ycxx::__detail::__simd_gather_t<_Vp, _Rp, _Ip>
unchecked_gather_from(_Rp&& in, const typename _Ip::mask_type& mask, const _Ip& indices, flags<_Flags...> __f = {}) {
  __ycxx::__detail::__simd_check_indices(indices, mask, ranges::size(in));
  return std::simd::partial_gather_from<_Vp>(in, mask, indices, __f);
}

template <__ycxx::__detail::__simd_vec_type _Vp, ranges::contiguous_range _Rp, __ycxx::__detail::__simd_integral _Ip, class... _Flags>
  requires(ranges::sized_range<_Rp> && _Vp::size() == _Ip::size() &&
           __ycxx::__detail::__simd_store_range<_Rp, typename _Vp::value_type>)
constexpr void partial_scatter_to(const _Vp& __v, _Rp&& out, const _Ip& indices, flags<_Flags...> = {}) {
  static_assert(__ycxx::__detail::__simd_check_store<typename _Vp::value_type, ranges::range_value_t<_Rp>, _Flags...>());
  __ycxx::__detail::__simd_scatter<_Flags...>(__v, ranges::data(out), ranges::size(out), nullptr, indices);
}
template <__ycxx::__detail::__simd_vec_type _Vp, ranges::contiguous_range _Rp, __ycxx::__detail::__simd_integral _Ip, class... _Flags>
  requires(ranges::sized_range<_Rp> && _Vp::size() == _Ip::size() &&
           __ycxx::__detail::__simd_store_range<_Rp, typename _Vp::value_type>)
constexpr void partial_scatter_to(const _Vp& __v, _Rp&& out, const typename _Ip::mask_type& mask, const _Ip& indices,
                                  flags<_Flags...> = {}) {
  static_assert(__ycxx::__detail::__simd_check_store<typename _Vp::value_type, ranges::range_value_t<_Rp>, _Flags...>());
  __ycxx::__detail::__simd_scatter<_Flags...>(__v, ranges::data(out), ranges::size(out), __builtin_addressof(mask), indices);
}
template <__ycxx::__detail::__simd_vec_type _Vp, ranges::contiguous_range _Rp, __ycxx::__detail::__simd_integral _Ip, class... _Flags>
  requires(ranges::sized_range<_Rp> && _Vp::size() == _Ip::size() &&
           __ycxx::__detail::__simd_store_range<_Rp, typename _Vp::value_type>)
constexpr void unchecked_scatter_to(const _Vp& __v, _Rp&& out, const _Ip& indices, flags<_Flags...> __f = {}) {
  __ycxx::__detail::__simd_check_indices(indices, typename _Ip::mask_type(true), ranges::size(out));
  std::simd::partial_scatter_to(__v, out, indices, __f);
}
template <__ycxx::__detail::__simd_vec_type _Vp, ranges::contiguous_range _Rp, __ycxx::__detail::__simd_integral _Ip, class... _Flags>
  requires(ranges::sized_range<_Rp> && _Vp::size() == _Ip::size() &&
           __ycxx::__detail::__simd_store_range<_Rp, typename _Vp::value_type>)
constexpr void unchecked_scatter_to(const _Vp& __v, _Rp&& out, const typename _Ip::mask_type& mask, const _Ip& indices,
                                    flags<_Flags...> __f = {}) {
  __ycxx::__detail::__simd_check_indices(indices, mask, ranges::size(out));
  std::simd::partial_scatter_to(__v, out, mask, indices, __f);
}

}}} // namespace std::simd

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Tp>
concept __simd_unsigned_element = __bit_unsigned<_Tp>;
template <class _Tp>
concept __simd_integer_element = __bit_integer<_Tp>;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace simd {

// [simd.bit]: the <bit> functions element-wise.
template <__ycxx::__detail::__simd_vec_type _Vp>
  requires integral<typename _Vp::value_type>
constexpr _Vp byteswap(const _Vp& __v) noexcept {
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return std::byteswap(__v[i]); });
}
template <__ycxx::__detail::__simd_integral _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr _Vp bit_reverse(const _Vp& __v) noexcept {
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return std::bit_reverse(__v[i]); });
}
template <__ycxx::__detail::__simd_vec_type _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr _Vp bit_ceil(const _Vp& __v) {
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return std::bit_ceil(__v[i]); });
}
template <__ycxx::__detail::__simd_vec_type _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr _Vp bit_floor(const _Vp& __v) noexcept {
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return std::bit_floor(__v[i]); });
}
template <__ycxx::__detail::__simd_vec_type _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr typename _Vp::mask_type has_single_bit(const _Vp& __v) noexcept {
  return __ycxx::__detail::__simd_build<typename _Vp::mask_type>([&](int i) { return std::has_single_bit(__v[i]); });
}
template <__ycxx::__detail::__simd_integral _VX, __ycxx::__detail::__simd_integral _VS>
  requires(__ycxx::__detail::__simd_integer_element<typename _VX::value_type> &&
           __ycxx::__detail::__simd_integer_element<typename _VS::value_type> && _VX::size() == _VS::size() &&
           sizeof(typename _VX::value_type) == sizeof(typename _VS::value_type))
constexpr _VX shl(const _VX& __x, const _VS& s) noexcept {
  return __ycxx::__detail::__simd_build<_VX>([&](int i) { return std::shl(__x[i], s[i]); });
}
template <__ycxx::__detail::__simd_integral _Vp, class _Sp>
  requires(__ycxx::__detail::__simd_integer_element<typename _Vp::value_type> && __ycxx::__detail::__simd_integer_element<_Sp>)
constexpr _Vp shl(const _Vp& __x, _Sp s) noexcept {
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return std::shl(__x[i], s); });
}
template <__ycxx::__detail::__simd_integral _VX, __ycxx::__detail::__simd_integral _VS>
  requires(__ycxx::__detail::__simd_integer_element<typename _VX::value_type> &&
           __ycxx::__detail::__simd_integer_element<typename _VS::value_type> && _VX::size() == _VS::size() &&
           sizeof(typename _VX::value_type) == sizeof(typename _VS::value_type))
constexpr _VX shr(const _VX& __x, const _VS& s) noexcept {
  return __ycxx::__detail::__simd_build<_VX>([&](int i) { return std::shr(__x[i], s[i]); });
}
template <__ycxx::__detail::__simd_integral _Vp, class _Sp>
  requires(__ycxx::__detail::__simd_integer_element<typename _Vp::value_type> && __ycxx::__detail::__simd_integer_element<_Sp>)
constexpr _Vp shr(const _Vp& __x, _Sp s) noexcept {
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return std::shr(__x[i], s); });
}
template <__ycxx::__detail::__simd_vec_type _V0, __ycxx::__detail::__simd_vec_type _V1>
  requires(__ycxx::__detail::__simd_unsigned_element<typename _V0::value_type> && integral<typename _V1::value_type> &&
           _V0::size() == _V1::size() && sizeof(typename _V0::value_type) == sizeof(typename _V1::value_type))
constexpr _V0 rotl(const _V0& __v, const _V1& s) noexcept {
  return __ycxx::__detail::__simd_build<_V0>([&](int i) { return std::rotl(__v[i], static_cast<int>(s[i])); });
}
template <__ycxx::__detail::__simd_vec_type _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr _Vp rotl(const _Vp& __v, int s) noexcept {
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return std::rotl(__v[i], s); });
}
template <__ycxx::__detail::__simd_vec_type _V0, __ycxx::__detail::__simd_vec_type _V1>
  requires(__ycxx::__detail::__simd_unsigned_element<typename _V0::value_type> && integral<typename _V1::value_type> &&
           _V0::size() == _V1::size() && sizeof(typename _V0::value_type) == sizeof(typename _V1::value_type))
constexpr _V0 rotr(const _V0& __v, const _V1& s) noexcept {
  return __ycxx::__detail::__simd_build<_V0>([&](int i) { return std::rotr(__v[i], static_cast<int>(s[i])); });
}
template <__ycxx::__detail::__simd_vec_type _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr _Vp rotr(const _Vp& __v, int s) noexcept {
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return std::rotr(__v[i], s); });
}
template <__ycxx::__detail::__simd_integral _V0, __ycxx::__detail::__simd_integral _V1>
  requires(__ycxx::__detail::__simd_unsigned_element<typename _V0::value_type> && _V0::size() == _V1::size() &&
           sizeof(typename _V0::value_type) == sizeof(typename _V1::value_type))
constexpr _V0 bit_repeat(const _V0& __v, const _V1& __l) {
  return __ycxx::__detail::__simd_build<_V0>([&](int i) { return std::bit_repeat(__v[i], static_cast<int>(__l[i])); });
}
template <__ycxx::__detail::__simd_integral _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr _Vp bit_repeat(const _Vp& __v, int __l) {
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return std::bit_repeat(__v[i], __l); });
}
template <__ycxx::__detail::__simd_vec_type _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp> bit_width(const _Vp& __v) noexcept {
  return __ycxx::__detail::__simd_build<rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>>(
      [&](int i) { return static_cast<make_signed_t<typename _Vp::value_type>>(std::bit_width(__v[i])); });
}
template <__ycxx::__detail::__simd_vec_type _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp> countl_zero(const _Vp& __v) noexcept {
  return __ycxx::__detail::__simd_build<rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>>(
      [&](int i) { return static_cast<make_signed_t<typename _Vp::value_type>>(std::countl_zero(__v[i])); });
}
template <__ycxx::__detail::__simd_vec_type _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp> countl_one(const _Vp& __v) noexcept {
  return __ycxx::__detail::__simd_build<rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>>(
      [&](int i) { return static_cast<make_signed_t<typename _Vp::value_type>>(std::countl_one(__v[i])); });
}
template <__ycxx::__detail::__simd_vec_type _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp> countr_zero(const _Vp& __v) noexcept {
  return __ycxx::__detail::__simd_build<rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>>(
      [&](int i) { return static_cast<make_signed_t<typename _Vp::value_type>>(std::countr_zero(__v[i])); });
}
template <__ycxx::__detail::__simd_vec_type _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp> countr_one(const _Vp& __v) noexcept {
  return __ycxx::__detail::__simd_build<rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>>(
      [&](int i) { return static_cast<make_signed_t<typename _Vp::value_type>>(std::countr_one(__v[i])); });
}
template <__ycxx::__detail::__simd_vec_type _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr rebind_t<make_signed_t<typename _Vp::value_type>, _Vp> popcount(const _Vp& __v) noexcept {
  return __ycxx::__detail::__simd_build<rebind_t<make_signed_t<typename _Vp::value_type>, _Vp>>(
      [&](int i) { return static_cast<make_signed_t<typename _Vp::value_type>>(std::popcount(__v[i])); });
}
template <__ycxx::__detail::__simd_integral _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr _Vp bit_compress(const _Vp& __v, const _Vp& m) noexcept {
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return std::bit_compress(__v[i], m[i]); });
}
template <__ycxx::__detail::__simd_integral _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr _Vp bit_expand(const _Vp& __v, const _Vp& m) noexcept {
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return std::bit_expand(__v[i], m[i]); });
}
template <__ycxx::__detail::__simd_integral _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr _Vp bit_compress(const _Vp& __v, typename _Vp::value_type m) noexcept {
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return std::bit_compress(__v[i], m); });
}
template <__ycxx::__detail::__simd_integral _Vp>
  requires __ycxx::__detail::__simd_unsigned_element<typename _Vp::value_type>
constexpr _Vp bit_expand(const _Vp& __v, typename _Vp::value_type m) noexcept {
  return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return std::bit_expand(__v[i], m); });
}

}}} // namespace std::simd

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// fabs on vector chunks: clear the sign bits.
template <class _Vp>
constexpr _Vp __simd_fabs(const _Vp& __x) {
  using _Sp = std::remove_cvref_t<decltype(__simd_access::data(std::declval<_Vp&>()))>;
  using _Tp = typename _Vp::value_type;
  if constexpr (_Sp::__is_vector) {
    using _Ip = __simd_integer_from<sizeof(_Tp)>;
    using _MC = __simd_vector<_Ip, _Sp::__lanes>;
    using _Cp = typename _Sp::__chunk_type;
    return __simd_access::__make<_Vp>(_Sp::map(__simd_access::data(__x), [](_Cp c) {
      return __builtin_bit_cast(_Cp, static_cast<_MC>(__builtin_bit_cast(_MC, c) & (_MC{} + std::numeric_limits<_Ip>::max())));
    }));
  } else {
    return __ycxx::__detail::__simd_build<_Vp>([&](int i) { return std::fabs(__x[i]); });
  }
}
}} // namespace __ycxx::__detail

#include <ycxx/core/simd_math.hpp>
