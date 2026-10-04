// [pair.astuple]/1: tuple_element<I, pair<T1, T2>>: "Mandates: I<2."
#include <utility>

using T = std::tuple_element<2, std::pair<int, long>>::type;
T t{};
