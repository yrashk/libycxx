// EXPECT-ERROR-GCC: error: no matching function for call to 'bit_reverse\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'bit_reverse'
// [bit.permute]/2 Constraints: T is an unsigned integer type.
// The call below has no viable candidate, so the program is ill-formed.
#include <bit>

void f() { (void)std::bit_reverse(u8'a'); }
