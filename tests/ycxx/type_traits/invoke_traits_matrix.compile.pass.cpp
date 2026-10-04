// [meta.trans.other] invoke_result: decltype(INVOKE(declval<Fn>(), declval<ArgTypes>()...)) if
// well-formed, otherwise no member; access checked in an unrelated context.
// [meta.rel] Table 56: is_invocable, is_invocable_r (INVOKE<R>), is_nothrow_invocable(_r).
// [func.require]/1: INVOKE(f, t1, ...) for pointers to member functions (1.1-1.3: object,
// reference_wrapper, otherwise (*t1).*f), pointers to data members (1.4-1.6), otherwise f(t1, ...).
// [func.require]/2: INVOKE<R> is static_cast<void>(INVOKE(...)) if R is cv void, otherwise
// INVOKE(...) "implicitly converted to R". "If reference_converts_from_temporary_v<R,
// decltype(INVOKE(f, t1, t2, ..., tN))> is true, INVOKE<R>(f, t1, t2, ..., tN) is ill-formed."
// The implicit conversion of a prvalue of type R to R is copy-initialization from a prvalue,
// which needs no copy or move constructor ([dcl.init.general]/17.6.1).
#include <type_traits>
#include <functional>
#include <memory>

template <class A, class B> constexpr bool same = std::is_same_v<A, B>;
template <class F, class... A> concept HasIR = requires { typename std::invoke_result<F, A...>::type; };

struct S {
  int data;
  const int cdata = 0;
  int f(long);
  int fc(long) const;
  int& fl() &;
  int&& fr() &&;
  int fcl() const &;
  void fv() volatile;
  int fn() noexcept;
  int vararg(int, ...);
};
struct DS : S {};
struct Ambig1 : S {};
struct Ambig2 : S {};
struct DAmbig : Ambig1, Ambig2 {};
struct Unrelated {};
struct Fancy { S& operator*() const; };            // dereferenceable: 1.3/1.6 apply
struct FancyNoexcept { S& operator*() const noexcept; };

using PMF = int (S::*)(long);
using PMFc = int (S::*)(long) const;
using PMFl = int& (S::*)() &;
using PMFr = int&& (S::*)() &&;
using PMFcl = int (S::*)() const &;
using PMFv = void (S::*)() volatile;
using PMFn = int (S::*)() noexcept;
using PMD = int S::*;
using PMDc = const int S::*;

// --- pointers to member functions
static_assert(same<std::invoke_result_t<PMF, S&, int>, int>);
static_assert(same<std::invoke_result_t<PMF, S, int>, int>);
static_assert(same<std::invoke_result_t<PMF, S*, int>, int>);
static_assert(same<std::invoke_result_t<PMF, DS&, int>, int>);
static_assert(same<std::invoke_result_t<PMF, DS*, int>, int>);
static_assert(same<std::invoke_result_t<PMF, std::reference_wrapper<S>, int>, int>);
static_assert(same<std::invoke_result_t<PMF, std::reference_wrapper<DS>, int>, int>);
static_assert(same<std::invoke_result_t<PMF, std::unique_ptr<S>&, int>, int>);
static_assert(same<std::invoke_result_t<PMF, std::shared_ptr<DS>, int>, int>);
static_assert(same<std::invoke_result_t<PMF, Fancy, int>, int>);
static_assert(same<std::invoke_result_t<const PMF&, S&, int>, int>);
static_assert(!HasIR<PMF, const S&, int>);         // non-const member, const object
static_assert(!HasIR<PMF, const S*, int>);
static_assert(!HasIR<PMF, std::reference_wrapper<const S>, int>);
static_assert(!HasIR<PMF, S&>);                    // missing argument
static_assert(!HasIR<PMF, S&, int, int>);
static_assert(!HasIR<PMF, S&, int*>);
static_assert(!HasIR<PMF>);
static_assert(!HasIR<PMF, Unrelated&, int>);
static_assert(!HasIR<PMF, DAmbig&, int>);          // ambiguous base
static_assert(!HasIR<PMF, int, int>);
static_assert(same<std::invoke_result_t<PMFc, const S&, int>, int>);
static_assert(same<std::invoke_result_t<PMFc, const DS*, int>, int>);
static_assert(same<std::invoke_result_t<PMFc, std::reference_wrapper<const S>, int>, int>);
static_assert(same<std::invoke_result_t<PMFl, S&>, int&>);
static_assert(!HasIR<PMFl, S>);
static_assert(!HasIR<PMFl, S&&>);
static_assert(same<std::invoke_result_t<PMFl, S*>, int&>);              // *p is an lvalue
static_assert(same<std::invoke_result_t<PMFl, std::reference_wrapper<S>>, int&>);   // t.get() is an lvalue
static_assert(same<std::invoke_result_t<PMFr, S>, int&&>);
static_assert(same<std::invoke_result_t<PMFr, S&&>, int&&>);
static_assert(!HasIR<PMFr, S&>);
static_assert(!HasIR<PMFr, S*>);
static_assert(!HasIR<PMFr, std::reference_wrapper<S>>);
static_assert(same<std::invoke_result_t<PMFcl, S>, int>);               // const & binds to rvalues
static_assert(same<std::invoke_result_t<PMFcl, const S&&>, int>);
static_assert(same<std::invoke_result_t<PMFv, volatile S&>, void>);
static_assert(!HasIR<PMFv, const S&>);
static_assert(same<std::invoke_result_t<int (S::*)(int, ...), S&, int, double, const char*>, int>);
// --- pointers to data members ([func.require]/1.4-1.6; [meta.trans.other] Example 2)
static_assert(same<std::invoke_result_t<PMD, S&>, int&>);
static_assert(same<std::invoke_result_t<PMD, const S&>, const int&>);
static_assert(same<std::invoke_result_t<PMD, volatile S&>, volatile int&>);
static_assert(same<std::invoke_result_t<PMD, S>, int&&>);
static_assert(same<std::invoke_result_t<PMD, S&&>, int&&>);
static_assert(same<std::invoke_result_t<PMD, const S&&>, const int&&>);
static_assert(same<std::invoke_result_t<PMD, const S>, const int&&>);
static_assert(same<std::invoke_result_t<PMD, S*>, int&>);
static_assert(same<std::invoke_result_t<PMD, const S*>, const int&>);
static_assert(same<std::invoke_result_t<PMD, S* const&>, int&>);
static_assert(same<std::invoke_result_t<PMD, DS>, int&&>);
static_assert(same<std::invoke_result_t<PMD, const DS*>, const int&>);
static_assert(same<std::invoke_result_t<PMD, std::reference_wrapper<S>>, int&>);
static_assert(same<std::invoke_result_t<PMD, std::reference_wrapper<const S>>, const int&>);
static_assert(same<std::invoke_result_t<PMD, const std::reference_wrapper<S>&>, int&>);
static_assert(same<std::invoke_result_t<PMD, std::unique_ptr<S>>, int&>);
static_assert(same<std::invoke_result_t<PMD, std::unique_ptr<const S>&>, const int&>);
static_assert(same<std::invoke_result_t<PMD, Fancy>, int&>);
static_assert(same<std::invoke_result_t<PMDc, S>, const int&&>);
static_assert(!HasIR<PMD>);
static_assert(!HasIR<PMD, S&, int>);               // a data member takes exactly one argument
static_assert(!HasIR<PMD, Unrelated&>);
static_assert(!HasIR<PMD, DAmbig&>);
static_assert(!HasIR<PMD, int*>);
// --- ordinary callables
int fn(int);
int& fnr();
int&& fnrr();
const S fnc_class();
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
const int fnc();
struct Callable {
  int operator()(int) &;
  long operator()(int) &&;
  char operator()(int) const &;
  void operator()(void*) noexcept;
};
struct Private { private: void operator()(); };
struct ConvToFp { using F = double (*)(int); operator F() const; };
using Lam = decltype([](auto x) { return x; });
using LamNx = decltype([](int) noexcept { return 1; });
static_assert(same<std::invoke_result_t<int (*)(int), int>, int>);
static_assert(same<std::invoke_result_t<int (&)(int), char>, int>);
static_assert(same<std::invoke_result_t<int(int), short>, int>);
static_assert(same<std::invoke_result_t<int (*const&)(int), int>, int>);
static_assert(same<std::invoke_result_t<decltype(fnr)>, int&>);
static_assert(same<std::invoke_result_t<decltype(fnrr)>, int&&>);
static_assert(same<std::invoke_result_t<decltype(fnc)>, int>);   // [expr.type]/2: cv dropped for non-class prvalues
static_assert(same<std::invoke_result_t<decltype(fnc_class)>, const S>);   // class prvalues keep cv
static_assert(same<std::invoke_result_t<Callable&, int>, int>);
static_assert(same<std::invoke_result_t<Callable, int>, long>);
static_assert(same<std::invoke_result_t<const Callable&, int>, char>);
static_assert(same<std::invoke_result_t<const Callable, int>, char>);
static_assert(same<std::invoke_result_t<Callable&, void*>, void>);
static_assert(same<std::invoke_result_t<ConvToFp, int>, double>);
static_assert(same<std::invoke_result_t<Lam, int&>, int>);
static_assert(same<std::invoke_result_t<Lam&, const char*>, const char*>);
static_assert(same<std::invoke_result_t<std::reference_wrapper<int(int)>, int>, int>);
static_assert(same<std::invoke_result_t<std::reference_wrapper<Callable>, int>, int>);  // get() is an lvalue
static_assert(!HasIR<Private>);
static_assert(!HasIR<int>);
static_assert(!HasIR<int*>);
static_assert(!HasIR<void>);
static_assert(!HasIR<int (*)(int)>);
static_assert(!HasIR<int (*)(int), void*>);
static_assert(!HasIR<Callable&, S>);
static_assert(!HasIR<void() const>);        // abominable function type: not callable
static_assert(!HasIR<void, int>);
static_assert(!std::is_invocable_v<void>);
static_assert(!std::is_invocable_v<void() const>);

// --- is_invocable / is_nothrow_invocable vs invoke_result
template <class F, class... A> constexpr bool agree = std::is_invocable_v<F, A...> == HasIR<F, A...> &&
    std::is_invocable<F, A...>::value == std::is_invocable_v<F, A...> &&
    std::is_base_of_v<std::bool_constant<std::is_invocable_v<F, A...>>, std::is_invocable<F, A...>>;
static_assert(agree<PMF, S&, int> && agree<PMF, const S&, int> && agree<PMD, S*> && agree<PMD, int*> &&
              agree<Callable, int> && agree<Private> && agree<void> && agree<int(int), int>);
static_assert(std::is_nothrow_invocable_v<PMFn, S&>);
static_assert(std::is_nothrow_invocable_v<PMFn, S*>);
static_assert(std::is_nothrow_invocable_v<PMFn, std::reference_wrapper<S>>);   // get() is noexcept
static_assert(!std::is_nothrow_invocable_v<PMFn, Fancy>);                     // operator* may throw
static_assert(std::is_nothrow_invocable_v<PMFn, FancyNoexcept>);
static_assert(!std::is_nothrow_invocable_v<PMF, S&, int>);
static_assert(std::is_nothrow_invocable_v<PMD, S&>);
static_assert(std::is_nothrow_invocable_v<PMD, S*>);
static_assert(!std::is_nothrow_invocable_v<PMD, Fancy>);
static_assert(std::is_nothrow_invocable_v<Callable&, void*>);
static_assert(!std::is_nothrow_invocable_v<Callable&, int>);
static_assert(std::is_nothrow_invocable_v<LamNx, int>);
static_assert(std::is_nothrow_invocable_v<void (*)() noexcept>);
static_assert(!std::is_nothrow_invocable_v<void (*)()>);
static_assert(!std::is_nothrow_invocable_v<PMFn, S&, int>);    // not invocable at all
// Argument conversion is part of the expression.
struct ThrowingConv { ThrowingConv(int); };
struct NothrowConv { NothrowConv(int) noexcept; };
void takes_tc(ThrowingConv) noexcept;
void takes_nc(NothrowConv) noexcept;
static_assert(!std::is_nothrow_invocable_v<decltype(&takes_tc), int>);
static_assert(std::is_nothrow_invocable_v<decltype(&takes_nc), int>);

// --- is_invocable_r
struct Explicit { explicit Explicit(int); };
struct Implicit { Implicit(int); };
struct ThrowConvTo { ThrowConvTo(int); };
struct NothrowConvTo { NothrowConvTo(int) noexcept; };
struct NonMovable { NonMovable(); NonMovable(NonMovable&&) = delete; };
NonMovable make_nm();
NonMovable& make_nm_ref();
int ret_int();
int& ret_int_ref();
long ret_long();
static_assert(std::is_invocable_r_v<int, decltype(ret_int)>);
static_assert(std::is_invocable_r_v<long, decltype(ret_int)>);
static_assert(std::is_invocable_r_v<void, decltype(ret_int)>);
static_assert(std::is_invocable_r_v<const void, decltype(ret_int)>);
static_assert(std::is_invocable_r_v<const volatile void, decltype(make_nm)>);
static_assert(std::is_invocable_r_v<void, PMD, S&>);
static_assert(!std::is_invocable_r_v<void, PMD, int*>);          // void R still requires INVOKE to be valid
static_assert(!std::is_invocable_r_v<void, void>);
static_assert(std::is_invocable_r_v<Implicit, decltype(ret_int)>);
static_assert(!std::is_invocable_r_v<Explicit, decltype(ret_int)>);  // only explicitly convertible
static_assert(!std::is_invocable_r_v<int*, decltype(ret_int)>);
static_assert(std::is_invocable_r_v<NonMovable, decltype(make_nm)>);       // prvalue initializes R directly
static_assert(!std::is_invocable_r_v<NonMovable, decltype(make_nm_ref)>);  // copy from an lvalue: no ctor
static_assert(std::is_invocable_r_v<NonMovable&, decltype(make_nm_ref)>);
static_assert(std::is_invocable_r_v<int&, decltype(ret_int_ref)>);
static_assert(!std::is_invocable_r_v<int&, decltype(ret_int)>);
static_assert(!std::is_invocable_r_v<long&, decltype(ret_int_ref)>);
// reference_converts_from_temporary: binding R to a temporary makes INVOKE<R> ill-formed.
static_assert(!std::is_invocable_r_v<const int&, decltype(ret_int)>);
static_assert(!std::is_invocable_r_v<int&&, decltype(ret_int)>);
static_assert(!std::is_invocable_r_v<const long&, decltype(ret_int_ref)>);
static_assert(!std::is_invocable_r_v<const long&, decltype(ret_long)>);
static_assert(std::is_invocable_r_v<const int&, decltype(ret_int_ref)>);
static_assert(std::is_invocable_r_v<const int&, PMD, S>);              // int&& binds directly
static_assert(std::is_invocable_r_v<int&&, PMD, S>);
static_assert(!std::is_invocable_r_v<const S&, decltype(make_nm)>);
static_assert(std::is_invocable_r_v<const S&, PMD, S> == false);      // int&& to const S&: no conversion
static_assert(!std::is_nothrow_invocable_r_v<const int&, decltype(ret_int)>);
// is_nothrow_invocable_r: the conversion to R counts.
int ret_int_nx() noexcept;
NonMovable make_nm_nx() noexcept;
static_assert(std::is_nothrow_invocable_r_v<long, decltype(ret_int_nx)>);
static_assert(std::is_nothrow_invocable_r_v<void, decltype(ret_int_nx)>);
static_assert(!std::is_nothrow_invocable_r_v<long, decltype(ret_int)>);
static_assert(!std::is_nothrow_invocable_r_v<ThrowConvTo, decltype(ret_int_nx)>);
static_assert(std::is_nothrow_invocable_r_v<NothrowConvTo, decltype(ret_int_nx)>);
static_assert(std::is_nothrow_invocable_r_v<NonMovable, decltype(make_nm_nx)>);
static_assert(!std::is_nothrow_invocable_r_v<Explicit, decltype(ret_int_nx)>);
static_assert(std::is_base_of_v<std::true_type, std::is_invocable_r<void, decltype(ret_int)>>);
static_assert(std::is_base_of_v<std::false_type, std::is_invocable_r<Explicit, decltype(ret_int)>>);
static_assert(std::is_base_of_v<std::false_type, std::is_nothrow_invocable_r<long, decltype(ret_int)>>);

int main() {}
