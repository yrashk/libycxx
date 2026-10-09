// EXPECT-ERROR: error: [^\n]*is deprecated: atomic_init is deprecated \(\[depr\.atomics\.nonmembers\]\); use store\(desired, memory_order::relaxed\)[^\n]*W(?:error|deprecated)
// [depr.atomics.nonmembers] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
// COUNTERPART: libcxx:depr/depr.atomics/depr.atomics.nonmembers/atomic_init.depr_in_cxx20.verify.cpp
#include <atomic>
#include <cstddef>

int main() {
  volatile std::atomic<int> a; std::atomic_init(&a, 1);
}
