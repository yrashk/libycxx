// [pairs.pair]/5-7: pair() is constrained on is_default_constructible_v of both members and
// is explicit iff either member is not implicitly default-constructible. /8-10: pair(const
// T1&, const T2&) explicit iff !is_convertible_v<const T1&, T1> || !is_convertible_v<const
// T2&, T2>. /11-13: pair(U1&&, U2&&) constrained on is_constructible_v, explicit iff
// !is_convertible_v<U1, T1> || !is_convertible_v<U2, T2>, default template arguments U1 = T1,
// U2 = T2 (so braced arguments work). /14-17: converting constructors from pair<U1, U2> in all
// four value categories and from pair-like types, explicit iff a get<i>(FWD(p)) is not
// convertible. /3: the destructor is trivial iff both members are trivially destructible.
#include <utility>
#include <array>
#include <tuple>
#include <type_traits>

struct ExplicitDefault {
  explicit ExplicitDefault() = default;
};
struct NoDefault {
  NoDefault(int);
};
struct ExplicitFromInt {
  explicit ExplicitFromInt(int);
};
struct ExplicitCopy {
  ExplicitCopy() = default;
  explicit ExplicitCopy(const ExplicitCopy&) = default;
};
struct FromLvalueOnly {
  FromLvalueOnly(int&);
};
struct NonTrivialDtor {
  ~NonTrivialDtor() {}
};

template <class T>
concept implicitly_default_constructible = requires { [](T) {}({}); };

// default constructor
static_assert(implicitly_default_constructible<std::pair<int, double>>);
static_assert(std::is_default_constructible_v<std::pair<int, ExplicitDefault>>);
static_assert(!implicitly_default_constructible<std::pair<int, ExplicitDefault>>);
static_assert(!std::is_default_constructible_v<std::pair<int, NoDefault>>);
// pair(const T1&, const T2&)
static_assert(std::is_convertible_v<const int&, int>);
static_assert(std::is_constructible_v<std::pair<ExplicitCopy, int>, const ExplicitCopy&, const int&>);
template <class P, class A, class B>
concept implicit_from_two = requires(const A& a, const B& b) { [](P) {}({a, b}); };
static_assert(implicit_from_two<std::pair<int, long>, int, long>);
static_assert(!implicit_from_two<std::pair<ExplicitCopy, int>, ExplicitCopy, int>);
// pair(U1&&, U2&&)
static_assert(std::is_constructible_v<std::pair<ExplicitFromInt, int>, int, int>);
static_assert(!std::is_convertible_v<std::pair<int, int>, std::pair<ExplicitFromInt, int>>);
static_assert(std::is_constructible_v<std::pair<FromLvalueOnly, int>, int&, int>);
static_assert(!std::is_constructible_v<std::pair<FromLvalueOnly, int>, int, int>);
static_assert(!std::is_constructible_v<std::pair<int, int>, int, int*>);
// braced arguments use the default template arguments U1 = T1, U2 = T2
std::pair<std::pair<int, int>, int> braced({1, 2}, {});
std::pair<int, NoDefault> implicit_conv = {1, 2};
// converting from other pairs
static_assert(std::is_convertible_v<std::pair<int, float>, std::pair<long, double>>);
static_assert(std::is_constructible_v<std::pair<ExplicitFromInt, int>, std::pair<int, int>>);
static_assert(!std::is_convertible_v<std::pair<int, int>, std::pair<ExplicitFromInt, int>>);
static_assert(!std::is_convertible_v<const std::pair<int, int>&, std::pair<ExplicitFromInt, int>>);
// value category of the source pair is used: pair<U1,U2>& gives an lvalue get<0>
static_assert(std::is_constructible_v<std::pair<FromLvalueOnly, int>, std::pair<int, int>&>);
static_assert(!std::is_constructible_v<std::pair<FromLvalueOnly, int>, const std::pair<int, int>&>);
static_assert(!std::is_constructible_v<std::pair<FromLvalueOnly, int>, std::pair<int, int>&&>);
// pair-like sources
static_assert(std::is_convertible_v<std::tuple<int, long>, std::pair<long, int>>);
static_assert(std::is_convertible_v<std::array<int, 2>, std::pair<long, long>>);
static_assert(!std::is_constructible_v<std::pair<int, int>, std::tuple<int>>);
static_assert(!std::is_constructible_v<std::pair<int, int>, std::array<int, 3>>);
static_assert(!std::is_convertible_v<std::tuple<int, int>, std::pair<ExplicitFromInt, int>>);
static_assert(std::is_constructible_v<std::pair<ExplicitFromInt, int>, std::tuple<int, int>>);
// triviality
static_assert(std::is_trivially_destructible_v<std::pair<int, double>>);
static_assert(!std::is_trivially_destructible_v<std::pair<int, NonTrivialDtor>>);
static_assert(std::is_trivially_copy_constructible_v<std::pair<int, double>>);
static_assert(std::is_trivially_move_constructible_v<std::pair<int, double>>);
