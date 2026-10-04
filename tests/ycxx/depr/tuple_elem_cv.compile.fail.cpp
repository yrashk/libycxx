// [depr.tuple] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <tuple>
#include <cstddef>

int main() {
  std::tuple_element<0, const volatile std::tuple<int>>::type* x = nullptr; (void)x;
}
