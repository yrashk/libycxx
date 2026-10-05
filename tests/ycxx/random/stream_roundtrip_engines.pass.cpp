// [rand.req.eng] Table 127, os << x: "With os.fmtflags set to ios_base::dec|ios_base::left and
// the fill character set to the space character, writes to os the textual representation of
// x's current state"; "Postconditions: The os.fmtflags and fill character are unchanged."
// is >> v: "With is.fmtflags set to ios_base::dec, sets v's state as determined by reading its
// textual representation from is." "If a textual representation written via os << x was
// subsequently read via is >> v, then x == v provided that there have been no intervening
// invocations of x or of v." "Postconditions: The is.fmtflags are unchanged."
// [rand.req.eng]/3: x == y means the infinite sequences of future values are equal.
// The streams here carry hostile formatting state (hex, showbase, showpos, uppercase,
// internal adjustment, a non-space fill, and on input skipws cleared): the operators set the
// flags they need themselves. Every engine and adaptor of [rand.eng] / [rand.adapt] and every
// predefined engine of [rand.predef] (philox: random/stream_roundtrip_philox), in the
// default-constructed state, after an odd number of calls (a mid-buffer position for
// mersenne_twister, subtract_with_carry, discard_block and shuffle_order), and after
// seeding; through char and wchar_t streams, and with several engines in one stream.
#include <random>
#include <sstream>
#include <string>
#include "check.hpp"

using B = std::ios_base;
constexpr B::fmtflags hostile_out = B::hex | B::showbase | B::showpos | B::uppercase | B::internal |
                                    B::boolalpha | B::scientific;
constexpr B::fmtflags hostile_in = B::hex | B::showbase;  // skipws cleared

template <class E, class Ch>
void roundtrip_one(const E& x) {
  std::basic_ostringstream<Ch> os;
  os.flags(hostile_out);
  os.fill(Ch('*'));
  os << x;
  CHECK(os.good());
  CHECK(os.flags() == hostile_out);
  CHECK(os.fill() == Ch('*'));

  std::basic_istringstream<Ch> is(os.str());
  is.flags(hostile_in);
  E v(12345u);  // a different state to be overwritten
  is >> v;
  CHECK(!is.fail());
  CHECK(is.flags() == hostile_in);
  CHECK(v == x);
  E xc = x;
  for (int i = 0; i < 2000; ++i) CHECK(xc() == v());
}

template <class E>
void roundtrip(const E& x) {
  roundtrip_one<E, char>(x);
  roundtrip_one<E, wchar_t>(x);
}

template <class E>
void check_engine() {
  E e;
  roundtrip(e);
  e();
  roundtrip(e);
  for (int i = 0; i < 300; ++i) e();  // 301 calls in total: odd, mid-buffer
  roundtrip(e);
  E s(42u);
  roundtrip(s);
  std::seed_seq q{1u, 2u, 3u};
  E t(q);
  t.discard(7);
  roundtrip(t);

  // Two engines in one stream, the representations separated by a space (skipped with ws:
  // the separator is not part of either representation).
  std::stringstream ss;
  ss << e << ' ' << t;
  E a, b;
  ss >> a >> std::ws >> b;
  CHECK(!ss.fail() && a == e && b == t);
}

int main() {
  check_engine<std::minstd_rand0>();
  check_engine<std::minstd_rand>();
  check_engine<std::mt19937>();
  check_engine<std::mt19937_64>();
  check_engine<std::ranlux24_base>();
  check_engine<std::ranlux48_base>();
  check_engine<std::ranlux24>();
  check_engine<std::ranlux48>();
  check_engine<std::knuth_b>();
  check_engine<std::default_random_engine>();
  // Engines with full-width or unusual parameters.
  check_engine<std::linear_congruential_engine<std::uint64_t, 6364136223846793005u, 1442695040888963407u, 0>>();
  check_engine<std::linear_congruential_engine<std::uint32_t, 3, 0, 0>>();
  check_engine<std::subtract_with_carry_engine<std::uint64_t, 64, 5, 12>>();
  check_engine<std::mersenne_twister_engine<std::uint32_t, 32, 5, 3, 1, 0x9908b0df, 11, 0xffffffff, 7,
                                            0x9d2c5680, 15, 0xefc60000, 18, 1812433253>>();
  check_engine<std::independent_bits_engine<std::mt19937, 7, std::uint16_t>>();
  check_engine<std::independent_bits_engine<std::ranlux48_base, 64, std::uint64_t>>();
  check_engine<std::discard_block_engine<std::minstd_rand, 5, 3>>();
  check_engine<std::shuffle_order_engine<std::mt19937_64, 7>>();
  check_engine<std::shuffle_order_engine<std::discard_block_engine<std::ranlux24_base, 11, 4>, 3>>();
  return 0;
}
