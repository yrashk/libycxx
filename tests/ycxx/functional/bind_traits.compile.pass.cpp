// [func.bind.isbind]/2: is_bind_expression<T> has base characteristic true_type "if T is a
// type returned from bind, otherwise ... false_type"; programs may specialize it.
// [func.bind.isplace]/2: is_placeholder<T> has base characteristic integral_constant<int, J>
// "if T is the type of std::placeholders::_J, otherwise ... integral_constant<int, 0>".
// [func.bind.place]/2: placeholder types are Cpp17DefaultConstructible and
// Cpp17CopyConstructible, with constexpr non-throwing default/copy/move constructors.
// [functional.syn]: is_bind_expression_v / is_placeholder_v.
#include <functional>
#include <type_traits>

using namespace std::placeholders;

int f(int, int);
template <class T>
using P = std::remove_cvref_t<T>;

static_assert(std::is_placeholder_v<P<decltype(_1)>> == 1);
static_assert(std::is_placeholder_v<P<decltype(_2)>> == 2);
static_assert(std::is_placeholder_v<P<decltype(_3)>> == 3);
static_assert(std::is_placeholder_v<P<decltype(_4)>> == 4);
static_assert(std::is_placeholder_v<P<decltype(_5)>> == 5);
static_assert(std::is_placeholder_v<P<decltype(_6)>> == 6);
static_assert(std::is_placeholder_v<P<decltype(_7)>> == 7);
static_assert(std::is_placeholder_v<P<decltype(_8)>> == 8);
static_assert(std::is_placeholder_v<P<decltype(_9)>> == 9);
static_assert(std::is_placeholder<P<decltype(_1)>>::value == 1);
static_assert(std::is_base_of_v<std::integral_constant<int, 1>, std::is_placeholder<P<decltype(_1)>>>);
static_assert(std::is_base_of_v<std::integral_constant<int, 9>, std::is_placeholder<P<decltype(_9)>>>);
static_assert(std::is_placeholder_v<int> == 0);
static_assert(std::is_base_of_v<std::integral_constant<int, 0>, std::is_placeholder<int>>);
static_assert(std::is_same_v<decltype(std::is_placeholder_v<int>), const int>);
static_assert(!std::is_same_v<P<decltype(_1)>, P<decltype(_2)>>);

using B = decltype(std::bind(f, _1, 2));
using BR = decltype(std::bind<long>(f, _1, 2));
static_assert(std::is_bind_expression_v<B>);
static_assert(std::is_bind_expression_v<BR>);
static_assert(std::is_bind_expression<B>::value);
static_assert(std::is_base_of_v<std::true_type, std::is_bind_expression<B>>);
static_assert(!std::is_bind_expression_v<int>);
static_assert(!std::is_bind_expression_v<P<decltype(_1)>>);
static_assert(!std::is_bind_expression_v<decltype(std::bind_front(f, 1))>);
static_assert(std::is_base_of_v<std::false_type, std::is_bind_expression<int>>);
static_assert(std::is_same_v<decltype(std::is_bind_expression_v<int>), const bool>);
static_assert(std::is_placeholder_v<B> == 0);

// placeholder types
using P1 = P<decltype(_1)>;
static_assert(std::is_nothrow_default_constructible_v<P1>);
static_assert(std::is_nothrow_copy_constructible_v<P1>);
static_assert(std::is_nothrow_move_constructible_v<P1>);
constexpr P1 made{};
constexpr P1 copied = made;
constexpr const auto& ref1 = _1;  // _1 is usable in constant expressions ([func.bind.place]/3)
static_assert(std::is_placeholder_v<P<decltype(ref1)>> == 1);

// program-defined specializations
struct MyPlaceholder {};
struct MyBindExpr {
  int operator()(int x) const { return x; }
};
template <>
struct std::is_placeholder<MyPlaceholder> : std::integral_constant<int, 2> {};
template <>
struct std::is_bind_expression<MyBindExpr> : std::true_type {};
static_assert(std::is_placeholder_v<MyPlaceholder> == 2);
static_assert(std::is_bind_expression_v<MyBindExpr>);
