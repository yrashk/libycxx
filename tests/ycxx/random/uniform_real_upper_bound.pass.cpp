// [rand.dist.uni.real]/1: "produces random numbers x, a <= x < b". The upper bound is excluded
// even for maximum engine draws and bounds where
// a + (b - a) * u can round up to b.
#include <random>
#include <cstdint>
#include "check.hpp"

int main() {
  // Standard full-period increment engines supply extreme draws, then progress
  // through their range if a distribution uses rejection sampling.
  using E32 = std::linear_congruential_engine<std::uint32_t, 1, 1, 0>;
  using E64 = std::linear_congruential_engine<std::uint64_t, 1, 1, 0>;
  E32 g;
  E64 h;

  std::uniform_real_distribution<float> f01(0.0f, 1.0f), f12(1.0f, 2.0f), fneg(-1.0f, 0.0f);
  std::uniform_real_distribution<double> d01(0.0, 1.0), d12(1.0, 2.0), dbig(1e10, 1e10 + 1);
  std::uniform_real_distribution<long double> l01(0.0L, 1.0L);
  auto check_interval = [](auto& distribution, auto& engine) {
    engine.seed(engine.max() - 1);  // first draw is max, later draws progress through min
    auto value = distribution(engine);
    CHECK(value >= distribution.a() && value < distribution.b());
  };
  for (int i = 0; i < 4; ++i) {
    check_interval(f01, g);
    check_interval(f12, g);
    check_interval(fneg, g);
    check_interval(d01, g);
    check_interval(d12, g);
    check_interval(dbig, g);
    check_interval(f12, h);
    check_interval(d12, h);
    check_interval(l01, h);
  }
  // The mapping of individual engine outputs is unspecified; minimum inputs stay in range.
  E32 z(E32::max());  // first draw is min; no required input-to-output mapping
  auto low_float = f12(z);
  z.seed(E32::max());
  auto low_double = d01(z);
  CHECK(low_float >= 1.0f && low_float < 2.0f);
  CHECK(low_double >= 0.0 && low_double < 1.0);
}
