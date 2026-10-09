// EXPECT-ERROR-GCC: error: call to non-'constexpr' function [^\n]*std::rint\(
// EXPECT-ERROR-CLANG: error: constexpr variable 'bad' must be initialized by a constant expression
// EXPECT-ERROR-CLANG: note: non-constexpr function 'rint<double>'
// [cmath.syn] declares nearbyint, rint, lrint and llrint without constexpr (their results
// depend on the dynamic rounding mode), while trunc/round/floor/ceil are constexpr.
// [constexpr.functions]/1: "An implementation shall not declare any standard library function
// signature as constexpr except for those where it is explicitly required." So std::rint
// cannot be called in a constant expression.
#include <cmath>

constexpr double ok = std::trunc(1.5) + std::round(2.5);  // control: constexpr functions
constexpr double bad = std::rint(1.5);

int main() { return static_cast<int>(ok + bad); }
