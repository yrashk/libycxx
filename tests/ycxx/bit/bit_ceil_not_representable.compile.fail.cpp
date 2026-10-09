// EXPECT-ERROR-GCC: error: call to non-'constexpr' function [^\n]*__assertion_failed
// EXPECT-ERROR-GCC: in 'constexpr' expansion of [^\n]*std::bit_ceil: result not representable
// EXPECT-ERROR-CLANG: error: constexpr variable 'v' must be initialized by a constant expression
// EXPECT-ERROR-CLANG: note: in call to [^\n]*std::bit_ceil: result not representable
// [bit.pow.two]/5,8: bit_ceil "Preconditions: N is representable as a value of type T" and
// "A function call expression that violates the precondition ... is not a core constant
// expression". The smallest power of 2 >= 200 is 256, not representable in uint8_t.
#include <bit>
#include <cstdint>

constexpr std::uint8_t v = std::bit_ceil(std::uint8_t(200));
