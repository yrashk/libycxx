// libycxx hosted runtime: strfromd, strfromf and strfroml (C23 7.24.1.3) for C libraries that
// lack them (_YCXX_C_HAS_STRFROM 0: cmake/ycxx-c-library.cmake found none). <cstdlib> then declares std::strfromd
// and friends as calls of these; elsewhere they are unused, but built everywhere so that every
// platform compiles them.
//
// C23 7.24.1.3: each is equivalent to snprintf(s, n, format, fp), except that the format may
// contain only '%', an optional precision without '*', and one of the conversion specifiers
// a A e E f F g G, which applies to the function's type rather than through a length modifier;
// any other format is undefined behavior. The return value is snprintf's: the length the full
// output would have, so the null-terminated output is complete exactly when it is nonnegative
// and less than n. So the format is checked and rebuilt as "%.*<L><conv>" (or without the
// precision) for the C library's snprintf, which also gives the C locale's decimal point as
// snprintf would. An invalid format sets errno to EINVAL and returns -1 instead of reaching
// snprintf with whatever it holds, and a precision beyond INT_MAX (more than snprintf can take)
// sets EOVERFLOW, as snprintf would for such output. float arguments arrive converted to double,
// which represents them exactly.
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>

namespace {

struct strfrom_format {
  char __spec[8]; // "%", ".*" when there is a precision, the length modifier, the conversion
  int precision;
  bool has_precision;
};

// 0, or the errno value for a format that cannot be used.
int parse(const char* __f, bool long_double, strfrom_format& out) noexcept {
  if (__f == nullptr || *__f++ != '%')
    return EINVAL;
  char* __o = out.__spec;
  *__o++ = '%';
  out.has_precision = false;
  out.precision = 0;
  if (*__f == '.') { // "." alone is precision 0
    ++__f;
    out.has_precision = true;
    long long p = 0;
    for (; *__f >= '0' && *__f <= '9'; ++__f)
      if ((p = p * 10 + (*__f - '0')) > INT_MAX)
        p = static_cast<long long>(INT_MAX) + 1; // saturated
    if (p > INT_MAX)
      return EOVERFLOW;
    out.precision = static_cast<int>(p);
    *__o++ = '.';
    *__o++ = '*';
  }
  if (long_double)
    *__o++ = 'L';
  switch (*__f) {
  case 'a':
  case 'A':
  case 'e':
  case 'E':
  case 'f':
  case 'F':
  case 'g':
  case 'G':
    *__o++ = *__f++;
    break;
  default:
    return EINVAL;
  }
  *__o = '\0';
  return *__f == '\0' ? 0 : EINVAL;
}

template <class _Tp>
int format_value(char* s, std::size_t n, const char* format, _Tp __fp) noexcept {
  strfrom_format __f;
  if (const int e = parse(format, __is_same(_Tp, long double), __f)) {
    errno = e;
    return -1;
  }
  return __f.has_precision ? std::snprintf(s, n, __f.__spec, __f.precision, __fp) : std::snprintf(s, n, __f.__spec, __fp);
}

} // namespace

int __ycxx::__detail::__strfrom(char* s, std::size_t n, const char* format, double __fp) noexcept {
  return format_value(s, n, format, __fp);
}

int __ycxx::__detail::__strfrom(char* s, std::size_t n, const char* format, long double __fp) noexcept {
  return format_value(s, n, format, __fp);
}
