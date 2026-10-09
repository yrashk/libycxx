// EXPECT-ERROR: error: [^\n]*is deprecated: is_pod is deprecated \(\[depr\.meta\.types\]\); use is_standard_layout and is_trivially_copyable[^\n]*W(?:error|deprecated)
// [depr.meta.types] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
// COUNTERPART: libcxx:utilities/meta/meta.unary/meta.unary.prop/is_pod.deprecated.verify.cpp
#include <type_traits>
#include <cstddef>

int main() {
  bool x = std::is_pod<int>::value; (void)x;
}
