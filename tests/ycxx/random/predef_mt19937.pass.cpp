// [rand.predef]/3-4: the 10000th invocation of a default-constructed mt19937 produces
// 4123659995, of a default-constructed mt19937_64 produces 9981545732273789042.
#include <random>
#include "check.hpp"

template <class E>
typename E::result_type nth(unsigned long long n) {
  E e;
  for (unsigned long long i = 1; i < n; ++i) e();
  return e();
}

int main() {
  CHECK(nth<std::mt19937>(10000) == 4123659995u);
  CHECK(nth<std::mt19937_64>(10000) == 9981545732273789042ull);
  std::mt19937 a;
  a.discard(9999);
  CHECK(a() == 4123659995u);
  std::mt19937_64 b;
  b.discard(9999);
  CHECK(b() == 9981545732273789042ull);
  // The default constructor uses default_seed = 5489 ([rand.eng.mers]).
  CHECK(std::mt19937(5489u) == std::mt19937());
  CHECK(std::mt19937_64(5489u) == std::mt19937_64());
}
