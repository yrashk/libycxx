// [rand.eng.mers]: the transition and generation algorithms of mersenne_twister_engine
// (twisted GFSR with tempering), seeding from a value (/6), and the engine characteristics.
#include <random>
#include <cstdint>
#include <type_traits>
#include "check.hpp"
#include "mt_reference.hpp"

using mt24 = std::mersenne_twister_engine<std::uint32_t, 24, 17, 5, 7, 0x9a3b1c, 7, 0xffffff, 5, 0x5a5a5a, 11,
                                          0xf0f0f0, 9, 69069>;
using mt40 = std::mersenne_twister_engine<std::uint64_t, 40, 23, 11, 13, 0xab12cd34efull, 13, 0xfffffffffull, 7,
                                          0x5555555555ull, 15, 0x7777700000ull, 17, 1812433253ull>;
using mt_m_eq_n = std::mersenne_twister_engine<std::uint32_t, 32, 11, 11, 5, 0x9908b0df, 11, 0xffffffff, 7,
                                               0x9d2c5680, 15, 0xefc60000, 18, 1812433253>;
using mt_m_1 = std::mersenne_twister_engine<std::uint32_t, 32, 9, 1, 3, 0x9908b0df, 11, 0xffffffff, 7,
                                            0x9d2c5680, 15, 0xefc60000, 18, 1812433253>;

template <class E>
void compare(typename E::result_type seed, int count) {
  E e(seed);
  ref_mt<E> r(seed);
  for (int i = 0; i < count; ++i) {
    auto v = e();
    CHECK(v == r());
    CHECK(v <= E::max());
  }
}

static_assert(std::mt19937::word_size == 32 && std::mt19937::state_size == 624 && std::mt19937::shift_size == 397);
static_assert(std::mt19937::mask_bits == 31 && std::mt19937::xor_mask == 0x9908b0df);
static_assert(std::mt19937::tempering_u == 11 && std::mt19937::tempering_d == 0xffffffff);
static_assert(std::mt19937::tempering_s == 7 && std::mt19937::tempering_b == 0x9d2c5680);
static_assert(std::mt19937::tempering_t == 15 && std::mt19937::tempering_c == 0xefc60000);
static_assert(std::mt19937::tempering_l == 18 && std::mt19937::initialization_multiplier == 1812433253);
static_assert(std::mt19937::default_seed == 5489u);
static_assert(std::is_same_v<decltype(std::mt19937::default_seed), const std::mt19937::result_type>);
static_assert(std::is_same_v<decltype(std::mt19937::word_size), const std::size_t>);
static_assert(std::is_same_v<decltype(std::mt19937::xor_mask), const std::mt19937::result_type>);
static_assert(std::mt19937::min() == 0 && std::mt19937::max() == 0xffffffffu);
static_assert(std::mt19937_64::max() == ~0ull);
static_assert(mt24::max() == 0xffffffu && mt40::max() == 0xffffffffffull);

int main() {
  std::mt19937 a;
  CHECK(a() == 3499211612u);  // the well-known first output for seed 5489
  std::mt19937_64 b;
  CHECK(b() == 14514284786278117030ull);

  compare<std::mt19937>(5489, 2000);
  compare<std::mt19937>(0, 2000);
  compare<std::mt19937>(0xffffffffu, 1500);
  compare<std::mt19937_64>(5489, 1000);
  compare<std::mt19937_64>(0x123456789abcdef0ull, 1000);
  compare<mt24>(1, 500);
  compare<mt24>(0xfedcba98u, 500);  // reduced mod 2^24
  compare<mt40>(42, 500);
  compare<mt40>(~0ull, 500);        // reduced mod 2^40
  compare<mt_m_eq_n>(7, 300);
  compare<mt_m_1>(7, 300);

  // Reduction of the seed: X_{-n} = value mod 2^w.
  CHECK(mt24(0x1000005u) == mt24(5u));
  CHECK(mt40((1ull << 40) + 9) == mt40(9));

  // The state size is n: the outputs of independently seeded engines differ, of equal seeds agree.
  std::mt19937 c(1), d(1), f(2);
  CHECK(c == d && c != f);
}
