// EXPECT-ERROR: error: [^\n]*is deprecated: has_denorm is deprecated \(\[depr\.numeric\.limits\.has\.denorm\]\)[^\n]*W(?:error|deprecated)
// [depr.numeric.limits.has.denorm] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <limits>
#include <cstddef>

int main() {
  auto x = std::numeric_limits<int>::has_denorm; (void)x;
}
