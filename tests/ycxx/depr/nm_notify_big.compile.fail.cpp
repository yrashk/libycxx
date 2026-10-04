// [depr.atomics.volatile] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <atomic>
#include <cstddef>

int main() {
  struct B { char c[64]; }; volatile std::atomic<B> a; std::atomic_notify_all(&a);
}
