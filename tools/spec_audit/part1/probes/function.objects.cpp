// [function.objects]: invoke/invoke_r, reference_wrapper (comparisons P2944, common_reference
// P2655), operator function objects, identity, not_fn/bind_front/bind_back with a constant
// callable (P2714), bind, mem_fn (constexpr), function, move_only_function, copyable_function,
// function_ref (constant_wrapper form), searchers, hash.
#include <functional>
#include <string_view>
#include <type_traits>
#include <utility>

constexpr int add(int a, int b) { return a + b; }
constexpr int sub(int a, int b) { return a - b; }
struct S { int m = 4; constexpr int f(int x) const { return m + x; } };

// [func.invoke]
static_assert(std::invoke(add, 1, 2) == 3 && std::invoke(&S::m, S()) == 4);
static_assert(std::is_same_v<decltype(std::invoke_r<long>(add, 1, 2)), long>);
static_assert(std::is_void_v<decltype(std::invoke_r<void>(add, 1, 2))>);
static_assert(noexcept(std::invoke([]() noexcept {})) && !noexcept(std::invoke([] {})));
template <class F, class... A>
concept invocable_r_long = requires(F f, A... a) { std::invoke_r<long>(f, a...); };
static_assert(!invocable_r_long<void (*)()>);
// [refwrap]
static_assert([] { int i = 1; auto r = std::ref(i); r.get() = 2; return i == 2 && r == 2 && r == std::ref(i); }());
static_assert(std::is_trivially_copyable_v<std::reference_wrapper<int>>);
template <class T>
concept ref_rvalue = requires { std::ref(std::declval<T>()); };
static_assert(!ref_rvalue<int>);
static_assert(std::is_same_v<std::common_reference_t<std::reference_wrapper<int>, int&>, int&>);
static_assert(std::is_same_v<std::common_reference_t<int&, std::reference_wrapper<int>&>, int&>);
static_assert(std::is_same_v<decltype(std::ref(std::declval<int&>()) <=> 1), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::reference_wrapper(std::declval<int&>())), std::reference_wrapper<int>>);
// operator function objects, transparent specializations
static_assert(std::plus<>()(1, 2L) == 3 && std::is_same_v<decltype(std::plus<>()(1, 2L)), long>);
static_assert(requires { typename std::less<>::is_transparent; typename std::ranges::less::is_transparent;
                         typename std::compare_three_way::is_transparent; typename std::identity::is_transparent; });
static_assert(std::bit_not<>()(0u) == ~0u && std::logical_not<>()(false));
static_assert(noexcept(std::identity()(1)) && std::identity()(5) == 5);
// [func.not.fn], [func.bind.partial] with a constant callable (P2714)
static_assert(std::not_fn(std::equal_to<>())(1, 2));
static_assert(std::not_fn<std::equal_to<>{}>()(1, 2) && noexcept(std::not_fn<std::equal_to<>{}>()));
static_assert(std::is_empty_v<decltype(std::not_fn<add>())>);
static_assert(std::bind_front(sub, 5)(1) == 4 && std::bind_back(sub, 5)(1) == -4);
static_assert(std::bind_front<sub>(5)(1) == 4 && std::bind_back<sub>(5)(1) == -4);
static_assert(std::is_empty_v<decltype(std::bind_front<sub>())>);
// [func.bind]: constexpr bind and placeholders
static_assert(std::bind(sub, std::placeholders::_2, std::placeholders::_1)(1, 5) == 4);
static_assert(std::is_placeholder_v<std::remove_cvref_t<decltype(std::placeholders::_1)>> == 1);
static_assert(std::is_bind_expression_v<decltype(std::bind(sub, 1, 2))>);
static_assert(std::bind<long>(sub, 3, 1)() == 2);
// [func.memfn]
static_assert(std::mem_fn(&S::f)(S(), 1) == 5 && noexcept(std::mem_fn(&S::f)));
// [func.wrap.func]
using F = std::function<int(int)>;
static_assert(std::is_same_v<F::result_type, int>);
static_assert(std::is_same_v<decltype(std::function(add)), std::function<int(int, int)>>);
static_assert(noexcept(std::declval<F&>() == nullptr) && noexcept(std::declval<F&>().swap(std::declval<F&>())));
static_assert(std::is_base_of_v<std::exception, std::bad_function_call>);
// [func.wrap.move], [func.wrap.copy]
using M = std::move_only_function<int(int) const noexcept>;
static_assert(!std::is_copy_constructible_v<M> && std::is_nothrow_move_constructible_v<M>);
static_assert(noexcept(std::declval<const M&>()(1)) && std::is_same_v<decltype(std::declval<const M&>()(1)), int>);
static_assert(!std::is_invocable_v<std::move_only_function<int() &&>&>);
static_assert(std::is_invocable_v<std::move_only_function<int() &&>&&>);
using C = std::copyable_function<int(int) const>;
static_assert(std::is_copy_constructible_v<C> && std::is_nothrow_move_constructible_v<C>);
static_assert(std::is_constructible_v<std::move_only_function<int(int) const>, C> && !std::is_constructible_v<M, C>);
// [func.wrap.ref]
using FR = std::function_ref<int(int, int) const noexcept>;
static_assert(std::is_trivially_copyable_v<std::function_ref<int(int)>>);
static_assert(!std::is_default_constructible_v<FR>);
constexpr auto mul = [](int a, int b) noexcept { return a * b; };
static_assert(std::is_nothrow_constructible_v<FR, std::constant_wrapper<mul>>);
constexpr int one = 1;
constexpr S s;
static_assert(std::is_constructible_v<std::function_ref<int(int)>, std::constant_wrapper<add>, const int&>);  // bound object
static_assert(!std::is_constructible_v<std::function_ref<int(int)>, std::constant_wrapper<add>, int>);       // not from an rvalue
static_assert(std::is_constructible_v<std::function_ref<int(int)>, std::constant_wrapper<&S::f>, const S*>);  // bound pointer
constexpr std::function_ref<int(int)> fr_constant(std::cw<add>, one);   // the constructors are constexpr (operator() is not)
static_assert(!noexcept(std::declval<std::function_ref<int(int)>&>()(1)) && noexcept(std::declval<FR&>()(1, 2)));
static_assert(std::is_same_v<decltype(std::function_ref(std::cw<add>)), std::function_ref<int(int, int)>>);
static_assert(std::is_same_v<decltype(std::function_ref(std::cw<&S::f>, s)), std::function_ref<int(int)>>);   // [func.wrap.ref.deduct]/6: no cv
static_assert(!std::is_assignable_v<std::function_ref<int(int, int) const noexcept>&, decltype(mul)>);   // operator=(T) = delete
static_assert(std::is_assignable_v<std::function_ref<int(int, int)>&, int (*)(int, int)>);   // (not for pointers: /21.2)
// [func.search]
static_assert([] {
  std::string_view h = "abcabd", n = "abd";
  return std::search(h.begin(), h.end(), std::default_searcher(n.begin(), n.end())) == h.begin() + 3;
}());
// [unord.hash]
static_assert(std::is_default_constructible_v<std::hash<int>> && noexcept(std::hash<int>()(1)));
struct NoHash {};
static_assert(!std::is_default_constructible_v<std::hash<NoHash>> && !std::is_invocable_v<std::hash<NoHash>, NoHash>);
static_assert(std::is_default_constructible_v<std::hash<std::nullptr_t>>);
