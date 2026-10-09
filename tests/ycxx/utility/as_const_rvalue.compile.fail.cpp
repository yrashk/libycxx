// EXPECT-ERROR-GCC: error: use of deleted function [^\n]*std::as_const\(const _Tp&&\)
// EXPECT-ERROR-CLANG: error: call to deleted function 'as_const'
// [utility.syn]: "template<class T> void as_const(const T&&) = delete;"
#include <utility>

void test() { (void)std::as_const(42); }
