// EXPECT-ERROR-GCC: error: uncaught exception of type 'std::invalid_argument';[^\n]*std::bitset: character is neither zero nor one
// EXPECT-ERROR-CLANG: error: constexpr variable 'b' must be initialized by a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::bitset: character is neither zero nor one
// [bitset.cons]/7: "Throws: ... invalid_argument if any of the rlen characters in str
// beginning at position pos is other than zero or one." An uncaught exception is not a
// core constant expression, so this constexpr variable is ill-formed.
#include <bitset>

constexpr std::bitset<4> b("1201");
