// [depr.meta.types] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
// COUNTERPART: libcxx:utilities/meta/meta.unary/meta.unary.prop/is_trivial.deprecated.verify.cpp
#include <type_traits>
#include <cstddef>

int main() {
  bool x = std::is_trivial<int>::value; (void)x;
}
