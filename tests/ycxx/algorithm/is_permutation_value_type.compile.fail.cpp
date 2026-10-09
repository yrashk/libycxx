// EXPECT-ERROR: error: static assertion failed[^\n]*std::is_permutation: the two ranges must have the same value type
// [alg.is.permutation]/1: "Mandates: ForwardIterator1 and ForwardIterator2 have the same
// value type."
#include <algorithm>

int main() {
  int a[] = {1, 2};
  long b[] = {2, 1};
  return std::is_permutation(a, a + 2, b) ? 0 : 1;
}
