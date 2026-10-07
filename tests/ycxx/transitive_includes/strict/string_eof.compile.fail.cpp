// DECISIONS §19: with YCXX_NO_TRANSITIVE_INCLUDES, <string> does not provide <cstdio>'s EOF
// (char_traits<char>::eof() returns its value, [char.traits.specializations.char], but the macro
// is <cstdio>'s); by default it does (../string.compile.pass.cpp).
// EXPECT-ERROR-GCC: .EOF. was not declared in this scope
// EXPECT-ERROR-CLANG: use of undeclared identifier 'EOF'
#define YCXX_NO_TRANSITIVE_INCLUDES
#include <string>

int f() { return EOF; }
