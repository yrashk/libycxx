// [depr.locale.category] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <locale>
#include <cstddef>

int main() {
  using F = std::codecvt<char16_t, char8_t, std::mbstate_t>; F* p = nullptr; (void)p;
}
