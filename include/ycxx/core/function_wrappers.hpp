// libycxx core: polymorphic function wrappers ([func.wrap]): bad_function_call, function,
// move_only_function, copyable_function and function_ref.
//
// The owning wrappers share one implementation, __ycxx::__adl_free::__fn_base: a three-pointer
// small buffer, a call thunk and a pointer to a per-type operations table (relocate, destroy,
// copy, type identity). Targets that fit and are nothrow-move-constructible live in the buffer;
// others are allocated with a plain new-expression (honouring a class-specific operator new, as
// std::any does). Function pointers, reference_wrappers and small lambdas never allocate. An
// empty wrapper holds a thunk that throws bad_function_call (function) or reports the violated
// precondition (the others), so a call is one indirect jump with no emptiness test.
//
// They are in core although only function_ref is freestanding in the draft: with no heap, the
// replaceable operator new reports bad_alloc through the error handler (DECISIONS §3).
//
// Arguments of scalar type travel to the thunk by value, others by reference, so an int is not
// spilled to memory to cross the type-erasure boundary.
#pragma once

#include <initializer_list>
#include <ycxx/core/constant_wrapper.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/invoke.hpp>
#include <ycxx/core/new.hpp>
#include <ycxx/core/typeinfo.hpp>
#include <ycxx/core/utility_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

// ---- [func.wrap.badcall] ----
class bad_function_call : public exception {
public:
  constexpr bad_function_call() noexcept {}
  constexpr bad_function_call(const bad_function_call&) noexcept = default;
  constexpr bad_function_call& operator=(const bad_function_call&) noexcept = default;
  constexpr ~bad_function_call() override {}
  const char* what() const noexcept override { return "bad function call"; }
};

template <class>
class function;
template <class...>
class move_only_function;
template <class...>
class copyable_function;
template <class...>
class function_ref;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
[[noreturn]] [[__gnu__::__cold__]] inline void __throw_bad_function_call() {
  ::__ycxx::__detail::__raise_with(ycxx_error_bad_function_call, "std::bad_function_call", [] { return std::bad_function_call(); });
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__fw {

enum class kind : unsigned char { function, __move_only, copyable };
// The cv/ref qualifiers of the call operator. `function` is invoked as FD& from a const call
// operator ([func.wrap.func.inv]).
enum class __quals : unsigned char { none, c, __lref, __clref, __rref, __crref, function };

// `_VT __cv ref` (the is-callable-from check) and `_VT __inv-__quals` (the invocation).
template <__quals _Qp, class _VT>
struct __quals_of;
template <class _VT>
struct __quals_of<__quals::none, _VT> {
  using __cvref = _VT;
  using __inv = _VT&;
};
template <class _VT>
struct __quals_of<__quals::c, _VT> {
  using __cvref = const _VT;
  using __inv = const _VT&;
};
template <class _VT>
struct __quals_of<__quals::__lref, _VT> {
  using __cvref = _VT&;
  using __inv = _VT&;
};
template <class _VT>
struct __quals_of<__quals::__clref, _VT> {
  using __cvref = const _VT&;
  using __inv = const _VT&;
};
template <class _VT>
struct __quals_of<__quals::__rref, _VT> {
  using __cvref = _VT&&;
  using __inv = _VT&&;
};
template <class _VT>
struct __quals_of<__quals::__crref, _VT> {
  using __cvref = const _VT&&;
  using __inv = const _VT&&;
};
template <class _VT>
struct __quals_of<__quals::function, _VT> {
  using __cvref = _VT&;
  using __inv = _VT&;
};

template <bool _Np, class _Rp, class _Fp, class... _Ap>
consteval bool __invocable_r() {
  if constexpr (_Np)
    return std::is_nothrow_invocable_r_v<_Rp, _Fp, _Ap...>;
  else
    return std::is_invocable_r_v<_Rp, _Fp, _Ap...>;
}

// How an argument crosses the thunk boundary.
template <class _Ap>
using __param_t = std::conditional_t<std::is_scalar_v<_Ap>, _Ap, _Ap&&>;

inline constexpr std::size_t __small_size = 3 * sizeof(void*);
// fn_base's move assignment relies on no wrapper fitting in the buffer.
union __storage {
  void* p;
  alignas(void*) unsigned char __buf[__small_size];
};

template <class _VT>
inline constexpr bool __is_small =
    sizeof(_VT) <= __small_size && alignof(_VT) <= alignof(__storage) && std::is_nothrow_move_constructible_v<_VT>;

template <class _VT>
[[__gnu__::__always_inline__]] inline _VT* target(__storage& s) noexcept {
  if constexpr (__is_small<_VT>)
    return std::launder(reinterpret_cast<_VT*>(s.__buf));
  else
    return static_cast<_VT*>(s.p);
}

struct __ops {
  void (*__relocate)(__storage& __dst, __storage& __src) noexcept; // nullptr: copy the storage bytes
  void (*destroy)(__storage&) noexcept;                     // nullptr: nothing to do
  void (*copy)(__storage& __dst, const __storage& __src);         // nullptr for move_only_function
  const void* tag;                                        // identifies the target type
  const std::type_info* type;                             // nullptr without RTTI
};

template <class _Tp>
inline constexpr char __type_tag = 0;

template <class _VT>
struct __handler {
  static void __relocate(__storage& d, __storage& s) noexcept {
    _VT* __src = ::__ycxx::__detail::__fw::target<_VT>(s);
    ::new (static_cast<void*>(d.__buf)) _VT(static_cast<_VT&&>(*__src));
    __src->~_VT();
  }
  static void destroy(__storage& s) noexcept {
    if constexpr (__is_small<_VT>)
      ::__ycxx::__detail::__fw::target<_VT>(s)->~_VT();
    else
      delete ::__ycxx::__detail::__fw::target<_VT>(s);
  }
  static void copy(__storage& d, const __storage& s) {
    const _VT& __src = *::__ycxx::__detail::__fw::target<_VT>(const_cast<__storage&>(s));
    if constexpr (__is_small<_VT>)
      ::new (static_cast<void*>(d.__buf)) _VT(__src);
    else
      d.p = new _VT(__src);
  }
};

// Each entry names a handler member only when it is needed, so a move-only or immovable target
// never instantiates the copy or relocate code.
template <class _VT>
consteval auto __relocate_fn() {
  using __fn = void (*)(__storage&, __storage&) noexcept;
  if constexpr (__is_small<_VT> && !std::is_trivially_copyable_v<_VT>)
    return __fn(&__handler<_VT>::__relocate);
  else
    return __fn(nullptr);
}
template <class _VT>
consteval auto __destroy_fn() {
  using __fn = void (*)(__storage&) noexcept;
  if constexpr (__is_small<_VT> && std::is_trivially_destructible_v<_VT>)
    return __fn(nullptr);
  else
    return __fn(&__handler<_VT>::destroy);
}
template <class _VT, bool _Copy>
consteval auto __copy_fn() {
  using __fn = void (*)(__storage&, const __storage&);
  if constexpr (_Copy)
    return __fn(&__handler<_VT>::copy);
  else
    return __fn(nullptr);
}

template <class _VT, bool _Copy>
inline constexpr __ops __ops_for = {__relocate_fn<_VT>(), __destroy_fn<_VT>(), __copy_fn<_VT, _Copy>(), &__type_tag<_VT>,
                                ::__ycxx::__detail::__type_id<_VT>};

[[__gnu__::__always_inline__]] inline void __relocate(const __ops* op, __storage& d, __storage& s) noexcept {
  if (op->__relocate)
    op->__relocate(d, s);
  else
    __builtin_memcpy(&d, &s, sizeof(__storage)); // implicitly creates the trivially copyable target
}

template <class _VT, class _Inv, bool _Np, class _Rp, class... _Ap>
_Rp __call_target(__storage& s, __param_t<_Ap>... a) noexcept(_Np) {
  return ::__ycxx::__detail::invoke_r<_Rp>(static_cast<_Inv>(*::__ycxx::__detail::__fw::target<_VT>(s)),
                                     static_cast<__param_t<_Ap>&&>(a)...);
}
template <bool _Np, class _Rp, class... _Ap>
[[noreturn]] _Rp __call_empty(__storage&, __param_t<_Ap>...) noexcept(_Np) {
  ::__ycxx::__detail::__assertion_failed("std::move_only_function/copyable_function: called with no target");
}
template <class _Rp, class... _Ap>
[[noreturn]] _Rp __call_empty_function(__storage&, __param_t<_Ap>...) {
  ::__ycxx::__detail::__throw_bad_function_call();
}

struct __empty_function_target {};

template <class _Tp>
inline constexpr bool __is_in_place_type = false;
template <class _Tp>
inline constexpr bool __is_in_place_type<std::in_place_type_t<_Tp>> = true;

template <class _Tp>
inline constexpr bool __is_function_spec = false;
template <class _Sp>
inline constexpr bool __is_function_spec<std::function<_Sp>> = true;
template <class _Tp>
inline constexpr bool __is_move_only_spec = false;
template <class... _Sp>
inline constexpr bool __is_move_only_spec<std::move_only_function<_Sp...>> = true;
template <class _Tp>
inline constexpr bool __is_copyable_spec = false;
template <class... _Sp>
inline constexpr bool __is_copyable_spec<std::copyable_function<_Sp...>> = true;

// Sources whose emptiness carries over ([func.wrap.func.con]/12.3, [func.wrap.move.ctor]/8.3,
// [func.wrap.copy.ctor]/10.3).
template <kind _Kp, class _Tp>
inline constexpr bool __empty_carries =
    _Kp == kind::function    ? __is_function_spec<_Tp>
    : _Kp == kind::copyable  ? __is_copyable_spec<_Tp>
                           : __is_move_only_spec<_Tp> || __is_copyable_spec<_Tp>;

template <class _VT>
inline constexpr bool __is_nullable_pointer =
    (std::is_pointer_v<_VT> && std::is_function_v<std::remove_pointer_t<_VT>>) || std::is_member_pointer_v<_VT>;

// ---- deduction-guide support ----
// R(G::*)(A...) cv &opt noexcept(E) -> R(A...) noexcept(E) ([func.wrap.func.con]/16.1,
// [func.wrap.ref.deduct]/5.1).
template <class _Mp>
struct __memfn_sig {};
template <class _Rp, class _Gp, class... _Ap, bool _Ep>
struct __memfn_sig<_Rp (_Gp::*)(_Ap...) noexcept(_Ep)> {
  using type = _Rp(_Ap...) noexcept(_Ep);
  using __plain = _Rp(_Ap...);
};
template <class _Rp, class _Gp, class... _Ap, bool _Ep>
struct __memfn_sig<_Rp (_Gp::*)(_Ap...) const noexcept(_Ep)> : __memfn_sig<_Rp (_Gp::*)(_Ap...) noexcept(_Ep)> {};
template <class _Rp, class _Gp, class... _Ap, bool _Ep>
struct __memfn_sig<_Rp (_Gp::*)(_Ap...) volatile noexcept(_Ep)> : __memfn_sig<_Rp (_Gp::*)(_Ap...) noexcept(_Ep)> {};
template <class _Rp, class _Gp, class... _Ap, bool _Ep>
struct __memfn_sig<_Rp (_Gp::*)(_Ap...) const volatile noexcept(_Ep)> : __memfn_sig<_Rp (_Gp::*)(_Ap...) noexcept(_Ep)> {};
template <class _Rp, class _Gp, class... _Ap, bool _Ep>
struct __memfn_sig<_Rp (_Gp::*)(_Ap...) & noexcept(_Ep)> : __memfn_sig<_Rp (_Gp::*)(_Ap...) noexcept(_Ep)> {};
template <class _Rp, class _Gp, class... _Ap, bool _Ep>
struct __memfn_sig<_Rp (_Gp::*)(_Ap...) const & noexcept(_Ep)> : __memfn_sig<_Rp (_Gp::*)(_Ap...) noexcept(_Ep)> {};
template <class _Rp, class _Gp, class... _Ap, bool _Ep>
struct __memfn_sig<_Rp (_Gp::*)(_Ap...) volatile & noexcept(_Ep)> : __memfn_sig<_Rp (_Gp::*)(_Ap...) noexcept(_Ep)> {};
template <class _Rp, class _Gp, class... _Ap, bool _Ep>
struct __memfn_sig<_Rp (_Gp::*)(_Ap...) const volatile & noexcept(_Ep)> : __memfn_sig<_Rp (_Gp::*)(_Ap...) noexcept(_Ep)> {};

// A function pointer from &F::operator() comes from an explicit-object member function or a
// static one ([func.wrap.func.con]/16). A static operator() accepts every parameter as an
// argument; an explicit-object one takes its first parameter from the object expression, so
// called with all of them it has one argument too many.
template <class _Fp, class _Rp, class... _Ap>
struct __fnptr_sig {};
template <class _Fp, class _Rp, class... _Ap>
  requires requires { std::declval<_Fp&>().operator()(std::declval<_Ap>()...); }
struct __fnptr_sig<_Fp, _Rp, _Ap...> {
  using __plain = _Rp(_Ap...);
};
template <class _Fp, class _Rp, class _Gp, class... _Ap>
  requires(!requires { std::declval<_Fp&>().operator()(std::declval<_Gp>(), std::declval<_Ap>()...); })
struct __fnptr_sig<_Fp, _Rp, _Gp, _Ap...> {
  using __plain = _Rp(_Ap...);
};

template <class _Fp, class _Mp>
struct __call_op_sig {};
template <class _Fp, class _Mp>
  requires requires { typename __memfn_sig<_Mp>::__plain; }
struct __call_op_sig<_Fp, _Mp> {
  using type = typename __memfn_sig<_Mp>::__plain;
};
template <class _Fp, class _Rp, class... _Ap, bool _Ep>
  requires requires { typename __fnptr_sig<_Fp, _Rp, _Ap...>::__plain; }
struct __call_op_sig<_Fp, _Rp (*)(_Ap...) noexcept(_Ep)> {
  using type = typename __fnptr_sig<_Fp, _Rp, _Ap...>::__plain;
};

template <class _Fp>
struct __function_guide {};
template <class _Fp>
  requires requires { &_Fp::operator(); }
struct __function_guide<_Fp> : __call_op_sig<_Fp, decltype(&_Fp::operator())> {};

// [func.wrap.ref.deduct]/5.
template <class _Fp, class _Tp>
struct __fref_bound_sig {};
template <class _Fp, class _Tp>
  requires std::is_member_function_pointer_v<_Fp> && requires { typename __memfn_sig<_Fp>::type; }
struct __fref_bound_sig<_Fp, _Tp> {
  using type = typename __memfn_sig<_Fp>::type;
};
template <class _Mp, class _Gp, class _Tp>
  requires std::is_object_v<_Mp> && requires { typename std::invoke_result<_Mp _Gp::*, _Tp&>::type; }
struct __fref_bound_sig<_Mp _Gp::*, _Tp> {
  using type = std::invoke_result_t<_Mp _Gp::*, _Tp&>() noexcept;
};
template <class _Rp, class _Gp, class... _Ap, bool _Ep, class _Tp>
struct __fref_bound_sig<_Rp (*)(_Gp, _Ap...) noexcept(_Ep), _Tp> {
  using type = _Rp(_Ap...) noexcept(_Ep);
};

// ---- function_ref support ----
union __bound_entity {
  const volatile void* __obj;
  void (*__fn)();
};

// is-convertible-from-specialization<F> for function_ref<R(A...) cv noexcept(N)>, where C is cv.
template <bool _Cp, bool _Np, class _Sig, class _Fp>
inline constexpr bool __fref_from_spec = false;
template <bool _Cp, bool _Np, class _Rp, class... _Ap, bool _N2>
inline constexpr bool __fref_from_spec<_Cp, _Np, _Rp(_Ap...), std::function_ref<_Rp(_Ap...) noexcept(_N2)>> = (_N2 || !_Np) && !_Cp;
template <bool _Cp, bool _Np, class _Rp, class... _Ap, bool _N2>
inline constexpr bool __fref_from_spec<_Cp, _Np, _Rp(_Ap...), std::function_ref<_Rp(_Ap...) const noexcept(_N2)>> = _N2 || !_Np;

template <class _Tp>
inline constexpr bool __is_constant_wrapper = false;
template <auto _Xp, class _Tp>
inline constexpr bool __is_constant_wrapper<std::constant_wrapper<_Xp, _Tp>> = true;

}} // namespace __ycxx::__detail::__fw

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// The owning wrappers. Self is the derived std:: class.
template <class _Self, ::__ycxx::__detail::__fw::kind _Kp, ::__ycxx::__detail::__fw::__quals _Qp, bool _Np, class _Rp, class... _Ap>
class __fn_base {
  template <class, ::__ycxx::__detail::__fw::kind, ::__ycxx::__detail::__fw::__quals, bool, class, class...>
  friend class __fn_base;
  using __storage = ::__ycxx::__detail::__fw::__storage;
  using __ops = ::__ycxx::__detail::__fw::__ops;
  using kind = ::__ycxx::__detail::__fw::kind;
  using __thunk_t = _Rp (*)(__storage&, ::__ycxx::__detail::__fw::__param_t<_Ap>...) noexcept(_Np);

  static constexpr bool copyable = _Kp != kind::__move_only;

  static constexpr __thunk_t __empty_thunk() noexcept {
    if constexpr (_Kp == kind::function)
      return &::__ycxx::__detail::__fw::__call_empty_function<_Rp, _Ap...>;
    else
      return &::__ycxx::__detail::__fw::__call_empty<_Np, _Rp, _Ap...>;
  }

  // is-callable-from<VT>
  template <class _VT>
  static consteval bool __callable_from() {
    using __q = ::__ycxx::__detail::__fw::__quals_of<_Qp, _VT>;
    return ::__ycxx::__detail::__fw::__invocable_r<_Np, _Rp, typename __q::__cvref, _Ap...>() &&
           ::__ycxx::__detail::__fw::__invocable_r<_Np, _Rp, typename __q::__inv, _Ap...>();
  }

  _Self& __self() noexcept { return static_cast<_Self&>(*this); }

  // Another owning wrapper with the same R and argument passing ([func.wrap.general]/3: avoid double wrapping).
  // Its target, invoked through its own thunk, is what invoking it would do; is-callable-from
  // has already checked that our qualifiers may call it.
  static_assert(sizeof(__storage) + 2 * sizeof(void*) > ::__ycxx::__detail::__fw::__small_size);

  // The thunks need only agree on how arguments travel: T and T&& both pass a class-type
  // argument as T&&, and [func.wrap.general]/2 lets the inner invocation alias it.
  template <class... _A2>
  static constexpr bool __same_thunk_args =
      std::is_same_v<void(::__ycxx::__detail::__fw::__param_t<_Ap>...), void(::__ycxx::__detail::__fw::__param_t<_A2>...)>;
  template <class _S2, kind _K2, ::__ycxx::__detail::__fw::__quals _Q2, bool _N2, class... _A2>
    requires __same_thunk_args<_A2...>
  static _S2* __self_of(__fn_base<_S2, _K2, _Q2, _N2, _Rp, _A2...>*);
  template <class _S2, kind _K2, ::__ycxx::__detail::__fw::__quals _Q2, bool _N2, class... _A2>
    requires __same_thunk_args<_A2...>
  static __fn_base<_S2, _K2, _Q2, _N2, _Rp, _A2...>* __base_of(__fn_base<_S2, _K2, _Q2, _N2, _Rp, _A2...>* p) noexcept {
    return p;
  }
  // Strengthened noexcept ([res.on.exception.handling]/5): nothing can throw when the target is
  // stored in place and constructed without throwing, or when another wrapper's target is taken.
  template <class _VT, class... _Args>
  static consteval bool __ctor_noexcept() {
    if constexpr (sizeof...(_Args) == 1 && __adoptable<_VT>())
      return (... && (std::is_rvalue_reference_v<_Args&&> && !std::is_const_v<std::remove_reference_t<_Args>>));
    else
      return ::__ycxx::__detail::__fw::__is_small<_VT> && std::is_nothrow_constructible_v<_VT, _Args...>;
  }

  // Not for std::function: its target_type() and target() expose the target's type, which must
  // be the source wrapper ([func.wrap.func.con]/13, [func.wrap.func.targ]).
  template <class _Src>
  static consteval bool __adoptable() {
    if constexpr (_Kp == kind::function)
      return false;
    else if constexpr (requires(_Src* p) { __fn_base::__self_of(p); })
      return std::is_same_v<decltype(__fn_base::__self_of(static_cast<_Src*>(nullptr))), _Src*>;
    else
      return false;
  }

  template <class _VT, class... _Args>
  void emplace(_Args&&... __args) {
    if constexpr (::__ycxx::__detail::__fw::__is_small<_VT>)
      ::new (static_cast<void*>(__s_.__buf)) _VT(static_cast<_Args&&>(__args)...);
    else
      __s_.p = new _VT(static_cast<_Args&&>(__args)...);
    __ops_ = &::__ycxx::__detail::__fw::__ops_for<_VT, copyable>;
    __call_ = &::__ycxx::__detail::__fw::__call_target<_VT, typename ::__ycxx::__detail::__fw::__quals_of<_Qp, _VT>::__inv, _Np, _Rp, _Ap...>;
  }

  void take(__fn_base& __o) noexcept {
    if (__o.__ops_) {
      ::__ycxx::__detail::__fw::__relocate(__o.__ops_, __s_, __o.__s_);
      __ops_ = __o.__ops_;
      __call_ = __o.__call_;
      __o.__ops_ = nullptr;
      __o.__call_ = __empty_thunk();
    }
  }

  void reset() noexcept {
    if (__ops_) {
      if (__ops_->destroy)
        __ops_->destroy(__s_);
      __ops_ = nullptr;
      __call_ = __empty_thunk();
    }
  }

  void __swap_impl(__fn_base& __o) noexcept {
    if (this == __builtin_addressof(__o))
      return;
    __storage __tmp;
    if (__o.__ops_)
      ::__ycxx::__detail::__fw::__relocate(__o.__ops_, __tmp, __o.__s_);
    if (__ops_)
      ::__ycxx::__detail::__fw::__relocate(__ops_, __o.__s_, __s_);
    if (__o.__ops_)
      ::__ycxx::__detail::__fw::__relocate(__o.__ops_, __s_, __tmp);
    const __ops* op = __ops_;
    __ops_ = __o.__ops_;
    __o.__ops_ = op;
    __thunk_t c = __call_;
    __call_ = __o.__call_;
    __o.__call_ = c;
  }

  template <class _Tp>
  bool __holds() const noexcept {
    if (!__ops_)
      return false;
    if (__ops_->tag == &::__ycxx::__detail::__fw::__type_tag<_Tp>)
      return true;
    // Two copies of one table can exist across shared libraries.
    if constexpr (::__ycxx::__detail::__cfg::__rtti)
      return *__ops_->type == *::__ycxx::__detail::__type_id<_Tp>;
    else
      return false;
  }

protected:
  // The buffer is not at offset 0: an empty target there could share its address with another
  // object of its type, such as an empty base of a class that has this wrapper as a member.
  __thunk_t __call_ = __empty_thunk();
  const __ops* __ops_ = nullptr;
  mutable __storage __s_;

public:
  using result_type = _Rp;

  // User-provided, as the wrappers' default constructors are in the draft, so `const function<F>
  // f;` is valid ([dcl.init.general]/8: s_ has no default member initializer).
  __fn_base() noexcept {}
  __fn_base(std::nullptr_t) noexcept {}
  __fn_base(__fn_base&& __o) noexcept { take(__o); }
  __fn_base(const __fn_base& __o)
    requires copyable
  {
    if (__o.__ops_) {
      __o.__ops_->copy(__s_, __o.__s_);
      __ops_ = __o.__ops_;
      __call_ = __o.__call_;
    }
  }

  template <class _Fp, class _VT = std::decay_t<_Fp>>
    requires(!std::is_same_v<std::remove_cvref_t<_Fp>, _Self>) && (!std::is_same_v<std::remove_cvref_t<_Fp>, __fn_base>) &&
            (!::__ycxx::__detail::__fw::__is_in_place_type<std::remove_cvref_t<_Fp>>) && (__callable_from<_VT>())
  __fn_base(_Fp&& __f) noexcept(__ctor_noexcept<_VT, _Fp>()) {
    static_assert(std::is_constructible_v<_VT, _Fp>, "std::function/move_only_function/copyable_function: Mandates: is_constructible_v<VT, F>");
    if constexpr (copyable)
      static_assert(std::is_copy_constructible_v<_VT>, "std::function/move_only_function/copyable_function: Mandates: VT is copy constructible");
    if constexpr (std::is_constructible_v<_VT, _Fp> && (!copyable || std::is_copy_constructible_v<_VT>)) {
      if constexpr (::__ycxx::__detail::__fw::__is_nullable_pointer<_VT>) {
        if (__f == nullptr)
          return;
      } else if constexpr (__adoptable<_VT>()) {
        auto* __src = __fn_base::__base_of(const_cast<_VT*>(__builtin_addressof(__f)));
        if (__src->__ops_) {
          if constexpr (std::is_rvalue_reference_v<_Fp&&> && !std::is_const_v<std::remove_reference_t<_Fp>>)
            ::__ycxx::__detail::__fw::__relocate(__src->__ops_, __s_, __src->__s_);
          else
            __src->__ops_->copy(__s_, __src->__s_);
          __ops_ = __src->__ops_;
          __call_ = __src->__call_;
          if constexpr (std::is_rvalue_reference_v<_Fp&&> && !std::is_const_v<std::remove_reference_t<_Fp>>) {
            __src->__ops_ = nullptr;
            __src->__call_ = __src->__empty_thunk();
          }
          return;
        }
        if constexpr (::__ycxx::__detail::__fw::__empty_carries<_Kp, _VT>) {
          return;
        } else {
          // An empty std::function becomes a target of a move_only_function or
          // copyable_function: a stateless stand-in that throws as invoking it would.
          __ops_ = &::__ycxx::__detail::__fw::__ops_for<::__ycxx::__detail::__fw::__empty_function_target, copyable>;
          __call_ = &::__ycxx::__detail::__fw::__call_empty_function<_Rp, _Ap...>;
          return;
        }
      } else if constexpr (::__ycxx::__detail::__fw::__empty_carries<_Kp, _VT>) {
        if (!static_cast<bool>(__f))
          return;
      }
      emplace<_VT>(static_cast<_Fp&&>(__f));
    }
  }

  template <class _Tp, class... _Args, class _VT = std::decay_t<_Tp>>
    requires(_Kp != kind::function) && std::is_constructible_v<_VT, _Args...> && (__callable_from<_VT>())
  explicit __fn_base(std::in_place_type_t<_Tp>, _Args&&... __args) noexcept(__ctor_noexcept<_VT, _Args...>()) {
    static_assert(std::is_same_v<_VT, _Tp>, "std::function/move_only_function/copyable_function: Mandates: VT is the same type as T");
    if constexpr (copyable)
      static_assert(std::is_copy_constructible_v<_VT>, "std::function/move_only_function/copyable_function: Mandates: VT is copy constructible");
    if constexpr (std::is_same_v<_VT, _Tp> && (!copyable || std::is_copy_constructible_v<_VT>))
      emplace<_VT>(static_cast<_Args&&>(__args)...);
  }
  template <class _Tp, class _Up, class... _Args, class _VT = std::decay_t<_Tp>>
    requires(_Kp != kind::function) && std::is_constructible_v<_VT, std::initializer_list<_Up>&, _Args...> &&
            (__callable_from<_VT>())
  explicit __fn_base(std::in_place_type_t<_Tp>, std::initializer_list<_Up> il, _Args&&... __args) {
    static_assert(std::is_same_v<_VT, _Tp>, "std::function/move_only_function/copyable_function: Mandates: VT is the same type as T");
    if constexpr (copyable)
      static_assert(std::is_copy_constructible_v<_VT>, "std::function/move_only_function/copyable_function: Mandates: VT is copy constructible");
    if constexpr (std::is_same_v<_VT, _Tp> && (!copyable || std::is_copy_constructible_v<_VT>))
      emplace<_VT>(il, static_cast<_Args&&>(__args)...);
  }

  // "Equivalent to: W(std::move(f)).swap(*this)": the old target is destroyed only after the
  // source's has been taken, since the source may live inside it. The old target is first moved
  // aside; one that can contain a wrapper (at least 40 bytes) is never in the 24-byte buffer, so
  // that move leaves it, and the source inside it, where they are. The source's target is
  // relocated once.
  __fn_base& operator=(__fn_base&& __o) noexcept {
    if (this != __builtin_addressof(__o)) {
      __storage __old;
      const __ops* __old_ops = __ops_;
      if (__old_ops)
        ::__ycxx::__detail::__fw::__relocate(__old_ops, __old, __s_);
      __ops_ = nullptr;
      __call_ = __empty_thunk();
      take(__o);
      if (__old_ops && __old_ops->destroy)
        __old_ops->destroy(__old);
    }
    return *this;
  }
  // "Equivalent to: W(f).swap(*this)": self-assignment copies too. Releasing the old target and
  // taking the copy's is that swap with one relocation fewer.
  __fn_base& operator=(const __fn_base& __o)
    requires copyable
  {
    __fn_base __tmp(__o);
    reset();
    take(__tmp);
    return *this;
  }
  _Self& operator=(std::nullptr_t) noexcept {
    reset();
    return __self();
  }
  template <class _Fp>
    requires(!std::is_same_v<std::remove_cvref_t<_Fp>, _Self>) && (!std::is_same_v<std::remove_cvref_t<_Fp>, __fn_base>) &&
            (_Kp == kind::function ? std::is_invocable_r_v<_Rp, std::decay_t<_Fp>&, _Ap...>
                                 : std::is_constructible_v<_Self, _Fp>)
  _Self& operator=(_Fp&& __f) {
    _Self __tmp(static_cast<_Fp&&>(__f));
    reset();
    take(__tmp);
    return __self();
  }

  ~__fn_base() {
    if (__ops_ && __ops_->destroy)
      __ops_->destroy(__s_);
  }

  void swap(_Self& other) noexcept { __swap_impl(other); }
  explicit operator bool() const noexcept { return __ops_ != nullptr; }

  // ---- [func.wrap.func.targ] ----
  const std::type_info& target_type() const noexcept
    requires(_Kp == kind::function) && ::__ycxx::__detail::__cfg::__rtti
  {
    return __ops_ ? *__ops_->type : *::__ycxx::__detail::__type_id<void>;
  }
  template <class _Tp>
    requires(_Kp == kind::function)
  _Tp* target() noexcept {
    using _Up = std::remove_cv_t<_Tp>;
    if constexpr (std::is_object_v<_Up> && !std::is_array_v<_Up>) {
      if (__holds<_Up>())
        return ::__ycxx::__detail::__fw::target<_Up>(__s_);
    }
    return nullptr;
  }
  template <class _Tp>
    requires(_Kp == kind::function)
  const _Tp* target() const noexcept {
    return const_cast<__fn_base*>(this)->template target<_Tp>();
  }

  friend void swap(_Self& a, _Self& b) noexcept { a.swap(b); }
  friend bool operator==(const _Self& __f, std::nullptr_t) noexcept { return !__f; }
};

// function_ref. Self is the derived std:: class; C is the cv placeholder.
template <class _Self, bool _Cp, bool _Np, class _Rp, class... _Ap>
class __fref_base {
  template <class, bool, bool, class, class...>
  friend class __fref_base;

  using __bound_entity = ::__ycxx::__detail::__fw::__bound_entity;
  using __thunk_t = _Rp (*)(__bound_entity, ::__ycxx::__detail::__fw::__param_t<_Ap>...) noexcept(_Np);
  template <class _Tp>
  using __cv = std::conditional_t<_Cp, const _Tp, _Tp>;

  template <class... _Tp>
  static consteval bool __invocable_using() {
    return ::__ycxx::__detail::__fw::__invocable_r<_Np, _Rp, _Tp..., _Ap...>();
  }

  template <class _Fp>
  static _Rp __fn_thunk(__bound_entity __be, ::__ycxx::__detail::__fw::__param_t<_Ap>... a) noexcept(_Np) {
    return ::__ycxx::__detail::invoke_r<_Rp>(reinterpret_cast<_Fp*>(__be.__fn), static_cast<::__ycxx::__detail::__fw::__param_t<_Ap>&&>(a)...);
  }
  template <class _Tp> // T is cv-qualified as stored
  static _Rp __obj_thunk(__bound_entity __be, ::__ycxx::__detail::__fw::__param_t<_Ap>... a) noexcept(_Np) {
    return ::__ycxx::__detail::invoke_r<_Rp>(*static_cast<_Tp*>(const_cast<void*>(__be.__obj)),
                                       static_cast<::__ycxx::__detail::__fw::__param_t<_Ap>&&>(a)...);
  }
  template <class _CW>
  static _Rp __cw_thunk(__bound_entity, ::__ycxx::__detail::__fw::__param_t<_Ap>... a) noexcept(_Np) {
    return ::__ycxx::__detail::invoke_r<_Rp>(_CW::value, static_cast<::__ycxx::__detail::__fw::__param_t<_Ap>&&>(a)...);
  }
  template <class _CW, class _Tp>
  static _Rp __cw_obj_thunk(__bound_entity __be, ::__ycxx::__detail::__fw::__param_t<_Ap>... a) noexcept(_Np) {
    _Tp* p;
    if constexpr (std::is_function_v<_Tp>)
      p = reinterpret_cast<_Tp*>(__be.__fn);
    else
      p = static_cast<_Tp*>(const_cast<void*>(__be.__obj));
    return ::__ycxx::__detail::invoke_r<_Rp>(_CW::value, *p, static_cast<::__ycxx::__detail::__fw::__param_t<_Ap>&&>(a)...);
  }
  template <class _CW, class _Pp>
  static _Rp __cw_ptr_thunk(__bound_entity __be, ::__ycxx::__detail::__fw::__param_t<_Ap>... a) noexcept(_Np) {
    _Pp p;
    if constexpr (std::is_function_v<std::remove_pointer_t<_Pp>>)
      p = reinterpret_cast<_Pp>(__be.__fn);
    else
      p = static_cast<_Pp>(const_cast<void*>(__be.__obj));
    return ::__ycxx::__detail::invoke_r<_Rp>(_CW::value, p, static_cast<::__ycxx::__detail::__fw::__param_t<_Ap>&&>(a)...);
  }

  template <class _CW>
  static consteval void __check_cw_not_null() {
    using _Fp = typename _CW::value_type;
    if constexpr (std::is_pointer_v<_Fp> || std::is_member_pointer_v<_Fp>)
      static_assert(_CW::value != nullptr, "std::function_ref: Mandates: f.value != nullptr");
  }

  __bound_entity __be_;
  __thunk_t __thunk_;

public:
  // ---- [func.wrap.ref.ctor] ----
  template <class _Fp>
    requires std::is_function_v<_Fp> && (__invocable_using<_Fp>())
  __fref_base(_Fp* __f) noexcept {
    ::__ycxx::__detail::__precondition(__f != nullptr, "std::function_ref: null function pointer");
    __be_.__fn = reinterpret_cast<void (*)()>(__f);
    __thunk_ = &__fn_thunk<_Fp>;
  }

  template <class _Fp, class _Tp = std::remove_reference_t<_Fp>>
    requires(!std::is_same_v<std::remove_cvref_t<_Fp>, _Self>) && (!std::is_same_v<std::remove_cvref_t<_Fp>, __fref_base>) &&
            (!std::is_member_pointer_v<_Tp>) &&
            (__invocable_using<__cv<_Tp>&>())
  constexpr __fref_base(_Fp&& __f) noexcept {
    if constexpr (::__ycxx::__detail::__fw::__fref_from_spec<_Cp, _Np, _Rp(_Ap...), std::remove_cv_t<_Tp>>) {
      __be_ = __f.__be_;
      __thunk_ = __f.__thunk_;
    } else {
      // (A function lvalue picks the F* constructor by partial ordering.)
      __be_.__obj = __builtin_addressof(__f);
      __thunk_ = &__obj_thunk<__cv<_Tp>>;
    }
  }

  template <auto c, class _Fp>
    requires(__invocable_using<const _Fp&>())
  constexpr __fref_base(std::constant_wrapper<c, _Fp>) noexcept {
    using _CW = std::constant_wrapper<c, _Fp>;
    __check_cw_not_null<_CW>();
    if constexpr (sizeof...(_Ap) != 0)
      static_assert(!::__ycxx::__detail::__cw_constant_call<_CW, _Ap...>,
                    "std::function_ref: Mandates: the call does not produce a constant_wrapper");
    __be_.__obj = nullptr;
    __thunk_ = &__cw_thunk<_CW>;
  }

  template <auto c, class _Fp, class _Up, class _Tp = std::remove_reference_t<_Up>>
    requires(!std::is_rvalue_reference_v<_Up &&>) && (__invocable_using<const _Fp&, __cv<_Tp>&>())
  constexpr __fref_base(std::constant_wrapper<c, _Fp>, _Up&& __obj) noexcept {
    using _CW = std::constant_wrapper<c, _Fp>;
    __check_cw_not_null<_CW>();
    if constexpr (std::is_function_v<_Tp>)
      __be_.__fn = reinterpret_cast<void (*)()>(&__obj);
    else
      __be_.__obj = __builtin_addressof(__obj);
    __thunk_ = &__cw_obj_thunk<_CW, __cv<_Tp>>;
  }

  template <auto c, class _Fp, class _Tp>
    requires(!_Cp) && (__invocable_using<const _Fp&, _Tp*>())
  constexpr __fref_base(std::constant_wrapper<c, _Fp>, _Tp* __obj) noexcept {
    using _CW = std::constant_wrapper<c, _Fp>;
    __check_cw_not_null<_CW>();
    if constexpr (std::is_member_pointer_v<_Fp>)
      ::__ycxx::__detail::__precondition(__obj != nullptr, "std::function_ref: null object pointer");
    if constexpr (std::is_function_v<_Tp>)
      __be_.__fn = reinterpret_cast<void (*)()>(__obj);
    else
      __be_.__obj = __obj;
    __thunk_ = &__cw_ptr_thunk<_CW, _Tp*>;
  }
  template <auto c, class _Fp, class _Tp>
    requires _Cp && (__invocable_using<const _Fp&, const _Tp*>())
  constexpr __fref_base(std::constant_wrapper<c, _Fp>, const _Tp* __obj) noexcept {
    using _CW = std::constant_wrapper<c, _Fp>;
    __check_cw_not_null<_CW>();
    if constexpr (std::is_member_pointer_v<_Fp>)
      ::__ycxx::__detail::__precondition(__obj != nullptr, "std::function_ref: null object pointer");
    if constexpr (std::is_function_v<_Tp>)
      __be_.__fn = reinterpret_cast<void (*)()>(__obj);
    else
      __be_.__obj = __obj;
    __thunk_ = &__cw_ptr_thunk<_CW, const _Tp*>;
  }

  constexpr __fref_base(const __fref_base&) noexcept = default;
  constexpr __fref_base& operator=(const __fref_base&) noexcept = default;

  // ---- [func.wrap.ref.inv] ----
  _Rp operator()(_Ap... a) const noexcept(_Np) { return __thunk_(__be_, static_cast<_Ap&&>(a)...); }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

// ---- [func.wrap.func] ----
template <class _Rp, class... _Ap>
class function<_Rp(_Ap...)>
    : public ::__ycxx::__adl_free::__fn_base<function<_Rp(_Ap...)>, ::__ycxx::__detail::__fw::kind::function,
                                       ::__ycxx::__detail::__fw::__quals::function, false, _Rp, _Ap...> {
  using base = ::__ycxx::__adl_free::__fn_base<function, ::__ycxx::__detail::__fw::kind::function,
                                         ::__ycxx::__detail::__fw::__quals::function, false, _Rp, _Ap...>;

public:
  using base::base;
  using base::operator=;

  template <class _Fp>
  function& operator=(reference_wrapper<_Fp> __f) noexcept {
    function(__f).swap(*this);
    return *this;
  }

  _Rp operator()(_Ap... a) const { return this->__call_(this->__s_, static_cast<_Ap&&>(a)...); }
};

template <class _Rp, class... _Ap>
function(_Rp (*)(_Ap...)) -> function<_Rp(_Ap...)>;
template <class _Fp>
  requires requires { typename ::__ycxx::__detail::__fw::__function_guide<_Fp>::type; }
function(_Fp) -> function<typename ::__ycxx::__detail::__fw::__function_guide<_Fp>::type>;

// ---- [func.wrap.move] / [func.wrap.copy] ----
// One partial specialization per cv/ref combination; noex is deduced.
template <class _Rp, class... _Ap, bool _Np>
class move_only_function<_Rp(_Ap...) noexcept(_Np)>
    : public ::__ycxx::__adl_free::__fn_base<move_only_function<_Rp(_Ap...) noexcept(_Np)>, ::__ycxx::__detail::__fw::kind::__move_only,
                                       ::__ycxx::__detail::__fw::__quals::none, _Np, _Rp, _Ap...> {
  using base = typename move_only_function::__fn_base;

public:
  using base::base;
  using base::operator=;
  _Rp operator()(_Ap... a) noexcept(_Np) { return this->__call_(this->__s_, static_cast<_Ap&&>(a)...); }
};
template <class _Rp, class... _Ap, bool _Np>
class move_only_function<_Rp(_Ap...) const noexcept(_Np)>
    : public ::__ycxx::__adl_free::__fn_base<move_only_function<_Rp(_Ap...) const noexcept(_Np)>,
                                       ::__ycxx::__detail::__fw::kind::__move_only, ::__ycxx::__detail::__fw::__quals::c, _Np, _Rp, _Ap...> {
  using base = typename move_only_function::__fn_base;

public:
  using base::base;
  using base::operator=;
  _Rp operator()(_Ap... a) const noexcept(_Np) { return this->__call_(this->__s_, static_cast<_Ap&&>(a)...); }
};
template <class _Rp, class... _Ap, bool _Np>
class move_only_function<_Rp(_Ap...) & noexcept(_Np)>
    : public ::__ycxx::__adl_free::__fn_base<move_only_function<_Rp(_Ap...) & noexcept(_Np)>, ::__ycxx::__detail::__fw::kind::__move_only,
                                       ::__ycxx::__detail::__fw::__quals::__lref, _Np, _Rp, _Ap...> {
  using base = typename move_only_function::__fn_base;

public:
  using base::base;
  using base::operator=;
  _Rp operator()(_Ap... a) & noexcept(_Np) { return this->__call_(this->__s_, static_cast<_Ap&&>(a)...); }
};
template <class _Rp, class... _Ap, bool _Np>
class move_only_function<_Rp(_Ap...) const & noexcept(_Np)>
    : public ::__ycxx::__adl_free::__fn_base<move_only_function<_Rp(_Ap...) const & noexcept(_Np)>,
                                       ::__ycxx::__detail::__fw::kind::__move_only, ::__ycxx::__detail::__fw::__quals::__clref, _Np, _Rp,
                                       _Ap...> {
  using base = typename move_only_function::__fn_base;

public:
  using base::base;
  using base::operator=;
  _Rp operator()(_Ap... a) const & noexcept(_Np) { return this->__call_(this->__s_, static_cast<_Ap&&>(a)...); }
};
template <class _Rp, class... _Ap, bool _Np>
class move_only_function<_Rp(_Ap...) && noexcept(_Np)>
    : public ::__ycxx::__adl_free::__fn_base<move_only_function<_Rp(_Ap...) && noexcept(_Np)>,
                                       ::__ycxx::__detail::__fw::kind::__move_only, ::__ycxx::__detail::__fw::__quals::__rref, _Np, _Rp, _Ap...> {
  using base = typename move_only_function::__fn_base;

public:
  using base::base;
  using base::operator=;
  _Rp operator()(_Ap... a) && noexcept(_Np) { return this->__call_(this->__s_, static_cast<_Ap&&>(a)...); }
};
template <class _Rp, class... _Ap, bool _Np>
class move_only_function<_Rp(_Ap...) const && noexcept(_Np)>
    : public ::__ycxx::__adl_free::__fn_base<move_only_function<_Rp(_Ap...) const && noexcept(_Np)>,
                                       ::__ycxx::__detail::__fw::kind::__move_only, ::__ycxx::__detail::__fw::__quals::__crref, _Np, _Rp,
                                       _Ap...> {
  using base = typename move_only_function::__fn_base;

public:
  using base::base;
  using base::operator=;
  _Rp operator()(_Ap... a) const && noexcept(_Np) { return this->__call_(this->__s_, static_cast<_Ap&&>(a)...); }
};

template <class _Rp, class... _Ap, bool _Np>
class copyable_function<_Rp(_Ap...) noexcept(_Np)>
    : public ::__ycxx::__adl_free::__fn_base<copyable_function<_Rp(_Ap...) noexcept(_Np)>, ::__ycxx::__detail::__fw::kind::copyable,
                                       ::__ycxx::__detail::__fw::__quals::none, _Np, _Rp, _Ap...> {
  using base = typename copyable_function::__fn_base;

public:
  using base::base;
  using base::operator=;
  _Rp operator()(_Ap... a) noexcept(_Np) { return this->__call_(this->__s_, static_cast<_Ap&&>(a)...); }
};
template <class _Rp, class... _Ap, bool _Np>
class copyable_function<_Rp(_Ap...) const noexcept(_Np)>
    : public ::__ycxx::__adl_free::__fn_base<copyable_function<_Rp(_Ap...) const noexcept(_Np)>,
                                       ::__ycxx::__detail::__fw::kind::copyable, ::__ycxx::__detail::__fw::__quals::c, _Np, _Rp, _Ap...> {
  using base = typename copyable_function::__fn_base;

public:
  using base::base;
  using base::operator=;
  _Rp operator()(_Ap... a) const noexcept(_Np) { return this->__call_(this->__s_, static_cast<_Ap&&>(a)...); }
};
template <class _Rp, class... _Ap, bool _Np>
class copyable_function<_Rp(_Ap...) & noexcept(_Np)>
    : public ::__ycxx::__adl_free::__fn_base<copyable_function<_Rp(_Ap...) & noexcept(_Np)>, ::__ycxx::__detail::__fw::kind::copyable,
                                       ::__ycxx::__detail::__fw::__quals::__lref, _Np, _Rp, _Ap...> {
  using base = typename copyable_function::__fn_base;

public:
  using base::base;
  using base::operator=;
  _Rp operator()(_Ap... a) & noexcept(_Np) { return this->__call_(this->__s_, static_cast<_Ap&&>(a)...); }
};
template <class _Rp, class... _Ap, bool _Np>
class copyable_function<_Rp(_Ap...) const & noexcept(_Np)>
    : public ::__ycxx::__adl_free::__fn_base<copyable_function<_Rp(_Ap...) const & noexcept(_Np)>,
                                       ::__ycxx::__detail::__fw::kind::copyable, ::__ycxx::__detail::__fw::__quals::__clref, _Np, _Rp, _Ap...> {
  using base = typename copyable_function::__fn_base;

public:
  using base::base;
  using base::operator=;
  _Rp operator()(_Ap... a) const & noexcept(_Np) { return this->__call_(this->__s_, static_cast<_Ap&&>(a)...); }
};
template <class _Rp, class... _Ap, bool _Np>
class copyable_function<_Rp(_Ap...) && noexcept(_Np)>
    : public ::__ycxx::__adl_free::__fn_base<copyable_function<_Rp(_Ap...) && noexcept(_Np)>, ::__ycxx::__detail::__fw::kind::copyable,
                                       ::__ycxx::__detail::__fw::__quals::__rref, _Np, _Rp, _Ap...> {
  using base = typename copyable_function::__fn_base;

public:
  using base::base;
  using base::operator=;
  _Rp operator()(_Ap... a) && noexcept(_Np) { return this->__call_(this->__s_, static_cast<_Ap&&>(a)...); }
};
template <class _Rp, class... _Ap, bool _Np>
class copyable_function<_Rp(_Ap...) const && noexcept(_Np)>
    : public ::__ycxx::__adl_free::__fn_base<copyable_function<_Rp(_Ap...) const && noexcept(_Np)>,
                                       ::__ycxx::__detail::__fw::kind::copyable, ::__ycxx::__detail::__fw::__quals::__crref, _Np, _Rp,
                                       _Ap...> {
  using base = typename copyable_function::__fn_base;

public:
  using base::base;
  using base::operator=;
  _Rp operator()(_Ap... a) const && noexcept(_Np) { return this->__call_(this->__s_, static_cast<_Ap&&>(a)...); }
};

// ---- [func.wrap.ref] ----
template <class _Rp, class... _Ap, bool _Np>
class function_ref<_Rp(_Ap...) noexcept(_Np)>
    : public ::__ycxx::__adl_free::__fref_base<function_ref<_Rp(_Ap...) noexcept(_Np)>, false, _Np, _Rp, _Ap...> {
  using base = typename function_ref::__fref_base;

public:
  using base::base;
  // Declared here, not in the base: a using-declaration would also bring in the base's copy
  // assignment, which competes with this class's for any argument convertible to both.
  template <class _Tp>
    requires(!::__ycxx::__detail::__fw::__fref_from_spec<false, _Np, _Rp(_Ap...), _Tp>) && (!is_pointer_v<_Tp>) &&
            (!::__ycxx::__detail::__fw::__is_constant_wrapper<_Tp>)
  function_ref& operator=(_Tp) = delete;
};
template <class _Rp, class... _Ap, bool _Np>
class function_ref<_Rp(_Ap...) const noexcept(_Np)>
    : public ::__ycxx::__adl_free::__fref_base<function_ref<_Rp(_Ap...) const noexcept(_Np)>, true, _Np, _Rp, _Ap...> {
  using base = typename function_ref::__fref_base;

public:
  using base::base;
  // Declared here, not in the base: a using-declaration would also bring in the base's copy
  // assignment, which competes with this class's for any argument convertible to both.
  template <class _Tp>
    requires(!::__ycxx::__detail::__fw::__fref_from_spec<true, _Np, _Rp(_Ap...), _Tp>) && (!is_pointer_v<_Tp>) &&
            (!::__ycxx::__detail::__fw::__is_constant_wrapper<_Tp>)
  function_ref& operator=(_Tp) = delete;
};

template <class _Fp>
  requires is_function_v<_Fp>
function_ref(_Fp*) -> function_ref<_Fp>;
template <auto c, class _F0>
  requires is_function_v<remove_pointer_t<_F0>>
function_ref(constant_wrapper<c, _F0>) -> function_ref<remove_pointer_t<_F0>>;
template <auto c, class _Fp, class _Tp>
  requires requires { typename ::__ycxx::__detail::__fw::__fref_bound_sig<_Fp, _Tp>::type; }
function_ref(constant_wrapper<c, _Fp>, _Tp&&) -> function_ref<typename ::__ycxx::__detail::__fw::__fref_bound_sig<_Fp, _Tp>::type>;

} // namespace std
