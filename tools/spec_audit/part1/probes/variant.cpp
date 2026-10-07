// [variant]: variant, variant_size/alternative, get/get_if/holds_alternative, visit (free and
// member, P2637), monostate, constrained comparisons (P2944), bad_variant_access, hash.
// Freestanding except get (freestanding-deleted) and the throwing visit.
// FREESTANDING
#include <variant>
#include <compare>
#include <type_traits>
#include <utility>

struct NoEq {};
template <class A, class B>
concept eq = requires(const A& a, const B& b) { a == b; };
template <class A, class B>
concept lt = requires(const A& a, const B& b) { a < b; };

using V = std::variant<int, double>;
static_assert(std::variant_size_v<V> == 2 && std::variant_size_v<const V> == 2);
static_assert(std::is_same_v<std::variant_alternative_t<1, V>, double> &&
              std::is_same_v<std::variant_alternative_t<1, const V>, const double>);
static_assert(std::variant_npos == static_cast<std::size_t>(-1));
static_assert(std::is_trivially_copyable_v<V>);
static_assert([] { V v = 1; v = 2.0; v.emplace<0>(3); return v.index() == 0 && *std::get_if<0>(&v) == 3; }());
static_assert(std::holds_alternative<int>(V(1)) && noexcept(std::holds_alternative<int>(std::declval<const V&>())));
static_assert(noexcept(std::get_if<int>(std::declval<V*>())) && std::get_if<double>(static_cast<V*>(nullptr)) == nullptr);
static_assert(std::get<1>(V(2.5)) == 2.5);
// converting constructor (P0608): no narrowing, bool not from pointers
static_assert(std::variant<float, long>(0).index() == 1);
static_assert(!std::is_constructible_v<std::variant<float>, int>);
static_assert(std::variant<bool, std::nullptr_t>(nullptr).index() == 1);
// [variant.visit]
static_assert(std::visit([](auto x) { return int(x) + 1; }, V(1)) == 2);
static_assert(std::is_same_v<decltype(std::visit<long>([](auto x) { return x; }, V(1))), long>);
static_assert(V(4).visit([](auto x) { return int(x); }) == 4);                 // member visit
static_assert(std::is_same_v<decltype(V(4).visit<long>([](auto x) { return x; })), long>);
struct Derived : V { using V::V; };
static_assert(std::visit([](auto x) { return int(x); }, Derived(5)) == 5);     // as-variant
// [variant.relops] constraints (P2944)
static_assert(!eq<std::variant<NoEq>, std::variant<NoEq>> && !lt<std::variant<NoEq>, std::variant<NoEq>>);
static_assert(eq<V, V> && lt<V, V>);
static_assert(std::is_same_v<decltype(V() <=> V()), std::partial_ordering>);
// [variant.monostate]
static_assert(std::is_trivially_copyable_v<std::monostate> && std::monostate() == std::monostate());
static_assert(noexcept(std::monostate() <=> std::monostate()) &&
              std::is_same_v<decltype(std::monostate() <=> std::monostate()), std::strong_ordering>);
// [variant.swap], [variant.specalg]
static_assert([] { V a = 1, b = 2.0; a.swap(b); std::swap(a, b); return a.index() == 0; }());
template <class T>
concept swappable = std::is_swappable_v<T>;
struct NoMove { NoMove(NoMove&&) = delete; NoMove& operator=(NoMove&&) = delete; };
static_assert(!swappable<std::variant<NoMove>>);
// [variant.bad.access]
static_assert(std::is_base_of_v<std::exception, std::bad_variant_access> && noexcept(std::bad_variant_access().what()));
