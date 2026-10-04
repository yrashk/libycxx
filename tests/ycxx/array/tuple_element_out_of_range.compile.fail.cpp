// [array.tuple]/1: tuple_element<I, array<T, N>>: "Mandates: I < N is true."
#include <array>

using T = std::tuple_element<3, std::array<int, 3>>::type;
T* p = nullptr;
