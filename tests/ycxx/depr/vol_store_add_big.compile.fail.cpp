// [depr.atomics.volatile]/1 (Annex D): a volatile store_add of an atomic type that is not always
// lock-free is deprecated. [depr.general]/2: "An implementation may declare library names and
// entities described in this Clause with the deprecated attribute"; libycxx does (DECISIONS.md
// §6): this use is diagnosed. The compiler probe verifies the long-double atomic fixture
// is not always lock-free; size alone does not determine that property.
// FLAGS: -Werror=deprecated-declarations
// REQUIRES: non-lockfree-long-double-atomic
// EXPECT-ERROR: error: [^\n]*store_add[^\n]*deprecated
#include <atomic>

int main() {
  static_assert(!std::atomic<long double>::is_always_lock_free, "the test needs a non-lock-free type");
  volatile std::atomic<long double> a; a.store_add(1.0L);
}
