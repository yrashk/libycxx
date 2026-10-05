// [depr.cerrno] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
// COUNTERPART: libcxx:depr.cerro/system.error.syn.verify.cpp
#include <system_error>
#include <cstddef>

int main() {
  auto x = std::errc::not_a_stream; (void)x;
}
