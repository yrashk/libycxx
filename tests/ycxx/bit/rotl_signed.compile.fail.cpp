// EXPECT-ERROR-GCC: error: no matching function for call to 'rotl\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'rotl'
// [bit.rotate]/2 Constraints: T is an unsigned integer type.
// The call below has no viable candidate, so the program is ill-formed.
#include <bit>

void f() { (void)std::rotl(1, 1); }
