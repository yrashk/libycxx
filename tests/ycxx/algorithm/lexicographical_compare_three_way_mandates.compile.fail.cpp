// [alg.three.way]/2: "Mandates: decltype(comp(*b1, *b2)) is a comparison category type."
// A comparator returning int is ill-formed.
#include <algorithm>

int main() {
  int a[] = {1, 2};
  int b[] = {1, 3};
  auto r = std::lexicographical_compare_three_way(a, a + 2, b, b + 2, [](int x, int y) { return x - y; });
  (void)r;
  return 0;
}
