// EXPECT-ERROR: error: [^\n]*is deprecated: variant_alternative<I, const volatile T> is deprecated \(\[depr\.variant\]\)[^\n]*W(?:error|deprecated)
// [depr.variant] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <variant>
#include <cstddef>

int main() {
  std::variant_alternative<0, const volatile std::variant<int>>::type* x = nullptr; (void)x;
}
