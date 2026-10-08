// DECISIONS §19: with YCXX_NO_TRANSITIVE_INCLUDES, <vector> does not provide <string> (std::string
// may be declared, it is not defined); by default it is (../vector.compile.pass.cpp), as libstdc++
// and libc++ both provide it.
// EXPECT-ERROR-GCC: .std::string s. has incomplete type|.string. in namespace .std. does not name a type
// EXPECT-ERROR-CLANG: implicit instantiation of undefined template 'std::basic_string<char|variable has incomplete type 'std::string'|no (type|member) named 'string' in namespace 'std'
#define YCXX_NO_TRANSITIVE_INCLUDES
#include <vector>

std::string s;
