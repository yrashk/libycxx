// [stable.sort]/5: "Complexity: Let N be last - first. If enough extra memory is available,
// N log(N) comparisons. Otherwise, at most N log^2(N) comparisons. In either case, twice as
// many projections as the number of comparisons." The N log^2 N bound holds in either case,
// so it is checked (N = 2000, log base 2), as is projections <= 2 * comparisons.
#include <algorithm>
#include "sort_support.hpp"
#include "check.hpp"

constexpr int N = 2000;
int a[N], orig[N];

int main() {
  const long long lg = ceil_log2(N);
  const long long bound = N * lg * lg;
  for (Pattern p : all_patterns) {
    fill_pattern(a, N, p);
    for (int i = 0; i < N; ++i) orig[i] = a[i];
    int comps = 0;
    std::stable_sort(a, a + N, CountingLess{&comps});
    CHECK(comps <= bound);
    CHECK(sorted_by(a, a + N));
    CHECK(same_multiset(a, orig, N));

    fill_pattern(a, N, p);
    comps = 0;
    int projs = 0;
    std::ranges::stable_sort(a, CountingLess{&comps}, CountingProj{&projs});
    CHECK(comps <= bound);
    CHECK(projs <= 2 * comps);
    CHECK(sorted_by(a, a + N));
  }
  return 0;
}
