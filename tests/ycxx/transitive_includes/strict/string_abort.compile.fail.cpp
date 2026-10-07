// DECISIONS §19: with YCXX_NO_TRANSITIVE_INCLUDES, <string> does not provide <cstdlib>; by default
// it does (Catch2 calls std::abort after <string>; ../string.compile.pass.cpp).
// EXPECT-ERROR-GCC: .abort. is not a member of .std.
// EXPECT-ERROR-CLANG: no member named 'abort' in namespace 'std'
#define YCXX_NO_TRANSITIVE_INCLUDES
#include <string>

void f() { std::abort(); }
