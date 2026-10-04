// [rand.predef]/9: "The 10000th consecutive invocation of a default-constructed object of type
// knuth_b produces the value 1112339016."
#include <random>
#include "check.hpp"

int main() {
  std::knuth_b e;
  for (int i = 1; i < 10000; ++i) e();
  CHECK(e() == 1112339016u);
  std::knuth_b f;
  f.discard(9999);
  CHECK(f() == 1112339016u);
}
