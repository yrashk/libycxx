// [rand.eng.lcong]: TA(x) = (a*x + c) mod m, GA(x) = TA(x); m == 0 means
// numeric_limits<result_type>::max() + 1. min() is 1 if c == 0 else 0; max() is m - 1u.
// E(s): "If c mod m is 0 and s mod m is 0, sets the engine's state to 1, otherwise sets the
// engine's state to s mod m." default_seed is 1u.
#include <random>
#include <cstdint>
#include <limits>
#include <type_traits>
#include "check.hpp"

using u128 = unsigned __int128;

template <class E>
struct ref_lcg {
  using T = typename E::result_type;
  static constexpr u128 M = E::modulus == 0 ? u128(std::numeric_limits<T>::max()) + 1 : u128(E::modulus);
  u128 x;
  explicit ref_lcg(u128 s) {
    x = (u128(E::increment) % M == 0 && s % M == 0) ? 1 : s % M;
  }
  T operator()() {
    x = (u128(E::multiplier) * x + E::increment) % M;
    return T(x);
  }
};

template <class E>
void compare(typename E::result_type s, int n = 2000) {
  E e(s);
  ref_lcg<E> r(s);
  for (int i = 0; i < n; ++i) {
    auto v = e();
    CHECK(v == r());
    CHECK(E::min() <= v && v <= E::max());
  }
}

using minstd0 = std::minstd_rand0;
using numrec = std::linear_congruential_engine<std::uint32_t, 1664525u, 1013904223u, 0u>;
using knuth64 = std::linear_congruential_engine<std::uint64_t, 6364136223846793005ull, 1442695040888963407ull, 0u>;
using lcg48 = std::linear_congruential_engine<std::uint64_t, 25214903917ull, 11u, 1ull << 48>;
using big = std::linear_congruential_engine<std::uint64_t, 3512401965023503517ull, 0u, (1ull << 63) - 25>;
using small_c = std::linear_congruential_engine<std::uint32_t, 3u, 1u, 10u>;
using small_0 = std::linear_congruential_engine<std::uint32_t, 3u, 0u, 7u>;
using ushort_e = std::linear_congruential_engine<unsigned short, 75u, 74u, 0u>;

static_assert(std::is_same_v<numrec::result_type, std::uint32_t>);
static_assert(numrec::multiplier == 1664525u && numrec::increment == 1013904223u && numrec::modulus == 0u);
static_assert(minstd0::default_seed == 1u);
static_assert(std::is_same_v<decltype(minstd0::default_seed), const minstd0::result_type>);
static_assert(minstd0::min() == 1 && minstd0::max() == 2147483646u);
static_assert(numrec::min() == 0 && numrec::max() == 0xffffffffu);
static_assert(knuth64::min() == 0 && knuth64::max() == ~0ull);
static_assert(lcg48::max() == (1ull << 48) - 1);
static_assert(small_c::min() == 0 && small_c::max() == 9);
static_assert(small_0::min() == 1 && small_0::max() == 6);
static_assert(ushort_e::max() == 65535);
static_assert(std::is_same_v<decltype(minstd0::min()), minstd0::result_type>);

int main() {
  // minstd_rand0 from the default seed 1: 16807, 282475249, 1622650073, ...
  minstd0 m;
  CHECK(m() == 16807u);
  CHECK(m() == 282475249u);
  CHECK(m() == 1622650073u);

  compare<minstd0>(1);
  compare<minstd0>(12345);
  compare<minstd0>(2147483647u);  // s mod m == 0 and c == 0: state 1
  compare<minstd0>(2147483648u + 5);
  CHECK(minstd0(2147483647u) == minstd0());
  CHECK(minstd0(0) == minstd0());
  CHECK(minstd0(2147483647u + 6u) == minstd0(6));
  compare<std::minstd_rand>(99);
  compare<numrec>(0);
  compare<numrec>(0xdeadbeefu);
  compare<knuth64>(0);
  compare<knuth64>(~0ull);
  compare<lcg48>(0x1234abcd5678ull);
  compare<lcg48>(~0ull);
  compare<big>(1);
  compare<big>(~0ull);
  compare<small_c>(10);  // c != 0: state s mod m == 0
  compare<small_c>(27);
  compare<small_0>(14);  // c == 0, s mod m == 0: state 1
  compare<ushort_e>(0);
  compare<ushort_e>(65535);

  small_c sc(10);
  CHECK(sc() == 1u);  // (3 * 0 + 1) mod 10
  small_0 s0(14);
  CHECK(s0() == 3u);  // state 1: 3 * 1 mod 7

  // seed(s) and seed() postconditions ([rand.req.eng]).
  numrec a(77), b;
  a();
  a.seed(5);
  CHECK(a == numrec(5));
  a.seed();
  CHECK(a == numrec());
  CHECK(numrec() == numrec(numrec::default_seed));
}
