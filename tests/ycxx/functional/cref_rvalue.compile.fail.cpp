// EXPECT-ERROR-GCC: error: use of deleted function [^\n]*std::cref\(const _Tp&&\)
// EXPECT-ERROR-CLANG: error: call to deleted function 'cref'
// [functional.syn]: "template<class T> void cref(const T&&) = delete;"
#include <functional>

auto r = std::cref(42);
