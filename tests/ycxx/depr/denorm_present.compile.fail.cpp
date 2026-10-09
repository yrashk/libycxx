// EXPECT-ERROR: error: [^\n]*is deprecated: denorm_present is deprecated \(\[depr\.numeric\.limits\.has\.denorm\]\)[^\n]*W(?:error|deprecated)
// [depr.numeric.limits.has.denorm] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
// COUNTERPART: libcxx:depr/depr.numeric.limits.has.denorm/deprecated.verify.cpp
#include <limits>
#include <cstddef>

int main() {
  auto x = std::denorm_present; (void)x;
}
