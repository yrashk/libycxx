// [concepts]: every concept of <concepts> with a model and a non-model, and ranges::swap.
// FREESTANDING
#include <concepts>
#include <type_traits>

struct Base {};
struct Der : Base {};
struct PrivDer : private Base {};
struct NoCmp {};
struct OnlyEq { friend bool operator==(OnlyEq, OnlyEq) = default; };
struct NoMove { NoMove(NoMove&&) = delete; };
struct NoDefault { NoDefault(int); };

static_assert(std::same_as<int, int> && !std::same_as<int, const int>);
static_assert(std::derived_from<Der, Base> && !std::derived_from<PrivDer, Base> && !std::derived_from<int, int>);
static_assert(std::convertible_to<int, long> && !std::convertible_to<Base, Der>);
static_assert(std::common_reference_with<int&, const int&> && std::common_with<int, long>);
static_assert(std::integral<bool> && !std::integral<float> && std::signed_integral<int> && std::unsigned_integral<unsigned char>);
static_assert(!std::signed_integral<unsigned> && std::floating_point<double>);
static_assert(std::assignable_from<int&, long> && !std::assignable_from<int, int>);
static_assert(std::swappable<int> && !std::swappable<NoMove> && std::swappable_with<int&, int&>);
static_assert(std::destructible<int> && std::constructible_from<NoDefault, int> && !std::default_initializable<NoDefault>);
static_assert(!std::default_initializable<const int> && std::default_initializable<int>);
static_assert(std::move_constructible<int> && !std::move_constructible<NoMove> && std::copy_constructible<int>);
static_assert(std::equality_comparable<OnlyEq> && !std::equality_comparable<NoCmp> && std::equality_comparable_with<int, long>);
static_assert(std::totally_ordered<int> && !std::totally_ordered<OnlyEq> && std::totally_ordered_with<int, double>);
static_assert(std::movable<int> && std::copyable<int> && std::semiregular<int> && std::regular<int> && !std::regular<NoDefault>);
constexpr auto lam = [](int) { return true; };
static_assert(std::invocable<decltype(lam), int> && std::regular_invocable<decltype(lam), int>);
static_assert(std::predicate<decltype(lam), int> && !std::predicate<decltype(lam)>);
static_assert(std::relation<bool (*)(int, int), int, int> && std::equivalence_relation<bool (*)(int, int), int, int>);
static_assert(std::strict_weak_order<bool (*)(int, int), int, int>);
// [concept.swappable]: ranges::swap is a constexpr CPO, noexcept when the move is
static_assert([] { int a = 1, b = 2; std::ranges::swap(a, b); return a == 2; }());
static_assert(noexcept(std::ranges::swap(std::declval<int&>(), std::declval<int&>())));
static_assert([] { int a[2] = {1, 2}, b[2] = {3, 4}; std::ranges::swap(a, b); return a[1] == 4; }());
