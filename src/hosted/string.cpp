// libycxx hosted runtime: the <string> numeric conversions that need the C library
// ([string.conversions]): sto* through strto*/wcsto*, and the floating-point to_string/to_wstring.
#include <string>
#include <cerrno>
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

// ---- floating-point to_string: format("{}", v), the shortest round-trip representation ----

template <class T>
T parse(const char* s) {
  if constexpr (std::is_same_v<T, float>)
    return std::strtof(s, nullptr);
  else if constexpr (std::is_same_v<T, double>)
    return std::strtod(s, nullptr);
  else
    return std::strtold(s, nullptr);
}

// Decimal digits d[0..n) and exponent x of a value d0.d1d2... * 10^x.
struct decimal {
  char d[48];
  int n;
  int x;
};

// Writes digits and exponent in %e style ("d.ddde+XX") to buf, for a round-trip test.
int write_e(const decimal& m, char* buf) {
  int k = 0;
  buf[k++] = m.d[0];
  if (m.n > 1) {
    buf[k++] = '.';
    for (int i = 1; i < m.n; ++i)
      buf[k++] = m.d[i];
  }
  buf[k++] = 'e';
  int x = m.x;
  buf[k++] = x < 0 ? '-' : '+';
  if (x < 0)
    x = -x;
  char e[8];
  int ne = 0;
  do {
    e[ne++] = static_cast<char>('0' + x % 10);
    x /= 10;
  } while (x != 0);
  if (ne < 2)
    e[ne++] = '0';
  while (ne > 0)
    buf[k++] = e[--ne];
  buf[k] = '\0';
  return k;
}

// Parses snprintf's "%.*e" output (no sign) into digits and exponent.
decimal from_e(const char* s) {
  decimal m{};
  for (; *s != 'e'; ++s)
    if (*s != '.')
      m.d[m.n++] = *s;
  ++s;
  const bool neg = *s == '-';
  ++s;
  for (; *s; ++s)
    m.x = m.x * 10 + (*s - '0');
  if (neg)
    m.x = -m.x;
  return m;
}

// The neighbouring decimal with the same number of significant digits: delta (+1 or -1) units
// of the last digit, with carry or borrow (9.99e5 + 1 = 1.00e6, 1.00e6 - 1 = 9.99e5).
void step_last_digit(decimal& m, int delta) {
  for (int i = m.n - 1; i >= 0; --i) {
    const int v = m.d[i] - '0' + delta;
    if (v >= 0 && v <= 9) {
      m.d[i] = static_cast<char>('0' + v);
      break;
    }
    m.d[i] = delta > 0 ? '0' : '9';
    if (i == 0) { // carried out of the leading digit
      m.d[0] = '1';
      ++m.x;
    }
  }
  if (m.d[0] == '0') { // borrowed from a leading 1
    for (int i = 0; i < m.n; ++i)
      m.d[i] = '9';
    --m.x;
  }
}

template <class T>
decimal shortest(T v) {
  // v > 0 and finite. The candidate with p + 1 significant digits that snprintf produces is the
  // nearest such decimal; when it does not round-trip, its neighbour on the other side of v can
  // still do so (the rounding interval is asymmetric at powers of two).
  char buf[64];
  constexpr int max_p = std::numeric_limits<T>::max_digits10 - 1;
  for (int p = 0;; ++p) {
    std::snprintf(buf, sizeof buf, "%.*Le", p, static_cast<long double>(v));
    decimal m = from_e(buf);
    if (p == max_p || parse<T>(buf) == v)
      return m;
    const bool above = parse<T>(buf) > v;
    decimal alt = m;
    step_last_digit(alt, above ? -1 : 1);
    write_e(alt, buf);
    if (parse<T>(buf) == v)
      return alt;
  }
}

template <class T>
std::string fp_to_string(T v) {
  std::string r;
  if (__builtin_signbit(v))
    r.push_back('-');
  if (__builtin_isnan(v))
    return r += "nan";
  if (__builtin_isinf(v))
    return r += "inf";
  if (v == 0)
    return r += "0";
  decimal m = shortest(v < 0 ? -v : v);
  while (m.n > 1 && m.d[m.n - 1] == '0')
    --m.n;

  // Fixed (%f style) and scientific (%e style); the shorter wins, fixed on a tie.
  std::string fixed;
  if (m.x >= m.n - 1) {
    fixed.append(m.d, static_cast<std::size_t>(m.n));
    fixed.append(static_cast<std::size_t>(m.x - (m.n - 1)), '0');
  } else if (m.x >= 0) {
    fixed.append(m.d, static_cast<std::size_t>(m.x + 1));
    fixed.push_back('.');
    fixed.append(m.d + m.x + 1, static_cast<std::size_t>(m.n - m.x - 1));
  } else {
    fixed.append("0.");
    fixed.append(static_cast<std::size_t>(-m.x - 1), '0');
    fixed.append(m.d, static_cast<std::size_t>(m.n));
  }
  char sci[64];
  const int nsci = write_e(m, sci);
  if (fixed.size() <= static_cast<std::size_t>(nsci))
    return r += fixed;
  return r.append(sci, static_cast<std::size_t>(nsci));
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
