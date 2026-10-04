// libycxx core: <simd> part 1 ([simd.expos], [simd.traits], [simd.flags], [simd.iterator]): the
// vectorizable types, the ABI tags, element storage, the type traits, the load/store flags and
// simd-iterator. The classes are in simd.hpp.
//
// ABI tags. deduce-abi-t<T, N> is ycxx::adl_free::simd_abi<N, R> for every vectorizable T and
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

namespace ycxx::adl_free {
// The ABI tags (see above). R, the register width the layout is built for, is part of the type,
// so translation units built for different widths do not share a type with two layouts; only the
// tags of this translation unit's width are enabled.
template <int N, int R = ycxx::detail::cfg::simd_register_bytes>
struct simd_abi {};
// deduce-abi-t<T, N> when T is not vectorizable or N is out of range: no basic_vec is enabled.
struct simd_abi_none {};
// convert-flag, aligned-flag, overaligned-flag<N> ([simd.flags]).
struct simd_convert_flag {};
struct simd_aligned_flag {};
template <std::size_t N>
struct simd_overaligned_flag {};
} // namespace ycxx::adl_free

namespace ycxx::detail {
template <class F>
inline constexpr bool simd_is_flag = false;
template <>
inline constexpr bool simd_is_flag<ycxx::adl_free::simd_convert_flag> = true;
template <>
inline constexpr bool simd_is_flag<ycxx::adl_free::simd_aligned_flag> = true;
template <std::size_t N>
inline constexpr bool simd_is_flag<ycxx::adl_free::simd_overaligned_flag<N>> = true;
} // namespace ycxx::detail

namespace std::simd {
template <class... Flags>
  requires(ycxx::detail::simd_is_flag<Flags> && ...)
struct flags;
template <size_t Bytes, class Abi>
class basic_mask;
} // namespace std::simd

namespace ycxx::detail {

// simd-size-type
using simd_size_t = int;
// The largest width deduce-abi-t accepts ([simd.expos.abi]/4).
inline constexpr simd_size_t simd_max_width = 64;

// integer-from<Bytes>
template <std::size_t Bytes>
struct simd_integer_from_impl {};
template <>
struct simd_integer_from_impl<1> {
  using type = signed char;
};
template <>
struct simd_integer_from_impl<2> {
  using type = short;
};
template <>
struct simd_integer_from_impl<4> {
  using type = int;
};
template <>
struct simd_integer_from_impl<8> {
  using type = long long;
};
template <std::size_t Bytes>
using simd_integer_from = typename simd_integer_from_impl<Bytes>::type;

// The vectorizable arithmetic types ([simd.general]/2.1-2.2): these get vector storage.
template <class T>
inline constexpr bool simd_arithmetic =
    __is_same(T, signed char) || __is_same(T, short) || __is_same(T, int) || __is_same(T, long) ||
    __is_same(T, long long) || __is_same(T, unsigned char) || __is_same(T, unsigned short) ||
    __is_same(T, unsigned int) || __is_same(T, unsigned long) || __is_same(T, unsigned long long) ||
    __is_same(T, char) || __is_same(T, wchar_t) || __is_same(T, char8_t) || __is_same(T, char16_t) ||
    __is_same(T, char32_t) || __is_same(T, float) || __is_same(T, double) || __is_same(T, float16) ||
    __is_same(T, float32) || __is_same(T, float64);
template <class T>
inline constexpr bool simd_is_complex = false;
template <class T>
inline constexpr bool simd_is_complex<std::complex<T>> = true;
// [simd.general]/2.3: complex<T> for a vectorizable floating-point T.
template <class T>
inline constexpr bool simd_vectorizable = simd_arithmetic<T>;
template <class T>
inline constexpr bool simd_vectorizable<std::complex<T>> = simd_arithmetic<T> && is_floating_v<T>;

// [simd.general]/8: every value of From is a value of To. A complex type counts as its value type
// (a real value is a complex value with a zero imaginary part); complex to real never preserves.
template <class From, class To>
consteval bool simd_value_preserving() {
  if constexpr (simd_is_complex<To>) {
    if constexpr (simd_is_complex<From>)
      return ycxx::detail::simd_value_preserving<typename From::value_type, typename To::value_type>();
    else
      return ycxx::detail::simd_value_preserving<From, typename To::value_type>();
  } else if constexpr (simd_is_complex<From> || !is_arithmetic_v<From> || !is_arithmetic_v<To>) {
    return false;
  } else if constexpr (is_integral_v<From> && is_integral_v<To>) {
    using LF = std::numeric_limits<From>;
    using LT = std::numeric_limits<To>;
    return (!LF::is_signed || LT::is_signed) && LF::digits <= LT::digits;
  } else if constexpr (is_integral_v<From>) {
    return std::numeric_limits<From>::digits <= fp_format<To>.digits;
  } else if constexpr (is_integral_v<To>) {
    return false;
  } else {
    return fp_values_subset<From, To>;
  }
}

// Integer conversion rank ([conv.rank]/1): bool 0, then the standard types by size of their
// rank class; char8_t/char16_t/char32_t/wchar_t have the rank of their underlying type.
template <class T>
consteval int simd_int_rank() {
  if constexpr (__is_same(T, bool))
    return 0;
  else if constexpr (__is_same(T, signed char) || __is_same(T, unsigned char) || __is_same(T, char))
    return 1;
  else if constexpr (__is_same(T, short) || __is_same(T, unsigned short))
    return 2;
  else if constexpr (__is_same(T, int) || __is_same(T, unsigned))
    return 3;
  else if constexpr (__is_same(T, long) || __is_same(T, unsigned long))
    return 4;
  else if constexpr (__is_same(T, long long) || __is_same(T, unsigned long long))
    return 5;
  else if constexpr (sizeof(T) == 1)
    return 1;
  else if constexpr (sizeof(T) == sizeof(short))
    return 2;
  else if constexpr (sizeof(T) == sizeof(int))
    return 3;
  else if constexpr (sizeof(T) == sizeof(long))
    return 4;
  else
    return 5;
}

// [simd.ctor]/6: the converting constructor is implicit iff this holds.
template <class U, class T>
consteval bool simd_implicit_conversion() {
  if constexpr (!ycxx::detail::simd_value_preserving<U, T>())
    return false;
  else if constexpr (is_integral_v<U> && is_integral_v<T>)
    return ycxx::detail::simd_int_rank<U>() <= ycxx::detail::simd_int_rank<T>();
  else if constexpr (is_floating_v<U> && is_floating_v<T>)
    return !(fp_values_subset<T, U> && !fp_values_subset<U, T>); // rank of U not greater
  else
    return true;
}

// a < b for integers of any types (cmp_less, also for the character types; no bool).
template <class A, class B>
constexpr bool simd_int_less(A a, B b) noexcept {
  if constexpr (std::numeric_limits<A>::is_signed == std::numeric_limits<B>::is_signed)
    return a < b;
  else if constexpr (std::numeric_limits<A>::is_signed)
    return a < 0 || static_cast<std::make_unsigned_t<A>>(a) < b;
  else
    return b >= 0 && a < static_cast<std::make_unsigned_t<B>>(b);
}

// [simd.ctor]/2.3: "From::value is representable by value_type".
template <auto V, class T>
consteval bool simd_representable() {
  using F = std::remove_cvref_t<decltype(V)>;
  if constexpr (simd_is_complex<T>) {
    return ycxx::detail::simd_representable<V, typename T::value_type>();
  } else if constexpr (__is_same(F, bool)) {
    return true;
  } else if constexpr (is_integral_v<F>) {
    if constexpr (is_integral_v<T>) {
      return !ycxx::detail::simd_int_less(V, std::numeric_limits<T>::min()) &&
             !ycxx::detail::simd_int_less(std::numeric_limits<T>::max(), V);
    } else {
      // Exactly representable: the significant bits fit the significand, the magnitude the
      // exponent range.
      using U = std::make_unsigned_t<F>;
      U a = V < 0 ? static_cast<U>(U(0) - static_cast<U>(V)) : static_cast<U>(V);
      if (a == 0)
        return true;
      int width = std::bit_width(a);
      return width - std::countr_zero(a) <= fp_format<T>.digits && width <= fp_format<T>.max_exp;
    }
  } else if constexpr (is_floating_v<F>) {
    if (V != V)
      return is_floating_v<T>;
    if constexpr (is_integral_v<T>) {
      // In range (the bounds are powers of two, exact in long double) and integral-valued.
      long double lo = 0, hi = 1;
      for (int i = 0; i < std::numeric_limits<T>::digits; ++i)
        hi *= 2;
      if constexpr (std::numeric_limits<T>::is_signed)
        lo = -hi;
      long double v = static_cast<long double>(V);
      return v >= lo && v < hi && static_cast<long double>(static_cast<T>(v)) == v;
    } else {
      return static_cast<F>(static_cast<T>(V)) == V;
    }
  } else {
    return false;
  }
}

template <class From, class To>
concept simd_explicitly_convertible_to = requires { static_cast<To>(std::declval<From>()); };

// The vector chunk of the native layout (DECISIONS §1: the attribute, not a macro).
template <class E, int Lanes>
using simd_vector [[gnu::vector_size(sizeof(E) * Lanes)]] = E;

// Lanes per chunk of the storage of N elements of type E (1: plain elements).
template <class E, int N>
consteval int simd_chunk_lanes() {
  if constexpr (!simd_arithmetic<E> || N < 2 || (N & (N - 1)) != 0)
    return 1;
  else if constexpr (N * sizeof(E) <= cfg::simd_register_bytes)
    return N;
  else
    return static_cast<int>(cfg::simd_register_bytes / sizeof(E));
}

template <class E, int Lanes>
struct simd_chunk_type {
  using type = simd_vector<E, Lanes>;
};
template <class E>
struct simd_chunk_type<E, 1> {
  using type = E;
};

// N elements as a plain array: the common currency between the layouts.
template <class E, int N>
struct simd_array {
  E v[N];
};

// The element storage of a data-parallel object: an array of chunks, each a vector of `lanes`
// elements or (lanes == 1) a single element. The object representation is that of E[N] in
// every layout, so the layouts convert to and from simd_array with bit_cast.
template <class E, int N>
struct simd_storage {
  static constexpr int lanes = ycxx::detail::simd_chunk_lanes<E, N>();
  static constexpr int chunks = N / lanes;
  static constexpr bool is_vector = lanes > 1;
  using element_type = E;
  using chunk_type = typename simd_chunk_type<E, lanes>::type;

  chunk_type c[chunks];

  constexpr E get(int i) const noexcept {
    if constexpr (is_vector)
      return c[i / lanes][i % lanes];
    else
      return c[i];
  }

  constexpr simd_array<E, N> to_array() const noexcept {
    if constexpr (is_vector) {
      return __builtin_bit_cast(simd_array<E, N>, *this);
    } else {
      simd_array<E, N> a;
      for (int i = 0; i < N; ++i)
        a.v[i] = c[i];
      return a;
    }
  }
  static constexpr simd_storage from_array(const simd_array<E, N>& a) noexcept {
    if constexpr (is_vector) {
      return __builtin_bit_cast(simd_storage, a);
    } else {
      simd_storage s;
      for (int i = 0; i < N; ++i)
        s.c[i] = a.v[i];
      return s;
    }
  }
  // f(i) for i = 0, 1, ..., N - 1, in that order.
  template <class F>
  static constexpr simd_storage generate(F&& f) {
    simd_array<E, N> a;
    for (int i = 0; i < N; ++i)
      a.v[i] = f(i);
    return simd_storage::from_array(a);
  }
  static constexpr chunk_type splat(E x) noexcept {
    if constexpr (is_vector)
      return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return chunk_type{((void)I, x)...};
      }(std::make_index_sequence<lanes>());
    else
      return x;
  }
  static constexpr simd_storage broadcast(E x) noexcept {
    simd_storage s;
    chunk_type v = simd_storage::splat(x);
    for (int k = 0; k < chunks; ++k)
      s.c[k] = v;
    return s;
  }
  template <class F>
  static constexpr simd_storage map(const simd_storage& a, F f) {
    simd_storage r;
    for (int k = 0; k < chunks; ++k)
      r.c[k] = static_cast<chunk_type>(f(a.c[k]));
    return r;
  }
  template <class F>
  static constexpr simd_storage map2(const simd_storage& a, const simd_storage& b, F f) {
    simd_storage r;
    for (int k = 0; k < chunks; ++k)
      r.c[k] = static_cast<chunk_type>(f(a.c[k], b.c[k]));
    return r;
  }
};

// The mask element type for Bytes: integer-from<Bytes>, 64 bits for 16-byte (complex) elements.
template <std::size_t Bytes>
using simd_mask_element = simd_integer_from<(Bytes > 8 ? 8 : Bytes)>;

// simd-size-v and mask-size-v ([simd.expos.defn]/3-4).
template <class T, class Abi>
inline constexpr simd_size_t simd_size_v = 0;
template <class T, int N>
  requires(simd_vectorizable<T> && N >= 1 && N <= simd_max_width)
inline constexpr simd_size_t simd_size_v<T, ycxx::adl_free::simd_abi<N>> = N;
template <std::size_t Bytes, class Abi>
inline constexpr simd_size_t simd_mask_size_v = 0;
template <std::size_t Bytes, int N>
  requires((Bytes == 1 || Bytes == 2 || Bytes == 4 || Bytes == 8 || Bytes == 16) && N >= 1 && N <= simd_max_width)
inline constexpr simd_size_t simd_mask_size_v<Bytes, ycxx::adl_free::simd_abi<N>> = N;

// deduce-abi-t and native-abi ([simd.expos.abi]).
template <class T, simd_size_t N>
using simd_deduce_abi_t =
    std::conditional_t<simd_vectorizable<T> && N >= 1 && N <= simd_max_width, ycxx::adl_free::simd_abi<N>,
                  ycxx::adl_free::simd_abi_none>;
template <class T>
consteval simd_size_t simd_native_width() {
  if constexpr (!simd_vectorizable<T>)
    return 1;
  else if constexpr (sizeof(T) >= cfg::simd_register_bytes)
    return 1;
  else
    return static_cast<simd_size_t>(cfg::simd_register_bytes / sizeof(T));
}
template <class T>
using simd_native_abi = ycxx::adl_free::simd_abi<ycxx::detail::simd_native_width<T>()>;

// mask-element-size
template <class T>
inline constexpr std::size_t simd_mask_element_size = 0;
template <std::size_t Bytes, class Abi>
inline constexpr std::size_t simd_mask_element_size<std::simd::basic_mask<Bytes, Abi>> = Bytes;

// Flags.

template <class F>
inline constexpr std::size_t simd_flag_alignment = 0;
template <std::size_t N>
inline constexpr std::size_t simd_flag_alignment<ycxx::adl_free::simd_overaligned_flag<N>> = N;

template <class... Flags>
inline constexpr bool simd_has_convert = (__is_same(Flags, ycxx::adl_free::simd_convert_flag) || ...);
template <class... Flags>
inline constexpr bool simd_has_aligned = (__is_same(Flags, ycxx::adl_free::simd_aligned_flag) || ...);
// The largest overaligned-flag<N> in Flags, 0 if none.
template <class... Flags>
consteval std::size_t simd_overalignment() {
  std::size_t r = 0;
  ((r = simd_flag_alignment<Flags> > r ? simd_flag_alignment<Flags> : r), ...);
  return r;
}

// flags<Fs...> | flags<Os...>: Fs, then the Os not yet present.
template <class F, class... O>
struct simd_flags_union {
  using type = F;
};
template <class... Fs, class O, class... Os>
struct simd_flags_union<std::simd::flags<Fs...>, O, Os...>
    : simd_flags_union<std::conditional_t<(__is_same(O, Fs) || ...), std::simd::flags<Fs...>, std::simd::flags<Fs..., O>>,
                       Os...> {};

// The extent of a contiguous range when ranges::size(r) is a constant expression, else
// dynamic_extent: what span deduction finds (arrays, std::array, spans of static extent), or a
// static constexpr size() member.
template <class R>
consteval std::size_t simd_find_static_size() {
  if constexpr (requires { decltype(std::span(std::declval<R&>()))::extent; }) {
    if constexpr (decltype(std::span(std::declval<R&>()))::extent != std::dynamic_extent)
      return decltype(std::span(std::declval<R&>()))::extent;
  }
  if constexpr (requires { typename std::integral_constant<std::size_t, std::remove_cvref_t<R>::size()>; })
    return std::remove_cvref_t<R>::size();
  return std::dynamic_extent;
}
template <class R>
inline constexpr std::size_t simd_static_size = ycxx::detail::simd_find_static_size<R>();

// Gives the non-member functions access to the representation of basic_vec and basic_mask.
struct simd_access {
  template <class V>
  static constexpr auto& data(V& v) noexcept {
    return v.data_;
  }
  template <class V, class S>
  static constexpr V make(const S& s) noexcept {
    return V(simd_access{}, s);
  }
};

} // namespace ycxx::detail

namespace std::simd {

// [simd.flags.overview]
template <class... Flags>
  requires(ycxx::detail::simd_is_flag<Flags> && ...)
struct flags {
  // [simd.flags.oper]
  template <class... Other>
  friend consteval auto operator|(flags, flags<Other...>) {
    return typename ycxx::detail::simd_flags_union<flags, Other...>::type{};
  }
};
inline constexpr flags<> flag_default{};
inline constexpr flags<ycxx::adl_free::simd_convert_flag> flag_convert{};
inline constexpr flags<ycxx::adl_free::simd_aligned_flag> flag_aligned{};
template <size_t N>
  requires(std::has_single_bit(N))
inline constexpr flags<ycxx::adl_free::simd_overaligned_flag<N>> flag_overaligned{};

} // namespace std::simd

namespace ycxx::adl_free {

// [simd.iterator]: simd-iterator<V>, a random-access iterator over the elements of a basic_vec or
// basic_mask, yielding prvalues.
template <class V>
class simd_iterator {
  V* data_ = nullptr;
  ycxx::detail::simd_size_t offset_ = 0;

  friend std::remove_const_t<V>;
  template <class>
  friend class simd_iterator;
  constexpr simd_iterator(V& d, ycxx::detail::simd_size_t off) noexcept : data_(__builtin_addressof(d)), offset_(off) {}

public:
  using value_type = typename V::value_type;
  using iterator_category = std::input_iterator_tag;
  using iterator_concept = std::random_access_iterator_tag;
  using difference_type = ycxx::detail::simd_size_t;

  constexpr simd_iterator() = default;
  constexpr simd_iterator(const simd_iterator&) = default;
  constexpr simd_iterator& operator=(const simd_iterator&) = default;
  constexpr simd_iterator(const simd_iterator<std::remove_const_t<V>>& i)
    requires std::is_const_v<V>
      : data_(i.data_), offset_(i.offset_) {}

  constexpr value_type operator*() const { return (*data_)[offset_]; }
  constexpr simd_iterator& operator++() { return *this += 1; }
  constexpr simd_iterator operator++(int) {
    simd_iterator tmp = *this;
    *this += 1;
    return tmp;
  }
  constexpr simd_iterator& operator--() { return *this -= 1; }
  constexpr simd_iterator operator--(int) {
    simd_iterator tmp = *this;
    *this -= 1;
    return tmp;
  }
  constexpr simd_iterator& operator+=(difference_type n) {
    ycxx::detail::precondition(offset_ + n >= 0 && offset_ + n <= V::size(), "simd-iterator: out of range");
    offset_ += n;
    return *this;
  }
  constexpr simd_iterator& operator-=(difference_type n) {
    ycxx::detail::precondition(offset_ - n >= 0 && offset_ - n <= V::size(), "simd-iterator: out of range");
    offset_ -= n;
    return *this;
  }
  constexpr value_type operator[](difference_type n) const { return (*data_)[offset_ + n]; }

  friend constexpr bool operator==(simd_iterator a, simd_iterator b) = default;
  friend constexpr bool operator==(simd_iterator a, std::default_sentinel_t) noexcept { return a.offset_ == V::size(); }
  friend constexpr auto operator<=>(simd_iterator a, simd_iterator b) {
    ycxx::detail::precondition(a.data_ == b.data_, "simd-iterator: iterators into different objects");
    return a.offset_ <=> b.offset_;
  }
  friend constexpr simd_iterator operator+(simd_iterator i, difference_type n) { return i += n; }
  friend constexpr simd_iterator operator+(difference_type n, simd_iterator i) { return i += n; }
  friend constexpr simd_iterator operator-(simd_iterator i, difference_type n) { return i -= n; }
  friend constexpr difference_type operator-(simd_iterator a, simd_iterator b) {
    ycxx::detail::precondition(a.data_ == b.data_, "simd-iterator: iterators into different objects");
    return a.offset_ - b.offset_;
  }
  friend constexpr difference_type operator-(simd_iterator i, std::default_sentinel_t) noexcept {
    return i.offset_ - V::size();
  }
  friend constexpr difference_type operator-(std::default_sentinel_t, simd_iterator i) noexcept {
    return V::size() - i.offset_;
  }
};

} // namespace ycxx::adl_free
