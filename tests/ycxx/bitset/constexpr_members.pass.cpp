// [template.bitset.general] (P2417, C++23): every bitset member that does not need
// basic_string is constexpr, including test(), to_ulong(), to_ullong(), the reference proxy
// and the const operator[].
#include <bitset>
#include <cstddef>
#include "check.hpp"

constexpr std::bitset<12> make() {
  std::bitset<12> b;
  b[1] = true;
  b[11] = b[1];
  b[1].flip();
  b.set(5).flip(6).reset(5);
  return b;
}
constexpr std::bitset<12> k = make();
static_assert(k.test(11) && k.test(6) && !k.test(1) && !k.test(5));
static_assert(k[11] && !k[0]);
static_assert(k.count() == 2);
static_assert(k.to_ulong() == ((1ul << 11) | (1ul << 6)));
static_assert(k.to_ullong() == k.to_ulong());
static_assert((k ^ k).none());
static_assert((k | ~k).all());
static_assert((k & ~k).none());
static_assert(k == make());
static_assert(k != std::bitset<12>());

int main() {
  CHECK(make() == k);
  return 0;
}
