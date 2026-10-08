// DECISIONS §19: YCXX_NO_TRANSITIVE_INCLUDES is tested with defined() only (as NDEBUG is), so a
// definition as 0 also selects the strict mode: <string> without <algorithm>.
// EXPECT-ERROR-GCC: .min. is not a member of .std.
// EXPECT-ERROR-CLANG: no member named 'min' in namespace 'std'
#define YCXX_NO_TRANSITIVE_INCLUDES 0
#include <string>

int f() { return std::min(1, 2); }
