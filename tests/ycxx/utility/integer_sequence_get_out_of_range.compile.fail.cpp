// EXPECT-ERROR-GCC: error: static assertion failed[^\n]*index out of range
// EXPECT-ERROR-GCC: In instantiation of [^\n]*std::get\(integer_sequence
// EXPECT-ERROR-CLANG: error: static assertion failed[^\n]*index out of range
// EXPECT-ERROR-CLANG: note: in instantiation of function template specialization 'std::get<3UL, int, 1, 2, 3>'
// [intseq.binding]/2: get "Mandates: I < sizeof...(Values)."
#include <utility>

int v = std::get<3>(std::integer_sequence<int, 1, 2, 3>{});
