// [intseq.binding]/1: tuple_element<I, integer_sequence<T, Values...>> "Mandates:
// I < sizeof...(Values)."
#include <utility>

using T = std::tuple_element<2, std::integer_sequence<int, 1, 2>>::type;
T v = 0;
