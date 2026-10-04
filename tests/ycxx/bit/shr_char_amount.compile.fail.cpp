// [bit.shift]/4 Constraints: Each of T and S is a signed or unsigned integer type.
// The call below has no viable candidate, so the program is ill-formed.
#include <bit>

void f() { (void)std::shr(1u, 'a'); }
