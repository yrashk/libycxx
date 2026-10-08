// libycxx core: <simd> part 1 ([simd.expos], [simd.traits], [simd.flags], [simd.iterator]): the
// vectorizable types, the ABI tags, element storage, the type traits, the load/store flags and
// simd-iterator. The classes are in simd.hpp.
//
// ABI tags. deduce-abi-t<T, N> is __ycxx::__adl_free::__simd_abi<N, R> for every vectorizable T and
// every N in [1, 64], R being the widest vector register (cfg::simd_register_bytes);
// native-abi<T> is simd_abi<R / sizeof(T), R> (at least 1). The tag does not name the element
// type, so masks of equal element size and width are one type, rebind_t and resize_t only swap
// the width, and the element type picks the representation:
//  - an arithmetic element type and a power-of-two width N >= 2: GCC/Clang vector-extension
//    chunks (vector_size), one vector of N elements while N * sizeof(T) fits the widest vector
//    register (cfg::simd_register_bytes), else an array of register-sized vectors. Operators map
//    to the vector operators;
//  - any other width, and complex<T>: an array of N elements, operated on element by element.
// A mask stores one integer of the element size per element (integer-from<Bytes>, 64 bits for
// 16-byte complex elements), all bits set for true, in the same layout as a vec of that integer
// type, so comparisons and masked selects are bitwise operations on the chunks.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/array.hpp>
#include <ycxx/core/bit.hpp>
#include <ycxx/core/complex.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/integer_sequence.hpp>
#include <ycxx/core/iterator_ops.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/span.hpp>
#include <ycxx/core/type_traits.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {
// The ABI tags (see above). R, the register width the layout is built for, is part of the type,
// so translation units built for different widths do not share a type with two layouts; only the
// tags of this translation unit's width are enabled.
template <int _Np, int _Rp = __ycxx::__detail::__cfg::__simd_register_bytes>
struct __simd_abi {};
// deduce-abi-t<T, N> when T is not vectorizable or N is out of range: no basic_vec is enabled.
struct __simd_abi_none {};
// convert-flag, aligned-flag, overaligned-flag<N> ([simd.flags]).
struct __simd_convert_flag {};
struct __simd_aligned_flag {};
template <std::size_t _Np>
struct __simd_overaligned_flag {};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Fp>
inline constexpr bool __simd_is_flag = false;
template <>
inline constexpr bool __simd_is_flag<__ycxx::__adl_free::__simd_convert_flag> = true;
template <>
inline constexpr bool __simd_is_flag<__ycxx::__adl_free::__simd_aligned_flag> = true;
template <std::size_t _Np>
inline constexpr bool __simd_is_flag<__ycxx::__adl_free::__simd_overaligned_flag<_Np>> = true;
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace simd {
template <class... _Flags>
  requires(__ycxx::__detail::__simd_is_flag<_Flags> && ...)
struct flags;
template <size_t _Bytes, class _Abi>
class basic_mask;
}}} // namespace std::simd

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// simd-size-type
using __simd_size_t = int;
// The largest width deduce-abi-t accepts ([simd.expos.abi]/4).
inline constexpr __simd_size_t __simd_max_width = 64;

// integer-from<Bytes>
template <std::size_t _Bytes>
struct __simd_integer_from_impl {};
template <>
struct __simd_integer_from_impl<1> {
  using type = signed char;
};
template <>
struct __simd_integer_from_impl<2> {
  using type = short;
};
template <>
struct __simd_integer_from_impl<4> {
  using type = int;
};
template <>
struct __simd_integer_from_impl<8> {
  using type = long long;
};
template <std::size_t _Bytes>
using __simd_integer_from = typename __simd_integer_from_impl<_Bytes>::type;

// The vectorizable arithmetic types ([simd.general]/2.1-2.2): these get vector storage.
template <class _Tp>
inline constexpr bool __simd_arithmetic =
    __is_same(_Tp, signed char) || __is_same(_Tp, short) || __is_same(_Tp, int) || __is_same(_Tp, long) ||
    __is_same(_Tp, long long) || __is_same(_Tp, unsigned char) || __is_same(_Tp, unsigned short) ||
    __is_same(_Tp, unsigned int) || __is_same(_Tp, unsigned long) || __is_same(_Tp, unsigned long long) ||
    __is_same(_Tp, char) || __is_same(_Tp, wchar_t) || __is_same(_Tp, char8_t) || __is_same(_Tp, char16_t) ||
    __is_same(_Tp, char32_t) || __is_same(_Tp, float) || __is_same(_Tp, double) || __is_same(_Tp, __float16) ||
    __is_same(_Tp, __float32) || __is_same(_Tp, __float64);
template <class _Tp>
inline constexpr bool __simd_is_complex = false;
template <class _Tp>
inline constexpr bool __simd_is_complex<std::complex<_Tp>> = true;
// [simd.general]/2.3: complex<T> for a vectorizable floating-point T.
template <class _Tp>
inline constexpr bool __simd_vectorizable = __simd_arithmetic<_Tp>;
template <class _Tp>
inline constexpr bool __simd_vectorizable<std::complex<_Tp>> = __simd_arithmetic<_Tp> && __is_floating_v<_Tp>;

// [simd.general]/8: every value of From is a value of To. A complex type counts as its value type
// (a real value is a complex value with a zero imaginary part); complex to real never preserves.
template <class _From, class _To>
consteval bool __simd_value_preserving() {
  if constexpr (__simd_is_complex<_To>) {
    if constexpr (__simd_is_complex<_From>)
      return __ycxx::__detail::__simd_value_preserving<typename _From::value_type, typename _To::value_type>();
    else
      return __ycxx::__detail::__simd_value_preserving<_From, typename _To::value_type>();
  } else if constexpr (__simd_is_complex<_From> || !is_arithmetic_v<_From> || !is_arithmetic_v<_To>) {
    return false;
  } else if constexpr (is_integral_v<_From> && is_integral_v<_To>) {
    using _LF = std::numeric_limits<_From>;
    using _LT = std::numeric_limits<_To>;
    return (!_LF::is_signed || _LT::is_signed) && _LF::digits <= _LT::digits;
  } else if constexpr (is_integral_v<_From>) {
    return std::numeric_limits<_From>::digits <= __fp_format<_To>.digits;
  } else if constexpr (is_integral_v<_To>) {
    return false;
  } else {
    return __fp_values_subset<_From, _To>;
  }
}

// Integer conversion rank ([conv.rank]/1): bool 0, then the standard types by size of their
// rank class; char8_t/char16_t/char32_t/wchar_t have the rank of their underlying type.
template <class _Tp>
consteval int __simd_int_rank() {
  if constexpr (__is_same(_Tp, bool))
    return 0;
  else if constexpr (__is_same(_Tp, signed char) || __is_same(_Tp, unsigned char) || __is_same(_Tp, char))
    return 1;
  else if constexpr (__is_same(_Tp, short) || __is_same(_Tp, unsigned short))
    return 2;
  else if constexpr (__is_same(_Tp, int) || __is_same(_Tp, unsigned))
    return 3;
  else if constexpr (__is_same(_Tp, long) || __is_same(_Tp, unsigned long))
    return 4;
  else if constexpr (__is_same(_Tp, long long) || __is_same(_Tp, unsigned long long))
    return 5;
  else if constexpr (sizeof(_Tp) == 1)
    return 1;
  else if constexpr (sizeof(_Tp) == sizeof(short))
    return 2;
  else if constexpr (sizeof(_Tp) == sizeof(int))
    return 3;
  else if constexpr (sizeof(_Tp) == sizeof(long))
    return 4;
  else
    return 5;
}

// [simd.ctor]/6: the converting constructor is implicit iff this holds.
template <class _Up, class _Tp>
consteval bool __simd_implicit_conversion() {
  if constexpr (!__ycxx::__detail::__simd_value_preserving<_Up, _Tp>())
    return false;
  else if constexpr (is_integral_v<_Up> && is_integral_v<_Tp>)
    return __ycxx::__detail::__simd_int_rank<_Up>() <= __ycxx::__detail::__simd_int_rank<_Tp>();
  else if constexpr (__is_floating_v<_Up> && __is_floating_v<_Tp>)
    return !(__fp_values_subset<_Tp, _Up> && !__fp_values_subset<_Up, _Tp>); // rank of U not greater
  else
    return true;
}

// a < b for integers of any types (cmp_less, also for the character types; no bool).
template <class _Ap, class _Bp>
constexpr bool __simd_int_less(_Ap a, _Bp b) noexcept {
  if constexpr (std::numeric_limits<_Ap>::is_signed == std::numeric_limits<_Bp>::is_signed)
    return a < b;
  else if constexpr (std::numeric_limits<_Ap>::is_signed)
    return a < 0 || static_cast<std::make_unsigned_t<_Ap>>(a) < b;
  else
    return b >= 0 && a < static_cast<std::make_unsigned_t<_Bp>>(b);
}

// [simd.ctor]/2.3: "From::value is representable by value_type".
template <auto _Vp, class _Tp>
consteval bool __simd_representable() {
  using _Fp = std::remove_cvref_t<decltype(_Vp)>;
  if constexpr (__simd_is_complex<_Tp>) {
    return __ycxx::__detail::__simd_representable<_Vp, typename _Tp::value_type>();
  } else if constexpr (__is_same(_Fp, bool)) {
    return true;
  } else if constexpr (is_integral_v<_Fp>) {
    if constexpr (is_integral_v<_Tp>) {
      return !__ycxx::__detail::__simd_int_less(_Vp, std::numeric_limits<_Tp>::min()) &&
             !__ycxx::__detail::__simd_int_less(std::numeric_limits<_Tp>::max(), _Vp);
    } else {
      // Exactly representable: the significant bits fit the significand, the magnitude the
      // exponent range.
      using _Up = std::make_unsigned_t<_Fp>;
      _Up a = _Vp < 0 ? static_cast<_Up>(_Up(0) - static_cast<_Up>(_Vp)) : static_cast<_Up>(_Vp);
      if (a == 0)
        return true;
      int width = std::bit_width(a);
      return width - std::countr_zero(a) <= __fp_format<_Tp>.digits && width <= __fp_format<_Tp>.__max_exp;
    }
  } else if constexpr (__is_floating_v<_Fp>) {
    if (_Vp != _Vp)
      return __is_floating_v<_Tp>;
    if constexpr (is_integral_v<_Tp>) {
      // In range (the bounds are powers of two, exact in long double) and integral-valued.
      long double __lo = 0, __hi = 1;
      for (int i = 0; i < std::numeric_limits<_Tp>::digits; ++i)
        __hi *= 2;
      if constexpr (std::numeric_limits<_Tp>::is_signed)
        __lo = -__hi;
      long double __v = static_cast<long double>(_Vp);
      return __v >= __lo && __v < __hi && static_cast<long double>(static_cast<_Tp>(__v)) == __v;
    } else {
      return static_cast<_Fp>(static_cast<_Tp>(_Vp)) == _Vp;
    }
  } else {
    return false;
  }
}

template <class _From, class _To>
concept __simd_explicitly_convertible_to = requires { static_cast<_To>(std::declval<_From>()); };

// The vector chunk of the native layout (DECISIONS §1: the attribute, not a macro).
template <class _Ep, int _Lanes>
using __simd_vector [[__gnu__::__vector_size__(sizeof(_Ep) * _Lanes)]] = _Ep;

// Lanes per chunk of the storage of N elements of type E (1: plain elements).
template <class _Ep, int _Np>
consteval int __simd_chunk_lanes() {
  if constexpr (!__simd_arithmetic<_Ep> || _Np < 2 || (_Np & (_Np - 1)) != 0)
    return 1;
  else if constexpr (_Np * sizeof(_Ep) <= __cfg::__simd_register_bytes)
    return _Np;
  else
    return static_cast<int>(__cfg::__simd_register_bytes / sizeof(_Ep));
}

template <class _Ep, int _Lanes>
struct __simd_chunk_type {
  using type = __simd_vector<_Ep, _Lanes>;
};
template <class _Ep>
struct __simd_chunk_type<_Ep, 1> {
  using type = _Ep;
};

// N elements as a plain array: the common currency between the layouts.
template <class _Ep, int _Np>
struct __simd_array {
  _Ep __v[_Np];
};

// The element storage of a data-parallel object: an array of chunks, each a vector of `__lanes`
// elements or (lanes == 1) a single element. The object representation is that of E[N] in
// every layout, so the layouts convert to and from simd_array with bit_cast.
template <class _Ep, int _Np>
struct __simd_storage {
  static constexpr int __lanes = __ycxx::__detail::__simd_chunk_lanes<_Ep, _Np>();
  static constexpr int __chunks = _Np / __lanes;
  static constexpr bool __is_vector = __lanes > 1;
  using element_type = _Ep;
  using __chunk_type = typename __simd_chunk_type<_Ep, __lanes>::type;

  __chunk_type c[__chunks];

  constexpr _Ep get(int i) const noexcept {
    if constexpr (__is_vector)
      return c[i / __lanes][i % __lanes];
    else
      return c[i];
  }

  constexpr __simd_array<_Ep, _Np> to_array() const noexcept {
    if constexpr (__is_vector) {
      return __builtin_bit_cast(__simd_array<_Ep, _Np>, *this);
    } else {
      __simd_array<_Ep, _Np> a;
      for (int i = 0; i < _Np; ++i)
        a.__v[i] = c[i];
      return a;
    }
  }
  static constexpr __simd_storage __from_array(const __simd_array<_Ep, _Np>& a) noexcept {
    if constexpr (__is_vector) {
      return __builtin_bit_cast(__simd_storage, a);
    } else {
      __simd_storage s;
      for (int i = 0; i < _Np; ++i)
        s.c[i] = a.__v[i];
      return s;
    }
  }
  // f(i) for i = 0, 1, ..., N - 1, in that order.
  template <class _Fp>
  static constexpr __simd_storage generate(_Fp&& __f) {
    __simd_array<_Ep, _Np> a;
    for (int i = 0; i < _Np; ++i)
      a.__v[i] = __f(i);
    return __simd_storage::__from_array(a);
  }
  static constexpr __chunk_type __splat(_Ep __x) noexcept {
    if constexpr (__is_vector)
      return [&]<std::size_t... _Ip>(std::index_sequence<_Ip...>) {
        return __chunk_type{((void)_Ip, __x)...};
      }(std::make_index_sequence<__lanes>());
    else
      return __x;
  }
  static constexpr __simd_storage __broadcast(_Ep __x) noexcept {
    __simd_storage s;
    __chunk_type __v = __simd_storage::__splat(__x);
    for (int k = 0; k < __chunks; ++k)
      s.c[k] = __v;
    return s;
  }
  template <class _Fp>
  static constexpr __simd_storage map(const __simd_storage& a, _Fp __f) {
    __simd_storage r;
    for (int k = 0; k < __chunks; ++k)
      r.c[k] = static_cast<__chunk_type>(__f(a.c[k]));
    return r;
  }
  template <class _Fp>
  static constexpr __simd_storage __map2(const __simd_storage& a, const __simd_storage& b, _Fp __f) {
    __simd_storage r;
    for (int k = 0; k < __chunks; ++k)
      r.c[k] = static_cast<__chunk_type>(__f(a.c[k], b.c[k]));
    return r;
  }
};

// The mask element type for Bytes: integer-from<Bytes>, 64 bits for 16-byte (complex) elements.
template <std::size_t _Bytes>
using __simd_mask_element = __simd_integer_from<(_Bytes > 8 ? 8 : _Bytes)>;

// simd-size-v and mask-size-v ([simd.expos.defn]/3-4).
template <class _Tp, class _Abi>
inline constexpr __simd_size_t __simd_size_v = 0;
template <class _Tp, int _Np>
  requires(__simd_vectorizable<_Tp> && _Np >= 1 && _Np <= __simd_max_width)
inline constexpr __simd_size_t __simd_size_v<_Tp, __ycxx::__adl_free::__simd_abi<_Np>> = _Np;
template <std::size_t _Bytes, class _Abi>
inline constexpr __simd_size_t __simd_mask_size_v = 0;
template <std::size_t _Bytes, int _Np>
  requires((_Bytes == 1 || _Bytes == 2 || _Bytes == 4 || _Bytes == 8 || _Bytes == 16) && _Np >= 1 && _Np <= __simd_max_width)
inline constexpr __simd_size_t __simd_mask_size_v<_Bytes, __ycxx::__adl_free::__simd_abi<_Np>> = _Np;

// deduce-abi-t and native-abi ([simd.expos.abi]).
template <class _Tp, __simd_size_t _Np>
using __simd_deduce_abi_t =
    std::conditional_t<__simd_vectorizable<_Tp> && _Np >= 1 && _Np <= __simd_max_width, __ycxx::__adl_free::__simd_abi<_Np>,
                  __ycxx::__adl_free::__simd_abi_none>;
template <class _Tp>
consteval __simd_size_t __simd_native_width() {
  if constexpr (!__simd_vectorizable<_Tp>)
    return 1;
  else if constexpr (sizeof(_Tp) >= __cfg::__simd_register_bytes)
    return 1;
  else
    return static_cast<__simd_size_t>(__cfg::__simd_register_bytes / sizeof(_Tp));
}
template <class _Tp>
using __simd_native_abi = __ycxx::__adl_free::__simd_abi<__ycxx::__detail::__simd_native_width<_Tp>()>;

// mask-element-size
template <class _Tp>
inline constexpr std::size_t __simd_mask_element_size = 0;
template <std::size_t _Bytes, class _Abi>
inline constexpr std::size_t __simd_mask_element_size<std::simd::basic_mask<_Bytes, _Abi>> = _Bytes;

// Flags.

template <class _Fp>
inline constexpr std::size_t __simd_flag_alignment = 0;
template <std::size_t _Np>
inline constexpr std::size_t __simd_flag_alignment<__ycxx::__adl_free::__simd_overaligned_flag<_Np>> = _Np;

template <class... _Flags>
inline constexpr bool __simd_has_convert = (__is_same(_Flags, __ycxx::__adl_free::__simd_convert_flag) || ...);
template <class... _Flags>
inline constexpr bool __simd_has_aligned = (__is_same(_Flags, __ycxx::__adl_free::__simd_aligned_flag) || ...);
// The largest overaligned-flag<N> in Flags, 0 if none.
template <class... _Flags>
consteval std::size_t __simd_overalignment() {
  std::size_t r = 0;
  ((r = __simd_flag_alignment<_Flags> > r ? __simd_flag_alignment<_Flags> : r), ...);
  return r;
}

// flags<Fs...> | flags<Os...>: Fs, then the Os not yet present.
template <class _Fp, class... _Op>
struct __simd_flags_union {
  using type = _Fp;
};
template <class... _Fs, class _Op, class... _Os>
struct __simd_flags_union<std::simd::flags<_Fs...>, _Op, _Os...>
    : __simd_flags_union<
          std::conditional_t<(__is_same(_Op, _Fs) || ...), std::simd::flags<_Fs...>, std::simd::flags<_Fs..., _Op>>, _Os...> {};

// The extent of a contiguous range when ranges::size(r) is a constant expression, else
// dynamic_extent: what span deduction finds (arrays, std::array, spans of static extent), or a
// static constexpr size() member.
template <class _Rp>
consteval std::size_t __simd_find_static_size() {
  if constexpr (requires { decltype(std::span(std::declval<_Rp&>()))::extent; }) {
    if constexpr (decltype(std::span(std::declval<_Rp&>()))::extent != std::dynamic_extent)
      return decltype(std::span(std::declval<_Rp&>()))::extent;
  }
  if constexpr (requires { typename std::integral_constant<std::size_t, std::remove_cvref_t<_Rp>::size()>; })
    return std::remove_cvref_t<_Rp>::size();
  return std::dynamic_extent;
}
template <class _Rp>
inline constexpr std::size_t __simd_static_size = __ycxx::__detail::__simd_find_static_size<_Rp>();

// Gives the non-member functions access to the representation of basic_vec and basic_mask.
struct __simd_access {
  template <class _Vp>
  static constexpr auto& data(_Vp& __v) noexcept {
    return __v.__data_;
  }
  template <class _Vp, class _Sp>
  static constexpr _Vp __make(const _Sp& s) noexcept {
    return _Vp(__simd_access{}, s);
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace simd {

// [simd.flags.overview]
template <class... _Flags>
  requires(__ycxx::__detail::__simd_is_flag<_Flags> && ...)
struct flags {
  // [simd.flags.oper]
  template <class... _Other>
  friend consteval auto operator|(flags, flags<_Other...>) {
    return typename __ycxx::__detail::__simd_flags_union<flags, _Other...>::type{};
  }
};
inline constexpr flags<> flag_default{};
inline constexpr flags<__ycxx::__adl_free::__simd_convert_flag> flag_convert{};
inline constexpr flags<__ycxx::__adl_free::__simd_aligned_flag> flag_aligned{};
template <size_t _Np>
  requires(std::has_single_bit(_Np))
inline constexpr flags<__ycxx::__adl_free::__simd_overaligned_flag<_Np>> flag_overaligned{};

}}} // namespace std::simd

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {

// [simd.iterator]: simd-iterator<V>, a random-access iterator over the elements of a basic_vec or
// basic_mask, yielding prvalues.
template <class _Vp>
class __simd_iterator {
  _Vp* __data_ = nullptr;
  __ycxx::__detail::__simd_size_t __offset_ = 0;

  friend std::remove_const_t<_Vp>;
  template <class>
  friend class __simd_iterator;
  constexpr __simd_iterator(_Vp& d, __ycxx::__detail::__simd_size_t __off) noexcept : __data_(__builtin_addressof(d)), __offset_(__off) {}

public:
  using value_type = typename _Vp::value_type;
  using iterator_category = std::input_iterator_tag;
  using iterator_concept = std::random_access_iterator_tag;
  using difference_type = __ycxx::__detail::__simd_size_t;

  constexpr __simd_iterator() = default;
  constexpr __simd_iterator(const __simd_iterator&) = default;
  constexpr __simd_iterator& operator=(const __simd_iterator&) = default;
  constexpr __simd_iterator(const __simd_iterator<std::remove_const_t<_Vp>>& i)
    requires std::is_const_v<_Vp>
      : __data_(i.__data_), __offset_(i.__offset_) {}

  constexpr value_type operator*() const { return (*__data_)[__offset_]; }
  constexpr __simd_iterator& operator++() { return *this += 1; }
  constexpr __simd_iterator operator++(int) {
    __simd_iterator __tmp = *this;
    *this += 1;
    return __tmp;
  }
  constexpr __simd_iterator& operator--() { return *this -= 1; }
  constexpr __simd_iterator operator--(int) {
    __simd_iterator __tmp = *this;
    *this -= 1;
    return __tmp;
  }
  constexpr __simd_iterator& operator+=(difference_type n) {
    __ycxx::__detail::__precondition(__offset_ + n >= 0 && __offset_ + n <= _Vp::size(), "simd-iterator: out of range");
    __offset_ += n;
    return *this;
  }
  constexpr __simd_iterator& operator-=(difference_type n) {
    __ycxx::__detail::__precondition(__offset_ - n >= 0 && __offset_ - n <= _Vp::size(), "simd-iterator: out of range");
    __offset_ -= n;
    return *this;
  }
  constexpr value_type operator[](difference_type n) const { return (*__data_)[__offset_ + n]; }

  friend constexpr bool operator==(__simd_iterator a, __simd_iterator b) = default;
  friend constexpr bool operator==(__simd_iterator a, std::default_sentinel_t) noexcept { return a.__offset_ == _Vp::size(); }
  friend constexpr auto operator<=>(__simd_iterator a, __simd_iterator b) {
    __ycxx::__detail::__precondition(a.__data_ == b.__data_, "simd-iterator: iterators into different objects");
    return a.__offset_ <=> b.__offset_;
  }
  friend constexpr __simd_iterator operator+(__simd_iterator i, difference_type n) { return i += n; }
  friend constexpr __simd_iterator operator+(difference_type n, __simd_iterator i) { return i += n; }
  friend constexpr __simd_iterator operator-(__simd_iterator i, difference_type n) { return i -= n; }
  friend constexpr difference_type operator-(__simd_iterator a, __simd_iterator b) {
    __ycxx::__detail::__precondition(a.__data_ == b.__data_, "simd-iterator: iterators into different objects");
    return a.__offset_ - b.__offset_;
  }
  friend constexpr difference_type operator-(__simd_iterator i, std::default_sentinel_t) noexcept {
    return i.__offset_ - _Vp::size();
  }
  friend constexpr difference_type operator-(std::default_sentinel_t, __simd_iterator i) noexcept {
    return _Vp::size() - i.__offset_;
  }
};

}} // namespace __ycxx::__adl_free
