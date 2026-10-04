// Heap complexity bounds: push_heap "At most log(last - first) comparisons and twice as
// many projections"; pop_heap "At most 2 log(last - first) comparisons and twice as many
// projections"; make_heap "At most 3(last - first) comparisons and twice as many
// projections"; sort_heap "At most 2N log N comparisons ... and twice as many projections";
// is_heap_until "Linear". log is taken as log2, rounded up.
#include <algorithm>
#include "sort_support.hpp"
#include "check.hpp"

constexpr int N = 1500;
int a[N];

int main() {
  for (Pattern p : all_patterns) {
    fill_pattern(a, N, p);
    int comps = 0, projs = 0;
    std::ranges::make_heap(a, CountingLess{&comps}, CountingProj{&projs});
    CHECK(comps <= 3 * N);
    CHECK(projs <= 2 * comps);
    CHECK(std::is_heap(a, a + N));

    // pop everything, one at a time
    for (int n = N; n > 1; --n) {
      comps = projs = 0;
      std::ranges::pop_heap(a, a + n, CountingLess{&comps}, CountingProj{&projs});
      CHECK(comps <= 2 * ceil_log2(n));
      CHECK(projs <= 2 * comps);
    }
    CHECK(sorted_by(a, a + N));

    // push everything, one at a time (ascending input: every push sifts to the root)
    for (int n = 2; n <= N; ++n) {
      comps = projs = 0;
      std::ranges::push_heap(a, a + n, CountingLess{&comps}, CountingProj{&projs});
      CHECK(comps <= ceil_log2(n));
      CHECK(projs <= 2 * comps);
    }
    CHECK(std::is_heap(a, a + N));

    comps = projs = 0;
    std::ranges::sort_heap(a, CountingLess{&comps}, CountingProj{&projs});
    CHECK(comps <= 2LL * N * ceil_log2(N));
    CHECK(projs <= 2 * comps);
    CHECK(sorted_by(a, a + N));

    comps = 0;
    std::make_heap(a, a + N, CountingLess{&comps});
    CHECK(comps <= 3 * N);
    comps = 0;
    CHECK(std::is_heap_until(a, a + N, CountingLess{&comps}) == a + N);
    CHECK(comps <= N);
  }
  return 0;
}
