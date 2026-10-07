// DECISIONS §19: with YCXX_NO_TRANSITIVE_INCLUDES, <string> does not provide <algorithm>
// ([string.syn] includes only <compare> and <initializer_list>); by default it does
// (../string.compile.pass.cpp), as libstdc++ and libc++ both do.
// EXPECT-ERROR-GCC: .min. is not a member of .std.
// EXPECT-ERROR-CLANG: no member named 'min' in namespace 'std'
#define YCXX_NO_TRANSITIVE_INCLUDES
#include <string>

int f() { return std::min(1, 2); }
