// [template.bitset.general]/1: bitset<N> "describes an object that can store a sequence
// consisting of a fixed number of bits, N". Operations that set every bit (~, set(), flip(),
// the unsigned long long constructor, shifts) must not let bits at positions >= N influence
// count(), all(), ==, to_ullong() or hash ([bitset.members], [bitset.hash]). Checked for sizes
// that are not multiples of common word sizes.
// [bitset.members]/40: to_ullong "Throws: overflow_error if the integral value x
// corresponding to the bits in *this cannot be represented as type unsigned long long."
#include <bitset>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include "check.hpp"

template <std::size_t N>
constexpr bool test() {
  using B = std::bitset<N>;
  B all1 = ~B();
  B set1 = B().set();
  B flip1 = B().flip();
  if (all1 != set1 || flip1 != set1 || !(all1 == set1)) return false;
  if (all1.count() != N || !all1.all() || set1.count() != N || flip1.count() != N) return false;
  // shifting left then right clears the top bits; no stale bits come back
  B s = (all1 << 3) >> 3;
  if (s.count() != N - 3 || s.test(N - 1) || s.test(N - 3)) return false;
  B r = (all1 >> 3) << 3;
  if (r.count() != N - 3 || r.test(0) || !r.test(N - 1)) return false;
  // the constructor from unsigned long long keeps only the first N bits
  B fromull(~0ull);
  if (fromull.count() != (N < 64 ? N : 64)) return false;
  if (N < 64 && fromull != all1) return false;
  // ~ twice is the identity, and ~all is none
  if (~~all1 != all1 || (~all1).any()) return false;
  // flip of a single bit at N - 1 then all()
  B x = all1;
  x.flip(N - 1);
  if (x.all() || x.count() != N - 1) return false;
  return true;
}

static_assert(test<5>());
static_assert(test<33>());
static_assert(test<63>());
static_assert(test<65>());
static_assert(test<100>());
static_assert(test<129>());

int main() {
  CHECK(test<5>() && test<33>() && test<63>() && test<65>() && test<100>() && test<129>());
  // equal values hash equally even when reached by different operations
  std::hash<std::bitset<70>> h;
  CHECK(h(~std::bitset<70>()) == h(std::bitset<70>().set()));
  CHECK(h((~std::bitset<70>() << 69) >> 69) == h(std::bitset<70>(1)));
  // to_ullong: all of the first 64 bits set, the rest clear: representable
  std::bitset<70> low = ~std::bitset<70>() >> 6;
  CHECK(low.count() == 64);
  CHECK(low.to_ullong() == ~0ull);
  // bit 64 set: not representable
  bool threw = false;
  try {
    (void)(low << 1).to_ullong();
  } catch (const std::overflow_error&) {
    threw = true;
  }
  CHECK(threw);
  // to_ulong on a 33-bit all-ones value fits iff unsigned long has at least 33 bits
  CHECK(std::bitset<33>().set().to_ullong() == 0x1FFFFFFFFull);
  return 0;
}
