// [alg.unique]: unique "Eliminates all elements referred to by the iterator i in the range
// [first, last) for which E(i) is true", E(i) = pred(*(i - 1), *i) (i != first); returns the
// end j (ranges: {j, last}). "Complexity: For nonempty ranges, exactly (last - first) - 1
// applications of the corresponding predicate". unique_copy copies the elements for which
// E(i) is false and returns result + N (ranges: {last, result + N}); it works from input
// iterators.
#include <algorithm>
#include <ranges>
#include <type_traits>
#include "test_iterators.hpp"
#include "check.hpp"

struct Pt {
  int k, v;
};

constexpr bool test() {
  int a[] = {1, 1, 2, 2, 2, 3, 1, 1};
  int* e = std::unique(a, a + 8);
  if (e != a + 4 || a[0] != 1 || a[1] != 2 || a[2] != 3 || a[3] != 1) return false;

  // pred must be an equivalence relation: here "same tens digit"
  int b[] = {1, 5, 12, 18, 25, 3, 7};
  int calls = 0;
  e = std::unique(b, b + 7, [&calls](int x, int y) {
    ++calls;
    return x / 10 == y / 10;
  });
  if (calls != 6) return false;
  // eliminated: 5, 18, 7
  if (e != b + 4 || b[0] != 1 || b[1] != 12 || b[2] != 25 || b[3] != 3) return false;

  int empty[1] = {};
  if (std::unique(empty, empty) != empty) return false;

  int src[] = {3, 3, 4, 3, 3};
  int out[5] = {};
  int* oe = std::unique_copy(InputIter<int>(src), InputIter<int>(src + 5), out);
  if (oe != out + 3 || out[0] != 3 || out[1] != 4 || out[2] != 3) return false;
  int src2[] = {1, 3, 2, 4, 5};
  oe = std::unique_copy(src2, src2 + 5, out, [](int x, int y) { return x % 2 == y % 2; });
  if (oe != out + 3 || out[0] != 1 || out[1] != 2 || out[2] != 5) return false;

  // ranges with projection
  Pt ps[] = {{1, 10}, {1, 11}, {2, 20}, {2, 21}, {1, 30}};
  auto sr = std::ranges::unique(ps, {}, &Pt::k);
  if (sr.begin() != ps + 3 || sr.end() != ps + 5) return false;
  if (ps[0].v != 10 || ps[1].v != 20 || ps[2].v != 30) return false;

  int r[] = {5, 5, 6};
  int o2[3] = {};
  auto rc = std::ranges::unique_copy(r, o2);
  static_assert(std::is_same_v<decltype(rc), std::ranges::unique_copy_result<int*, int*>>);
  if (rc.in != r + 3 || rc.out != o2 + 2 || o2[1] != 6) return false;

  // input range into a forward output
  int o3[3] = {};
  InputRange<int> ir{r, r + 3};
  auto rc2 = std::ranges::unique_copy(ir, o3);
  if (rc2.in.p != r + 3 || rc2.out != o3 + 2) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
