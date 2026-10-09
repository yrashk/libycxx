// EXPECT-ERROR-GCC: error: no matching function for call to 'bit_repeat\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'bit_repeat'
// [bit.permute]/5 Constraints: T is an unsigned integer type.
// The call below has no viable candidate, so the program is ill-formed.
#include <bit>

void f() { (void)std::bit_repeat(5L, 2); }
