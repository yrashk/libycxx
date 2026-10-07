// DECISIONS §19: with YCXX_NO_TRANSITIVE_INCLUDES, <mutex> does not provide <cerrno>'s errno; by
// default it does (doctest uses errno after <mutex>; ../mutex.compile.pass.cpp).
// EXPECT-ERROR-GCC: .errno. was not declared in this scope
// EXPECT-ERROR-CLANG: use of undeclared identifier 'errno'
#define YCXX_NO_TRANSITIVE_INCLUDES
#include <mutex>

int f() { return errno; }
