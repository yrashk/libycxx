// EXPECT-ERROR: error: [^\n]*is deprecated: aligned_storage is deprecated \(\[depr\.meta\.types\]\); use alignas\(Align\) std::byte\[Len\][^\n]*W(?:error|deprecated)
// [depr.meta.types] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
// COUNTERPART: libcxx:utilities/meta/meta.trans/meta.trans.other/aligned_storage.depr.verify.cpp
#include <type_traits>
#include <cstddef>

int main() {
  std::aligned_storage<4,4>::type x; (void)x;
}
