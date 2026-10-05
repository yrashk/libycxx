// operator>> for every standard integer type at and beyond its limits, in each basefield,
// with long runs of leading zeros and with nothing convertible.
// [facet.num.get.virtuals] Stage 1 (Table 95): %o for oct, %X for hex, %i for basefield 0,
// else %d/%u; Stage 2 accumulates every character that may continue the field (so an
// overflowing field is consumed whole); Stage 3 converts with strtoll/strtoull and stores
// "zero, if the conversion function does not convert the entire field", "the most positive (or
// negative) representable value, if the field to be converted to a signed integer type
// represents a value too large positive (or negative)", "the most positive representable
// value, if the field to be converted to an unsigned integer type represents a value that
// cannot be represented", "the converted value, otherwise", with failbit in the first three
// cases; /5: eofbit when Stage 2 ended at the end of input. [istream.formatted.arithmetic]/2-3:
// short and int are read as long and then clamped to their range with failbit.
// (Negative values for unsigned types are covered elsewhere.)
#include <sstream>
#include <string>
#include <limits>
#include <ios>
#include "check.hpp"

template <class T>
struct Res {
  T v;
  bool fail, eof;
  std::string rest;
};

template <class T>
static Res<T> get(const std::string& in, std::ios_base::fmtflags base) {
  std::istringstream is(in);
  is.setf(base, std::ios_base::basefield);
  T v = T(77);
  is >> v;
  Res<T> r{v, is.fail(), is.eof(), {}};
  is.clear();
  std::getline(is, r.rest);
  return r;
}

template <class T>
static void expect(const std::string& in, std::ios_base::fmtflags base, T v, bool fail, const char* rest = "") {
  Res<T> r = get<T>(in, base);
  if (r.v != v || r.fail != fail || r.rest != rest)
    dprintf(2, "\"%s\" (base flags %x): got %lld fail=%d rest=\"%s\"\n", in.c_str(), static_cast<unsigned>(base),
            static_cast<long long>(r.v), r.fail, r.rest.c_str());
  CHECK(r.v == v && r.fail == fail && r.rest == rest);
  CHECK(r.eof == (*rest == '\0'));
}

static std::string dec(unsigned long long m, bool neg) { return (neg ? "-" : "") + std::to_string(m); }

static std::string in_base(unsigned long long m, int base) {
  if (m == 0) return "0";
  std::string r;
  while (m) {
    r.insert(r.begin(), "0123456789abcdef"[m % static_cast<unsigned>(base)]);
    m /= static_cast<unsigned>(base);
  }
  return r;
}

// the magnitude plus one, as a decimal string (works for ULLONG_MAX too)
static std::string plus_one(std::string s) {
  int i = static_cast<int>(s.size()) - 1;
  while (i >= 0 && s[static_cast<std::size_t>(i)] == '9') s[static_cast<std::size_t>(i--)] = '0';
  if (i < 0) s.insert(s.begin(), '1');
  else ++s[static_cast<std::size_t>(i)];
  return s;
}

template <class T>
static void run() {
  using L = std::numeric_limits<T>;
  const auto dec_f = std::ios_base::dec;
  const auto hex_f = std::ios_base::hex;
  const auto oct_f = std::ios_base::oct;
  const std::ios_base::fmtflags any_f{};
  const unsigned long long maxm = static_cast<unsigned long long>(L::max());
  // the maximum, and one more
  expect<T>(dec(maxm, false), dec_f, L::max(), false);
  expect<T>(plus_one(dec(maxm, false)), dec_f, L::max(), true);
  expect<T>("+" + dec(maxm, false), dec_f, L::max(), false);
  expect<T>(std::string(40, '0') + dec(maxm, false), dec_f, L::max(), false);
  expect<T>("123456789012345678901234567890 tail", dec_f, L::max(), true, " tail");
  expect<T>(in_base(maxm, 16), hex_f, L::max(), false);
  expect<T>(in_base(maxm, 8), oct_f, L::max(), false);
  expect<T>("0x" + in_base(maxm, 16), any_f, L::max(), false);
  expect<T>("0" + in_base(maxm, 8), any_f, L::max(), false);
  expect<T>(in_base(maxm, 10), any_f, L::max(), false);
  expect<T>(std::string(30, 'f'), hex_f, L::max(), true);
  expect<T>(std::string(30, '7') + "8", oct_f, L::max(), true, "8");
  if constexpr (L::is_signed) {
    const unsigned long long minm = 0ull - static_cast<unsigned long long>(L::min());
    expect<T>(dec(minm, true), dec_f, L::min(), false);
    expect<T>("-" + plus_one(dec(minm, false)), dec_f, L::min(), true);
    expect<T>("-" + std::string(25, '0') + dec(minm, false), dec_f, L::min(), false);
    expect<T>("-99999999999999999999999", dec_f, L::min(), true);
    expect<T>("-" + in_base(minm, 16), hex_f, L::min(), false);
    expect<T>("-0x" + in_base(minm, 16), any_f, L::min(), false);
    expect<T>("-0" + in_base(minm, 8), any_f, L::min(), false);
    expect<T>("-" + in_base(minm + 1, 16), hex_f, L::min(), true);
    expect<T>("-0", dec_f, T(0), false);
  } else {
    expect<T>(in_base(maxm, 16) + "0", hex_f, L::max(), true);
  }
  expect<T>("0", dec_f, T(0), false);
  expect<T>("+0", dec_f, T(0), false);
  expect<T>(std::string(70, '0'), dec_f, T(0), false);
  expect<T>(std::string(70, '0') + "1x", dec_f, T(1), false, "x");
  expect<T>("1a", dec_f, T(1), false, "a");
  expect<T>("1A", hex_f, T(26), false);
  expect<T>("19", oct_f, T(1), false, "9");
  expect<T>("010", any_f, T(8), false);
  expect<T>("0X1f", any_f, T(31), false);
  // nothing convertible: zero and failbit
  expect<T>("-", dec_f, T(0), true);
  expect<T>("+", dec_f, T(0), true);
  expect<T>("x", dec_f, T(0), true, "x");
  expect<T>("g", hex_f, T(0), true, "g");
  // eofbit only, no failbit, when reading stopped at the end of a valid field
  Res<T> r = get<T>("42", dec_f);
  CHECK(r.v == T(42) && !r.fail && r.eof);
  // successive extractions keep going after a clamped one
  std::istringstream is("99999999999999999999999 7");
  T a{}, b{};
  is >> a;
  CHECK(a == L::max() && is.fail());
  is.clear();
  is >> b;
  CHECK(b == T(7) && !is.fail());
}

int main() {
  run<short>();
  run<unsigned short>();
  run<int>();
  run<unsigned>();
  run<long>();
  run<unsigned long>();
  run<long long>();
  run<unsigned long long>();
  return 0;
}
