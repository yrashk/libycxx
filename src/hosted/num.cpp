// libycxx hosted runtime: the character-level stages of num_put and num_get
// ([facet.num.put.virtuals] stage 1, [facet.num.get.virtuals] stage 3), in the "C" locale.
//
// Floating-point values are formatted and parsed with <charconv>, so the results are correctly
// rounded and independent of the C library's locale; printf's %g and the '#' flag are built
// from the fixed and scientific forms.
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <locale>
#include <string>

namespace {

using std::size_t;

char to_upper(char c) { return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c; }

// A growable output for stage 1: the caller's buffer while it suffices, then a std::string.
struct __sink {
  char* __buf;
  size_t __cap;
  size_t __len = 0;
  void put(char c) {
    if (__len < __cap)
      __buf[__len] = c;
    ++__len;
  }
  void put(const char* s, size_t n) {
    for (size_t i = 0; i < n; ++i)
      put(s[i]);
  }
};

// Whether the C library's printf writes a sign before a NaN: '-' for one with its sign bit set
// ("%f"), '+' with the + flag ("%+f"). Stage 1 converts "as if by" printf
// ([facet.num.put.virtuals]); glibc writes both signs, Darwin's printf neither ("nan"; libc++'s
// put_long_double test asks the C library the same way).
struct __nan_signs {
  bool __minus, __plus;
};
const __nan_signs& nan_signs() {
  static const __nan_signs s = [] {
    char __b[16];
    __nan_signs r{};
    std::snprintf(__b, sizeof __b, "%f", -__builtin_nan(""));
    r.__minus = __b[0] == '-';
    std::snprintf(__b, sizeof __b, "%+f", __builtin_nan(""));
    r.__plus = __b[0] == '+';
    return r;
  }();
  return s;
}

// Writes v with to_chars in format fmt and precision prec (< 0: none) into s.
template <class _Fp>
std::string __chars(_Fp __v, std::chars_format __fmt, int __prec) {
  char __y_local[256];
  std::to_chars_result r = __prec < 0 ? std::to_chars(__y_local, __y_local + sizeof __y_local, __v, __fmt)
                                    : std::to_chars(__y_local, __y_local + sizeof __y_local, __v, __fmt, __prec);
  if (r.ec == std::errc())
    return std::string(__y_local, r.ptr);
  // large values in fixed notation, or large precisions
  std::string big(1024, '\0');
  for (;;) {
    r = __prec < 0 ? std::to_chars(big.data(), big.data() + big.size(), __v, __fmt)
                 : std::to_chars(big.data(), big.data() + big.size(), __v, __fmt, __prec);
    if (r.ec == std::errc()) {
      big.resize(static_cast<size_t>(r.ptr - big.data()));
      return big;
    }
    big.resize(big.size() * 4);
  }
}

// The decimal exponent of a scientific-notation string ("d.ddde+XX").
int exponent_of(const std::string& s) {
  const size_t e = s.find('e');
  return e == std::string::npos ? 0 : std::atoi(s.c_str() + e + 1);
}

// Inserts a decimal point after the mantissa digits if there is none ('#').
void ensure_point(std::string& s) {
  if (s.find('.') != std::string::npos)
    return;
  const size_t e = s.find_first_of("ep");
  s.insert(e == std::string::npos ? s.size() : e, 1, '.');
}

template <class _Fp>
size_t format_float(char* __buf, size_t __cap, _Fp __v, std::ios_base::fmtflags flags, std::streamsize precision, size_t* __pad) {
  using _Bp = std::ios_base;
  const _Bp::fmtflags floatfield = flags & _Bp::floatfield;
  const bool upper = (flags & _Bp::uppercase) != 0, showpos = (flags & _Bp::showpos) != 0,
             showpoint = (flags & _Bp::showpoint) != 0;
  __sink out{__buf, __cap};
  const bool __nan = __builtin_isnan(__v);
  if (__builtin_signbit(__v)) {
    if (!__nan || nan_signs().__minus)
      out.put('-');
  } else if (showpos && (!__nan || nan_signs().__plus)) {
    out.put('+');
  }
  *__pad = out.__len;
  const _Fp a = __builtin_signbit(__v) ? -__v : __v;
  std::string __body;
  if (__builtin_isnan(__v)) {
    __body = "nan";
  } else if (__builtin_isinf(__v)) {
    __body = "inf";
  } else if (floatfield == _Bp::fixed) {
    const int p = precision < 0 ? 6 : static_cast<int>(precision);
    __body = __chars(a, std::chars_format::fixed, p);
    if (showpoint)
      ensure_point(__body);
  } else if (floatfield == _Bp::scientific) {
    const int p = precision < 0 ? 6 : static_cast<int>(precision);
    __body = __chars(a, std::chars_format::scientific, p);
    if (showpoint)
      ensure_point(__body);
  } else if (floatfield == (_Bp::fixed | _Bp::scientific)) {
    __body = "0x" + __chars(a, std::chars_format::hex, -1);
    if (showpoint)
      ensure_point(__body);
    if (*__pad == 0) // [tab:facet.num.put.fill]: after a sign if there is one, else after 0x
      *__pad = 2;
  } else {
    const int p = precision < 0 ? 6 : precision == 0 ? 1 : static_cast<int>(precision);
    if (!showpoint) {
      __body = __chars(a, std::chars_format::general, p);
    } else {
      // %#g: the style of %g, trailing zeros kept, always a decimal point
      const int __x = a == 0 ? 0 : exponent_of(__chars(a, std::chars_format::scientific, p - 1));
      if (p > __x && __x >= -4)
        __body = __chars(a, std::chars_format::fixed, p - 1 - __x);
      else
        __body = __chars(a, std::chars_format::scientific, p - 1);
      ensure_point(__body);
    }
  }
  for (char c : __body)
    out.put(upper ? to_upper(c) : c);
  return out.__len;
}

} // namespace

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

size_t __num_put_integer(char* __buf, unsigned long long __v, bool __neg, bool is_signed, std::ios_base::fmtflags flags,
                       size_t* __pad) noexcept {
  using _Bp = std::ios_base;
  const _Bp::fmtflags base = flags & _Bp::basefield;
  const bool upper = (flags & _Bp::uppercase) != 0, showbase = (flags & _Bp::showbase) != 0;
  char digits[72];
  size_t n = 0;
  size_t __len = 0;
  if (base == _Bp::oct) {
    do {
      digits[n++] = static_cast<char>('0' + (__v & 7));
      __v >>= 3;
    } while (__v != 0);
    if (showbase && !(n == 1 && digits[0] == '0'))
      digits[n++] = '0'; // %#o: a leading 0, not padding
    *__pad = 0;
  } else if (base == _Bp::hex) {
    const char* const set = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    const bool zero = __v == 0;
    do {
      digits[n++] = set[__v & 15];
      __v >>= 4;
    } while (__v != 0);
    if (showbase && !zero) {
      __buf[__len++] = '0';
      __buf[__len++] = upper ? 'X' : 'x';
    }
    *__pad = __len;
  } else {
    if (is_signed) {
      if (__neg)
        __buf[__len++] = '-';
      else if (flags & _Bp::showpos)
        __buf[__len++] = '+';
    }
    *__pad = __len;
    return static_cast<size_t>(std::to_chars(__buf + __len, __buf + 72, __v).ptr - __buf);
  }
  while (n != 0)
    __buf[__len++] = digits[--n];
  return __len;
}

size_t __num_put_float(char* __buf, size_t __cap, double __v, std::ios_base::fmtflags flags, std::streamsize __prec,
                     size_t* __pad) noexcept {
  return format_float(__buf, __cap, __v, flags, __prec, __pad);
}
size_t __num_put_float(char* __buf, size_t __cap, long double __v, std::ios_base::fmtflags flags, std::streamsize __prec,
                     size_t* __pad) noexcept {
  return format_float(__buf, __cap, __v, flags, __prec, __pad);
}

size_t __num_put_pointer(char* __buf, const void* __v, size_t* __pad) noexcept {
  const int n = std::snprintf(__buf, 72, "%p", __v);
  const size_t __len = n < 0 ? 0 : static_cast<size_t>(n);
  *__pad = __len >= 2 && __buf[0] == '0' && (__buf[1] == 'x' || __buf[1] == 'X') ? 2 : 0;
  return __len;
}

__num_parse __num_get_integer(const char* s, size_t n, int base, unsigned long long* __magnitude, bool* __neg) noexcept {
  size_t i = 0;
  *__neg = false;
  *__magnitude = 0;
  if (i < n && (s[i] == '+' || s[i] == '-')) {
    *__neg = s[i] == '-';
    ++i;
  }
  const bool prefix = i + 1 < n && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X');
  if (base == 0)
    base = prefix ? 16 : (i < n && s[i] == '0') ? 8 : 10;
  if (base == 16 && prefix)
    i += 2;
  if (i == n)
    return __num_parse::__not_converted; // no digits ("", "-", "0x": strtoull stops before them)
  unsigned long long __v = 0;
  bool overflow = false;
  for (; i < n; ++i) {
    const char c = s[i];
    const int d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 99;
    if (d >= base)
      return __num_parse::__not_converted;
    unsigned long long next;
    if (__builtin_mul_overflow(__v, static_cast<unsigned>(base), &next) ||
        __builtin_add_overflow(next, static_cast<unsigned>(d), &next))
      overflow = true;
    else
      __v = next;
  }
  *__magnitude = __v;
  return overflow ? __num_parse::overflow : __num_parse::ok;
}

}} // namespace __ycxx::__detail

namespace {

// For a field whose value is out of range: whether its magnitude is at least 1 (overflow) or
// below (underflow). digits: the mantissa (hex: after 0x), exp_char 'e' or 'p'.
bool magnitude_at_least_one(const char* s, size_t n, bool hex) {
  const char exp_char = hex ? 'p' : 'e';
  size_t i = 0;
  long long __int_digits = 0; // digits before the point
  long long first = -1;     // position of the first nonzero digit (counted from the start)
  long long __pos = 0;
  bool __point = false;
  long long point_pos = -1;
  for (; i < n && s[i] != exp_char && s[i] != (hex ? 'P' : 'E'); ++i) {
    if (s[i] == '.') {
      __point = true;
      point_pos = __pos;
      continue;
    }
    if (first < 0 && s[i] != '0')
      first = __pos;
    ++__pos;
    if (!__point)
      ++__int_digits;
  }
  (void)point_pos;
  if (first < 0)
    return false;
  long long e = 0;
  if (i < n) {
    ++i;
    bool eneg = false;
    if (i < n && (s[i] == '+' || s[i] == '-')) {
      eneg = s[i] == '-';
      ++i;
    }
    for (; i < n; ++i)
      if (e < 100000000)
        e = e * 10 + (s[i] - '0');
    if (eneg)
      e = -e;
  }
  // the power of the radix of the first nonzero digit
  const long long __mag = __int_digits - first - 1;
  return (hex ? 4 * __mag : __mag) + e >= 0;
}

template <class _Fp>
__ycxx::__detail::__num_parse parse_float(const char* s, size_t n, _Fp* __v) noexcept {
  using __ycxx::__detail::__num_parse;
  size_t i = 0;
  bool __neg = false;
  if (i < n && (s[i] == '+' || s[i] == '-')) {
    __neg = s[i] == '-';
    ++i;
  }
  const bool hex = i + 1 < n && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X');
  if (hex)
    i += 2;
  if (i == n)
    return __num_parse::__not_converted;
  _Fp r{};
  const std::from_chars_result __fc =
      std::from_chars(s + i, s + n, r, hex ? std::chars_format::hex : std::chars_format::general);
  if (__fc.ec == std::errc::invalid_argument || __fc.ptr != s + n)
    return __num_parse::__not_converted;
  __num_parse status = __num_parse::ok;
  if (__fc.ec == std::errc::result_out_of_range) {
    if (magnitude_at_least_one(s + i, n - i, hex)) {
      r = static_cast<_Fp>(__builtin_huge_vall());
      status = __num_parse::overflow;
    } else {
      r = _Fp();
      status = __num_parse::underflow;
    }
  }
  *__v = __neg ? -r : r;
  return status;
}

} // namespace

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

__num_parse __num_get_float(const char* field, size_t n, float* __v) noexcept { return parse_float(field, n, __v); }
__num_parse __num_get_float(const char* field, size_t n, double* __v) noexcept { return parse_float(field, n, __v); }
__num_parse __num_get_float(const char* field, size_t n, long double* __v) noexcept { return parse_float(field, n, __v); }

bool __num_grouping_ok(const std::string& grouping, const unsigned* __groups, size_t n) noexcept {
  for (size_t i = 0; i < n; ++i) { // i: the group's position from the right
    const unsigned __g = __groups[n - 1 - i];
    const char size = grouping[i < grouping.size() ? i : grouping.size() - 1];
    const bool __unlimited = static_cast<signed char>(size) <= 0 || size == std::numeric_limits<char>::max();
    if (i == n - 1) {
      if (__g == 0 || (!__unlimited && __g > static_cast<unsigned char>(size)))
        return false;
    } else if (__unlimited || __g != static_cast<unsigned char>(size)) {
      return false;
    }
  }
  return true;
}

}} // namespace __ycxx::__detail
