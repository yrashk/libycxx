// EXPECT-ERROR: error: [^\n]*is deprecated: kill_dependency is deprecated \(\[depr\.atomics\.order\]\)[^\n]*W(?:error|deprecated)
// [depr.atomics.order] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <atomic>
#include <cstddef>

int main() {
  int x = std::kill_dependency(1); (void)x;
}
