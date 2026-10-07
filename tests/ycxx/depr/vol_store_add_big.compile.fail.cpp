// [depr.atomics.volatile]/1 (Annex D): a volatile store_add of an atomic type that is not always
// lock-free is deprecated. [depr.general]/2: "An implementation may declare library names and
// entities described in this Clause with the deprecated attribute"; libycxx does (DECISIONS.md
// §6): this use is diagnosed. (x86-64's 16-byte long double is not always lock-free.)
// FLAGS: -Werror=deprecated-declarations
// REQUIRES: linux
// EXPECT-ERROR: deprecated
#include <atomic>

int main() {
  static_assert(!std::atomic<long double>::is_always_lock_free, "the test needs a non-lock-free type");
  volatile std::atomic<long double> a; a.store_add(1.0L);
}
