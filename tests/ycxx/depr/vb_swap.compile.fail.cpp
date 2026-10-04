// [depr.vector.bool.swap] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <vector>
#include <cstddef>

int main() {
  std::vector<bool> v(2); std::vector<bool>::swap(v[0], v[1]);
}
