// DECISIONS §19: with YCXX_NO_TRANSITIVE_INCLUDES, <vector> does not provide <string>; by default
// it does (../vector.compile.pass.cpp), as libstdc++ and libc++ both do.
// EXPECT-ERROR-GCC: .string. in namespace .std. does not name a type|.string. is not a member of .std.
// EXPECT-ERROR-CLANG: no (type|member) named 'string' in namespace 'std'
#define YCXX_NO_TRANSITIVE_INCLUDES
#include <vector>

std::string s;
