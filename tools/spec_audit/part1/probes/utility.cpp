// [utility]: <utility> (all freestanding): swap, exchange, forward_like, as_const, integer
// comparison, to_underlying, unreachable, observable_checkpoint, constant_wrapper ([const.wrap.class]).
// FREESTANDING
#include <utility>
#include <type_traits>

// [utility.swap]
struct NoMove { NoMove(NoMove&&) = delete; };
static_assert(!std::is_swappable_v<NoMove>);
static_assert(noexcept(std::swap(std::declval<int&>(), std::declval<int&>())));
static_assert([] { int a[2] = {1, 2}, b[2] = {3, 4}; std::swap(a, b); return a[0] == 3; }());
// [utility.exchange]
static_assert([] { int x = 1; return std::exchange(x, 2) == 1 && x == 2; }());
static_assert(noexcept(std::exchange(std::declval<int&>(), 1)));
// [forward], [utility.as.const]
static_assert(std::is_same_v<decltype(std::forward_like<const int&>(std::declval<long&>())), const long&>);
static_assert(std::is_same_v<decltype(std::forward_like<int>(std::declval<long&>())), long&&>);
static_assert(noexcept(std::forward_like<int>(std::declval<long&>())));
template <class T>
concept as_const_rvalue = requires { std::as_const(std::declval<T>()); };
static_assert(!as_const_rvalue<int>);   // as_const(const T&&) = delete
static_assert(std::is_same_v<decltype(std::move_if_noexcept(std::declval<int&>())), int&&>);
// [utility.intcmp]
static_assert(std::cmp_less(-1, 0u) && !std::cmp_equal(-1, 0xffffffffu) && std::cmp_greater_equal(0u, -1));
static_assert(std::in_range<unsigned char>(255) && !std::in_range<unsigned char>(-1) && noexcept(std::in_range<int>(1)));
template <class T, class U>
concept intcmp = requires(T t, U u) { std::cmp_equal(t, u); };
static_assert(!intcmp<bool, int> && !intcmp<char, int> && !intcmp<char8_t, int> && !intcmp<float, int> && intcmp<signed char, long>);
// [utility.underlying]
enum class E : short { a = 3 };
static_assert(std::to_underlying(E::a) == 3 && std::is_same_v<decltype(std::to_underlying(E::a)), short>);
static_assert(noexcept(std::to_underlying(E::a)));
// [utility.unreachable], [utility.undefined]
static_assert(std::is_same_v<decltype(std::unreachable()), void>);
static_assert(noexcept(std::observable_checkpoint()) && std::is_same_v<decltype(std::observable_checkpoint()), void>);
// [pairs.pair] piecewise_construct, in_place
static_assert(std::is_trivially_copyable_v<std::piecewise_construct_t> && std::is_default_constructible_v<std::in_place_t>);
template <class T>
concept copy_list_init_from_empty = requires(void (*f)(T)) { f({}); };
static_assert(!copy_list_init_from_empty<std::in_place_t> && !copy_list_init_from_empty<std::piecewise_construct_t>);   // explicit default ctors
static_assert(std::is_same_v<decltype(std::in_place_type<int>), const std::in_place_type_t<int>>);
static_assert(std::is_same_v<decltype(std::in_place_index<1>), const std::in_place_index_t<1>>);

// [const.wrap.class]
static_assert(std::is_same_v<decltype(std::cw<42>), const std::constant_wrapper<42>>);
static_assert(std::is_same_v<std::constant_wrapper<42>::value_type, int>);
static_assert(std::is_same_v<std::constant_wrapper<42>::type, std::constant_wrapper<42>>);
static_assert(std::cw<42>.value == 42 && std::constant_wrapper<1>::value == 1);
static_assert(std::is_same_v<decltype(std::cw<42> + std::cw<13>), std::constant_wrapper<55>>);
static_assert(noexcept(std::cw<1> + std::cw<2>));
static_assert(std::is_same_v<decltype(-std::cw<1>), std::constant_wrapper<-1>>);
static_assert(std::is_same_v<decltype(std::cw<1> < std::cw<2>), std::constant_wrapper<true>>);
static_assert(std::is_same_v<decltype(std::cw<5> % std::cw<3>), std::constant_wrapper<2>>);
static_assert(std::is_same_v<decltype(std::cw<1> << std::cw<3>), std::constant_wrapper<8>>);
constexpr int (*twice)(int) = [](int x) { return 2 * x; };
static_assert(std::is_same_v<decltype(std::cw<twice>(std::cw<4>)), std::constant_wrapper<8>>);  // /4
static_assert(std::cw<twice>(5) == 10);                                                         // INVOKE fallback
constexpr int arr[] = {1, 2, 3};
static_assert(std::is_same_v<decltype(std::cw<arr>[std::cw<1>]), std::constant_wrapper<2>>);
template <class L, class R>
concept comma = requires(L l, R r) { l, r; };
static_assert(!comma<std::constant_wrapper<1>, std::constant_wrapper<2>>);   // operator, = delete
constexpr int conv = std::cw<7>;                                            // operator decltype(value)
static_assert(conv == 7);
static_assert(std::is_empty_v<std::constant_wrapper<1>> && std::is_trivially_copyable_v<std::constant_wrapper<1>>);
// operator&& / || only when a value is not convertible to bool
static_assert(std::is_same_v<decltype(std::cw<true> && std::cw<false>), bool>);   // built-in &&, through bool
