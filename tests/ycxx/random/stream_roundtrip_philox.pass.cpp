// [rand.req.eng] Table 127 for philox_engine ([rand.eng.philox]): os << x / is >> v round-trip
// the whole state (the counter, the key, the buffered results and the position in the buffer)
// "With os.fmtflags set to ios_base::dec|ios_base::left and the fill character set to the space
// character" / "With is.fmtflags set to ios_base::dec"; the streams' flags and fill are
// unchanged afterwards. Checked at every position in the four-word buffer and after
// set_counter, with hostile formatting state on both streams.
#include <random>
#include <sstream>
#include <array>
#include "check.hpp"

using B = std::ios_base;

template <class E>
void roundtrip(const E& x) {
  std::ostringstream os;
  const auto of = B::hex | B::showbase | B::showpos | B::uppercase | B::right;
  os.flags(of);
  os.fill('0');
  os << x;
  CHECK(os.good() && os.flags() == of && os.fill() == '0');
  std::istringstream is(os.str());
  is.flags(B::oct);
  E v(99u);
  is >> v;
  CHECK(!is.fail() && is.flags() == B::oct);
  CHECK(v == x);
  E xc = x;
  for (int i = 0; i < 100; ++i) CHECK(xc() == v());
}

template <class E>
void check_engine() {
  E e;
  for (int i = 0; i < 9; ++i) {
    roundtrip(e);
    e();
  }
  E s(7u);
  std::array<typename E::result_type, 4> c{1, 2, 3, ~typename E::result_type(0)};
  s.set_counter(c);
  roundtrip(s);
  s();
  roundtrip(s);
}

int main() {
  check_engine<std::philox4x32>();
  check_engine<std::philox4x64>();
  return 0;
}
