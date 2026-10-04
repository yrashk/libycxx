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
struct sink {
  char* buf;
  size_t cap;
  size_t len = 0;
  void put(char c) {
    if (len < cap)
      buf[len] = c;
    ++len;
  }
  void put(const char* s, size_t n) {
    for (size_t i = 0; i < n; ++i)
      put(s[i]);
  }
};

// Writes v with to_chars in format fmt and precision prec (< 0: none) into s.
template <class F>
std::string chars(F v, std::chars_format fmt, int prec) {
  char local[256];
  std::to_chars_result r = prec < 0 ? std::to_chars(local, local + sizeof local, v, fmt)
                                    : std::to_chars(local, local + sizeof local, v, fmt, prec);
  if (r.ec == std::errc())
    return std::string(local, r.ptr);
  // large values in fixed notation, or large precisions
  std::string big(1024, '\0');
  for (;;) {
    r = prec < 0 ? std::to_chars(big.data(), big.data() + big.size(), v, fmt)
                 : std::to_chars(big.data(), big.data() + big.size(), v, fmt, prec);
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

template <class F>
size_t format_float(char* buf, size_t cap, F v, std::ios_base::fmtflags flags, std::streamsize precision, size_t* pad) {
  using B = std::ios_base;
  const B::fmtflags floatfield = flags & B::floatfield;
  const bool upper = (flags & B::uppercase) != 0, showpos = (flags & B::showpos) != 0,
             showpoint = (flags & B::showpoint) != 0;
  sink out{buf, cap};
  if (__builtin_signbit(v))
    out.put('-');
  else if (showpos)
    out.put('+');
  *pad = out.len;
  const F a = __builtin_signbit(v) ? -v : v;
  std::string body;
  if (__builtin_isnan(v)) {
    body = "nan";
  } else if (__builtin_isinf(v)) {
    body = "inf";
  } else if (floatfield == B::fixed) {
    const int p = precision < 0 ? 6 : static_cast<int>(precision);
    body = chars(a, std::chars_format::fixed, p);
    if (showpoint)
      ensure_point(body);
  } else if (floatfield == B::scientific) {
    const int p = precision < 0 ? 6 : static_cast<int>(precision);
    body = chars(a, std::chars_format::scientific, p);
    if (showpoint)
      ensure_point(body);
  } else if (floatfield == (B::fixed | B::scientific)) {
    body = "0x" + chars(a, std::chars_format::hex, -1);
    if (showpoint)
      ensure_point(body);
    if (*pad == 0) // [tab:facet.num.put.fill]: after a sign if there is one, else after 0x
      *pad = 2;
  } else {
    const int p = precision < 0 ? 6 : precision == 0 ? 1 : static_cast<int>(precision);
    if (!showpoint) {
      body = chars(a, std::chars_format::general, p);
    } else {
      // %#g: the style of %g, trailing zeros kept, always a decimal point
      const int x = a == 0 ? 0 : exponent_of(chars(a, std::chars_format::scientific, p - 1));
      if (p > x && x >= -4)
        body = chars(a, std::chars_format::fixed, p - 1 - x);
      else
        body = chars(a, std::chars_format::scientific, p - 1);
      ensure_point(body);
    }
  }
  for (char c : body)
    out.put(upper ? to_upper(c) : c);
  return out.len;
}

} // namespace

namespace ycxx::detail {

size_t num_put_integer(char* buf, unsigned long long v, bool neg, bool is_signed, std::ios_base::fmtflags flags,
                       size_t* pad) noexcept {
  using B = std::ios_base;
  const B::fmtflags base = flags & B::basefield;
  const bool upper = (flags & B::uppercase) != 0, showbase = (flags & B::showbase) != 0;
  char digits[72];
  size_t n = 0;
  size_t len = 0;
  if (base == B::oct) {
    do {
      digits[n++] = static_cast<char>('0' + (v & 7));
      v >>= 3;
    } while (v != 0);
    if (showbase && !(n == 1 && digits[0] == '0'))
      digits[n++] = '0'; // %#o: a leading 0, not padding
    *pad = 0;
  } else if (base == B::hex) {
    const char* const set = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    const bool zero = v == 0;
    do {
      digits[n++] = set[v & 15];
      v >>= 4;
    } while (v != 0);
    if (showbase && !zero) {
      buf[len++] = '0';
      buf[len++] = upper ? 'X' : 'x';
    }
    *pad = len;
  } else {
    do {
      digits[n++] = static_cast<char>('0' + v % 10);
      v /= 10;
    } while (v != 0);
    if (is_signed) {
      if (neg)
        buf[len++] = '-';
      else if (flags & B::showpos)
        buf[len++] = '+';
    }
    *pad = len;
  }
  while (n != 0)
    buf[len++] = digits[--n];
  return len;
}

size_t num_put_float(char* buf, size_t cap, double v, std::ios_base::fmtflags flags, std::streamsize prec,
                     size_t* pad) noexcept {
  return format_float(buf, cap, v, flags, prec, pad);
}
size_t num_put_float(char* buf, size_t cap, long double v, std::ios_base::fmtflags flags, std::streamsize prec,
                     size_t* pad) noexcept {
  return format_float(buf, cap, v, flags, prec, pad);
}

size_t num_put_pointer(char* buf, const void* v, size_t* pad) noexcept {
  const int n = std::snprintf(buf, 72, "%p", v);
  const size_t len = n < 0 ? 0 : static_cast<size_t>(n);
  *pad = len >= 2 && buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X') ? 2 : 0;
  return len;
}

num_parse num_get_integer(const char* s, size_t n, int base, unsigned long long* magnitude, bool* neg) noexcept {
  size_t i = 0;
  *neg = false;
  *magnitude = 0;
  if (i < n && (s[i] == '+' || s[i] == '-')) {
    *neg = s[i] == '-';
    ++i;
  }
  const bool prefix = i + 1 < n && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X');
  if (base == 0)
    base = prefix ? 16 : (i < n && s[i] == '0') ? 8 : 10;
  if (base == 16 && prefix)
    i += 2;
  if (i == n)
    return num_parse::not_converted; // no digits ("", "-", "0x": strtoull stops before them)
  unsigned long long v = 0;
  bool overflow = false;
  for (; i < n; ++i) {
    const char c = s[i];
    const int d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 99;
    if (d >= base)
      return num_parse::not_converted;
    if (v > (~0ull - static_cast<unsigned>(d)) / static_cast<unsigned>(base))
      overflow = true;
    else
      v = v * static_cast<unsigned>(base) + static_cast<unsigned>(d);
  }
  *magnitude = v;
  return overflow ? num_parse::overflow : num_parse::ok;
}

} // namespace ycxx::detail

namespace {

// For a field whose value is out of range: whether its magnitude is at least 1 (overflow) or
// below (underflow). digits: the mantissa (hex: after 0x), exp_char 'e' or 'p'.
bool magnitude_at_least_one(const char* s, size_t n, bool hex) {
  const char exp_char = hex ? 'p' : 'e';
  size_t i = 0;
  long long int_digits = 0; // digits before the point
  long long first = -1;     // position of the first nonzero digit (counted from the start)
  long long pos = 0;
  bool point = false;
  long long point_pos = -1;
  for (; i < n && s[i] != exp_char && s[i] != (hex ? 'P' : 'E'); ++i) {
    if (s[i] == '.') {
      point = true;
      point_pos = pos;
      continue;
    }
    if (first < 0 && s[i] != '0')
      first = pos;
    ++pos;
    if (!point)
      ++int_digits;
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
  const long long mag = int_digits - first - 1;
  return (hex ? 4 * mag : mag) + e >= 0;
}

template <class F>
ycxx::detail::num_parse parse_float(const char* s, size_t n, F* v) noexcept {
  using ycxx::detail::num_parse;
  size_t i = 0;
  bool neg = false;
  if (i < n && (s[i] == '+' || s[i] == '-')) {
    neg = s[i] == '-';
    ++i;
  }
  const bool hex = i + 1 < n && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X');
  if (hex)
    i += 2;
  if (i == n)
    return num_parse::not_converted;
  F r{};
  const std::from_chars_result fc =
      std::from_chars(s + i, s + n, r, hex ? std::chars_format::hex : std::chars_format::general);
  if (fc.ec == std::errc::invalid_argument || fc.ptr != s + n)
    return num_parse::not_converted;
  num_parse status = num_parse::ok;
  if (fc.ec == std::errc::result_out_of_range) {
    if (magnitude_at_least_one(s + i, n - i, hex)) {
      r = static_cast<F>(__builtin_huge_vall());
      status = num_parse::overflow;
    } else {
      r = F();
      status = num_parse::underflow;
    }
  }
  *v = neg ? -r : r;
  return status;
}

} // namespace

namespace ycxx::detail {

num_parse num_get_float(const char* field, size_t n, float* v) noexcept { return parse_float(field, n, v); }
num_parse num_get_float(const char* field, size_t n, double* v) noexcept { return parse_float(field, n, v); }
num_parse num_get_float(const char* field, size_t n, long double* v) noexcept { return parse_float(field, n, v); }

bool num_grouping_ok(const std::string& grouping, const unsigned* groups, size_t n) noexcept {
  for (size_t i = 0; i < n; ++i) { // i: the group's position from the right
    const unsigned g = groups[n - 1 - i];
    const char size = grouping[i < grouping.size() ? i : grouping.size() - 1];
    const bool unlimited = static_cast<signed char>(size) <= 0 || size == std::numeric_limits<char>::max();
    if (i == n - 1) {
      if (g == 0 || (!unlimited && g > static_cast<unsigned char>(size)))
        return false;
    } else if (unlimited || g != static_cast<unsigned char>(size)) {
      return false;
    }
  }
  return true;
}

} // namespace ycxx::detail
