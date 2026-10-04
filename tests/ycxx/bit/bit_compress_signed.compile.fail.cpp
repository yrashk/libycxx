// [bit.permute]/11 Constraints: T is an unsigned integer type.
// The call below has no viable candidate, so the program is ill-formed.
#include <bit>

void f() { (void)std::bit_compress(5, 3); }
