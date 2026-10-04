// [cmp.common]: common_comparison_category<Ts...>::type is void if any Ti is not a comparison
// category type; otherwise strong_ordering if empty, partial_ordering if any is
// partial_ordering, weak_ordering if any is weak_ordering, else strong_ordering.
// [cmp.concept]: three_way_comparable, three_way_comparable_with.
// [cmp.result]: compare_three_way_result<T, U> has member type decltype(declval<const
// remove_reference_t<T>&>() <=> declval<const remove_reference_t<U>&>()) if well-formed,
// otherwise no member.
#include <compare>
#include <type_traits>

using PO = std::partial_ordering;
using WO = std::weak_ordering;
using SO = std::strong_ordering;

static_assert(std::is_same_v<std::common_comparison_category_t<>, SO>);
static_assert(std::is_same_v<std::common_comparison_category_t<SO>, SO>);
static_assert(std::is_same_v<std::common_comparison_category_t<SO, WO>, WO>);
static_assert(std::is_same_v<std::common_comparison_category_t<SO, WO, PO>, PO>);
static_assert(std::is_same_v<std::common_comparison_category_t<WO, WO>, WO>);
static_assert(std::is_same_v<std::common_comparison_category_t<SO, int>, void>);
static_assert(std::is_same_v<std::common_comparison_category_t<const SO>, void>);  // cv-qualified is not a category
static_assert(std::is_same_v<std::common_comparison_category_t<SO&>, void>);
static_assert(std::is_same_v<std::common_comparison_category_t<bool>, void>);

struct Weak {
  friend WO operator<=>(const Weak&, const Weak&);
  friend bool operator==(const Weak&, const Weak&);
};
struct Partial {
  friend PO operator<=>(const Partial&, const Partial&);
  friend bool operator==(const Partial&, const Partial&);
};
struct NoSpaceship {
  friend bool operator==(const NoSpaceship&, const NoSpaceship&);
};
struct NonConstSpaceship {
  SO operator<=>(const NonConstSpaceship&);  // not callable on const lvalues
  bool operator==(const NonConstSpaceship&) const;
};

static_assert(std::three_way_comparable<int>);
static_assert(std::three_way_comparable<int, std::strong_ordering>);
static_assert(std::three_way_comparable<double, std::partial_ordering>);
static_assert(!std::three_way_comparable<double, std::weak_ordering>);
static_assert(std::three_way_comparable<Weak, std::weak_ordering>);
static_assert(!std::three_way_comparable<Weak, std::strong_ordering>);
static_assert(std::three_way_comparable<Partial>);
static_assert(!std::three_way_comparable<NoSpaceship>);
static_assert(!std::three_way_comparable<NonConstSpaceship>);
static_assert(std::three_way_comparable_with<int, long>);
static_assert(std::three_way_comparable_with<int, double, std::partial_ordering>);
static_assert(!std::three_way_comparable_with<int, int*>);

template <class T, class U>
concept has_result = requires { typename std::compare_three_way_result<T, U>::type; };
static_assert(std::is_same_v<std::compare_three_way_result_t<int>, SO>);
static_assert(std::is_same_v<std::compare_three_way_result_t<int, double>, PO>);
static_assert(std::is_same_v<std::compare_three_way_result_t<Weak&>, WO>);
static_assert(!has_result<NoSpaceship, NoSpaceship>);
static_assert(!has_result<NonConstSpaceship, NonConstSpaceship>);
static_assert(!has_result<int, int*>);
