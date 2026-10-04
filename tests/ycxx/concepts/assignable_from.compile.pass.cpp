// [concept.assignable]: assignable_from<LHS, RHS> = is_lvalue_reference_v<LHS> &&
// common_reference_with<const remove_reference_t<LHS>&, const remove_reference_t<RHS>&> &&
// requires(LHS lhs, RHS&& rhs) { { lhs = std::forward<RHS>(rhs) } -> same_as<LHS>; };
#include <concepts>

struct ReturnsVoid {
  void operator=(const ReturnsVoid&);
};
struct ReturnsValue {
  ReturnsValue operator=(const ReturnsValue&);
};
struct OnlyFromInt {
  OnlyFromInt& operator=(int);
};
struct Ok {
  Ok& operator=(const Ok&) = default;
};

static_assert(std::assignable_from<int&, int>);
static_assert(std::assignable_from<int&, long>);
static_assert(std::assignable_from<int&, const int&>);
static_assert(!std::assignable_from<int, int>);         // LHS must be an lvalue reference
static_assert(!std::assignable_from<int&&, int>);
static_assert(!std::assignable_from<const int&, int>);
static_assert(!std::assignable_from<ReturnsVoid&, const ReturnsVoid&>);    // result must be LHS
static_assert(!std::assignable_from<ReturnsValue&, const ReturnsValue&>);
static_assert(std::assignable_from<Ok&, Ok>);
static_assert(!std::assignable_from<OnlyFromInt&, int>);  // no common reference with int
static_assert(!std::assignable_from<int(&)[2], int(&)[2]>);
