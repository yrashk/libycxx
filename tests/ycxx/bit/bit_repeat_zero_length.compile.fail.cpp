// EXPECT-ERROR-GCC: error: call to non-'constexpr' function [^\n]*__assertion_failed
// EXPECT-ERROR-GCC: in 'constexpr' expansion of [^\n]*std::bit_repeat: l must be positive
// EXPECT-ERROR-CLANG: error: constexpr variable 'v' must be initialized by a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::bit_repeat: l must be positive
// [bit.permute]/6,9: bit_repeat "Preconditions: l is greater than zero" and "A function call
// expression that violates the precondition ... is not a core constant expression".
#include <bit>

constexpr unsigned v = std::bit_repeat(1u, 0);
