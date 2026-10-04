// [variant.relops]: ==, !=, <, >, <=, >= compare index first, then the contained values with
// the *same* operator (not synthesized from another); each is constrained on that operator
// being valid and convertible to bool for all alternatives. operator<=> requires
// three_way_comparable for all Types and returns
// common_comparison_category_t<compare_three_way_result_t<Types>...>.
#include <variant>
#include <compare>
#include <type_traits>
#include "check.hpp"

// Each operator yields a distinct, recognisable answer so that we can detect an
// implementation that synthesizes one operator from another.
struct Weird {
  int v;
  constexpr friend bool operator==(Weird, Weird) { return true; }
  constexpr friend bool operator!=(Weird, Weird) { return true; }
  constexpr friend bool operator<(Weird, Weird) { return true; }
  constexpr friend bool operator>(Weird, Weird) { return true; }
  constexpr friend bool operator<=(Weird, Weird) { return false; }
  constexpr friend bool operator>=(Weird, Weird) { return false; }
};

struct OnlyEq {
  constexpr friend bool operator==(OnlyEq, OnlyEq) { return true; }
};

struct BoolLike {
  bool b;
  constexpr operator bool() const { return b; }
};
struct ConvEq {
  int v;
  constexpr friend BoolLike operator==(ConvEq a, ConvEq b) { return {a.v == b.v}; }
};

template <class T> concept has_eq = requires(const T& a) { a == a; };
template <class T> concept has_ne = requires(const T& a) { a != a; };
template <class T> concept has_lt = requires(const T& a) { a < a; };
template <class T> concept has_spaceship = requires(const T& a) { a <=> a; };

static_assert(has_eq<std::variant<int, OnlyEq>>);
static_assert(has_ne<std::variant<int, OnlyEq>>);  // rewritten from ==
static_assert(!has_lt<std::variant<int, OnlyEq>>);
static_assert(!has_spaceship<std::variant<int, OnlyEq>>);
static_assert(!has_spaceship<std::variant<int, Weird>>);
static_assert(has_lt<std::variant<int, Weird>>);

static_assert(std::is_same_v<decltype(std::declval<std::variant<int, long>>() <=> std::declval<std::variant<int, long>>()),
                             std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::declval<std::variant<int, double>>() <=> std::declval<std::variant<int, double>>()),
                             std::partial_ordering>);
static_assert(std::is_same_v<decltype(std::declval<std::variant<int>&>() == std::declval<std::variant<int>&>()), bool>);
static_assert(std::is_same_v<decltype(std::declval<std::variant<int>&>() < std::declval<std::variant<int>&>()), bool>);

constexpr bool test() {
  using V = std::variant<int, double>;
  V a(1), b(2), c(0.5);
  if (!(a == a) || a == b || !(a != b) || a != a) return false;
  if (!(a < b) || b < a || !(a <= b) || !(b > a) || !(b >= a)) return false;
  // index dominates: int(any) < double(any)
  if (!(b < c) || !(c > b) || c <= b || b >= c) return false;
  if ((a <=> b) != std::partial_ordering::less) return false;
  if ((c <=> a) != std::partial_ordering::greater) return false;
  if ((a <=> V(1)) != std::partial_ordering::equivalent) return false;
  V nan(__builtin_nan(""));
  if ((nan <=> nan) != std::partial_ordering::unordered) return false;
  if (nan == nan) return false;

  using W = std::variant<int, Weird>;
  W w1(std::in_place_index<1>, 1), w2(std::in_place_index<1>, 2);
  if (!(w1 == w2) || !(w1 != w2) || !(w1 < w2) || !(w1 > w2) || (w1 <= w2) || (w1 >= w2)) return false;
  // different index: index decides, Weird's operators are not consulted
  W wi(0);
  if (wi == w1 || !(wi != w1) || !(wi < w1) || wi > w1 || !(wi <= w1) || wi >= w1) return false;

  std::variant<ConvEq> e1(ConvEq{1}), e2(ConvEq{1}), e3(ConvEq{2});
  if (!(e1 == e2) || e1 == e3) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
