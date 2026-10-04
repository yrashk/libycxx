// [depr.tuple] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <array>
#include <cstddef>

int main() {
  std::tuple_element<0, volatile std::array<int,1>>::type* x = nullptr; (void)x;
}
