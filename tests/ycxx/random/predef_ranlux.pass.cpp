// [rand.predef]/5-8: the 10000th invocation of a default-constructed
//   ranlux24_base produces 7937952,       ranlux48_base produces 61839128582725,
//   ranlux24      produces 9901578,       ranlux48      produces 249142670248501.
#include <random>
#include "check.hpp"

template <class E>
typename E::result_type nth(unsigned long long n) {
  E e;
  for (unsigned long long i = 1; i < n; ++i) e();
  return e();
}

template <class E>
typename E::result_type nth_discard(unsigned long long n) {
  E e;
  e.discard(n - 1);
  return e();
}

int main() {
  CHECK(nth<std::ranlux24_base>(10000) == 7937952u);
  CHECK(nth<std::ranlux48_base>(10000) == 61839128582725ull);
  CHECK(nth<std::ranlux24>(10000) == 9901578u);
  CHECK(nth<std::ranlux48>(10000) == 249142670248501ull);
  CHECK(nth_discard<std::ranlux24_base>(10000) == 7937952u);
  CHECK(nth_discard<std::ranlux48_base>(10000) == 61839128582725ull);
  CHECK(nth_discard<std::ranlux24>(10000) == 9901578u);
  CHECK(nth_discard<std::ranlux48>(10000) == 249142670248501ull);
}
