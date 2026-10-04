// Freestanding smoke test: exercises core headers with no OS, no libc, no exceptions, no RTTI.
#include <atomic>
#include <bit>
#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <stdbit.h>
#include <system_error>
#include <charconv>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace {
int sum(int n, ...) {
  std::va_list ap;
  va_start(ap, n);
  int s = 0;
  for (int i = 0; i != n; ++i)
    s += va_arg(ap, int);
  va_end(ap);
  return s;
}
int by_value(const void* a, const void* b) {
  int x = *static_cast<const int*>(a), y = *static_cast<const int*>(b);
  return (x > y) - (x < y);
}
struct Pt {
  int x, y;
  auto operator<=>(const Pt&) const = default;
};
} // namespace

extern "C" int ycxx_freestanding_main() {
  int r = 0;
  std::pair<int, long> p{1, 2};
  auto [a, b] = p;
  r += a + static_cast<int>(b);
  r += std::popcount(0xffu) + std::bit_width(5u);
  r += std::numeric_limits<std::int8_t>::max() > 100;
  r += (Pt{1, 2} < Pt{1, 3});
  alignas(int) unsigned char buf[sizeof(int)];
  int* ip = std::construct_at(reinterpret_cast<int*>(buf), 5);
  r += *ip;
  std::destroy_at(ip);
  // <charconv>: integers in the header, floating point from the runtime archive.
  char text[32];
  auto tc = std::to_chars(text, text + sizeof text, 0.1 * r);
  double back = 0;
  if (std::from_chars(text, tc.ptr, back) && back == 0.1 * r)
    ++r;
  int i = 0;
  tc = std::to_chars(text, text + sizeof text, r, 7);
  if (std::from_chars(text, tc.ptr, i, 7) && i == r)
    ++r;
  // <atomic>: lock-free operations inline, a lock-based type and waiting from the runtime archive.
  std::atomic<int> ai(r);
  ai.fetch_add(1);
  ai.wait(0); // returns: the value differs
  ai.notify_all();
  struct big {
    long a, b, c;
  };
  std::atomic<big> ab(big{1, 2, 3});
  big e{1, 2, 3};
  if (ab.compare_exchange_strong(e, big{4, 5, 6}) && ab.load().c == 6)
    ++r;
  r += ai.load() - r;
  assert(r > 0);
  // The freestanding subsets of the C library headers, without a C library.
  char s1[16];
  std::strcpy(s1, "freestanding");
  r += static_cast<int>(std::strlen(s1)) + (std::strchr(s1, 'n') - s1) + (std::strstr(s1, "stand") != nullptr);
  r += std::memccpy(s1, "a=b", '=', 3) != nullptr;
  std::memset_explicit(s1, 0, sizeof s1);
  r += static_cast<int>(std::wcslen(L"wide")) + std::wcscmp(L"a", L"b");
  int v[] = {3, 1, 2};
  std::qsort(v, 3, sizeof(int), by_value);
  int key = 2;
  r += std::bsearch(&key, v, 3, sizeof(int), by_value) == v + 1;
  r += std::abs(-3) + std::div(7, 2).rem + static_cast<int>(std::memalignment(v));
  r += sum(3, 1, 2, 3) + (ERANGE == static_cast<int>(std::errc::result_out_of_range));
  r += static_cast<int>(stdc_count_ones(0xf0u) + stdc_bit_ceil_uc(5));
  return r;
}
