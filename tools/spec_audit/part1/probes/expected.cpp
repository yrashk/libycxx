// [expected]: unexpected, bad_expected_access, unexpect, expected<T, E>, expected<void, E>:
// constexpr, monadic operations, constrained equality, freestanding (value() freestanding-deleted).
// FREESTANDING
#include <expected>
#include <type_traits>
#include <utility>

struct NoEq {};
template <class A, class B>
concept eq = requires(const A& a, const B& b) { a == b; };

using X = std::expected<int, long>;
using V = std::expected<void, long>;
static_assert(std::is_trivially_copyable_v<X> && std::is_trivially_destructible_v<V>);
static_assert(std::is_same_v<X::value_type, int> && std::is_same_v<X::error_type, long> &&
              std::is_same_v<X::unexpected_type, std::unexpected<long>>);
static_assert(std::is_same_v<X::rebind<char>, std::expected<char, long>>);
// [expected.un.general]: unexpected<E>
static_assert(std::unexpected(3).error() == 3 && std::is_same_v<decltype(std::unexpected(3)), std::unexpected<int>>);
static_assert(noexcept(std::declval<std::unexpected<int>&>().error()) && noexcept(std::declval<std::unexpected<int>&&>().error()));
template <class E>
concept unexpected_ok = requires { typename std::unexpected<E>; sizeof(std::unexpected<E>); };
static_assert(std::is_constructible_v<std::unexpected<int>, std::in_place_t, int>);
// constexpr construction, assignment, swap, emplace
static_assert([] { X a = 1, b = std::unexpected(2L); a.swap(b); a.emplace(3); return *a == 3 && b.value_or(0) == 1; }());
static_assert([] { V v; v = std::unexpected(4L); v.emplace(); return v.has_value(); }());
static_assert(X(std::unexpect, 5).error() == 5 && !X(std::unexpect, 5).has_value());
// observers
static_assert(noexcept(*std::declval<X&>()) && noexcept(std::declval<X&>().has_value()) && noexcept(std::declval<X&>().error()) &&
              noexcept(static_cast<bool>(std::declval<X&>())) && noexcept(std::declval<X&>().operator->()));
static_assert(X().value_or(9) == 0 && X(std::unexpect).error_or(7) == 0 && X().error_or(7) == 7);
static_assert(std::is_same_v<decltype(X().value_or({})), int>);    // value_or's U = remove_cv_t<T>
static_assert(std::is_same_v<decltype(X().error_or({})), long>);   // error_or's G = E
// monadic
static_assert(X(1).and_then([](int x) { return X(x + 1); }) == 2);
static_assert(std::is_same_v<decltype(X(1).transform([](int) { return 1.0; })), std::expected<double, long>>);
static_assert(std::is_same_v<decltype(X(1).transform([](int) {})), std::expected<void, long>>);
static_assert(std::is_same_v<decltype(X(1).transform_error([](long) { return 'c'; })), std::expected<int, char>>);
static_assert(X(std::unexpect, 1).or_else([](long) { return X(5); }) == 5);
static_assert(V().and_then([] { return V(); }).has_value());
// [expected.object.eq] constraints (P2944)
static_assert(!eq<std::expected<NoEq, int>, std::expected<NoEq, int>> && !eq<std::expected<int, NoEq>, std::unexpected<NoEq>>);
static_assert(!eq<std::expected<NoEq, int>, NoEq> && eq<X, int> && eq<X, std::unexpected<int>> && eq<V, V>);
static_assert(!eq<std::expected<void, NoEq>, std::expected<void, NoEq>>);
// expected<T, E> with E = unexpected<...>, T = in_place_t, ... are ill-formed (Mandates); not probed
// [expected.bad]
static_assert(std::is_base_of_v<std::bad_expected_access<void>, std::bad_expected_access<int>>);
static_assert(std::is_base_of_v<std::exception, std::bad_expected_access<void>>);
static_assert(std::is_same_v<decltype(std::declval<std::bad_expected_access<int>&>().error()), int&>);
static_assert(std::is_same_v<decltype(std::declval<std::bad_expected_access<int>&&>().error()), int&&>);
static_assert(noexcept(std::declval<std::bad_expected_access<int>&>().error()));
static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::unexpect)>, std::unexpect_t>);
template <class T>
concept from_braces = requires(void (*f)(T)) { f({}); };
static_assert(!from_braces<std::unexpect_t>);
