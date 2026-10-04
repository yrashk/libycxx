// [rand.dist.uni.real]/1: "produces random numbers x, a <= x < b". The upper bound is excluded
// even for engines that keep returning their largest value and for bounds where
// a + (b - a) * u can round up to b.
#include <random>
#include <cstdint>
#include "check.hpp"
#include "random_support.hpp"

int main() {
  const std::uint32_t top32[] = {0xffffffffu};
  const std::uint64_t top64[] = {~0ull};
  rs::replay_urbg<std::uint32_t, 0, 0xffffffffu> g(top32, 1);
  rs::replay_urbg<std::uint64_t, 0, ~0ull> h(top64, 1);

  std::uniform_real_distribution<float> f01(0.0f, 1.0f), f12(1.0f, 2.0f), fneg(-1.0f, 0.0f);
  std::uniform_real_distribution<double> d01(0.0, 1.0), d12(1.0, 2.0), dbig(1e10, 1e10 + 1);
  std::uniform_real_distribution<long double> l01(0.0L, 1.0L);
  for (int i = 0; i < 4; ++i) {
    CHECK(f01(g) < 1.0f);
    CHECK(f12(g) < 2.0f);
    CHECK(fneg(g) < 0.0f);
    CHECK(d01(g) < 1.0);
    CHECK(d12(g) < 2.0);
    CHECK(dbig(g) < 1e10 + 1);
    CHECK(f12(h) < 2.0f);
    CHECK(d12(h) < 2.0);
    CHECK(l01(h) < 1.0L);
  }
  // And the lower bound is reached for an engine returning its minimum.
  const std::uint32_t zero[] = {0};
  rs::replay_urbg<std::uint32_t, 0, 0xffffffffu> z(zero, 1);
  CHECK(f12(z) == 1.0f);
  CHECK(d01(z) == 0.0);
}
