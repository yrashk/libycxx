// [bit.pow.two]/5,8: bit_ceil "Preconditions: N is representable as a value of type T" and
// "A function call expression that violates the precondition ... is not a core constant
// expression". The smallest power of 2 >= 200 is 256, not representable in uint8_t.
#include <bit>
#include <cstdint>

constexpr std::uint8_t v = std::bit_ceil(std::uint8_t(200));
