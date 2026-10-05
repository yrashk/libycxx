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

namespace std {

// [charconv.syn]: a bitmask type ([bitmask.types]).
enum class chars_format { scientific = 1, fixed = 2, hex = 4, general = fixed | scientific };

constexpr chars_format operator&(chars_format x, chars_format y) noexcept {
  return static_cast<chars_format>(static_cast<int>(x) & static_cast<int>(y));
}
constexpr chars_format operator|(chars_format x, chars_format y) noexcept {
  return static_cast<chars_format>(static_cast<int>(x) | static_cast<int>(y));
}
constexpr chars_format operator^(chars_format x, chars_format y) noexcept {
  return static_cast<chars_format>(static_cast<int>(x) ^ static_cast<int>(y));
}
constexpr chars_format operator~(chars_format x) noexcept {
  return static_cast<chars_format>(~static_cast<int>(x));
}
constexpr chars_format& operator&=(chars_format& x, chars_format y) noexcept { return x = x & y; }
constexpr chars_format& operator|=(chars_format& x, chars_format y) noexcept { return x = x | y; }
constexpr chars_format& operator^=(chars_format& x, chars_format y) noexcept { return x = x ^ y; }

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

namespace ycxx::detail {

// ---- integers ([charconv.to.chars]/4-6, [charconv.from.chars]/2-4) ---------------------------

struct charconv_digit_pairs {
  char c[200];
};
consteval charconv_digit_pairs make_charconv_digit_pairs() {
  charconv_digit_pairs t{};
  for (int i = 0; i < 100; ++i) {
    t.c[2 * i] = static_cast<char>('0' + i / 10);
    t.c[2 * i + 1] = static_cast<char>('0' + i % 10);
  }
  return t;
}
inline constexpr charconv_digit_pairs charconv_pairs = ycxx::detail::make_charconv_digit_pairs();
inline constexpr char charconv_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

// Writes the digits of `value` right-aligned ending at `end`; returns the first digit.
template <class U>
constexpr char* charconv_write_unsigned(char* end, U value, unsigned base) noexcept {
  char* p = end;
  if (base == 10) {
    if constexpr (sizeof(U) > sizeof(unsigned long long)) {
      // Peel 19-digit chunks so that the loop below runs on 64-bit words.
      while (value > static_cast<U>(~0ull)) {
        constexpr unsigned long long chunk = 10'000'000'000'000'000'000ull;
        unsigned long long low = static_cast<unsigned long long>(value % chunk);
        value /= chunk;
        for (int i = 0; i < 19; ++i, low /= 10)
          *--p = static_cast<char>('0' + low % 10);
      }
      return ycxx::detail::charconv_write_unsigned(p, static_cast<unsigned long long>(value), 10);
    } else {
      while (value >= 100) {
        unsigned r = static_cast<unsigned>(value % 100);
        value = static_cast<U>(value / 100);
        p -= 2;
        p[0] = ycxx::detail::charconv_pairs.c[2 * r];
        p[1] = ycxx::detail::charconv_pairs.c[2 * r + 1];
      }
      if (value >= 10) {
        p -= 2;
        p[0] = ycxx::detail::charconv_pairs.c[2 * value];
        p[1] = ycxx::detail::charconv_pairs.c[2 * value + 1];
      } else {
        *--p = static_cast<char>('0' + value);
      }
      return p;
    }
  }
  if ((base & (base - 1)) == 0) {
    int shift = __builtin_ctz(base);
    do {
      *--p = ycxx::detail::charconv_digits[static_cast<unsigned>(value & (base - 1))];
      value = static_cast<U>(value >> shift);
    } while (value != 0);
    return p;
  }
  do {
    *--p = ycxx::detail::charconv_digits[static_cast<unsigned>(value % base)];
    value = static_cast<U>(value / base);
  } while (value != 0);
  return p;
}

template <class T>
constexpr std::to_chars_result to_chars_integer(char* first, char* last, T value, int base) noexcept {
  ycxx::detail::precondition(2 <= base && base <= 36, "std::to_chars: base must be in [2, 36]");
  using U = std::make_unsigned_t<T>;
  U magnitude = static_cast<U>(value);
  bool negative = false;
  if constexpr (is_signed_v<T>) {
    if (value < 0) {
      negative = true;
      magnitude = static_cast<U>(U(0) - magnitude);
    }
  }
  [[indeterminate]] char buf[sizeof(U) * __CHAR_BIT__ + 1];
  char* end = buf + sizeof buf;
  char* p = ycxx::detail::charconv_write_unsigned(end, magnitude, static_cast<unsigned>(base));
  if (negative)
    *--p = '-';
  if (last - first < end - p)
    return {last, std::errc::value_too_large};
  while (p != end)
    *first++ = *p++;
  return {first, std::errc{}};
}

constexpr unsigned charconv_digit_value(char c) noexcept {
  if (c >= '0' && c <= '9')
    return static_cast<unsigned>(c - '0');
  if (c >= 'a' && c <= 'z')
    return static_cast<unsigned>(c - 'a' + 10);
  if (c >= 'A' && c <= 'Z')
    return static_cast<unsigned>(c - 'A' + 10);
  return 99;
}

template <class T>
constexpr std::from_chars_result from_chars_integer(const char* first, const char* last, T& value, int base) noexcept {
  ycxx::detail::precondition(2 <= base && base <= 36, "std::from_chars: base must be in [2, 36]");
  using U = std::make_unsigned_t<T>;
  const char* p = first;
  bool negative = false;
  if constexpr (is_signed_v<T>) {
    if (p != last && *p == '-') {
      negative = true;
      ++p;
    }
  }
  const char* digits = p;
  U acc = 0;
  bool overflow = false;
  for (; p != last; ++p) {
    unsigned d = ycxx::detail::charconv_digit_value(*p);
    if (d >= static_cast<unsigned>(base))
      break;
    if (!overflow)
      overflow = __builtin_mul_overflow(acc, static_cast<unsigned>(base), &acc) || __builtin_add_overflow(acc, d, &acc);
  }
  if (p == digits)
    return {first, std::errc::invalid_argument};
  if constexpr (is_signed_v<T>) {
    U limit = static_cast<U>(static_cast<U>(~U(0)) >> 1); // the largest T
    if (overflow || acc > limit + (negative ? 1 : 0))
      return {p, std::errc::result_out_of_range};
    value = negative ? static_cast<T>(U(0) - acc) : static_cast<T>(acc);
  } else {
    if (overflow)
      return {p, std::errc::result_out_of_range};
    value = acc;
  }
  return {p, std::errc{}};
}

// ---- floating point --------------------------------------------------------------------------

// The binary interchange formats libycxx converts. Each floating-point type maps to one.
enum class fp_kind : unsigned char { binary16, bfloat16, binary32, binary64, x87_extended, binary128 };

template <class T>
consteval fp_kind fp_kind_of() {
  constexpr int digits = fp_format<T>.digits;
  static_assert(digits == 11 || digits == 8 || digits == 24 || digits == 53 || digits == 64 || digits == 113,
                "libycxx <charconv>: unsupported floating-point format");
  if constexpr (digits == 11)
    return fp_kind::binary16;
  else if constexpr (digits == 8)
    return fp_kind::bfloat16;
  else if constexpr (digits == 24)
    return fp_kind::binary32;
  else if constexpr (digits == 53)
    return fp_kind::binary64;
  else if constexpr (digits == 64)
    return fp_kind::x87_extended;
  else
    return fp_kind::binary128;
}

// The value's object representation as a little-endian 128-bit integer (lo, hi). Only the bytes
// of the format are read: x87 long double has 6 padding bytes, which stay out of the result.
struct fp_raw {
  unsigned long long lo = 0;
  unsigned long long hi = 0;
};
template <class T>
inline constexpr int fp_value_bytes = fp_kind_of<T>() == fp_kind::x87_extended ? 10 : static_cast<int>(sizeof(T));

template <class T>
fp_raw fp_to_raw(T value) noexcept {
  struct bytes {
    unsigned char b[sizeof(T)];
  };
  bytes in = __builtin_bit_cast(bytes, value);
  fp_raw r;
  for (int i = 0; i < fp_value_bytes<T>; ++i) {
    int pos = std::endian::native == std::endian::little ? i : fp_value_bytes<T> - 1 - i;
    unsigned long long byte = in.b[pos];
    if (i < 8)
      r.lo |= byte << (8 * i);
    else
      r.hi |= byte << (8 * (i - 8));
  }
  return r;
}
template <class T>
T fp_from_raw(fp_raw r) noexcept {
  struct bytes {
    unsigned char b[sizeof(T)];
  };
  bytes out{};
  for (int i = 0; i < fp_value_bytes<T>; ++i) {
    int pos = std::endian::native == std::endian::little ? i : fp_value_bytes<T> - 1 - i;
    out.b[pos] = static_cast<unsigned char>(i < 8 ? r.lo >> (8 * i) : r.hi >> (8 * (i - 8)));
  }
  return __builtin_bit_cast(T, out);
}

// Out-of-line conversions (src/runtime/charconv). `fmt` is a chars_format value, or 0 for the
// overload without one; `precision` is the requested precision, or -1 for the shortest form.
std::to_chars_result fp_to_chars(char* first, char* last, fp_kind kind, fp_raw bits, int fmt, int precision) noexcept;
// Stores the result in `bits` only when the conversion succeeds.
std::from_chars_result fp_from_chars(const char* first, const char* last, fp_kind kind, fp_raw& bits, int fmt) noexcept;

constexpr bool charconv_valid_format(std::chars_format fmt) noexcept {
  return fmt == std::chars_format::scientific || fmt == std::chars_format::fixed || fmt == std::chars_format::hex ||
         fmt == std::chars_format::general;
}

template <class T>
std::to_chars_result to_chars_float(char* first, char* last, T value) noexcept {
  return ycxx::detail::fp_to_chars(first, last, ycxx::detail::fp_kind_of<T>(), ycxx::detail::fp_to_raw(value), 0, -1);
}
template <class T>
std::to_chars_result to_chars_float(char* first, char* last, T value, std::chars_format fmt) noexcept {
  ycxx::detail::precondition(ycxx::detail::charconv_valid_format(fmt), "std::to_chars: invalid chars_format");
  return ycxx::detail::fp_to_chars(first, last, ycxx::detail::fp_kind_of<T>(), ycxx::detail::fp_to_raw(value),
                                   static_cast<int>(fmt), -1);
}
template <class T>
std::to_chars_result to_chars_float(char* first, char* last, T value, std::chars_format fmt, int precision) noexcept {
  ycxx::detail::precondition(ycxx::detail::charconv_valid_format(fmt), "std::to_chars: invalid chars_format");
  // A negative precision is taken as if it were omitted (C 7.23.6.1): 6 for e, f and g; for a,
  // the exact (shortest) hexadecimal representation.
  if (precision < 0)
    precision = fmt == std::chars_format::hex ? -1 : 6;
  return ycxx::detail::fp_to_chars(first, last, ycxx::detail::fp_kind_of<T>(), ycxx::detail::fp_to_raw(value),
                                   static_cast<int>(fmt), precision);
}
template <class T>
std::from_chars_result from_chars_float(const char* first, const char* last, T& value, std::chars_format fmt) noexcept {
  ycxx::detail::precondition(ycxx::detail::charconv_valid_format(fmt), "std::from_chars: invalid chars_format");
  fp_raw bits;
  std::from_chars_result r =
      ycxx::detail::fp_from_chars(first, last, ycxx::detail::fp_kind_of<T>(), bits, static_cast<int>(fmt));
  if (r.ec == std::errc{})
    value = ycxx::detail::fp_from_raw<T>(bits);
  return r;
}

// The shortest round-trip form, exactly as std::to_chars(first, last, value) writes it (plain
// overload, [charconv.to.chars]/7), under an internal name for std::to_string and <format>: any
// floating-point type, without overload resolution against the integer overloads.
template <class T>
  requires is_floating_v<T>
std::to_chars_result to_chars_shortest(char* first, char* last, T value) noexcept {
  return ycxx::detail::to_chars_float(first, last, value);
}

template <class T>
concept charconv_extended_float = is_any_of<T, float16, float32, float64, float128, bfloat16>;
template <class T>
concept charconv_int128 = is_any_of<T, int128, uint128>;

} // namespace ycxx::detail

namespace std {

// [charconv.to.chars]: integers. One overload per type, as the synopsis specifies.
constexpr to_chars_result to_chars(char* first, char* last, char value, int base = 10) {
  return ycxx::detail::to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, signed char value, int base = 10) {
  return ycxx::detail::to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, unsigned char value, int base = 10) {
  return ycxx::detail::to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, short value, int base = 10) {
  return ycxx::detail::to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, unsigned short value, int base = 10) {
  return ycxx::detail::to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, int value, int base = 10) {
  return ycxx::detail::to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, unsigned int value, int base = 10) {
  return ycxx::detail::to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, long value, int base = 10) {
  return ycxx::detail::to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, unsigned long value, int base = 10) {
  return ycxx::detail::to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, long long value, int base = 10) {
  return ycxx::detail::to_chars_integer(first, last, value, base);
}
constexpr to_chars_result to_chars(char* first, char* last, unsigned long long value, int base = 10) {
  return ycxx::detail::to_chars_integer(first, last, value, base);
}
// __int128 (extension; an integer type where the compiler has it). A template, so that the
// declaration stays well-formed where the type does not exist.
template <ycxx::detail::charconv_int128 T>
constexpr to_chars_result to_chars(char* first, char* last, T value, int base = 10) {
  return ycxx::detail::to_chars_integer(first, last, value, base);
}
to_chars_result to_chars(char* first, char* last, bool value, int base = 10) = delete;

// [charconv.to.chars]: floating point.
inline to_chars_result to_chars(char* first, char* last, float value) noexcept {
  return ycxx::detail::to_chars_float(first, last, value);
}
inline to_chars_result to_chars(char* first, char* last, double value) noexcept {
  return ycxx::detail::to_chars_float(first, last, value);
}
inline to_chars_result to_chars(char* first, char* last, long double value) noexcept {
  return ycxx::detail::to_chars_float(first, last, value);
}
template <ycxx::detail::charconv_extended_float T>
to_chars_result to_chars(char* first, char* last, T value) noexcept {
  return ycxx::detail::to_chars_float(first, last, value);
}
inline to_chars_result to_chars(char* first, char* last, float value, chars_format fmt) noexcept {
  return ycxx::detail::to_chars_float(first, last, value, fmt);
}
inline to_chars_result to_chars(char* first, char* last, double value, chars_format fmt) noexcept {
  return ycxx::detail::to_chars_float(first, last, value, fmt);
}
inline to_chars_result to_chars(char* first, char* last, long double value, chars_format fmt) noexcept {
  return ycxx::detail::to_chars_float(first, last, value, fmt);
}
template <ycxx::detail::charconv_extended_float T>
to_chars_result to_chars(char* first, char* last, T value, chars_format fmt) noexcept {
  return ycxx::detail::to_chars_float(first, last, value, fmt);
}
inline to_chars_result to_chars(char* first, char* last, float value, chars_format fmt, int precision) noexcept {
  return ycxx::detail::to_chars_float(first, last, value, fmt, precision);
}
inline to_chars_result to_chars(char* first, char* last, double value, chars_format fmt, int precision) noexcept {
  return ycxx::detail::to_chars_float(first, last, value, fmt, precision);
}
inline to_chars_result to_chars(char* first, char* last, long double value, chars_format fmt, int precision) noexcept {
  return ycxx::detail::to_chars_float(first, last, value, fmt, precision);
}
template <ycxx::detail::charconv_extended_float T>
to_chars_result to_chars(char* first, char* last, T value, chars_format fmt, int precision) noexcept {
  return ycxx::detail::to_chars_float(first, last, value, fmt, precision);
}

// [charconv.from.chars]: integers.
constexpr from_chars_result from_chars(const char* first, const char* last, char& value, int base = 10) {
  return ycxx::detail::from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, signed char& value, int base = 10) {
  return ycxx::detail::from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, unsigned char& value, int base = 10) {
  return ycxx::detail::from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, short& value, int base = 10) {
  return ycxx::detail::from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, unsigned short& value, int base = 10) {
  return ycxx::detail::from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, int& value, int base = 10) {
  return ycxx::detail::from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, unsigned int& value, int base = 10) {
  return ycxx::detail::from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, long& value, int base = 10) {
  return ycxx::detail::from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, unsigned long& value, int base = 10) {
  return ycxx::detail::from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, long long& value, int base = 10) {
  return ycxx::detail::from_chars_integer(first, last, value, base);
}
constexpr from_chars_result from_chars(const char* first, const char* last, unsigned long long& value,
                                       int base = 10) {
  return ycxx::detail::from_chars_integer(first, last, value, base);
}
template <ycxx::detail::charconv_int128 T>
constexpr from_chars_result from_chars(const char* first, const char* last, T& value, int base = 10) {
  return ycxx::detail::from_chars_integer(first, last, value, base);
}

// [charconv.from.chars]: floating point.
inline from_chars_result from_chars(const char* first, const char* last, float& value,
                                    chars_format fmt = chars_format::general) noexcept {
  return ycxx::detail::from_chars_float(first, last, value, fmt);
}
inline from_chars_result from_chars(const char* first, const char* last, double& value,
                                    chars_format fmt = chars_format::general) noexcept {
  return ycxx::detail::from_chars_float(first, last, value, fmt);
}
inline from_chars_result from_chars(const char* first, const char* last, long double& value,
                                    chars_format fmt = chars_format::general) noexcept {
  return ycxx::detail::from_chars_float(first, last, value, fmt);
}
template <ycxx::detail::charconv_extended_float T>
from_chars_result from_chars(const char* first, const char* last, T& value,
                             chars_format fmt = chars_format::general) noexcept {
  return ycxx::detail::from_chars_float(first, last, value, fmt);
}

} // namespace std
