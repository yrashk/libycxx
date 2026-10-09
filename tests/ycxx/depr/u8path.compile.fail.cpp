// EXPECT-ERROR: error: [^\n]*is deprecated: u8path is deprecated \(\[depr\.fs\.path\.factory\]\); construct a path from a u8string[^\n]*W(?:error|deprecated)
// [depr.fs.path.factory] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <filesystem>
#include <cstddef>

int main() {
  auto p = std::filesystem::u8path("a"); (void)p;
}
