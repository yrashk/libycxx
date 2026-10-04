// [func.wrap.move.general]: partial specializations for every cv (const or empty), ref (&,
// && or empty) and noex; inv-quals is cv& when ref is empty, otherwise cv ref.
// [func.wrap.move.class]: "R operator()(ArgTypes...) cv ref noexcept(noex);"
// [func.wrap.move.ctor]/1: is-callable-from<VT> is is_invocable_r_v<R, VT cv ref, ArgTypes...>
// && is_invocable_r_v<R, VT inv-quals, ArgTypes...> (the nothrow forms when noex is true);
// /5: template<class F> move_only_function(F&&) is constrained on is-callable-from<VT>.
#include <functional>
#include <type_traits>

template <class S>
using MOF = std::move_only_function<S>;

// --- callability of the wrapper itself ---------------------------------------------------
template <class W, bool L, bool CL, bool R, bool CR, bool NX>
constexpr bool calls() {
  static_assert(std::is_invocable_v<W&> == L);
  static_assert(std::is_invocable_v<const W&> == CL);
  static_assert(std::is_invocable_v<W&&> == R);
  static_assert(std::is_invocable_v<const W&&> == CR);
  if constexpr (L) static_assert(std::is_nothrow_invocable_v<W&> == NX);
  if constexpr (R) static_assert(std::is_nothrow_invocable_v<W&&> == NX);
  if constexpr (CR) static_assert(std::is_nothrow_invocable_v<const W&&> == NX);
  return true;
}
//                         lvalue  const&  rvalue  const&& noexcept
static_assert(calls<MOF<int()>, true, false, true, false, false>());
static_assert(calls<MOF<int() const>, true, true, true, true, false>());
static_assert(calls<MOF<int() &>, true, false, false, false, false>());
static_assert(calls<MOF<int() const&>, true, true, true, true, false>());
static_assert(calls<MOF<int() &&>, false, false, true, false, false>());
static_assert(calls<MOF<int() const&&>, false, false, true, true, false>());
static_assert(calls<MOF<int() noexcept>, true, false, true, false, true>());
static_assert(calls<MOF<int() const noexcept>, true, true, true, true, true>());
static_assert(calls<MOF<int() & noexcept>, true, false, false, false, true>());
static_assert(calls<MOF<int() const & noexcept>, true, true, true, true, true>());
static_assert(calls<MOF<int() && noexcept>, false, false, true, false, true>());
static_assert(calls<MOF<int() const && noexcept>, false, false, true, true, true>());

// --- is-callable-from: which targets are accepted ----------------------------------------
struct All {  // callable in every value category and constness
  int operator()() &;
  int operator()() const&;
  int operator()() &&;
  int operator()() const&&;
};
struct AllNoexcept {
  int operator()() & noexcept;
  int operator()() const& noexcept;
  int operator()() && noexcept;
  int operator()() const&& noexcept;
};
struct LvalueOnly {  // & only: not callable as an rvalue
  int operator()() &;
};
struct ConstLvalueOnly {  // const& binds rvalues too
  int operator()() const&;
};
struct RvalueOnly {
  int operator()() &&;
};
struct ConstRvalueOnly {
  int operator()() const&&;
};
struct NonConst {  // unqualified non-const: lvalue and rvalue, never const
  int operator()();
};
struct ConstCall {
  int operator()() const;
};

template <class S, class T>
constexpr bool ok = std::is_constructible_v<MOF<S>, T>;

// ref empty: VT (rvalue) and VT& (inv-quals) must both work
static_assert(ok<int(), All>);
static_assert(ok<int(), NonConst>);
static_assert(ok<int(), ConstCall>);
static_assert(ok<int(), ConstLvalueOnly>);
static_assert(!ok<int(), LvalueOnly>);
static_assert(!ok<int(), RvalueOnly>);
static_assert(!ok<int(), ConstRvalueOnly>);
// const: const VT and const VT&
static_assert(ok<int() const, All>);
static_assert(ok<int() const, ConstCall>);
static_assert(ok<int() const, ConstLvalueOnly>);
static_assert(!ok<int() const, NonConst>);
static_assert(!ok<int() const, LvalueOnly>);
static_assert(!ok<int() const, ConstRvalueOnly>);
// &: VT& only
static_assert(ok<int() &, All>);
static_assert(ok<int() &, LvalueOnly>);
static_assert(ok<int() &, NonConst>);
static_assert(ok<int() &, ConstLvalueOnly>);
static_assert(!ok<int() &, RvalueOnly>);
static_assert(!ok<int() &, ConstRvalueOnly>);
// const&: const VT&
static_assert(ok<int() const&, ConstCall>);
static_assert(ok<int() const&, ConstLvalueOnly>);
static_assert(!ok<int() const&, LvalueOnly>);
static_assert(!ok<int() const&, NonConst>);
static_assert(!ok<int() const&, ConstRvalueOnly>);
// &&: VT&&
static_assert(ok<int() &&, All>);
static_assert(ok<int() &&, RvalueOnly>);
static_assert(ok<int() &&, NonConst>);
static_assert(ok<int() &&, ConstRvalueOnly>);
static_assert(ok<int() &&, ConstLvalueOnly>);
static_assert(!ok<int() &&, LvalueOnly>);
// const&&: const VT&&
static_assert(ok<int() const&&, ConstRvalueOnly>);
static_assert(ok<int() const&&, ConstCall>);
static_assert(ok<int() const&&, ConstLvalueOnly>);
static_assert(!ok<int() const&&, RvalueOnly>);
static_assert(!ok<int() const&&, NonConst>);
static_assert(!ok<int() const&&, LvalueOnly>);
// noexcept: the nothrow forms
static_assert(ok<int() noexcept, AllNoexcept>);
static_assert(ok<int() const && noexcept, AllNoexcept>);
static_assert(!ok<int() noexcept, All>);
static_assert(!ok<int() const noexcept, ConstCall>);
static_assert(!ok<int() & noexcept, LvalueOnly>);
static_assert(!ok<int() && noexcept, RvalueOnly>);
static_assert(ok<int() noexcept, decltype([]() noexcept { return 0; })>);
static_assert(!ok<int() noexcept, decltype([] { return 0; })>);
static_assert(ok<int(), decltype([]() noexcept { return 0; })>);
static_assert(ok<void() noexcept, int (*)() noexcept>);
static_assert(!ok<void() noexcept, int (*)()>);

// the same constraint applies to the in_place_type constructor and to operator=
static_assert(std::is_constructible_v<MOF<int()>, std::in_place_type_t<All>>);
static_assert(!std::is_constructible_v<MOF<int()>, std::in_place_type_t<LvalueOnly>>);
static_assert(!std::is_constructible_v<MOF<int() const>, std::in_place_type_t<NonConst>>);
static_assert(std::is_assignable_v<MOF<int() &&>&, RvalueOnly>);
static_assert(!std::is_assignable_v<MOF<int() &>&, RvalueOnly>);
static_assert(!std::is_assignable_v<MOF<int() noexcept>&, All>);

// different specializations are distinct types
static_assert(!std::is_same_v<MOF<int()>, MOF<int() const>>);
static_assert(!std::is_same_v<MOF<int() &>, MOF<int()>>);
static_assert(!std::is_same_v<MOF<int() noexcept>, MOF<int()>>);
