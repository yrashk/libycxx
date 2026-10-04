// [vector.bool.pspc]/4-11: reference simulates a reference to a single bit: copying it
// refers to the same bit; operator=(bool) / operator=(const reference&) / const
// operator=(bool) set the bit when the argument is true and clear it otherwise, returning
// *this; operator bool reads it; flip() is *this = !*this; swap(reference, reference),
// swap(reference, bool&), swap(bool&, reference) exchange the values. /12: vector::flip()
// complements every element.
#include <vector>
#include <utility>
#include "check.hpp"

constexpr bool test() {
  std::vector<bool> v{false, true, false, true};
  std::vector<bool>::reference r0 = v[0];
  std::vector<bool>::reference copy = r0;  // refers to the same bit
  r0 = true;
  if (!v[0] || !copy) return false;
  copy = false;
  if (v[0]) return false;
  v[2] = v[1];  // operator=(const reference&) copies the value, not the binding
  if (!v[2]) return false;
  v[1] = false;
  if (!v[2] || v[1]) return false;
  const std::vector<bool>::reference cr = v[3];
  cr = false;  // const operator=(bool)
  if (v[3]) return false;
  v[0].flip();
  if (!v[0]) return false;
  v[0].flip();
  if (v[0]) return false;
  // v == {false, false, true, false}
  swap(v[0], v[2]);
  if (!v[0] || v[2]) return false;
  bool b = false;
  swap(v[0], b);
  if (v[0] || !b) return false;
  swap(b, v[1]);
  if (b || !v[1]) return false;
  v.flip();
  if (!v[0] || v[1] || !v[2] || !v[3]) return false;
  bool sum = false;
  for (auto ref : v) sum = sum || ref;  // iterating yields references convertible to bool
  if (!sum) return false;
  for (auto ref : v) ref = false;  // writing through a by-value reference proxy
  for (bool x : v)
    if (x) return false;
  std::vector<bool> big(200, false);
  big.flip();
  for (bool x : big)
    if (!x) return false;
  big[150].flip();
  if (big[150] || !big[149] || !big[151]) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
