// [cmp.categories]: partial_ordering, weak_ordering, strong_ordering. Their values ([cmp.partialord],
// [cmp.weakord], [cmp.strongord]) compare with literal 0; implicit conversions only towards
// weaker categories (strong -> weak -> partial); equal == equivalent for strong_ordering; all
// comparisons are constexpr and noexcept; operator<=> with 0 reverses for 0 <=> v.
// [compare.syn]: is_eq, is_neq, is_lt, is_lteq, is_gt, is_gteq.
#include <compare>
#include <type_traits>
#include "check.hpp"

using PO = std::partial_ordering;
using WO = std::weak_ordering;
using SO = std::strong_ordering;

static_assert(std::is_convertible_v<SO, WO> && std::is_convertible_v<SO, PO> && std::is_convertible_v<WO, PO>);
static_assert(!std::is_convertible_v<WO, SO> && !std::is_convertible_v<PO, WO> && !std::is_convertible_v<PO, SO>);
static_assert(!std::is_default_constructible_v<SO> && !std::is_default_constructible_v<PO>);

static_assert(noexcept(SO::less == 0) && noexcept(PO::unordered < 0) && noexcept(0 <=> WO::less));
static_assert(std::is_same_v<decltype(SO::less <=> 0), SO>);
static_assert(std::is_same_v<decltype(0 <=> WO::less), WO>);
static_assert(std::is_same_v<decltype(PO::less <=> 0), PO>);
static_assert(std::is_same_v<decltype(SO::equal), const SO>);

static_assert(SO::equal == SO::equivalent);
static_assert(SO::less < 0 && SO::greater > 0 && SO::equal == 0 && SO::equal <= 0 && SO::equal >= 0);
static_assert(0 > SO::less && 0 < SO::greater && 0 == SO::equal && !(0 != SO::equal));
static_assert(WO::less < 0 && WO::equivalent == 0 && WO::greater > 0);
static_assert(PO::less < 0 && PO::equivalent == 0 && PO::greater > 0);
// unordered compares false with everything except !=
static_assert(!(PO::unordered == 0) && !(PO::unordered < 0) && !(PO::unordered > 0));
static_assert(!(PO::unordered <= 0) && !(PO::unordered >= 0) && PO::unordered != 0);
static_assert(!(0 < PO::unordered) && !(0 >= PO::unordered));
static_assert((PO::unordered <=> 0) == PO::unordered && (0 <=> PO::unordered) == PO::unordered);
// reversal
static_assert((0 <=> SO::less) == SO::greater && (0 <=> SO::greater) == SO::less && (0 <=> SO::equal) == SO::equal);
static_assert((0 <=> WO::less) == WO::greater && (0 <=> PO::greater) == PO::less);
static_assert((SO::less <=> 0) == SO::less);
// conversions
static_assert(WO(SO::less) == WO::less && WO(SO::equal) == WO::equivalent && WO(SO::greater) == WO::greater);
static_assert(PO(SO::equal) == PO::equivalent && PO(WO::greater) == PO::greater && PO(WO::less) == PO::less);
// same-category equality is defaulted
static_assert(SO::less == SO::less && SO::less != SO::greater && PO::unordered == PO::unordered);
static_assert(std::is_same_v<decltype(SO::less == SO::less), bool>);

static_assert(std::is_eq(SO::equal) && !std::is_eq(SO::less) && !std::is_eq(PO::unordered));
static_assert(std::is_neq(WO::less) && std::is_neq(PO::unordered) && !std::is_neq(PO::equivalent));
static_assert(std::is_lt(PO::less) && !std::is_lt(PO::unordered) && !std::is_lt(SO::equal));
static_assert(std::is_lteq(SO::equal) && std::is_lteq(SO::less) && !std::is_lteq(PO::unordered));
static_assert(std::is_gt(WO::greater) && !std::is_gt(PO::unordered));
static_assert(std::is_gteq(SO::equal) && !std::is_gteq(PO::unordered) && !std::is_gteq(SO::less));
static_assert(noexcept(std::is_eq(PO::less)));

int main() {
  volatile int a = 1, b = 2;
  SO r = a <=> b;
  CHECK(r == SO::less);
  CHECK(std::is_lt(r));
  PO p = 1.0 <=> __builtin_nan("");
  CHECK(p == PO::unordered);
  return 0;
}
