// [numeric.limits.general]: every member function is "static constexpr T f() noexcept", and
// /2 "specializations shall define these values in such a way that they are usable as
// constant expressions". Members are usable in constant expressions and in template arguments.
#include <limits>
#include <type_traits>

template <class T>
constexpr bool check() {
  using L = std::numeric_limits<T>;
  static_assert(noexcept(L::min()) && noexcept(L::max()) && noexcept(L::lowest()));
  static_assert(noexcept(L::epsilon()) && noexcept(L::round_error()) && noexcept(L::infinity()));
  static_assert(noexcept(L::quiet_NaN()) && noexcept(L::signaling_NaN()) && noexcept(L::denorm_min()));
  static_assert(std::is_same_v<decltype(L::max()), T>);
  static_assert(std::is_same_v<decltype(L::digits), const int>);
  static_assert(std::is_same_v<decltype(L::is_signed), const bool>);
  static_assert(std::is_same_v<decltype(L::round_style), const std::float_round_style>);
  constexpr T m = L::max();
  (void)m;
  return true;
}
static_assert(check<bool>());
static_assert(check<char>());
static_assert(check<int>());
static_assert(check<unsigned long long>());
static_assert(check<float>());
static_assert(check<long double>());
static_assert(check<char32_t>());

template <int N>
struct Tag {};
Tag<std::numeric_limits<short>::digits> tag;
// static data members are odr-usable (inline constexpr variables)
const int* p = &std::numeric_limits<int>::digits;
