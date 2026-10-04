// [complex.tuple]/1: tuple_element<I, complex<T>>: "Mandates: I < 2 is true."
#include <complex>

using T = std::tuple_element<2, std::complex<float>>::type;
T x = 0;
