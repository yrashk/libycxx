// EXPECT-ERROR: error: [^\n]*is deprecated: codecvt_byname<char32_t, char, mbstate_t> is deprecated \(\[depr\.locale\.category\]\)[^\n]*W(?:error|deprecated)
// [depr.locale.category] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
// COUNTERPART: libcxx:localization/locale.categories/category.ctype/locale.codecvt.byname/codecvt_byname_char32_t_char.depr_in_cxx20.verify.cpp
#include <locale>
#include <cstddef>

int main() {
  using F = std::codecvt_byname<char32_t, char, std::mbstate_t>; F* p = nullptr; (void)p;
}
