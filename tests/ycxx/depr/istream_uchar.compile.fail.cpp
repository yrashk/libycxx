// [depr.istream.extractors] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <istream>
#include <cstddef>

int main() {
  extern std::istream& in; unsigned char c; in >> c;
}
