// [complex.tuple]: tuple_size<complex<T>> is integral_constant<size_t, 2>,
// tuple_element<I, complex<T>>::type is T, and get<I> (for &, &&, const &, const &&) returns a
// reference to the real part for I == 0 and to the imaginary part otherwise; noexcept and
// constexpr. So complex supports structured bindings. [tuple.helper]: tuple_size and
// tuple_element for cv-qualified types are available when <complex> is included.
#include <complex>
#include <type_traits>
#include <utility>
#include "check.hpp"

using C = std::complex<double>;
static_assert(std::tuple_size<C>::value == 2);
static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 2>, std::tuple_size<std::complex<float>>>);
static_assert(std::tuple_size_v<const C> == 2);
static_assert(std::is_same_v<std::tuple_element_t<0, C>, double>);
static_assert(std::is_same_v<std::tuple_element_t<1, std::complex<float>>, float>);
static_assert(std::is_same_v<std::tuple_element_t<1, const C>, const double>);

C c;
const C cc;
static_assert(std::is_same_v<decltype(std::get<0>(c)), double&>);
static_assert(std::is_same_v<decltype(std::get<1>(std::move(c))), double&&>);
static_assert(std::is_same_v<decltype(std::get<0>(cc)), const double&>);
static_assert(std::is_same_v<decltype(std::get<1>(std::move(cc))), const double&&>);
static_assert(noexcept(std::get<0>(c)) && noexcept(std::get<1>(cc)) && noexcept(std::get<0>(std::move(c))));

constexpr bool use() {
  C z(1, 2);
  std::get<0>(z) = 5;
  std::get<1>(z) += 1;
  if (z != C(5, 3)) return false;
  auto [re, im] = z;
  if (re != 5 || im != 3) return false;
  auto& [r2, i2] = z;
  r2 = -1;
  i2 = -2;
  if (z != C(-1, -2)) return false;
  const C k(7, 8);
  return std::get<0>(k) == 7 && std::get<1>(k) == 8 && std::get<1>(C(0, 9)) == 9;
}
static_assert(use());

int main() {
  CHECK(use());
  C z(1, 2);
  CHECK(&std::get<0>(z) == &reinterpret_cast<double(&)[2]>(z)[0]);
  CHECK(&std::get<1>(z) == &reinterpret_cast<double(&)[2]>(z)[1]);
  return 0;
}
