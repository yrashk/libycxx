// [pairs.spec]/1: operator== "Constraints: x.first == y.first and x.second == y.second are
// valid expressions and each of decltype(x.first == y.first) and decltype(x.second ==
// y.second) models boolean-testable." /3: operator<=> has the return type
// common_comparison_category_t<synth-three-way-result<T1, U1>, synth-three-way-result<T2, U2>>,
// so it is only usable when synth-three-way is usable for both members.
#include <utility>
#include <concepts>
#include <compare>

struct NoEq {};
struct NotBoolTestable {};
struct WeirdEq {
  friend NotBoolTestable operator==(const WeirdEq&, const WeirdEq&) { return {}; }
};
struct EqOnly {
  friend bool operator==(const EqOnly&, const EqOnly&) = default;
};

template <class A, class B>
concept eq = requires(const A& a, const B& b) { a == b; };
template <class A, class B>
concept three_way = requires(const A& a, const B& b) { a <=> b; };

static_assert(eq<std::pair<int, int>, std::pair<long, char>>);
static_assert(!eq<std::pair<int, NoEq>, std::pair<int, NoEq>>);
static_assert(!eq<std::pair<WeirdEq, int>, std::pair<WeirdEq, int>>);
static_assert(!eq<std::pair<int, int>, std::pair<int, int*>>);
static_assert(eq<std::pair<int, EqOnly>, std::pair<int, EqOnly>>);
static_assert(std::equality_comparable<std::pair<int, EqOnly>>);
static_assert(!std::equality_comparable<std::pair<int, NoEq>>);

static_assert(three_way<std::pair<int, int>, std::pair<int, int>>);
static_assert(!three_way<std::pair<int, EqOnly>, std::pair<int, EqOnly>>);
static_assert(std::three_way_comparable<std::pair<int, double>, std::partial_ordering>);
static_assert(std::totally_ordered<std::pair<int, char>>);
