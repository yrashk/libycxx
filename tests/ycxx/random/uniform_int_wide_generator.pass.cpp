// [rand.req.urng] permits extended unsigned generator types; [rand.dist.uni.int] and
// [rand.req.dist] require valid support with any such generator. Wider ranges must not be
// narrowed before an unbiased reduction. The bit-coverage assertion is a user-approved
// statistical smoke check, assuming ideal independent draws, and can have outliers.
// A 65-bit source range used to narrow to zero and cause unbounded recursion in libycxx.
#include <random>
#include "check.hpp"
template<unsigned __int128 Min, unsigned __int128 Max>
struct Wide {
  using result_type = unsigned __int128;
  static constexpr result_type min() { return Min; }
  static constexpr result_type max() { return Max; }
  std::mt19937_64 engine{12345};
  result_type operator()() {
    for (;;) {
      result_type value = (result_type(engine()) << 64) | engine();
      if constexpr (Max - Min != ~result_type(0))
        value &= (result_type(1) << 66) - 1;
      if (value <= Max - Min) return Min + value;
    }
  }
};
template<class G> void check(G generator) {
  std::uniform_int_distribution<> small(-3, 3);
  for (int i = 0; i < 1000; ++i) {
    int value = small(generator);
    CHECK(-3 <= value && value <= 3);
  }
  std::uniform_int_distribution<unsigned long long> full;
  unsigned long long all_or = 0, all_and = ~0ull;
  for (int i = 0; i < 1000; ++i) {
    auto value = full(generator);
    all_or |= value;
    all_and &= value;
  }
  CHECK(all_or == ~0ull && all_and == 0);  // labeled statistical smoke coverage
}
int main() {
  using UInt = unsigned __int128;
  static_assert(std::uniform_random_bit_generator<Wide<0, UInt(1) << 65>>);
  check(Wide<0, UInt(1) << 65>{});  // nonpower R = 2^65 + 1
  check(Wide<0, ~UInt(0)>{});      // full 128-bit source
  constexpr UInt min = UInt(1) << 96;
  check(Wide<min, min + (UInt(1) << 65)>{});  // high nonzero minimum
}
