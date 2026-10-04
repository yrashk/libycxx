// [rand.eng.sub]: subtract_with_carry_engine transition (Y = X_{i-s} - X_{i-r} - c, X_i = Y mod m,
// c = (Y < 0)), seeding from a value through linear_congruential_engine<uint_least32_t, 40014u,
// 0u, 2147483563u> (/7), the default constructor (seed 0u, so default_seed 19780503u is used),
// and the engine characteristics.
#include <random>
#include <cstdint>
#include <type_traits>
#include "check.hpp"
#include "swc_reference.hpp"

using swc64 = std::subtract_with_carry_engine<std::uint64_t, 64, 7, 19>;
using swc32 = std::subtract_with_carry_engine<std::uint32_t, 32, 3, 11>;
using swc33 = std::subtract_with_carry_engine<std::uint64_t, 33, 4, 9>;
using swc5 = std::subtract_with_carry_engine<std::uint16_t, 5, 2, 3>;

template <class E>
void compare(typename E::result_type seed, int count) {
  E e(seed);
  ref_swc<E> ref(seed);
  for (int i = 0; i < count; ++i) {
    auto v = e();
    CHECK(v == ref());
    CHECK(v <= E::max());
  }
}

static_assert(std::ranlux24_base::word_size == 24 && std::ranlux24_base::short_lag == 10 &&
              std::ranlux24_base::long_lag == 24);
static_assert(std::is_same_v<decltype(std::ranlux24_base::word_size), const std::size_t>);
static_assert(std::ranlux24_base::default_seed == 19780503u);
static_assert(std::is_same_v<decltype(std::ranlux48_base::default_seed), const std::uint_least32_t>);
static_assert(std::ranlux24_base::min() == 0 && std::ranlux24_base::max() == (1u << 24) - 1);
static_assert(std::ranlux48_base::max() == (1ull << 48) - 1);
static_assert(swc64::max() == ~0ull && swc32::max() == 0xffffffffu && swc5::max() == 31);

int main() {
  compare<std::ranlux24_base>(0, 3000);
  compare<std::ranlux24_base>(19780503u, 3000);
  compare<std::ranlux24_base>(2147483563u, 500);  // value % 2147483563 == 0: the LCG starts at 1
  compare<std::ranlux24_base>(0xffffffffu, 500);
  compare<std::ranlux48_base>(0, 3000);
  compare<std::ranlux48_base>((1ull << 40) + 3, 500);  // reduced mod 2147483563 before the cast
  compare<swc64>(1, 2000);
  compare<swc32>(77, 2000);
  compare<swc33>(5, 2000);
  compare<swc5>(9, 500);

  CHECK(std::ranlux24_base() == std::ranlux24_base(0u));
  CHECK(std::ranlux24_base() == std::ranlux24_base(19780503u));
  CHECK(std::ranlux48_base() == std::ranlux48_base(19780503u));
  std::ranlux24_base e(5);
  e();
  e.seed();
  CHECK(e == std::ranlux24_base());
  e.seed(7);
  CHECK(e == std::ranlux24_base(7));
}
