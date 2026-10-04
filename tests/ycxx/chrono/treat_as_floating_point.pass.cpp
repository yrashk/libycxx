// [time.traits.is.fp]: treat_as_floating_point<Rep> derives from is_floating_point<Rep>;
// treat_as_floating_point_v; a program may specialize it for a class emulating an arithmetic type,
// which then allows implicit conversions among durations ([time.duration.cons]).
#include <chrono>
#include <ratio>
#include <type_traits>
#include "check.hpp"

struct my_fixed {  // a class emulating an arithmetic type (common_type with integers: my_fixed)
  double v = 0;
  constexpr my_fixed() = default;
  template <class T>
    requires std::is_arithmetic_v<T>
  constexpr my_fixed(T x) : v(double(x)) {}
  friend constexpr my_fixed operator*(my_fixed a, my_fixed b) { return my_fixed(a.v * b.v); }
  friend constexpr my_fixed operator/(my_fixed a, my_fixed b) { return my_fixed(a.v / b.v); }
  friend constexpr my_fixed operator+(my_fixed a, my_fixed b) { return my_fixed(a.v + b.v); }
  friend constexpr bool operator==(my_fixed a, my_fixed b) { return a.v == b.v; }
  constexpr explicit operator long long() const { return (long long)v; }
};
template <> struct std::chrono::treat_as_floating_point<my_fixed> : std::true_type {};

using namespace std::chrono;

static_assert(std::is_base_of_v<std::is_floating_point<double>, treat_as_floating_point<double>>);
static_assert(std::is_base_of_v<std::is_floating_point<int>, treat_as_floating_point<int>>);
static_assert(treat_as_floating_point_v<float> && treat_as_floating_point_v<double> &&
              treat_as_floating_point_v<long double>);
static_assert(!treat_as_floating_point_v<int> && !treat_as_floating_point_v<unsigned long long>);
static_assert(!treat_as_floating_point_v<bool>);
static_assert(treat_as_floating_point_v<my_fixed>);
static_assert(std::is_same_v<decltype(treat_as_floating_point_v<int>), const bool>);

// With the specialization, conversions that would truncate are implicit.
static_assert(std::is_convertible_v<milliseconds, duration<my_fixed>>);
static_assert(std::is_constructible_v<duration<my_fixed>, double>);

int main() {
  duration<my_fixed> d = milliseconds(1500);
  CHECK(d.count() == my_fixed(1.5));
}
