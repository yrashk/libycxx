// [bitset.cons]/7: "Throws: ... invalid_argument if any of the rlen characters in str
// beginning at position pos is other than zero or one." An uncaught exception is not a
// core constant expression, so this constexpr variable is ill-formed.
#include <bitset>

constexpr std::bitset<4> b("1201");
