// EXPECT-ERROR-GCC: error: no matching function for call to 'countl_zero\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'countl_zero'
// [bit.count]/2 Constraints: T is an unsigned integer type.
// The call below has no viable candidate, so the program is ill-formed.
#include <bit>

void f() { (void)std::countl_zero('a'); }
