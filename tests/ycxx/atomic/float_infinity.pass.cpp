// [atomics.types.float]/6-7: fetch_add / fetch_sub atomically replace the value with the result
// of the computation (Table 155: addition / subtraction) and return the previous value; /8:
// "If the result is not a representable value for its type ([expr.pre]) the result is
// unspecified, but the operations otherwise have no undefined behavior." With
// numeric_limits<T>::has_infinity, infinities are representable values, and adding a finite
// value to an infinity, or an infinity to a finite value, yields that infinity. Only such
// mathematically defined cases are checked (inf - inf and NaN arithmetic are not). Also an
// overflowing addition is not undefined behaviour (/8): it just leaves some value. The same for
// atomic_ref ([atomics.ref.float]/6-8).
// FLAGS: -latomic
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include <cmath>
#include <limits>
#include "check.hpp"
#include "watchdog.hpp"

template<class T>
void run() {
  static_assert(std::numeric_limits<T>::has_infinity);
  constexpr T inf = std::numeric_limits<T>::infinity();
  std::atomic<T> a(inf);
  CHECK(a.fetch_add(T(1)) == inf);
  CHECK(a.load() == inf);
  CHECK(a.fetch_sub(T(1e10)) == inf);
  CHECK(a.load() == inf);
  CHECK((a += T(-3)) == inf);
  a.store(T(5));
  CHECK(a.fetch_sub(inf) == T(5));
  CHECK(a.load() == -inf);
  CHECK((a -= T(2)) == -inf);
  a.store(T(5));
  CHECK((a += inf) == inf);

  // overflow: no undefined behaviour; the value is unspecified
  constexpr T mx = std::numeric_limits<T>::max();
  a.store(mx);
  CHECK(a.fetch_add(mx) == mx);
  (void)a.load();

  // a stored NaN round-trips through load / exchange (no arithmetic)
  a.store(std::numeric_limits<T>::quiet_NaN());
  CHECK(std::isnan(a.load()));
  CHECK(std::isnan(a.exchange(T(1))));

  alignas(std::atomic_ref<T>::required_alignment) T x = -inf;
  std::atomic_ref<T> r(x);
  CHECK(r.fetch_add(T(7)) == -inf);
  CHECK(x == -inf);
  CHECK((r -= T(7)) == -inf);
  x = T(0);
  CHECK(r.fetch_add(inf) == T(0));
  CHECK(x == inf);
}

int main() {
  watchdog(10);
  run<float>();
  run<double>();
  run<long double>();
  return 0;
}
