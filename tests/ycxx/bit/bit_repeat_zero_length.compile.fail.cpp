// [bit.permute]/6,9: bit_repeat "Preconditions: l is greater than zero" and "A function call
// expression that violates the precondition ... is not a core constant expression".
#include <bit>

constexpr unsigned v = std::bit_repeat(1u, 0);
