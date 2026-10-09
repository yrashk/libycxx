// EXPECT-ERROR-GCC: error: no matching function for call to 'byteswap\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'byteswap'
// [bit.byteswap]/1 Constraints: T models integral.
// The call below has no viable candidate, so the program is ill-formed.
#include <bit>

void f() { (void)std::byteswap(1.0f); }
