// [func.wrap.move.ctor]/5: template<class F> move_only_function(F&&) "Constraints:
// remove_cvref_t<F> is not the same type as move_only_function, ... is-callable-from<VT> is
// true." /1: is-callable-from<VT> needs VT to be invocable as both VT cv ref and VT inv-quals
// (nothrow when noex). Applied to sources that are themselves move_only_function /
// copyable_function / function specializations, this decides which conversions exist.
// [func.wrap.copy.ctor]/7 likewise for copyable_function. A move_only_function is never
// constructible from an lvalue of its own type (the generic constructor excludes it and the copy
// constructor does not exist).
#include <functional>
#include <type_traits>

template <class S>
using MOF = std::move_only_function<S>;
template <class S>
using CF = std::copyable_function<S>;
template <class To, class From>
constexpr bool ok = std::is_constructible_v<To, From>;

// move_only_function from move_only_function
static_assert(ok<MOF<int()>, MOF<int() const>>);
static_assert(!ok<MOF<int() const>, MOF<int()>>);
static_assert(ok<MOF<int()>, MOF<int() noexcept>>);
static_assert(!ok<MOF<int() noexcept>, MOF<int()>>);
static_assert(ok<MOF<int() &&>, MOF<int()>>);
static_assert(ok<MOF<int() &>, MOF<int()>>);
static_assert(!ok<MOF<int()>, MOF<int() &&>>);  // needs VT& to be callable
static_assert(!ok<MOF<int() &>, MOF<int() &&>>);
static_assert(ok<MOF<int() &&>, MOF<int() &&>&&>);  // same type: move constructor
static_assert(ok<MOF<void()>, MOF<int()>>);
static_assert(!ok<MOF<int()>, MOF<void()>>);
static_assert(ok<MOF<long(int)>, MOF<int(long)>>);
static_assert(!ok<MOF<int(int*)>, MOF<int(int)>>);
static_assert(!ok<MOF<int()>, MOF<int()>&>);
static_assert(!ok<MOF<int()>, const MOF<int()>&>);
static_assert(!ok<MOF<int() const>, const MOF<int() const>&>);
// move_only_function from copyable_function / function
static_assert(ok<MOF<int()>, CF<int()>&>);
static_assert(ok<MOF<int()>, const CF<int() const>&>);
static_assert(!ok<MOF<int() const>, CF<int()>&>);
static_assert(ok<MOF<int() const>, std::function<int()>&>);
static_assert(!ok<MOF<int() noexcept>, std::function<int()>>);
// copyable_function from copyable_function / function
static_assert(ok<CF<int()>, CF<int() const>>);
static_assert(!ok<CF<int() const>, CF<int()>>);
static_assert(ok<CF<int()>, const CF<int()>&>);  // copy constructor
static_assert(ok<CF<int() const>, std::function<int()>>);
static_assert(!ok<CF<int() noexcept>, CF<int()>>);
static_assert(ok<CF<int() &&>, CF<int() const&>>);
// conversions are implicit
static_assert(std::is_convertible_v<MOF<int() const>, MOF<int()>>);
static_assert(std::is_convertible_v<CF<int() const>, MOF<int()>>);
static_assert(std::is_convertible_v<CF<int() const>, CF<int()>>);
// assignment from a convertible source (operator=(F&&) itself is unconstrained)
static_assert(std::is_assignable_v<MOF<int()>&, MOF<int() const>>);
static_assert(std::is_assignable_v<CF<int()>&, const CF<int() const>&>);
