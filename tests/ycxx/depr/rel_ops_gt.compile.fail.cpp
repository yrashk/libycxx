// [depr.relops] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
#include <utility>
#include <cstddef>

int main() {
  struct A {
    bool operator<(const A&) const { return false; }
  };
  bool x = std::rel_ops::operator>(A{}, A{});
  (void)x;
}
