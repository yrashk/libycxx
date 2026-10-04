// [atomics.types.int]/8: "Except for fetch_max and fetch_min, for signed integer types the
// result is as if the object value and parameters were converted to their corresponding
// unsigned types, the computation performed on those types, and the result converted back
// to the signed type. [Note 2: There are no undefined results arising from the computation.]"
#include <atomic>
#include <climits>
#include "check.hpp"

int main() {
  std::atomic<int> a(INT_MAX);
  CHECK(a.fetch_add(1) == INT_MAX);
  CHECK(a.load() == INT_MIN);
  CHECK(a.fetch_sub(1) == INT_MIN);
  CHECK(a.load() == INT_MAX);
  CHECK(++a == INT_MIN);
  CHECK(--a == INT_MAX);
  CHECK((a += INT_MAX) == -2);
  a = INT_MIN;
  CHECK((a -= 1) == INT_MAX);

  std::atomic<long long> l(LLONG_MIN);
  CHECK(l-- == LLONG_MIN);
  CHECK(l.load() == LLONG_MAX);

  std::atomic<signed char> c(SCHAR_MAX);
  CHECK(c.fetch_add(1) == SCHAR_MAX);
  CHECK(c.load() == SCHAR_MIN);
  return 0;
}
