// EXPECT-ERROR-GCC: error: argument to '__integer_pack' must be between 0 and
// EXPECT-ERROR-GCC: required by substitution of [^\n]*std::make_integer_sequence[^\n]*= -1
// EXPECT-ERROR-CLANG: error: integer sequences must have non-negative sequence length
// [intseq.make]/1: make_integer_sequence<T, N>: "Mandates: N >= 0."
#include <utility>

std::make_integer_sequence<int, -1> s;
