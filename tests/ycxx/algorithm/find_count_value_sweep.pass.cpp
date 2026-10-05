// [alg.find]/1: find returns "the first iterator i in the range [first, last) for which E is
// true", E being *i == value (ranges::find: invoke(proj, *i) == value), or last;
// [alg.count]: count is the number of iterators for which E holds; [alg.contains]: contains
// is ranges::find(...) != last; [alg.find.last]: find_last returns the last such i.
// *i == value uses the usual arithmetic conversions ([expr.arith.conv]), so a value outside the
// element type's range matches only an element that compares equal after promotion (never,
// for a narrower element type, after a "truncation"). Every element value of each narrow type
// against a range of int/long/unsigned values, at every position (and alignment) of long
// contiguous ranges and through non-contiguous iterators, compared with a plain loop.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <list>
#include <string>
#include <vector>
#include "check.hpp"

#pragma GCC diagnostic ignored "-Wsign-compare"

template <class T, class V>
std::ptrdiff_t first_eq(const T* a, std::ptrdiff_t n, V v) {
  for (std::ptrdiff_t i = 0; i < n; ++i)
    if (a[i] == v) return i;
  return n;
}
template <class T, class V>
std::ptrdiff_t last_eq(const T* a, std::ptrdiff_t n, V v) {
  for (std::ptrdiff_t i = n; i-- > 0;)
    if (a[i] == v) return i;
  return n;
}
template <class T, class V>
std::ptrdiff_t count_eq(const T* a, std::ptrdiff_t n, V v) {
  std::ptrdiff_t c = 0;
  for (std::ptrdiff_t i = 0; i < n; ++i) c += a[i] == v;
  return c;
}

template <class T, class V>
void one(const T* a, std::ptrdiff_t n, V v) {
  const std::ptrdiff_t f = first_eq(a, n, v);
  CHECK(std::find(a, a + n, v) - a == f);
  CHECK(std::ranges::find(a, a + n, v) - a == f);
  CHECK(std::count(a, a + n, v) == count_eq(a, n, v));
  CHECK(std::ranges::count(a, a + n, v) == count_eq(a, n, v));
  CHECK(std::ranges::contains(a, a + n, v) == (f != n));
  CHECK(std::ranges::find_last(a, a + n, v).begin() - a == last_eq(a, n, v));
}

template <class T>
void sweep_type() {
  // element values: every value of T (for 8-bit types) or boundary values
  std::vector<T> vals;
  if constexpr (sizeof(T) == 1) {
    for (int i = 0; i < 256; ++i) vals.push_back(static_cast<T>(i));
  } else {
    for (long long x : {0LL, 1LL, -1LL, 127LL, 128LL, 255LL, 256LL, 32767LL, -32768LL, 65535LL, 2147483647LL,
                        -2147483647LL - 1, 4294967295LL})
      vals.push_back(static_cast<T>(x));
  }
  // long array: each element value appears once at an interesting position, among filler
  constexpr std::ptrdiff_t N = 200;
  T arr[N + 8];
  for (std::size_t vi = 0; vi < vals.size(); ++vi) {
    const T x = vals[vi];
    const T filler = static_cast<T>(x == static_cast<T>(7) ? 9 : 7);
    const std::ptrdiff_t pos = static_cast<std::ptrdiff_t>((vi * 37) % N);
    {
      const std::ptrdiff_t start = static_cast<std::ptrdiff_t>(vi % 8);  // alignment of the first element
      T* a = arr + start;
      for (std::ptrdiff_t i = 0; i < N; ++i) a[i] = filler;
      a[pos] = x;
      if (vi % 3 == 0 && pos + 50 < N) a[pos + 50] = x;  // a later duplicate
      for (long long v : {static_cast<long long>(x), static_cast<long long>(x) + 256, static_cast<long long>(x) - 256,
                          static_cast<long long>(x) + 65536, static_cast<long long>(x) + 4294967296LL, -1LL, 255LL,
                          -128LL, 128LL, 300LL, -300LL}) {
        one(a, N, static_cast<int>(v));
        one(a, N, v);
        one(a, N, static_cast<unsigned>(v));
        one(a, N, static_cast<unsigned long long>(v));
        one(a, N, static_cast<T>(v));
      }
      // every prefix length around the match (short ranges)
      for (std::ptrdiff_t len = 0; len < 40 && pos + len <= N; ++len) one(a + pos - (pos > 3 ? 3 : 0), len, x);
    }
  }
}

// non-contiguous iterators give the same answers
static void non_contiguous() {
  std::deque<signed char> d;
  std::list<unsigned char> l;
  for (int i = 0; i < 600; ++i) {
    d.push_back(static_cast<signed char>(i * 7));
    l.push_back(static_cast<unsigned char>(i * 7));
  }
  for (int v = -300; v <= 600; v += 7) {
    auto di = std::find(d.begin(), d.end(), v);
    auto li = std::find(l.begin(), l.end(), v);
    std::ptrdiff_t dn = 0, ln = 0;
    for (auto it = d.begin(); it != d.end() && *it != v; ++it) ++dn;
    for (auto it = l.begin(); it != l.end() && *it != v; ++it) ++ln;
    CHECK(di - d.begin() == dn);
    CHECK(std::distance(l.begin(), li) == ln);
    CHECK(std::ranges::count(d, v) == std::count_if(d.begin(), d.end(), [v](signed char c) { return c == v; }));
  }
}

// strings, char8_t and std::byte
static void others() {
  std::string s(1000, 'a');
  s[500] = '\xff';
  CHECK(std::find(s.begin(), s.end(), '\xff') - s.begin() == 500);
  CHECK(std::find(s.begin(), s.end(), 255) == s.end() || static_cast<char>(-1) == 255);  // char signed: no match
  CHECK(std::find(s.begin(), s.end(), -1) - s.begin() == (static_cast<char>(-1) == -1 ? 500 : 1000));
  std::u8string u(100, u8'x');
  u[60] = static_cast<char8_t>(0xe9);
  CHECK(std::find(u.begin(), u.end(), 0xe9) - u.begin() == 60);
  CHECK(std::find(u.begin(), u.end(), -23) == u.end());
  std::byte b[64] = {};
  b[33] = std::byte{0x80};
  CHECK(std::find(b, b + 64, std::byte{0x80}) == b + 33);
  CHECK(std::ranges::count(b, std::byte{0}) == 63);
  bool bs[50] = {};
  bs[20] = true;
  CHECK(std::find(bs, bs + 50, 2) == bs + 50);  // true == 2 is false (promotion to int)
  CHECK(std::find(bs, bs + 50, 1) == bs + 20);
  CHECK(std::find(bs, bs + 50, true) == bs + 20);
}

int main() {
  sweep_type<char>();
  sweep_type<signed char>();
  sweep_type<unsigned char>();
  sweep_type<std::int8_t>();
  sweep_type<char8_t>();
  sweep_type<short>();
  sweep_type<unsigned short>();
  sweep_type<int>();
  sweep_type<unsigned>();
  sweep_type<char16_t>();
  sweep_type<char32_t>();
  sweep_type<wchar_t>();
  non_contiguous();
  others();
}
