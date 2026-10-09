// EXPECT-ERROR-GCC: error: use of deleted function [^\n]*operator,\([^\n]*std::constant_wrapper
// EXPECT-ERROR-CLANG: error: overload resolution selected deleted operator ','
// [const.wrap.class]: cw-operators declares
// "template<constexpr-param L, constexpr-param R> friend constexpr auto operator,(L, R)
// noexcept = delete;"
#include <utility>

void test() { auto r = (std::cw<1>, std::cw<2>); }
