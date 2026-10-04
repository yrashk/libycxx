// libycxx hosted runtime: the <string> numeric conversions that need the C library
// ([string.conversions]): sto* through strto*/wcsto*, and the floating-point to_string/to_wstring.
#include <string>
#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <limits>
#include <ycxx/core/error.hpp>

namespace {

// Calls conv(s, &end), maps "no conversion" to invalid_argument and ERANGE or a value outside R
// to out_of_range, stores the index of the first unconverted character in *idx, and leaves
// errno as it was unless the call reported an error.
template <class R, class CharT, class Conv>
R convert(const char* what, const CharT* s, std::size_t* idx, Conv conv) {
  const int saved = errno;
  errno = 0;
  CharT* end = nullptr;
  const auto v = conv(s, &end);
  const int err = errno;
  if (err == 0)
    errno = saved;
  if (end == s)
    ycxx::detail::throw_invalid_argument(what);
  if (err == ERANGE)
    ycxx::detail::throw_out_of_range(what);
  if constexpr (std::is_integral_v<R> && !std::is_same_v<R, decltype(v)>) {
    if (v < std::numeric_limits<R>::min() || v > std::numeric_limits<R>::max())
      ycxx::detail::throw_out_of_range(what);
  }
  if (idx)
    *idx = static_cast<std::size_t>(end - s);
  return static_cast<R>(v);
}

// ---- floating-point to_string: format("{}", v), which is the plain to_chars(first, last, v) ----

template <class T>
std::string fp_to_string(T v) {
  // The plain overload prints the shortest round-trip form, in fixed notation only for
  // 1e-4 <= abs(v) < a power of ten near 2^(digits+1) ([charconv.to.chars]/7): a few dozen
  // characters at most.
  char buf[128];
  const std::to_chars_result r = ycxx::detail::to_chars_shortest(buf, buf + sizeof buf, v);
  return std::string(buf, r.ptr);
}

std::wstring widen(const std::string& s) {
  std::wstring w(s.size(), L'\0');
  for (std::size_t i = 0; i < s.size(); ++i)
    w[i] = static_cast<wchar_t>(s[i]); // the output is ASCII
  return w;
}

} // namespace

namespace std {

int stoi(const string& str, size_t* idx, int base) {
  return convert<int>("std::stoi", str.c_str(), idx, [base](const char* s, char** e) { return std::strtol(s, e, base); });
}
long stol(const string& str, size_t* idx, int base) {
  return convert<long>("std::stol", str.c_str(), idx, [base](const char* s, char** e) { return std::strtol(s, e, base); });
}
unsigned long stoul(const string& str, size_t* idx, int base) {
  return convert<unsigned long>("std::stoul", str.c_str(), idx,
                                [base](const char* s, char** e) { return std::strtoul(s, e, base); });
}
long long stoll(const string& str, size_t* idx, int base) {
  return convert<long long>("std::stoll", str.c_str(), idx,
                            [base](const char* s, char** e) { return std::strtoll(s, e, base); });
}
unsigned long long stoull(const string& str, size_t* idx, int base) {
  return convert<unsigned long long>("std::stoull", str.c_str(), idx,
                                     [base](const char* s, char** e) { return std::strtoull(s, e, base); });
}
float stof(const string& str, size_t* idx) {
  return convert<float>("std::stof", str.c_str(), idx, [](const char* s, char** e) { return std::strtof(s, e); });
}
double stod(const string& str, size_t* idx) {
  return convert<double>("std::stod", str.c_str(), idx, [](const char* s, char** e) { return std::strtod(s, e); });
}
long double stold(const string& str, size_t* idx) {
  return convert<long double>("std::stold", str.c_str(), idx, [](const char* s, char** e) { return std::strtold(s, e); });
}

int stoi(const wstring& str, size_t* idx, int base) {
  return convert<int>("std::stoi", str.c_str(), idx,
                      [base](const wchar_t* s, wchar_t** e) { return std::wcstol(s, e, base); });
}
long stol(const wstring& str, size_t* idx, int base) {
  return convert<long>("std::stol", str.c_str(), idx,
                       [base](const wchar_t* s, wchar_t** e) { return std::wcstol(s, e, base); });
}
unsigned long stoul(const wstring& str, size_t* idx, int base) {
  return convert<unsigned long>("std::stoul", str.c_str(), idx,
                                [base](const wchar_t* s, wchar_t** e) { return std::wcstoul(s, e, base); });
}
long long stoll(const wstring& str, size_t* idx, int base) {
  return convert<long long>("std::stoll", str.c_str(), idx,
                            [base](const wchar_t* s, wchar_t** e) { return std::wcstoll(s, e, base); });
}
unsigned long long stoull(const wstring& str, size_t* idx, int base) {
  return convert<unsigned long long>("std::stoull", str.c_str(), idx,
                                     [base](const wchar_t* s, wchar_t** e) { return std::wcstoull(s, e, base); });
}
float stof(const wstring& str, size_t* idx) {
  return convert<float>("std::stof", str.c_str(), idx, [](const wchar_t* s, wchar_t** e) { return std::wcstof(s, e); });
}
double stod(const wstring& str, size_t* idx) {
  return convert<double>("std::stod", str.c_str(), idx, [](const wchar_t* s, wchar_t** e) { return std::wcstod(s, e); });
}
long double stold(const wstring& str, size_t* idx) {
  return convert<long double>("std::stold", str.c_str(), idx,
                              [](const wchar_t* s, wchar_t** e) { return std::wcstold(s, e); });
}

string to_string(float val) { return fp_to_string(val); }
string to_string(double val) { return fp_to_string(val); }
string to_string(long double val) { return fp_to_string(val); }
wstring to_wstring(float val) { return widen(fp_to_string(val)); }
wstring to_wstring(double val) { return widen(fp_to_string(val)); }
wstring to_wstring(long double val) { return widen(fp_to_string(val)); }

} // namespace std
