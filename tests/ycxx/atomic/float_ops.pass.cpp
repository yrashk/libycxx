// [atomics.types.float]: atomic<floating-point-type> for all cv-unqualified floating-point
// types; value_type and difference_type are the floating-point type; fetch_add / fetch_sub
// return the previous value (/7); "operator op=: Equivalent to: return fetch_key(operand) op
// operand;". The default constructor value-initializes ([atomics.types.operations]/2).
// FLAGS: -latomic
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include "check.hpp"

template<class T>
void run() {
  std::atomic<T> z;
  CHECK(z.load() == T(0));
  std::atomic<T> a(T(1.5));
  CHECK(a.load() == T(1.5));
  CHECK((a = T(2.5)) == T(2.5));
  CHECK(static_cast<T>(a) == T(2.5));
  CHECK(a.exchange(T(4)) == T(2.5));
  CHECK(a.fetch_add(T(0.5)) == T(4));
  CHECK(a.load() == T(4.5));
  CHECK(a.fetch_sub(T(1.25), std::memory_order::relaxed) == T(4.5));
  CHECK(a.load() == T(3.25));
  CHECK((a += T(0.75)) == T(4));
  CHECK((a -= T(6)) == T(-2));
  CHECK(a.load() == T(-2));
  T e = T(-2);
  CHECK(a.compare_exchange_strong(e, T(8)));
  CHECK(a.load() == T(8));
  e = T(1);
  CHECK(!a.compare_exchange_strong(e, T(9)));
  CHECK(e == T(8));
}

int main() {
  run<float>();
  run<double>();
  run<long double>();
  return 0;
}
