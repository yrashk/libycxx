// [alg.find]: find over volatile narrow-character elements, and over values outside the
// element type's range, compares each element as *i == value (no shortcut may drop the
// volatile qualification or the usual arithmetic conversions).
#include <algorithm>
#include <cassert>
#include <iterator>

int main() {
  volatile char a[] = {'x', 'y', 'z', 'y'};
  volatile char* p = std::find(std::begin(a), std::end(a), 'y');
  assert(p == a + 1);
  assert(std::ranges::find(a, 'z') == a + 2);
  assert(std::find(std::begin(a), std::end(a), 'q') == std::end(a));

  const volatile unsigned char u[] = {1, 200, 3};
  assert(std::find(std::begin(u), std::end(u), 200) == u + 1);
  assert(std::find(std::begin(u), std::end(u), -56) == std::end(u));

  signed char s[] = {1, -56, 3};
  assert(std::find(std::begin(s), std::end(s), -56) == s + 1);
  assert(std::find(std::begin(s), std::end(s), 200) == std::end(s));
  assert(std::find(std::begin(s), std::end(s), 4294967240u) == s + 1); // -56 converts to it
  assert(std::ranges::find(s, 456) == std::end(s));                     // 456 % 256 == 200
}
