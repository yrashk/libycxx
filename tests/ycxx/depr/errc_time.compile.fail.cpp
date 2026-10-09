// EXPECT-ERROR: error: [^\n]*is deprecated: errc::stream_timeout \(ETIME\) is deprecated \(\[depr\.cerrno\]\)[^\n]*W(?:error|deprecated)
// [depr.cerrno] (Annex D).
// [depr.general]/2: "An implementation may declare library names and entities described in this
// Clause with the deprecated attribute"; libycxx does (DECISIONS.md §6): this use is diagnosed.
// FLAGS: -Werror=deprecated-declarations
// COUNTERPART: libcxx:depr.cerro/system.error.syn.verify.cpp
#include <system_error>
#include <cstddef>

int main() {
  auto x = std::errc::stream_timeout; (void)x;
}
