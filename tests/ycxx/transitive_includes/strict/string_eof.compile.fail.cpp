// DECISIONS §19: with YCXX_NO_TRANSITIVE_INCLUDES, <string> does not provide <cstdio>'s EOF
// (char_traits<char>::eof() returns its value, [char.traits.specializations.char], but the macro
// is <cstdio>'s); by default it does (../string.compile.pass.cpp).
// REQUIRES: !darwin
// (platform: Darwin's <wchar.h>, which <string> includes for char_traits<wchar_t>'s mbstate_t and
// wint_t, includes <stdio.h> itself, unconditionally, so EOF is the C library's there)
// EXPECT-ERROR-GCC: .EOF. was not declared in this scope
// EXPECT-ERROR-CLANG: use of undeclared identifier 'EOF'
#define YCXX_NO_TRANSITIVE_INCLUDES
#include <string>

int f() { return EOF; }
