// [func.wrap.ref.ctor]/3: function_ref(F*) "Constraints: is_function_v<F> is true, and
// is-invocable-using<F> is true." /7: function_ref(F&&) "Constraints: remove_cvref_t<F> is not
// the same type as function_ref, is_member_pointer_v<T> is false, and is-invocable-using<cv
// T&> is true." /1: is-invocable-using<T...> is is_nothrow_invocable_r_v<R, T..., ArgTypes...>
// if noex, otherwise is_invocable_r_v. /14: function_ref(constant_wrapper<c, F>, U&&)
// requires is_rvalue_reference_v<U&&> to be false. /21: template<class T> operator=(T) =
// delete, constrained on T not being convertible-from-specialization, not a pointer and not a
// constant_wrapper specialization.
#include <functional>
#include <cstddef>
#include <type_traits>
#include <utility>

struct S {
  int v;
  int get() const { return v; }
  int set(int x) { return v = x; }
};
struct NonConst {
  int operator()() { return 0; }
};
struct ConstOnly {
  int operator()() const { return 0; }
};
struct MayThrow {
  int operator()() const { return 0; }
};
struct NoThrow {
  int operator()() const noexcept { return 0; }
};
struct LvalueOnly {
  int operator()() & { return 0; }
};
int f0();
int f0n() noexcept;

template <class Sig, class... A>
constexpr bool ok = std::is_constructible_v<std::function_ref<Sig>, A...>;

// function pointers and functions
static_assert(ok<int(), int (*)()>);
static_assert(ok<int(), decltype(f0)&>);
static_assert(ok<void(), int (*)()>);
static_assert(!ok<int(int), int (*)()>);
static_assert(ok<int() noexcept, int (*)() noexcept>);
static_assert(!ok<int() noexcept, int (*)()>);
static_assert(ok<int(), int (*)() noexcept>);
static_assert(!ok<int(), std::nullptr_t>);
static_assert(!ok<int(), int*>);
// member pointers are not accepted directly
static_assert(!ok<int(const S&), int (S::*)() const>);
static_assert(!ok<int(S&), int S::*>);
// cv T&
static_assert(ok<int(), NonConst&>);
static_assert(ok<int(), NonConst>);  // binds to the temporary
static_assert(!ok<int() const, NonConst&>);
static_assert(!ok<int(), const NonConst&>);
static_assert(ok<int() const, ConstOnly&>);
static_assert(ok<int() const, const ConstOnly&>);
static_assert(ok<int(), LvalueOnly>);  // invoked as an lvalue (cv T&) even if passed an rvalue
// noexcept
static_assert(ok<int() noexcept, NoThrow&>);
static_assert(!ok<int() noexcept, MayThrow&>);
static_assert(ok<int() const noexcept, NoThrow&>);
// constructors are implicit (except none are explicit)
static_assert(std::is_convertible_v<NonConst&, std::function_ref<int()>>);
static_assert(std::is_convertible_v<int (*)(), std::function_ref<int()>>);

// constant_wrapper forms
static_assert(ok<int(), decltype(std::cw<f0>)>);
static_assert(!ok<int(int), decltype(std::cw<f0>)>);
static_assert(ok<int() noexcept, decltype(std::cw<f0n>)>);
static_assert(!ok<int() noexcept, decltype(std::cw<f0>)>);
static_assert(ok<int(), decltype(std::cw<&S::get>), S&>);
static_assert(ok<int(), decltype(std::cw<&S::get>), const S&>);
static_assert(ok<int() const, decltype(std::cw<&S::get>), S&>);
static_assert(!ok<int(int) const, decltype(std::cw<&S::set>), S&>);  // cv T& = const S&
static_assert(ok<int(int), decltype(std::cw<&S::set>), S&>);
static_assert(!ok<int(int), decltype(std::cw<&S::set>), const S&>);
static_assert(!ok<int(), decltype(std::cw<&S::get>), S>);  // rvalues are rejected
static_assert(!ok<int(), decltype(std::cw<&S::get>), S&&>);
static_assert(ok<int(), decltype(std::cw<&S::get>), S*>);
static_assert(ok<int(), decltype(std::cw<&S::get>), const S*>);
static_assert(ok<int() const, decltype(std::cw<&S::get>), S*>);
static_assert(!ok<int(int) const, decltype(std::cw<&S::set>), S*>);  // cv T* = const S*
static_assert(ok<int&(), decltype(std::cw<&S::v>), S&>);

// conversions between specializations
static_assert(ok<int(), std::function_ref<int() noexcept>>);
static_assert(!ok<int() noexcept, std::function_ref<int()>>);
static_assert(ok<int(), std::function_ref<int() const>>);
static_assert(ok<void(), std::function_ref<int()>>);

// assignment
using FR = std::function_ref<int()>;
static_assert(std::is_assignable_v<FR&, const FR&>);
static_assert(std::is_assignable_v<FR&, int (*)()>);  // via function_ref(F*)
static_assert(std::is_assignable_v<FR&, std::function_ref<int() noexcept>>);
static_assert(std::is_assignable_v<FR&, decltype(std::cw<f0>)>);
static_assert(!std::is_assignable_v<FR&, NonConst&>);  // deleted operator=(T)
static_assert(!std::is_assignable_v<FR&, NonConst>);
static_assert(!std::is_assignable_v<FR&, decltype([] { return 0; })>);
