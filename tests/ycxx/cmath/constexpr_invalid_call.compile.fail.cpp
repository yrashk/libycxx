// [library.c]/3: a C library call that raises a floating-point exception other than
// FE_INEXACT is a non-constant library call. sqrt(-1) raises FE_INVALID (ISO/IEC 9899:2024
// F.10.4.10), so it is not a core constant expression, although sqrt is constexpr.
#include <cmath>

constexpr double ok = std::sqrt(4.0);  // well-formed: see constexpr_exact.pass.cpp
constexpr double r = std::sqrt(-1.0);

int main() { return static_cast<int>(r + ok); }
