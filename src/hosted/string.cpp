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
template <class _Rp, class _CharT, class Conv>
_Rp __convert(const char* what, const _CharT* s, std::size_t* __idx, Conv conv) {
  const int __saved = errno;
  errno = 0;
  _CharT* end = nullptr;
  const auto __v = conv(s, &end);
  const int __err = errno;
  if (__err == 0)
    errno = __saved;
  if (end == s)
    __ycxx::__detail::__throw_invalid_argument(what);
  if (__err == ERANGE)
    __ycxx::__detail::__throw_out_of_range(what);
  if constexpr (std::is_integral_v<_Rp> && !std::is_same_v<_Rp, decltype(__v)>) {
    if (__v < std::numeric_limits<_Rp>::min() || __v > std::numeric_limits<_Rp>::max())
      __ycxx::__detail::__throw_out_of_range(what);
  }
  if (__idx)
    *__idx = static_cast<std::size_t>(end - s);
  return static_cast<_Rp>(__v);
}

// ---- floating-point to_string: format("{}", v), which is the plain to_chars(first, last, v) ----

template <class _Tp>
std::string fp_to_string(_Tp __v) {
  // The plain overload prints the shortest round-trip form, in fixed notation only for
  // 1e-4 <= abs(v) < a power of ten near 2^(digits+1) ([charconv.to.chars]/7): a few dozen
  // characters at most.
  char __buf[128];
  const std::to_chars_result r = __ycxx::__detail::__to_chars_shortest(__buf, __buf + sizeof __buf, __v);
  return std::string(__buf, r.ptr);
}

std::wstring widen(const std::string& s) {
  std::wstring __w(s.size(), L'\0');
  for (std::size_t i = 0; i < s.size(); ++i)
    __w[i] = static_cast<wchar_t>(s[i]); // the output is ASCII
  return __w;
}

} // namespace

namespace [[__gnu__::__visibility__("hidden")]] std {

int stoi(const string& str, size_t* __idx, int base) {
  return __convert<int>("std::stoi", str.c_str(), __idx, [base](const char* s, char** e) { return std::strtol(s, e, base); });
}
long stol(const string& str, size_t* __idx, int base) {
  return __convert<long>("std::stol", str.c_str(), __idx, [base](const char* s, char** e) { return std::strtol(s, e, base); });
}
unsigned long stoul(const string& str, size_t* __idx, int base) {
  return __convert<unsigned long>("std::stoul", str.c_str(), __idx,
                                [base](const char* s, char** e) { return std::strtoul(s, e, base); });
}
long long stoll(const string& str, size_t* __idx, int base) {
  return __convert<long long>("std::stoll", str.c_str(), __idx,
                            [base](const char* s, char** e) { return std::strtoll(s, e, base); });
}
unsigned long long stoull(const string& str, size_t* __idx, int base) {
  return __convert<unsigned long long>("std::stoull", str.c_str(), __idx,
                                     [base](const char* s, char** e) { return std::strtoull(s, e, base); });
}
float stof(const string& str, size_t* __idx) {
  return __convert<float>("std::stof", str.c_str(), __idx, [](const char* s, char** e) { return std::strtof(s, e); });
}
double stod(const string& str, size_t* __idx) {
  return __convert<double>("std::stod", str.c_str(), __idx, [](const char* s, char** e) { return std::strtod(s, e); });
}
long double stold(const string& str, size_t* __idx) {
  return __convert<long double>("std::stold", str.c_str(), __idx, [](const char* s, char** e) { return std::strtold(s, e); });
}

int stoi(const wstring& str, size_t* __idx, int base) {
  return __convert<int>("std::stoi", str.c_str(), __idx,
                      [base](const wchar_t* s, wchar_t** e) { return std::wcstol(s, e, base); });
}
long stol(const wstring& str, size_t* __idx, int base) {
  return __convert<long>("std::stol", str.c_str(), __idx,
                       [base](const wchar_t* s, wchar_t** e) { return std::wcstol(s, e, base); });
}
unsigned long stoul(const wstring& str, size_t* __idx, int base) {
  return __convert<unsigned long>("std::stoul", str.c_str(), __idx,
                                [base](const wchar_t* s, wchar_t** e) { return std::wcstoul(s, e, base); });
}
long long stoll(const wstring& str, size_t* __idx, int base) {
  return __convert<long long>("std::stoll", str.c_str(), __idx,
                            [base](const wchar_t* s, wchar_t** e) { return std::wcstoll(s, e, base); });
}
unsigned long long stoull(const wstring& str, size_t* __idx, int base) {
  return __convert<unsigned long long>("std::stoull", str.c_str(), __idx,
                                     [base](const wchar_t* s, wchar_t** e) { return std::wcstoull(s, e, base); });
}
float stof(const wstring& str, size_t* __idx) {
  return __convert<float>("std::stof", str.c_str(), __idx, [](const wchar_t* s, wchar_t** e) { return std::wcstof(s, e); });
}
double stod(const wstring& str, size_t* __idx) {
  return __convert<double>("std::stod", str.c_str(), __idx, [](const wchar_t* s, wchar_t** e) { return std::wcstod(s, e); });
}
long double stold(const wstring& str, size_t* __idx) {
  return __convert<long double>("std::stold", str.c_str(), __idx,
                              [](const wchar_t* s, wchar_t** e) { return std::wcstold(s, e); });
}

string to_string(float __val) { return fp_to_string(__val); }
string to_string(double __val) { return fp_to_string(__val); }
string to_string(long double __val) { return fp_to_string(__val); }
wstring to_wstring(float __val) { return widen(fp_to_string(__val)); }
wstring to_wstring(double __val) { return widen(fp_to_string(__val)); }
wstring to_wstring(long double __val) { return widen(fp_to_string(__val)); }

} // namespace std
