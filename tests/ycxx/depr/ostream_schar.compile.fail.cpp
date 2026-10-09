// EXPECT-ERROR: error: [^\n]*is deprecated: signed char / unsigned char stream insertion is deprecated \(\[depr\.ostream\.inserters\]\); use char[^\n]*W(?:error|deprecated)
// [depr.ostream.inserters] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <ostream>
#include <cstddef>

int main() {
  extern std::ostream& out; signed char c = 'a'; out << c;
}
