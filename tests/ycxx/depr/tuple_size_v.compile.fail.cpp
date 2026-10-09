// EXPECT-ERROR: error: [^\n]*is deprecated: tuple_size<volatile T> is deprecated \(\[depr\.tuple\]\)[^\n]*W(?:error|deprecated)
// [depr.tuple] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <tuple>
#include <cstddef>

int main() {
  auto x = std::tuple_size<volatile std::tuple<int>>::value; (void)x;
}
