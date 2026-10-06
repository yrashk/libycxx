// libycxx core: <charconv> ([charconv]).
//
// Integer conversions are constexpr and live here. Floating-point conversions are out of line
// (src/runtime/charconv, in both libycxx.a and the freestanding runtime archive): the header
// turns a value into its raw bits plus a format tag (fp_kind) and calls one type-erased entry
// point per direction, so the extended floating-point types need no #if and no per-type symbols.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/errc.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/bit.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

// [charconv.syn]: a bitmask type ([bitmask.types]).
enum class chars_format { scientific = 1, fixed = 2, hex = 4, general = fixed | scientific };

constexpr chars_format operator&(chars_format __x, chars_format y) noexcept {
  return static_cast<chars_format>(static_cast<int>(__x) & static_cast<int>(y));
}
constexpr chars_format operator|(chars_format __x, chars_format y) noexcept {
  return static_cast<chars_format>(static_cast<int>(__x) | static_cast<int>(y));
}
constexpr chars_format operator^(chars_format __x, chars_format y) noexcept {
  return static_cast<chars_format>(static_cast<int>(__x) ^ static_cast<int>(y));
}
constexpr chars_format operator~(chars_format __x) noexcept {
  return static_cast<chars_format>(~static_cast<int>(__x));
}
constexpr chars_format& operator&=(chars_format& __x, chars_format y) noexcept { return __x = __x & y; }
constexpr chars_format& operator|=(chars_format& __x, chars_format y) noexcept { return __x = __x | y; }
constexpr chars_format& operator^=(chars_format& __x, chars_format y) noexcept { return __x = __x ^ y; }

struct to_chars_result {
  char* ptr;
  errc ec;
  friend bool operator==(const to_chars_result&, const to_chars_result&) = default;
  constexpr explicit operator bool() const noexcept { return ec == errc{}; }
};

struct from_chars_result {
  const char* ptr;
  errc ec;
  friend bool operator==(const from_chars_result&, const from_chars_result&) = default;
  constexpr explicit operator bool() const noexcept { return ec == errc{}; }
};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// ---- integers ([charconv.to.chars]/4-6, [charconv.from.chars]/2-4) ---------------------------

struct __charconv_digit_pairs {
  char c[200];
};
consteval __charconv_digit_pairs __make_charconv_digit_pairs() {
  __charconv_digit_pairs t{};
  for (int i = 0; i < 100; ++i) {
    t.c[2 * i] = static_cast<char>('0' + i / 10);
    t.c[2 * i + 1] = static_cast<char>('0' + i % 10);
  }
  return t;
}
inline constexpr __charconv_digit_pairs __charconv_pairs = __ycxx::__detail::__make_charconv_digit_pairs();
inline constexpr char __charconv_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

// Writes the digits of `value` right-aligned ending at `end`; returns the first digit.
template <class _Up>
constexpr char* __charconv_write_unsigned(char* end, _Up value, unsigned base) noexcept {
  char* p = end;
  if (base == 10) {
    if constexpr (sizeof(_Up) > sizeof(unsigned long long)) {
      // Peel 19-digit chunks so that the loop below runs on 64-bit words.
      while (value > static_cast<_Up>(~0ull)) {
        constexpr unsigned long long chunk = 10'000'000'000'000'000'000ull;
        unsigned long long __low = static_cast<unsigned long long>(value % chunk);
        value /= chunk;
        for (int i = 0; i < 19; ++i, __low /= 10)
          *--p = static_cast<char>('0' + __low % 10);
      }
      return __ycxx::__detail::__charconv_write_unsigned(p, static_cast<unsigned long long>(value), 10);
    } else {
      while (value >= 100) {
        unsigned r = static_cast<unsigned>(value % 100);
        value = static_cast<_Up>(value / 100);
        p -= 2;
        p[0] = __ycxx::__detail::__charconv_pairs.c[2 * r];
        p[1] = __ycxx::__detail::__charconv_pairs.c[2 * r + 1];
      }
      if (value >= 10) {
        p -= 2;
        p[0] = __ycxx::__detail::__charconv_pairs.c[2 * value];
        p[1] = __ycxx::__detail::__charconv_pairs.c[2 * value + 1];
      } else {
        *--p = static_cast<char>('0' + value);
      }
      return p;
    }
  }
  if ((base & (base - 1)) == 0) {
    int shift = __builtin_ctz(base);
    do {
      *--p = __ycxx::__detail::__charconv_digits[static_cast<unsigned>(value & (base - 1))];
      value = static_cast<_Up>(value >> shift);
    } while (value != 0);
    return p;
  }
  do {
    *--p = __ycxx::__detail::__charconv_digits[static_cast<unsigned>(value % base)];
    value = static_cast<_Up>(value / base);
  } while (value != 0);
  return p;
}

template <class _Tp>
constexpr std::to_chars_result __to_chars_integer(char* first, char* last, _Tp value, int base) noexcept {
  __ycxx::__detail::__precondition(2 <= base && base <= 36, "std::to_chars: base must be in [2, 36]");
  using _Up = std::make_unsigned_t<_Tp>;
  _Up __magnitude = static_cast<_Up>(value);
  bool __negative = false;
  if constexpr (is_signed_v<_Tp>) {
    if (value < 0) {
      __negative = true;
      __magnitude = static_cast<_Up>(_Up(0) - __magnitude);
    }
  }
  [[indeterminate]] char __buf[sizeof(_Up) * __CHAR_BIT__ + 1];
  char* end = __buf + sizeof __buf;
  char* p = __ycxx::__detail::__charconv_write_unsigned(end, __magnitude, static_cast<unsigned>(base));
  if (__negative)
    *--p = '-';
  if (last - first < end - p)
    return {last, std::errc::value_too_large};
  while (p != end)
    *first++ = *p++;
  return {first, std::errc{}};
}

constexpr unsigned __charconv_digit_value(char c) noexcept {
  if (c >= '0' && c <= '9')
    return static_cast<unsigned>(c - '0');
  if (c >= 'a' && c <= 'z')
    return static_cast<unsigned>(c - 'a' + 10);
  if (c >= 'A' && c <= 'Z')
    return static_cast<unsigned>(c - 'A' + 10);
  return 99;
}

template <class _Tp>
constexpr std::from_chars_result __from_chars_integer(const char* first, const char* last, _Tp& value, int base) noexcept {
  __ycxx::__detail::__precondition(2 <= base && base <= 36, "std::from_chars: base must be in [2, 36]");
  using _Up = std::make_unsigned_t<_Tp>;
  const char* p = first;
  bool __negative = false;
  if constexpr (is_signed_v<_Tp>) {
    if (p != last && *p == '-') {
      __negative = true;
      ++p;
    }
  }
  const char* digits = p;
  _Up __acc = 0;
  bool overflow = false;
  for (; p != last; ++p) {
    unsigned d = __ycxx::__detail::__charconv_digit_value(*p);
    if (d >= static_cast<unsigned>(base))
      break;
    if (!overflow)
      overflow = __builtin_mul_overflow(__acc, static_cast<unsigned>(base), &__acc) || __builtin_add_overflow(__acc, d, &__acc);
  }
  if (p == digits)
    return {first, std::errc::invalid_argument};
  if constexpr (is_signed_v<_Tp>) {
    _Up __limit = static_cast<_Up>(static_cast<_Up>(~_Up(0)) >> 1); // the largest T
    if (overflow || __acc > __limit + (__negative ? 1 : 0))
      return {p, std::errc::result_out_of_range};
    value = __negative ? static_cast<_Tp>(_Up(0) - __acc) : static_cast<_Tp>(__acc);
  } else {
    if (overflow)
      return {p, std::errc::result_out_of_range};
    value = __acc;
  }
  return {p, std::errc{}};
}

// ---- floating point --------------------------------------------------------------------------

// The binary interchange formats libycxx converts. Each floating-point type maps to one.
enum class __fp_kind : unsigned char { __binary16, __bfloat16, __binary32, __binary64, __x87_extended, __binary128 };

template <class _Tp>
consteval __fp_kind __fp_kind_of() {
  constexpr int digits = __fp_format<_Tp>.digits;
  static_assert(digits == 11 || digits == 8 || digits == 24 || digits == 53 || digits == 64 || digits == 113,
                "libycxx <charconv>: unsupported floating-point format");
  if constexpr (digits == 11)
    return __fp_kind::__binary16;
  else if constexpr (digits == 8)
    return __fp_kind::__bfloat16;
  else if constexpr (digits == 24)
    return __fp_kind::__binary32;
  else if constexpr (digits == 53)
    return __fp_kind::__binary64;
  else if constexpr (digits == 64)
    return __fp_kind::__x87_extended;
  else
    return __fp_kind::__binary128;
}

// The value's object representation as a little-endian 128-bit integer (lo, hi). Only the bytes
// of the format are read: x87 long double has 6 padding bytes, which stay out of the result.
struct __fp_raw {
  unsigned long long __lo = 0;
  unsigned long long __hi = 0;
};
template <class _Tp>
inline constexpr int __fp_value_bytes = __fp_kind_of<_Tp>() == __fp_kind::__x87_extended ? 10 : static_cast<int>(sizeof(_Tp));

template <class _Tp>
__fp_raw __fp_to_raw(_Tp value) noexcept {
  struct __bytes {
    unsigned char b[sizeof(_Tp)];
  };
  __bytes in = __builtin_bit_cast(__bytes, value);
  __fp_raw r;
  for (int i = 0; i < __fp_value_bytes<_Tp>; ++i) {
    int __pos = std::endian::native == std::endian::little ? i : __fp_value_bytes<_Tp> - 1 - i;
    unsigned long long byte = in.b[__pos];
    if (i < 8)
      r.__lo |= byte << (8 * i);
    else
      r.__hi |= byte << (8 * (i - 8));
  }
  return r;
}
template <class _Tp>
_Tp __fp_from_raw(__fp_raw r) noexcept {
  struct __bytes {
    unsigned char b[sizeof(_Tp)];
  };
  __bytes out{};
  for (int i = 0; i < __fp_value_bytes<_Tp>; ++i) {
    int __pos = std::endian::native == std::endian::little ? i : __fp_value_bytes<_Tp> - 1 - i;
    out.b[__pos] = static_cast<unsigned char>(i < 8 ? r.__lo >> (8 * i) : r.__hi >> (8 * (i - 8)));
  }
  return __builtin_bit_cast(_Tp, out);
}

// Out-of-line conversions (src/runtime/charconv). `__fmt` is a chars_format value, or 0 for the
// overload without one; `precision` is the requested precision, or -1 for the shortest form.
std::to_chars_result __fp_to_chars(char* first, char* last, __fp_kind kind, __fp_raw __bits, int __fmt, int precision) noexcept;
// Stores the result in `__bits` only when the conversion succeeds.
std::from_chars_result __fp_from_chars(const char* first, const char* last, __fp_kind kind, __fp_raw& __bits, int __fmt) noexcept;

constexpr bool __charconv_valid_format(std::chars_format __fmt) noexcept {
  return __fmt == std::chars_format::scientific || __fmt == std::chars_format::fixed || __fmt == std::chars_format::hex ||
         __fmt == std::chars_format::general;
}

template <class _Tp>
std::to_chars_result __to_chars_float(char* first, char* last, _Tp value) noexcept {
  return __ycxx::__detail::__fp_to_chars(first, last, __ycxx::__detail::__fp_kind_of<_Tp>(), __ycxx::__detail::__fp_to_raw(value), 0, -1);
}
template <class _Tp>
std::to_chars_result __to_chars_float(char* first, char* last, _Tp value, std::chars_format __fmt) noexcept {
  __ycxx::__detail::__precondition(__ycxx::__detail::__charconv_valid_format(__fmt), "std::to_chars: invalid chars_format");
  return __ycxx::__detail::__fp_to_chars(first, last, __ycxx::__detail::__fp_kind_of<_Tp>(), __ycxx::__detail::__fp_to_raw(value),
                                   static_cast<int>(__fmt), -1);
}
template <class _Tp>
std::to_chars_result __to_chars_float(char* first, char* last, _Tp value, std::chars_format __fmt, int precision) noexcept {
  __ycxx::__detail::__precondition(__ycxx::__detail::__charconv_valid_format(__fmt), "std::to_chars: invalid chars_format");
  // A negative precision is taken as if it were omitted (C 7.23.6.1): 6 for e, f and g; for a,
  // the exact (shortest) hexadecimal representation.
  if (precision < 0)
    precision = __fmt == std::chars_format::hex ? -1 : 6;
  return __ycxx::__detail::__fp_to_chars(first, last, __ycxx::__detail::__fp_kind_of<_Tp>(), __ycxx::__detail::__fp_to_raw(value),
                                   static_cast<int>(__fmt), precision);
}
template <class _Tp>
std::from_chars_result __from_chars_float(const char* first, const char* last, _Tp& value, std::chars_format __fmt) noexcept {
  __ycxx::__detail::__precondition(__ycxx::__detail::__charconv_valid_format(__fmt), "std::from_chars: invalid chars_format");
  __fp_raw __bits;
  std::from_chars_result r =
      __ycxx::__detail::__fp_from_chars(first, last, __ycxx::__detail::__fp_kind_of<_Tp>(), __bits, static_cast<int>(__fmt));
  if (r.ec == std::errc{})
    value = __ycxx::__detail::__fp_from_raw<_Tp>(__bits);
  return r;
}

// The shortest round-trip form, exactly as std::to_chars(first, last, value) writes it (plain
// overload, [charconv.to.chars]/7), under an internal name for std::to_string and <format>: any
// floating-point type, without overload resolution against the integer overloads.
template <class _Tp>
  requires __is_floating_v<_Tp>
std::to_chars_result __to_chars_shortest(char* first, char* last, _Tp value) noexcept {
  return __ycxx::__detail::__to_chars_float(first, last, value);
}

template <class _Tp>
concept __charconv_extended_float = __is_any_of<_Tp, __float16, __float32, __float64, __y_float128, __bfloat16>;
template <class _Tp>
concept __charconv_int128 = __is_any_of<_Tp, __y_int128, __uint128>;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [charconv.to.chars]: integers. One overload per type, as the synopsis specifies.
constexpr to_chars_result to_chars(char* first, char* last, char value, int base = 10) {
  return __ycxx::__detail::__to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, signed char value, int base = 10) {
  return __ycxx::__detail::__to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, unsigned char value, int base = 10) {
  return __ycxx::__detail::__to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, short value, int base = 10) {
  return __ycxx::__detail::__to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, unsigned short value, int base = 10) {
  return __ycxx::__detail::__to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, int value, int base = 10) {
  return __ycxx::__detail::__to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, unsigned int value, int base = 10) {
  return __ycxx::__detail::__to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, long value, int base = 10) {
  return __ycxx::__detail::__to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, unsigned long value, int base = 10) {
  return __ycxx::__detail::__to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, long long value, int base = 10) {
  return __ycxx::__detail::__to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, unsigned long long value, int base = 10) {
  return __ycxx::__detail::__to_chars_integer(first, last, value, base);
}
// __int128 (extension; an integer type where the compiler has it). A template, so that the
// declaration stays well-formed where the type does not exist.
template <__ycxx::__detail::__charconv_int128 _Tp>
constexpr to_chars_result to_chars(char* first, char* last, _Tp value, int base = 10) {
  return __ycxx::__detail::__to_chars_integer(first, last, value, base);
}
to_chars_result to_chars(char* first, char* last, bool value, int base = 10) = delete;

// [charconv.to.chars]: floating point.
inline to_chars_result to_chars(char* first, char* last, float value) noexcept {
  return __ycxx::__detail::__to_chars_float(first, last, value);
}
inline to_chars_result to_chars(char* first, char* last, double value) noexcept {
  return __ycxx::__detail::__to_chars_float(first, last, value);
}
inline to_chars_result to_chars(char* first, char* last, long double value) noexcept {
  return __ycxx::__detail::__to_chars_float(first, last, value);
}
template <__ycxx::__detail::__charconv_extended_float _Tp>
to_chars_result to_chars(char* first, char* last, _Tp value) noexcept {
  return __ycxx::__detail::__to_chars_float(first, last, value);
}
inline to_chars_result to_chars(char* first, char* last, float value, chars_format __fmt) noexcept {
  return __ycxx::__detail::__to_chars_float(first, last, value, __fmt);
}
inline to_chars_result to_chars(char* first, char* last, double value, chars_format __fmt) noexcept {
  return __ycxx::__detail::__to_chars_float(first, last, value, __fmt);
}
inline to_chars_result to_chars(char* first, char* last, long double value, chars_format __fmt) noexcept {
  return __ycxx::__detail::__to_chars_float(first, last, value, __fmt);
}
template <__ycxx::__detail::__charconv_extended_float _Tp>
to_chars_result to_chars(char* first, char* last, _Tp value, chars_format __fmt) noexcept {
  return __ycxx::__detail::__to_chars_float(first, last, value, __fmt);
}
inline to_chars_result to_chars(char* first, char* last, float value, chars_format __fmt, int precision) noexcept {
  return __ycxx::__detail::__to_chars_float(first, last, value, __fmt, precision);
}
inline to_chars_result to_chars(char* first, char* last, double value, chars_format __fmt, int precision) noexcept {
  return __ycxx::__detail::__to_chars_float(first, last, value, __fmt, precision);
}
inline to_chars_result to_chars(char* first, char* last, long double value, chars_format __fmt, int precision) noexcept {
  return __ycxx::__detail::__to_chars_float(first, last, value, __fmt, precision);
}
template <__ycxx::__detail::__charconv_extended_float _Tp>
to_chars_result to_chars(char* first, char* last, _Tp value, chars_format __fmt, int precision) noexcept {
  return __ycxx::__detail::__to_chars_float(first, last, value, __fmt, precision);
}

// [charconv.from.chars]: integers.
constexpr from_chars_result from_chars(const char* first, const char* last, char& value, int base = 10) {
  return __ycxx::__detail::__from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, signed char& value, int base = 10) {
  return __ycxx::__detail::__from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, unsigned char& value, int base = 10) {
  return __ycxx::__detail::__from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, short& value, int base = 10) {
  return __ycxx::__detail::__from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, unsigned short& value, int base = 10) {
  return __ycxx::__detail::__from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, int& value, int base = 10) {
  return __ycxx::__detail::__from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, unsigned int& value, int base = 10) {
  return __ycxx::__detail::__from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, long& value, int base = 10) {
  return __ycxx::__detail::__from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, unsigned long& value, int base = 10) {
  return __ycxx::__detail::__from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, long long& value, int base = 10) {
  return __ycxx::__detail::__from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, unsigned long long& value,
                                       int base = 10) {
  return __ycxx::__detail::__from_chars_integer(first, last, value, base);
}
template <__ycxx::__detail::__charconv_int128 _Tp>
constexpr from_chars_result from_chars(const char* first, const char* last, _Tp& value, int base = 10) {
  return __ycxx::__detail::__from_chars_integer(first, last, value, base);
}

// [charconv.from.chars]: floating point.
inline from_chars_result from_chars(const char* first, const char* last, float& value,
                                    chars_format __fmt = chars_format::general) noexcept {
  return __ycxx::__detail::__from_chars_float(first, last, value, __fmt);
}
inline from_chars_result from_chars(const char* first, const char* last, double& value,
                                    chars_format __fmt = chars_format::general) noexcept {
  return __ycxx::__detail::__from_chars_float(first, last, value, __fmt);
}
inline from_chars_result from_chars(const char* first, const char* last, long double& value,
                                    chars_format __fmt = chars_format::general) noexcept {
  return __ycxx::__detail::__from_chars_float(first, last, value, __fmt);
}
template <__ycxx::__detail::__charconv_extended_float _Tp>
from_chars_result from_chars(const char* first, const char* last, _Tp& value,
                             chars_format __fmt = chars_format::general) noexcept {
  return __ycxx::__detail::__from_chars_float(first, last, value, __fmt);
}

} // namespace std
