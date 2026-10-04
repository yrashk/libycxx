// [bit.permute]/14 Constraints: T is an unsigned integer type.
// The call below has no viable candidate, so the program is ill-formed.
#include <bit>

void f() { (void)std::bit_expand(u'a', u'b'); }
