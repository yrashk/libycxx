// EXPECT-ERROR-GCC: error: call to non-'constexpr' function [^\n]*__assertion_failed
// EXPECT-ERROR-GCC: in 'constexpr' expansion of [^\n]*std::saturating_div: division by zero
// EXPECT-ERROR-CLANG: error: constexpr variable 'r' must be initialized by a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*saturating_div: division by zero
// [numeric.sat.func]/9-10: saturating_div: "Preconditions: y != 0 is true." "Remarks: A
// function call expression that violates the precondition in the Preconditions element is
// not a core constant expression." So it cannot initialize a constexpr variable.
#include <numeric>

constexpr int r = std::saturating_div(1, 0);

int main() {
  return r;
}
