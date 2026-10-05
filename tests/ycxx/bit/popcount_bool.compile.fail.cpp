// [bit.count]/10 Constraints: T is an unsigned integer type.
// The call below has no viable candidate, so the program is ill-formed.
// EXPECT-ERROR: no matching function for call to .popcount
// EXPECT-ERROR-CLANG: constraints not satisfied \[with T = bool\]
#include <bit>

void f() { (void)std::popcount(true); }
