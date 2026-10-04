// [template.bitset.general], [bitset.members]/48-50: bitset<0> is valid; all() is
// count() == size() (true), any() is count() != 0 (false), none() is count() == 0 (true).
#include <bitset>
#include "check.hpp"

constexpr bool test() {
  std::bitset<0> z, w(~0ull);
  if (z.size() != 0 || z.count() != 0) return false;
  if (!z.all() || z.any() || !z.none()) return false;
  if (!(z == w)) return false;
  if ((~z).count() != 0 || (z << 3) != z || (z >> 3) != z) return false;
  z.set();
  z.flip();
  if (z.count() != 0) return false;
  if (z.to_ulong() != 0 || z.to_ullong() != 0) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
