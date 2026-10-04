// Freestanding smoke test: exercises core headers with no OS, no libc, no exceptions, no RTTI.
#include <atomic>
#include <bit>
#include <cassert>
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
  return r;
}
