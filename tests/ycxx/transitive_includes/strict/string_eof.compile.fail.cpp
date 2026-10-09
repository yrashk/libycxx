// DECISIONS §19: with YCXX_NO_TRANSITIVE_INCLUDES, <string> does not provide <cstdio>'s EOF
// (char_traits<char>::eof() returns its value, [char.traits.specializations.char], but the macro
// is <cstdio>'s); by default it does (../string.compile.pass.cpp).
// REQUIRES: linux
// Darwin's <wchar.h>, needed for hosted char_traits, includes <_stdio.h> and provides EOF
// independently of libycxx's optional transitive includes. This absence policy is Linux-only.
// EXPECT-ERROR-GCC: .EOF. was not declared in this scope
// EXPECT-ERROR-CLANG: use of undeclared identifier 'EOF'
#define YCXX_NO_TRANSITIVE_INCLUDES
#include <string>

int f() { return EOF; }
