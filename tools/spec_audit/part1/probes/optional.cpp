// [optional]: optional<T>, optional<T&> ([optional.optional.ref]), range support
// ([optional.iterators]), constrained comparisons ([optional.relops], P2944), nullopt,
// bad_optional_access, make_optional, hash. Freestanding except value() (freestanding-deleted).
// FREESTANDING
#include <optional>
#include <compare>
#include <type_traits>
#include <utility>

struct NoEq {};
struct BadEq { friend void operator==(BadEq, BadEq); };
template <class A, class B>
concept eq = requires(const A& a, const B& b) { a == b; };
template <class A, class B>
concept lt = requires(const A& a, const B& b) { a < b; };

using O = std::optional<int>;
static_assert(std::is_trivially_copyable_v<O> && std::is_trivially_destructible_v<O>);
static_assert(std::is_same_v<O::value_type, int>);
// [optional.iterators]: contiguous iterators, begin/end constexpr noexcept
static_assert(std::contiguous_iterator<O::iterator> && std::contiguous_iterator<O::const_iterator>);
static_assert(noexcept(std::declval<O&>().begin()) && noexcept(std::declval<const O&>().end()));
static_assert([] { O o = 3; int s = 0; for (int x : o) s += x; O e; return s == 3 && e.begin() == e.end(); }());
static_assert(std::ranges::enable_view<O> && !std::ranges::enable_borrowed_range<O>);
static_assert(std::ranges::enable_borrowed_range<std::optional<int&>>);
static_assert(std::format_kind<O> == std::range_format::disabled);
// [optional.ctor], [optional.assign], [optional.mod] constexpr
static_assert([] { O o; o.emplace(4); O p = o; p.reset(); p = 5; p.swap(o); return *o == 5 && *p == 4; }());
static_assert(noexcept(O().reset()) && noexcept(O().has_value()) && noexcept(*O()) && noexcept(O().operator->()) == noexcept(O().operator->()));
static_assert(noexcept(static_cast<bool>(O())));
// [optional.observe]: value_or's U defaults to remove_cv_t<T>
static_assert(O().value_or({}) == 0);
static_assert(std::is_same_v<decltype(O().value_or(1L)), int>);
// monadic: and_then, transform, or_else
static_assert(O(2).and_then([](int x) { return O(x + 1); }) == 3);
static_assert(std::is_same_v<decltype(O(2).transform([](int x) { return x * 1.0; })), std::optional<double>>);
static_assert(O().or_else([] { return O(7); }) == 7);
// [optional.relops] constraints (P2944)
static_assert(!eq<std::optional<NoEq>, std::optional<NoEq>> && !eq<std::optional<BadEq>, std::optional<BadEq>>);
static_assert(!lt<std::optional<NoEq>, std::optional<NoEq>> && !eq<std::optional<NoEq>, NoEq>);
static_assert(eq<O, long> && eq<O, std::nullopt_t> && lt<O, O>);
static_assert(std::is_same_v<decltype(O() <=> O()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(O() <=> 1.0), std::partial_ordering>);
static_assert(std::is_same_v<decltype(O() <=> std::nullopt), std::strong_ordering>);
static_assert(noexcept(O() == std::nullopt));
// [optional.nullopt]: no default constructor, not an aggregate initializable from {}
static_assert(!std::is_default_constructible_v<std::nullopt_t> && std::is_trivially_copyable_v<std::nullopt_t>);
template <class T>
concept from_braces = requires(void (*f)(T)) { f({}); };
static_assert(!from_braces<std::nullopt_t>);
// [optional.optional.ref]
using R = std::optional<int&>;
static_assert(std::is_trivially_copyable_v<R> && sizeof(R) == sizeof(int*));
static_assert(std::is_same_v<R::value_type, int> || std::is_same_v<R::value_type, int&>);
static_assert([] { int i = 1, j = 2; R r = i; r = j; *r = 5; return j == 5 && i == 1 && r.has_value(); }());
static_assert(!std::is_constructible_v<std::optional<const int&>, int&&>);   // no dangling temporary
static_assert(std::is_same_v<decltype(std::declval<R>().transform([](int& x) -> int& { return x; })), R>);
static_assert(std::is_same_v<decltype(*std::declval<const R&>()), int&>);    // shallow const
static_assert(std::is_same_v<decltype(std::declval<R&>().begin()), R::iterator>);
// [optional.specalg], [optional.hash]
static_assert(std::is_same_v<decltype(std::make_optional(1)), O>);
static_assert(std::is_same_v<decltype(std::make_optional<long>(1)), std::optional<long>>);
static_assert(std::make_optional<std::pair<int, int>>(1, 2)->second == 2);
template <class T>
concept swappable_optional = std::is_swappable_v<std::optional<T>>;
struct NoSwapMove { NoSwapMove(NoSwapMove&&) = delete; };
static_assert(!swappable_optional<NoSwapMove>);
// [optional.bad.access]
static_assert(std::is_base_of_v<std::exception, std::bad_optional_access>);
static_assert(noexcept(std::bad_optional_access().what()));
