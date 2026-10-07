// [tuple], [pairs]: tuple and pair (constexpr, const assignment P2321, tuple-like construction
// and comparison P2165, constrained comparisons P2944, CTAD), apply (and its 2026 form with
// is_applicable/apply_result), make_from_tuple, tuple_cat, forward_as_tuple, tie, ignore,
// basic_common_reference/common_type specializations. All freestanding.
// FREESTANDING
#include <tuple>
#include <array>
#include <memory>
#include <compare>
#include <type_traits>
#include <utility>

struct NoEq {};
template <class A, class B>
concept eq = requires(const A& a, const B& b) { a == b; };
template <class A, class B>
concept lt = requires(const A& a, const B& b) { a < b; };

using T = std::tuple<int, long>;
using P = std::pair<int, long>;
static_assert(std::tuple_size_v<T> == 2 && std::is_same_v<std::tuple_element_t<1, T>, long>);
static_assert(std::tuple_size_v<P> == 2 && std::is_same_v<std::tuple_element_t<0, const P>, const int>);
static_assert(std::is_trivially_copy_constructible_v<P> && std::is_trivially_destructible_v<T>);
// construction from tuple-like (P2165): tuple from array, pair from tuple
static_assert(std::get<1>(T(std::array<int, 2>{1, 2})) == 2);
static_assert(P(std::tuple<int, long>(1, 2)).second == 2);
static_assert(std::is_convertible_v<std::array<int, 2>, std::tuple<int, int>>);
// comparison with tuple-like (P2165) and constraints (P2944)
static_assert(T(1, 2) == std::tuple<long, int>(1, 2) && T(1, 2) == std::array<long, 2>{1, 2} && (T(1, 2) <=> std::array<long, 2>{1, 3}) < 0);
static_assert(eq<T, std::tuple<long, int>> && !eq<std::tuple<NoEq>, std::tuple<NoEq>> && !eq<std::pair<NoEq, int>, std::pair<NoEq, int>>);
static_assert(std::is_same_v<decltype(T() <=> T()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(P() <=> P()), std::strong_ordering>);
// const-qualified assignment (P2321)
static_assert(std::is_assignable_v<const std::tuple<int&>&, const std::tuple<int>&>);
static_assert(std::is_assignable_v<const std::pair<int&, int&>&, std::pair<int, int>>);
// swap
static_assert([] { T a(1, 2), b(3, 4); a.swap(b); std::swap(a, b); return std::get<0>(a) == 1; }());
static_assert(noexcept(std::declval<T&>().swap(std::declval<T&>())));
// CTAD
static_assert(std::is_same_v<decltype(std::tuple(1, 2L)), T> && std::is_same_v<decltype(std::pair(1, 2L)), P>);
static_assert(std::is_same_v<decltype(std::tuple(P())), T>);
static_assert(std::is_same_v<decltype(std::tuple(std::allocator_arg, std::allocator<int>(), 1)), std::tuple<int>>);
// get by type, by index
static_assert(std::get<long>(T(1, 2)) == 2 && std::get<int>(P(1, 2)) == 1);
static_assert(noexcept(std::get<0>(std::declval<T&>())));
// [tuple.creation]
static_assert(std::is_same_v<decltype(std::make_tuple(1, std::ref(std::declval<int&>()))), std::tuple<int, int&>>);
static_assert(std::is_same_v<decltype(std::forward_as_tuple(1)), std::tuple<int&&>>);
static_assert(std::is_same_v<decltype(std::tuple_cat(T(), std::array<char, 1>(), P())), std::tuple<int, long, char, int, long>>);
static_assert(noexcept(std::tie(std::declval<int&>())));
static_assert([] { int a = 0; std::tie(a, std::ignore) = std::tuple(1, 2); std::ignore = 5; return a == 1; }());
// [tuple.apply]
static_assert(std::apply([](int a, long b) { return a + b; }, T(1, 2)) == 3);
static_assert(noexcept(std::apply([](int, long) noexcept {}, std::declval<T>())));
static_assert(!noexcept(std::apply([](int, long) {}, std::declval<T>())));
template <class F, class Tp>
concept applicable = requires(F f, Tp t) { std::apply(f, t); };
static_assert(!applicable<void (*)(int), T>);     // Constraints: is_applicable_v (apply 202603L)
static_assert(std::is_applicable_v<void (*)(int, long), T> && !std::is_applicable_v<void (*)(int), T>);
static_assert(std::is_nothrow_applicable_v<void (*)(int, long) noexcept, T>);
static_assert(std::is_same_v<std::apply_result_t<double (*)(int, long), P>, double>);
struct S { int a; long b; constexpr S(int x, long y) : a(x), b(y) {} };
static_assert(std::make_from_tuple<S>(T(1, 2)).b == 2);
// [tuple.common.ref]
static_assert(std::is_same_v<std::common_type_t<T, std::tuple<long, int>>, std::tuple<long, long>>);
static_assert(std::is_same_v<std::common_reference_t<std::tuple<int&>&, std::tuple<int>&>, std::tuple<int&>>);
static_assert(std::is_same_v<std::common_type_t<P, std::pair<long, int>>, std::pair<long, long>>);
// pair members
static_assert(std::is_same_v<P::first_type, int> && std::is_same_v<P::second_type, long>);
static_assert(std::is_same_v<decltype(std::make_pair(1, std::ref(std::declval<int&>()))), std::pair<int, int&>>);
static_assert(P(std::piecewise_construct, std::tuple(1), std::tuple(2L)).second == 2);
