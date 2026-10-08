// DECISIONS §19: with YCXX_NO_TRANSITIVE_INCLUDES, <iostream> does not provide <optional>; by
// default it does (../iostream.compile.pass.cpp), as libstdc++ and libc++ both do.
// EXPECT-ERROR-GCC: .optional. in namespace .std. does not name a template type|.optional. is not a member of .std.
// EXPECT-ERROR-CLANG: no (template|member) named 'optional' in namespace 'std'
#define YCXX_NO_TRANSITIVE_INCLUDES
#include <iostream>

std::optional<int> o;
