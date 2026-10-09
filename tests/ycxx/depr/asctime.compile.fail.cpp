// EXPECT-ERROR: error: [^\n]*is deprecated: asctime is deprecated \(\[depr\.ctime\]\); use strftime or std::format[^\n]*W(?:error|deprecated)
// [depr.ctime] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <ctime>
#include <cstddef>

int main() {
  std::tm t{}; (void)std::asctime(&t);
}
