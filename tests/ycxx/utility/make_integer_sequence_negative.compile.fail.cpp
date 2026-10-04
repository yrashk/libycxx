// [intseq.make]/1: make_integer_sequence<T, N>: "Mandates: N >= 0."
#include <utility>

std::make_integer_sequence<int, -1> s;
