// [rand.predef]: the predefined engines are the stated specializations;
// default_random_engine is an engine of implementation-defined type ([rand.predef]/10).
#include <random>
#include <cstdint>
#include <concepts>
#include <type_traits>

using namespace std;
static_assert(is_same_v<minstd_rand0, linear_congruential_engine<uint_fast32_t, 16'807, 0, 2'147'483'647>>);
static_assert(is_same_v<minstd_rand, linear_congruential_engine<uint_fast32_t, 48'271, 0, 2'147'483'647>>);
static_assert(is_same_v<mt19937, mersenne_twister_engine<uint_fast32_t, 32, 624, 397, 31, 0x9908'b0df, 11,
                                                         0xffff'ffff, 7, 0x9d2c'5680, 15, 0xefc6'0000, 18,
                                                         1'812'433'253>>);
static_assert(is_same_v<mt19937_64,
                        mersenne_twister_engine<uint_fast64_t, 64, 312, 156, 31, 0xb502'6f5a'a966'19e9, 29,
                                                0x5555'5555'5555'5555, 17, 0x71d6'7fff'eda6'0000, 37,
                                                0xfff7'eee0'0000'0000, 43, 6'364'136'223'846'793'005>>);
static_assert(is_same_v<ranlux24_base, subtract_with_carry_engine<uint_fast32_t, 24, 10, 24>>);
static_assert(is_same_v<ranlux48_base, subtract_with_carry_engine<uint_fast64_t, 48, 5, 12>>);
static_assert(is_same_v<ranlux24, discard_block_engine<ranlux24_base, 223, 23>>);
static_assert(is_same_v<ranlux48, discard_block_engine<ranlux48_base, 389, 11>>);
static_assert(is_same_v<knuth_b, shuffle_order_engine<minstd_rand0, 256>>);
static_assert(is_same_v<philox4x32, philox_engine<uint_fast32_t, 32, 4, 10, 0xCD9E8D57, 0x9E3779B9,
                                                  0xD2511F53, 0xBB67AE85>>);
static_assert(is_same_v<philox4x64,
                        philox_engine<uint_fast64_t, 64, 4, 10, 0xCA5A826395121157, 0x9E3779B97F4A7C15,
                                      0xD2E7470EE14C6C93, 0xBB67AE8584CAA73B>>);

static_assert(uniform_random_bit_generator<default_random_engine>);
static_assert(is_default_constructible_v<default_random_engine>);
static_assert(is_constructible_v<default_random_engine, default_random_engine::result_type>);
static_assert(requires(default_random_engine e, seed_seq& q, unsigned long long z) {
  e.seed();
  e.seed(q);
  e.discard(z);
  { e == e } -> same_as<bool>;
});


int main() {}
