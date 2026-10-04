// [bitset.members]/17,22,29,47: set(pos, val), reset(pos), flip(pos), test(pos) "Throws:
// out_of_range if pos does not correspond to a valid bit position." [bitset.members]/38,40:
// to_ulong/to_ullong "Throws: overflow_error if the integral value x ... cannot be represented".
#include <bitset>
#include <climits>
#include <stdexcept>
#include "check.hpp"

template <class F>
bool throws_out_of_range(F f) {
  try {
    f();
  } catch (const std::out_of_range&) {
    return true;
  }
  return false;
}
template <class F>
bool throws_overflow(F f) {
  try {
    f();
  } catch (const std::overflow_error&) {
    return true;
  }
  return false;
}

int main() {
  std::bitset<10> b;
  CHECK(throws_out_of_range([&] { b.set(10); }));
  CHECK(throws_out_of_range([&] { b.set(11, false); }));
  CHECK(throws_out_of_range([&] { b.reset(10); }));
  CHECK(throws_out_of_range([&] { b.flip(100); }));
  CHECK(throws_out_of_range([&] { (void)b.test(10); }));
  CHECK(b.none());
  CHECK(!throws_out_of_range([&] { b.set(9); }));
  CHECK(b.test(9));

  std::bitset<0> z;
  CHECK(throws_out_of_range([&] { (void)z.test(0); }));

  std::bitset<sizeof(unsigned long long) * CHAR_BIT + 1> wide;
  CHECK(wide.to_ullong() == 0);
  wide.set(sizeof(unsigned long long) * CHAR_BIT);
  CHECK(throws_overflow([&] { (void)wide.to_ullong(); }));
  CHECK(throws_overflow([&] { (void)wide.to_ulong(); }));
  wide.reset();
  wide.set(sizeof(unsigned long long) * CHAR_BIT - 1);
  CHECK(wide.to_ullong() == 1ull << (sizeof(unsigned long long) * CHAR_BIT - 1));
  if constexpr (sizeof(unsigned long) < sizeof(unsigned long long)) {
    CHECK(throws_overflow([&] { (void)wide.to_ulong(); }));
  } else {
    CHECK(wide.to_ulong() == 1ul << (sizeof(unsigned long) * CHAR_BIT - 1));
  }
  return 0;
}
