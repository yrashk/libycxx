// [numeric.limits.general]/3: "For the numeric_limits primary template, all data members are
// value-initialized and all member functions return a value-initialized object." /5: "The
// value of each member of a specialization of numeric_limits on a cv-qualified type cv T shall
// be equal to the value of the corresponding member of the specialization on the unqualified
// type T." /6: non-arithmetic standard types shall not have specializations.
// [round.style], [numeric.limits]: float_round_style enumerators and their values.
#include <limits>
#include <cstddef>
#include <type_traits>

struct Udt {
  int v = 7;
  constexpr bool operator==(const Udt&) const = default;
};
enum E { e = 3 };

using P = std::numeric_limits<Udt>;
static_assert(!P::is_specialized);
static_assert(P::min() == Udt() && P::max() == Udt() && P::lowest() == Udt() && P::epsilon() == Udt());
static_assert(P::infinity() == Udt() && P::quiet_NaN() == Udt() && P::denorm_min() == Udt());
static_assert(P::digits == 0 && P::radix == 0 && !P::is_signed && !P::is_bounded && !P::is_iec559);
static_assert(P::round_style == std::round_toward_zero);
static_assert(std::is_same_v<decltype(P::max()), Udt>);
static_assert(!std::numeric_limits<E>::is_specialized);
static_assert(std::numeric_limits<E>::max() == E());
static_assert(!std::numeric_limits<int*>::is_specialized);
static_assert(std::numeric_limits<int*>::max() == nullptr);
static_assert(!std::numeric_limits<std::byte>::is_specialized);

template <class T, class U>
constexpr bool same_values() {
  using A = std::numeric_limits<T>;
  using B = std::numeric_limits<U>;
  return A::is_specialized == B::is_specialized && A::min() == B::min() && A::max() == B::max() &&
         A::lowest() == B::lowest() && A::digits == B::digits && A::digits10 == B::digits10 &&
         A::is_signed == B::is_signed && A::is_integer == B::is_integer && A::epsilon() == B::epsilon() &&
         A::radix == B::radix && A::is_iec559 == B::is_iec559 && A::is_modulo == B::is_modulo &&
         A::round_style == B::round_style && A::has_infinity == B::has_infinity &&
         A::max_exponent == B::max_exponent;
}
static_assert(same_values<const int, int>());
static_assert(same_values<volatile unsigned, unsigned>());
static_assert(same_values<const volatile long long, long long>());
static_assert(same_values<const double, double>());
static_assert(same_values<const volatile float, float>());
static_assert(same_values<const bool, bool>());
static_assert(std::numeric_limits<const int>::is_specialized);

static_assert(std::round_indeterminate == -1);
static_assert(std::round_toward_zero == 0);
static_assert(std::round_to_nearest == 1);
static_assert(std::round_toward_infinity == 2);
static_assert(std::round_toward_neg_infinity == 3);
static_assert(std::is_enum_v<std::float_round_style>);
