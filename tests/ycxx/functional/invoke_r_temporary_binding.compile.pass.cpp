// [func.require]/2: INVOKE<R>(f, args...) is INVOKE(f, args...) implicitly converted to R, and
// "If reference_converts_from_temporary_v<R, decltype(INVOKE(f, t1, t2, ..., tN))> is true,
// INVOKE<R>(f, t1, t2, ..., tN) is ill-formed." [meta.rel]: is_invocable_r_v is true only if
// INVOKE<R> is well-formed when treated as an unevaluated operand. Every wrapper whose
// constructor is constrained on it therefore rejects a callable that would make the call
// return a dangling reference:
// [func.invoke]/3 invoke_r: Constraints: is_invocable_r_v<R, F, Args...>;
// [func.wrap.func.con]/8: F is Lvalue-Callable ([func.wrap.func]/2: "INVOKE<R>(...) is
//   well-formed");
// [func.wrap.move.ctor]/1, [func.wrap.copy.ctor]/1: is-callable-from<VT> uses is_invocable_r_v;
// [func.wrap.ref.ctor]/2: is-invocable-using uses is_invocable_r_v (is_nothrow_invocable_r_v).
// [variant.visit]/5: visit<R> is "INVOKE<R>(...)", so it is constrained the same way (the
// expression is only valid if INVOKE<R> is).
#include <functional>
#include <type_traits>
#include <variant>

using IntF = int (*)();
using RefF = int& (*)();
using LongRefF = long& (*)();

static_assert(!std::is_invocable_r_v<const int&, IntF>);
static_assert(!std::is_invocable_r_v<const long&, RefF>);  // int& -> const long& binds a temporary
static_assert(std::is_invocable_r_v<const int&, RefF>);
static_assert(std::is_invocable_r_v<int, RefF>);
static_assert(std::is_invocable_r_v<void, IntF>);
static_assert(!std::is_nothrow_invocable_r_v<const int&, int (*)() noexcept>);
static_assert(std::is_nothrow_invocable_r_v<const int&, int& (*)() noexcept>);

template <class R, class F>
concept can_invoke_r = requires(F f) { std::invoke_r<R>(f); };
static_assert(!can_invoke_r<const int&, IntF>);
static_assert(!can_invoke_r<const int&, LongRefF>);
static_assert(can_invoke_r<const int&, RefF>);
static_assert(can_invoke_r<long, IntF>);

static_assert(!std::is_constructible_v<std::function<const int&()>, IntF>);
static_assert(!std::is_constructible_v<std::function<const int&()>, LongRefF>);
static_assert(std::is_constructible_v<std::function<const int&()>, RefF>);
static_assert(!std::is_constructible_v<std::move_only_function<const int&()>, IntF>);
static_assert(!std::is_constructible_v<std::move_only_function<const long&()>, RefF>);
static_assert(std::is_constructible_v<std::move_only_function<const int&()>, RefF>);
static_assert(!std::is_constructible_v<std::copyable_function<const int&()>, IntF>);
static_assert(std::is_constructible_v<std::copyable_function<const int&() const>, RefF>);
static_assert(!std::is_constructible_v<std::function_ref<const int&()>, IntF>);
static_assert(!std::is_constructible_v<std::function_ref<const long&()>, RefF&>);
static_assert(std::is_constructible_v<std::function_ref<const int&()>, RefF&>);

struct RetInt {
  int operator()(auto) const { return 1; }
};
struct RetRef {
  static inline int x = 0;
  int& operator()(auto) const { return x; }
};
template <class R, class F, class V>
concept can_visit_r = requires(F f, V v) { std::visit<R>(f, v); };
static_assert(can_visit_r<long, RetInt, std::variant<int>>);
static_assert(can_visit_r<const int&, RetRef, std::variant<int>>);
static_assert(can_visit_r<void, RetInt, std::variant<int, long>>);
