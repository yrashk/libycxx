// EXPECT-ERROR: error: [^\n]*is deprecated: tuple_element<I, const volatile T> is deprecated \(\[depr\.tuple\]\)[^\n]*W(?:error|deprecated)
// [depr.tuple] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <tuple>
#include <cstddef>

int main() {
  std::tuple_element<0, const volatile std::tuple<int>>::type* x = nullptr; (void)x;
}
