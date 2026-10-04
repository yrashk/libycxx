// [depr.meta.types] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <type_traits>
#include <cstddef>

int main() {
  std::aligned_storage_t<4,4> x; (void)x;
}
