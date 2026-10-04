// [intseq.binding]/2: get "Mandates: I < sizeof...(Values)."
#include <utility>

int v = std::get<3>(std::integer_sequence<int, 1, 2, 3>{});
