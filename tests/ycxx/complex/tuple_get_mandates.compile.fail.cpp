// [complex.tuple]/2: get<I>(complex<T>&): "Mandates: I < 2 is true."
#include <complex>

std::complex<double> z;
double& r = std::get<2>(z);
