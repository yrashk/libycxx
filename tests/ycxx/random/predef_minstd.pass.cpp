// [rand.predef]/1-2: "Required behavior: The 10000th consecutive invocation of a
// default-constructed object of type minstd_rand0 produces the value 1043618065." and
// "... of type minstd_rand produces the value 399268537."
#include <random>
#include <type_traits>
#include "check.hpp"

template <class E>
typename E::result_type nth(unsigned long long n) {
  E e;
  for (unsigned long long i = 1; i < n; ++i) e();
  return e();
}

int main() {
  CHECK(nth<std::minstd_rand0>(10000) == 1043618065u);
  CHECK(nth<std::minstd_rand>(10000) == 399268537u);
  // discard(z) is equivalent to z calls ([rand.req.eng]).
  std::minstd_rand0 a;
  a.discard(9999);
  CHECK(a() == 1043618065u);
  std::minstd_rand b;
  b.discard(9999);
  CHECK(b() == 399268537u);
}
