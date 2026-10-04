// [rand.util.canonical] (C++26 wording): with r = radix, R = g.max() - g.min() + 1,
// d = min(digits, numeric_limits<RealType>::digits), k the smallest integer with R^k >= r^d and
// x = floor(R^k / r^d), an attempt is k invocations of g giving S = sum (g_i - g.min()) * R^i;
// "Attempts are made until S < x r^d." Returns floor(S/x) / r^d, so 0 <= c < 1. "Complexity:
// Exactly k invocations of g per attempt."
#include <random>
#include <cstdint>
#include <limits>
#include <type_traits>
#include "check.hpp"
#include "random_support.hpp"

using g32 = rs::replay_urbg<std::uint32_t, 0, 0xffffffffu>;
using g64 = rs::replay_urbg<std::uint64_t, 0, ~0ull>;
using g10 = rs::replay_urbg<std::uint32_t, 0, 9>;
using g10_off = rs::replay_urbg<std::uint32_t, 5, 14>;
using g3 = rs::replay_urbg<std::uint32_t, 0, 2>;

int main() {
  static_assert(std::is_same_v<decltype(std::generate_canonical<float, 24>(std::declval<g32&>())), float>);
  static_assert(std::is_same_v<decltype(std::generate_canonical<long double, 64>(std::declval<g32&>())),
                               long double>);

  {  // R = 2^32, float: k = 1, x = 2^8, one attempt.
    const std::uint32_t v[] = {0xffffffffu, 0, 0x12345678u, 0xff};
    g32 g(v, 4);
    float c = std::generate_canonical<float, 24>(g);
    CHECK(c == float(0xffffff) / 16777216.0f);
    CHECK(c < 1.0f);
    CHECK(g.calls == 1);
    CHECK(std::generate_canonical<float, 24>(g) == 0.0f);
    CHECK(std::generate_canonical<float, 24>(g) == float(0x123456) / 16777216.0f);
    CHECK(std::generate_canonical<float, 24>(g) == 0.0f);  // 0xff / 2^8 rounds down to 0
    CHECK(g.calls == 4);
    // digits larger than numeric_limits<float>::digits: d = 24.
    g32 h(v, 4);
    CHECK(std::generate_canonical<float, 1000>(h) == float(0xffffff) / 16777216.0f);
    CHECK(h.calls == 1);
    // digits smaller: d = 8, k = 1, x = 2^24.
    g32 i(v, 4);
    CHECK(std::generate_canonical<float, 8>(i) == 255.0f / 256.0f);
  }
  {  // R = 2^32, double: k = 2, x = 2^11; S = g0 + g1 * 2^32.
    const std::uint32_t v[] = {0xffffffffu, 0xffffffffu, 0x00000800u, 0x00000001u};
    g32 g(v, 4);
    double c = std::generate_canonical<double, 53>(g);
    CHECK(c == double((1ull << 53) - 1) / 9007199254740992.0);
    CHECK(c < 1.0);
    CHECK(g.calls == 2);
    // S = 0x800 + 2^32 = 2^32 + 2^11: floor(S / 2^11) = 2^21 + 1.
    CHECK(std::generate_canonical<double, 53>(g) == double((1ull << 21) + 1) / 9007199254740992.0);
    CHECK(g.calls == 4);
  }
  {  // R = 2^64, double: k = 1, x = 2^11.
    const std::uint64_t v[] = {~0ull, 0x8000000000000000ull};
    g64 g(v, 2);
    double c = std::generate_canonical<double, 53>(g);
    CHECK(c == double((1ull << 53) - 1) / 9007199254740992.0);
    CHECK(std::generate_canonical<double, 53>(g) == 0.5);
    CHECK(g.calls == 2);
  }
  {  // R = 10, d = 4: r^d = 16, k = 2, x = 6, attempts until S < 96.
    const std::uint32_t v[] = {9, 9, 5, 3};  // S = 99 (rejected), then S = 5 + 3*10 = 35
    g10 g(v, 4);
    float c = std::generate_canonical<float, 4>(g);
    CHECK(c == 5.0f / 16.0f);  // floor(35 / 6) = 5
    CHECK(g.calls == 4);
    const std::uint32_t w[] = {5, 9};  // S = 95: accepted, floor(95/6) = 15
    g10 h(w, 2);
    CHECK(std::generate_canonical<double, 4>(h) == 15.0 / 16.0);
    CHECK(h.calls == 2);
    // g.min() is subtracted.
    const std::uint32_t o[] = {14, 14, 10, 8};
    g10_off k(o, 4);
    CHECK(std::generate_canonical<float, 4>(k) == 5.0f / 16.0f);
    CHECK(k.calls == 4);
  }
  {  // R = 3, float: 3^16 >= 2^24 > 3^15, so k = 16, x = 2; S = 3^16 - 1 >= 2^25 is rejected.
    std::uint32_t v[32];
    for (int i = 0; i < 16; ++i) v[i] = 2;
    for (int i = 16; i < 32; ++i) v[i] = 0;
    g3 g(v, 32);
    CHECK(std::generate_canonical<float, 24>(g) == 0.0f);
    CHECK(g.calls == 32);
  }
  {  // The result is always in [0, 1), here with an engine that only returns its maximum.
    const std::uint32_t v[] = {0xffffffffu};
    g32 g(v, 1);
    for (int i = 0; i < 4; ++i) {
      CHECK(std::generate_canonical<float, 24>(g) < 1.0f);
      CHECK(std::generate_canonical<double, 53>(g) < 1.0);
      CHECK(std::generate_canonical<long double, std::numeric_limits<long double>::digits>(g) < 1.0L);
      CHECK(std::generate_canonical<float, 64>(g) < 1.0f);
    }
  }
  {  // With a real engine: in [0, 1) and roughly uniform.
    std::mt19937 e(12345);
    double sum = 0;
    const int N = 100000;
    for (int i = 0; i < N; ++i) {
      double c = std::generate_canonical<double, 53>(e);
      CHECK(0.0 <= c && c < 1.0);
      sum += c;
    }
    CHECK(rs::near(sum / N, 0.5, 0.01));
    std::minstd_rand m(3);
    for (int i = 0; i < 10000; ++i) {
      float f = std::generate_canonical<float, 24>(m);
      CHECK(0.0f <= f && f < 1.0f);
    }
  }
}
