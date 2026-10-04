// [utility.intcmp]/7: cmp_less_equal is "Equivalent to: return !cmp_greater(t, u);", which
// reaches cmp_less, whose "Mandates: Each of T and U is a signed or unsigned integer type" --
// an unscoped enumeration type is not (T is deduced as the enumeration, without promotion).
#include <utility>

enum E { e0, e1 };
bool b = std::cmp_less_equal(e1, 1);
