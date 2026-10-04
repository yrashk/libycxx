// [rand.predef]/11-12: the 10000th invocation of a default-constructed philox4x32 produces
// 1955073260, of a default-constructed philox4x64 produces 3409172418970261260.
#include <random>
#include "check.hpp"

template <class E>
typename E::result_type nth(unsigned long long n) {
  E e;
  for (unsigned long long i = 1; i < n; ++i) e();
  return e();
}

int main() {
  CHECK(nth<std::philox4x32>(10000) == 1955073260u);
  CHECK(nth<std::philox4x64>(10000) == 3409172418970261260ull);
  std::philox4x32 a;
  a.discard(9999);
  CHECK(a() == 1955073260u);
  std::philox4x64 b;
  b.discard(9999);
  CHECK(b() == 3409172418970261260ull);
  // Discarding across several counter blocks at once, from an unaligned position.
  std::philox4x32 c, d;
  c(); d();
  c.discard(9998);
  for (int i = 0; i < 9998; ++i) d();
  CHECK(c == d);
  CHECK(c() == 1955073260u);
}
