// EXPECT-ERROR: error: [^\n]*is deprecated: path::generic_string\(\) is deprecated \(\[depr\.fs\.path\.obs\]\); use generic_native_encoded_string\(\) or generic_display_string\(\)[^\n]*W(?:error|deprecated)
// [depr.fs.path.obs] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <filesystem>
#include <cstddef>

int main() {
  std::filesystem::path p("a"); auto s = p.generic_string(); (void)s;
}
