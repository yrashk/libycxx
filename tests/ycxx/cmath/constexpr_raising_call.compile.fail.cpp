// EXPECT-ERROR-GCC: error: '__builtin_log\(0\.0\)' is not a constant expression
// EXPECT-ERROR-CLANG: error: constexpr variable 'r' must be initialized by a constant expression
// EXPECT-ERROR-CLANG: note: in call to 'log<double>\(0\.000000e\+00\)'
// [library.c]/3: "A call to a C standard library function is a non-constant library call
// ([defns.nonconst.libcall]) if it raises a floating-point exception other than FE_INEXACT."
// log(0) raises FE_DIVBYZERO (ISO/IEC 9899:2024 F.10.3.11), so it cannot initialize a
// constexpr variable ([expr.const]: a non-constant library call is not a core constant
// expression), although log is constexpr ([cmath.syn]).
#include <cmath>

constexpr double ok = std::log(1.0);  // well-formed: see constexpr_special_values.pass.cpp
constexpr double r = std::log(0.0);

int main() { return static_cast<int>(r + ok); }
