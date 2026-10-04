// [func.wrap.ref.ctor]/7: function_ref(F&& f) "Constraints: remove_cvref_t<F> is not the same
// type as function_ref, is_member_pointer_v<T> is false, and is-invocable-using<cv T&> is
// true." Applied to the owning wrappers: a function_ref<R(Args...) cv noexcept(noex)> can refer
// to a move_only_function / copyable_function / function exactly when that wrapper is
// invocable as cv T& with the required noexcept-ness. The constructor is noexcept.
#include <functional>
#include <type_traits>

template <class Sig, class... A>
constexpr bool ok = std::is_constructible_v<std::function_ref<Sig>, A...>;
template <class S>
using MOF = std::move_only_function<S>;
template <class S>
using CF = std::copyable_function<S>;

// move_only_function
static_assert(ok<int(), MOF<int()>&>);
static_assert(ok<int(), MOF<int()>>);                // T& even for an rvalue argument
static_assert(!ok<int(), const MOF<int()>&>);        // const T& is not invocable
static_assert(!ok<int() const, MOF<int()>&>);
static_assert(ok<int() const, MOF<int() const>&>);
static_assert(ok<int() const, const MOF<int() const>&>);
static_assert(ok<int(), MOF<int() &>&>);
static_assert(!ok<int(), MOF<int() &&>&>);           // never called as an rvalue
static_assert(!ok<int(), MOF<int() &&>>);
static_assert(ok<int() noexcept, MOF<int() noexcept>&>);
static_assert(!ok<int() noexcept, MOF<int()>&>);
static_assert(ok<long(), MOF<int()>&>);
static_assert(ok<void(), MOF<int()>&>);
static_assert(!ok<int(), MOF<void()>&>);
static_assert(!ok<int(int), MOF<int()>&>);
// copyable_function
static_assert(ok<int(int), CF<int(int)>&>);
static_assert(!ok<int(int) const, CF<int(int)>&>);
static_assert(ok<int(int) const, const CF<int(int) const>&>);
static_assert(!ok<int(int) noexcept, CF<int(int)>&>);
// function (operator() is const and not noexcept)
static_assert(ok<int(int), std::function<int(int)>&>);
static_assert(ok<int(int) const, const std::function<int(int)>&>);
static_assert(!ok<int(int) noexcept, std::function<int(int)>&>);
// noexcept construction
static_assert(std::is_nothrow_constructible_v<std::function_ref<int()>, MOF<int()>&>);
static_assert(std::is_nothrow_constructible_v<std::function_ref<int(int) const>, const std::function<int(int)>&>);
// implicit
static_assert(std::is_convertible_v<MOF<int()>&, std::function_ref<int()>>);
