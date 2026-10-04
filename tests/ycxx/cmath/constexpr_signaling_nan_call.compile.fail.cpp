// [library.c]/3: a call to a C standard library function that raises a floating-point exception
// other than FE_INEXACT is a non-constant library call. ISO/IEC 9899:2024 F.10.4.10 / IEC 60559:
// sqrt of a signaling NaN raises FE_INVALID (every computational operation does, F.2.1), so
// sqrt(signaling_NaN()) is not a constant expression, although sqrt is constexpr and
// classifying the same signaling NaN is (classification_signaling_nan.pass.cpp).
#include <cmath>
#include <limits>

constexpr double snan = std::numeric_limits<double>::signaling_NaN();
constexpr bool ok = std::isnan(snan) && std::sqrt(4.0) == 2.0;  // control
constexpr double bad = std::sqrt(snan);

int main() { return ok + static_cast<int>(bad == bad); }
