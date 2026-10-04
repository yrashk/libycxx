// [comparisons.general]/2: "For templates less, greater, less_equal, and greater_equal, the
// specializations for any pointer type yield a result consistent with the
// implementation-defined strict total order over pointers." /3 the same for the void
// specializations when the call would invoke a built-in operator comparing pointers.
// [range.cmp]: ranges::less etc. likewise. Unrelated objects are therefore ordered
// consistently (and usable in constant evaluation for pointers into the same array).
#include <functional>
#include <compare>
#include "check.hpp"

int a, b;
int arr[3];

constexpr bool test_const() {
  return std::less<int*>{}(arr, arr + 1) && std::less<>{}(arr + 1, arr + 2) &&
         std::ranges::less{}(arr + 0, arr + 2) && std::greater<const int*>{}(arr + 2, arr) &&
         std::less_equal<>{}(arr + 0, arr + 0) && std::ranges::greater_equal{}(arr + 1, arr + 1);
}
static_assert(test_const());

int main() {
  CHECK(test_const());
  int* p = &a;
  int* q = &b;
  std::less<int*> lt;
  bool pq = lt(p, q), qp = lt(q, p);
  CHECK(pq != qp);  // a strict total order: exactly one holds for distinct pointers
  CHECK(std::less<>{}(p, q) == pq);
  CHECK(std::ranges::less{}(p, q) == pq);
  CHECK(std::greater<int*>{}(q, p) == pq);
  CHECK(std::less_equal<int*>{}(p, q) == pq);
  CHECK(std::greater_equal<>{}(q, p) == pq);
  CHECK(std::compare_three_way{}(p, q) == (pq ? std::strong_ordering::less : std::strong_ordering::greater));
  CHECK(!lt(p, p));
  return 0;
}
