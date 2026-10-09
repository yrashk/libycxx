// EXPECT-ERROR: error: static assertion failed[^\n]*\[complex\.tuple\]/1: tuple_element index out of range for std::complex
// [complex.tuple]/1: tuple_element<I, complex<T>>: "Mandates: I < 2 is true."
#include <complex>

using T = std::tuple_element<2, std::complex<float>>::type;
T x = 0;
