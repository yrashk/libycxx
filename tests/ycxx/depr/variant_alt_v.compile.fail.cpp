// [depr.variant] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <variant>
#include <cstddef>

int main() {
  std::variant_alternative<0, volatile std::variant<int>>::type* x = nullptr; (void)x;
}
