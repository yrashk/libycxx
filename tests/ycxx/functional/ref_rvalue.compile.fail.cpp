// EXPECT-ERROR-GCC: error: use of deleted function [^\n]*std::ref\(const _Tp&&\)
// EXPECT-ERROR-CLANG: error: call to deleted function 'ref'
// [functional.syn]: "template<class T> void ref(const T&&) = delete;"
#include <functional>

auto r = std::ref(42);
