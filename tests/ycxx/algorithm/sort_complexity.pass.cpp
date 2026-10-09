// Libycxx performance policy: 4 N ceil(log2 N) comparisons and 8 N ceil(log2 N) projections for the fixed shapes.
// The finite comparison budget is a regression heuristic, not an exact draft bound or
// proof of asymptotic/average-case complexity. Normative effects remain independent.
// [sort]/5: "Complexity: Let N be last - first. O(N log N) comparisons and projections."
// Checked at run time on several input shapes (including ones that defeat naive pivot
// choices) for N = 2000, with a generous constant: at most 4 N log2 N comparisons and as
// many projections per comparison operand. Also [sort]/3 on every shape.
#include <algorithm>
#include <functional>
#include "sort_support.hpp"
#include "check.hpp"

constexpr int N = 2000;
int a[N], orig[N];

int main() {
  const long long bound = 4LL * N * ceil_log2(N);
  for (Pattern p : all_patterns) {
    fill_pattern(a, N, p);
    for (int i = 0; i < N; ++i) orig[i] = a[i];
    int comps = 0;
    std::sort(a, a + N, CountingLess{&comps});
    CHECK(comps <= bound);
    CHECK(sorted_by(a, a + N));
    CHECK(same_multiset(a, orig, N));

    fill_pattern(a, N, p);
    comps = 0;
    int projs = 0;
    std::ranges::sort(a, CountingLess{&comps}, CountingProj{&projs});
    CHECK(comps <= bound);
    CHECK(projs <= 2 * bound);
    CHECK(sorted_by(a, a + N));
    CHECK(same_multiset(a, orig, N));
  }
  return 0;
}
