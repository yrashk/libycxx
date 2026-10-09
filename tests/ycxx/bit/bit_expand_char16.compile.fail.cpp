// EXPECT-ERROR-GCC: error: no matching function for call to 'bit_expand\(
// EXPECT-ERROR-CLANG: error: no matching function for call to 'bit_expand'
// [bit.permute]/14 Constraints: T is an unsigned integer type.
// The call below has no viable candidate, so the program is ill-formed.
#include <bit>

void f() { (void)std::bit_expand(u'a', u'b'); }
