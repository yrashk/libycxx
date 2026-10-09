// EXPECT-ERROR: error: static assertion failed[^\n]*integer_sequence requires an integer type
// [intseq.intseq]/1: integer_sequence<T, I...>: "Mandates: T is an integer type."
#include <utility>

std::integer_sequence<double> s;
