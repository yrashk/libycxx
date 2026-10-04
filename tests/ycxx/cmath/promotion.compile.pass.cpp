// [cmath.syn]/2-3: an overload for each cv-unqualified floating-point type; for mixed
// arithmetic arguments (other than abs) every argument is effectively cast to the
// floating-point type with the greatest floating-point conversion rank (and subrank), where
// integer arguments count as double. [c.math.abs]: abs has int, long, long long and
// floating-point-type overloads, so a small integer argument promotes to int.
#include <cmath>
#include <type_traits>

using std::is_same_v;
static_assert(is_same_v<decltype(std::pow(2.0f, 3.0f)), float>);
static_assert(is_same_v<decltype(std::pow(2.0f, 3)), double>);
static_assert(is_same_v<decltype(std::pow(2, 3)), double>);
static_assert(is_same_v<decltype(std::pow(2.0f, 3.0L)), long double>);
static_assert(is_same_v<decltype(std::pow(2LL, 3.0L)), long double>);
static_assert(is_same_v<decltype(std::atan2(1.0f, 2.0f)), float>);
static_assert(is_same_v<decltype(std::atan2(1u, 2.0f)), double>);
static_assert(is_same_v<decltype(std::fmod('a', 2.0f)), double>);
static_assert(is_same_v<decltype(std::fma(1.0f, 2.0f, 3.0f)), float>);
static_assert(is_same_v<decltype(std::fma(1.0f, 2.0f, 3)), double>);
static_assert(is_same_v<decltype(std::fma(1.0f, 2.0L, 3.0f)), long double>);
static_assert(is_same_v<decltype(std::remquo(1.0f, 2, nullptr)), double>);
static_assert(is_same_v<decltype(std::copysign(1, -1.0f)), double>);
static_assert(is_same_v<decltype(std::nextafter(1.0f, 2.0f)), float>);
static_assert(is_same_v<decltype(std::nexttoward(1.0f, 2.0L)), float>);
static_assert(is_same_v<decltype(std::nexttoward(1, 2.0L)), double>);
static_assert(is_same_v<decltype(std::ldexp(1.0f, 2)), float>);
static_assert(is_same_v<decltype(std::ldexp(1, 2)), double>);
static_assert(is_same_v<decltype(std::frexp(1.0L, nullptr)), long double>);
static_assert(is_same_v<decltype(std::ilogb(1.0f)), int>);
static_assert(is_same_v<decltype(std::lround(1.0f)), long>);
static_assert(is_same_v<decltype(std::llrint(1.0)), long long>);
static_assert(is_same_v<decltype(std::sin(1)), double>);
static_assert(is_same_v<decltype(std::sin(1.0f)), float>);
static_assert(is_same_v<decltype(std::sqrt(1.0L)), long double>);
static_assert(is_same_v<decltype(std::cbrt(8u)), double>);
static_assert(is_same_v<decltype(std::fabs(-1)), double>);
static_assert(is_same_v<decltype(std::abs(1.0f)), float>);
static_assert(is_same_v<decltype(std::abs(1.0L)), long double>);
static_assert(is_same_v<decltype(std::abs(short(-1))), int>);
static_assert(is_same_v<decltype(std::abs(-1L)), long>);
static_assert(is_same_v<decltype(std::hypot(1.0f, 2.0f)), float>);
static_assert(is_same_v<decltype(std::hypot(1.0f, 2)), double>);
static_assert(is_same_v<decltype(std::fdim(1.0L, 2)), long double>);
static_assert(is_same_v<decltype(std::fmin(1.0f, 2.0f)), float>);
static_assert(is_same_v<decltype(std::isgreater(1.0f, 2)), bool>);
static_assert(is_same_v<decltype(std::sinf(1.0f)), float>);
static_assert(is_same_v<decltype(std::sinl(1.0L)), long double>);
static_assert(is_same_v<decltype(std::legendre(2u, 0.5f)), float>);
static_assert(is_same_v<decltype(std::legendre(2u, 1)), double>);
static_assert(is_same_v<decltype(std::beta(1.0f, 2)), double>);
static_assert(is_same_v<decltype(std::beta(1.0f, 2.0f)), float>);
static_assert(is_same_v<decltype(std::ellint_3(0.5f, 0.5L, 1.0f)), long double>);

// The C type aliases float_t and double_t ([cmath.syn]) are floating-point types.
static_assert(std::is_floating_point_v<std::float_t> && std::is_floating_point_v<std::double_t>);

// Each overload accepts its own type: overload resolution is exact for every standard type.
template <class T> concept has_round = requires(T x) { { std::round(x) } -> std::same_as<T>; };
static_assert(has_round<float> && has_round<double> && has_round<long double>);
