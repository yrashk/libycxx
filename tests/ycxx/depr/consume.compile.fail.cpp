// EXPECT-ERROR: error: [^\n]*is deprecated: memory_order::consume is deprecated \(\[depr\.atomics\.order\]\); use acquire[^\n]*W(?:error|deprecated)
// [depr.atomics.order] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <atomic>
#include <cstddef>

int main() {
  auto o = std::memory_order::consume; (void)o;
}
