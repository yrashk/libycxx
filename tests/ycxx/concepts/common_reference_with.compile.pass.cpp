// [concept.commonref]: common_reference_with<T, U> = same_as<common_reference_t<T, U>,
// common_reference_t<U, T>> && convertible_to<T, C> && convertible_to<U, C>.
// [concept.common]: common_with additionally requires common_type and the
// const-lvalue common references. Users may customise via basic_common_reference and
// common_type specialisations.
#include <concepts>
#include <type_traits>

struct A {};
struct B {
  B(A);
};
struct X {};
struct Y {};
struct XY {
  XY(X);
  XY(Y);
};
template <>
struct std::common_type<X, Y> {
  using type = XY;
};
template <>
struct std::common_type<Y, X> {
  using type = XY;
};

static_assert(std::common_reference_with<int&, const int&>);
static_assert(std::common_reference_with<int&, long>);
static_assert(std::common_reference_with<int, int&&>);
static_assert(!std::common_reference_with<int*, double*>);
static_assert(std::common_reference_with<A, B>);
static_assert(std::common_with<int, long>);
static_assert(std::common_with<int, double>);
static_assert(std::common_with<A, B>);
static_assert(!std::common_with<int*, double*>);
static_assert(std::common_with<X, Y>);  // via the common_type specialisations
static_assert(!std::common_with<X, A>);
static_assert(std::common_with<void, void>);
