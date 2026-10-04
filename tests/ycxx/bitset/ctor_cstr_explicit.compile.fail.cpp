// [bitset.cons]/8: "template<class charT> constexpr explicit bitset(const charT* str, ...)":
// copy-initialisation from a string literal is ill-formed.
#include <bitset>

std::bitset<4> b = "1010";
