// EXPECT-ERROR-GCC: error: no matching function for call to 'shl\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'shl'
// [bit.shift]/2 Constraints: Each of T and S is a signed or unsigned integer type.
// The call below has no viable candidate, so the program is ill-formed.
#include <bit>

void f() { (void)std::shl(true, 1); }
