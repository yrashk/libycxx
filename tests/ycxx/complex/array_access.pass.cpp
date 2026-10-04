// [complex.numbers.general]/4: for an lvalue z of type cv complex<T>,
// reinterpret_cast<cv T(&)[2]>(z) is well-formed, and elements 0 and 1 designate the real and
// imaginary parts; for a pointer a to an array of complex<T>, reinterpret_cast<cv T*>(a)[2*i]
// and [2*i+1] designate the real and imaginary parts of a[i].
#include <complex>
#include "check.hpp"

template <class T>
void check() {
  std::complex<T> z(1, 2);
  T(&a)[2] = reinterpret_cast<T(&)[2]>(z);
  CHECK(a[0] == 1 && a[1] == 2);
  a[0] = 5;
  a[1] = -6;
  CHECK(z.real() == 5 && z.imag() == -6);
  const std::complex<T> cz(3, 4);
  const T(&ca)[2] = reinterpret_cast<const T(&)[2]>(cz);
  CHECK(ca[0] == 3 && ca[1] == 4);

  std::complex<T> arr[3] = {{1, 2}, {3, 4}, {5, 6}};
  T* p = reinterpret_cast<T*>(arr);
  for (int i = 0; i < 3; ++i) {
    CHECK(p[2 * i] == arr[i].real());
    CHECK(p[2 * i + 1] == arr[i].imag());
  }
  p[5] = 60;
  CHECK(arr[2].imag() == 60);
  static_assert(sizeof(std::complex<T>) == 2 * sizeof(T));
}

int main() {
  check<float>();
  check<double>();
  check<long double>();
  return 0;
}
