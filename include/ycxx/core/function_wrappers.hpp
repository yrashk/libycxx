// libycxx core: polymorphic function wrappers ([func.wrap]): bad_function_call, function,
// move_only_function, copyable_function and function_ref.
//
// The owning wrappers share one implementation, ycxx::adl_free::fn_base: a three-pointer
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

namespace [[gnu::visibility("hidden")]] std {

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

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
[[noreturn]] [[gnu::cold]] inline void throw_bad_function_call() {
  ::ycxx::detail::raise_with(ycxx_error_bad_function_call, "std::bad_function_call", [] { return std::bad_function_call(); });
}
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::fw {

enum class kind : unsigned char { function, move_only, copyable };
// The cv/ref qualifiers of the call operator. `function` is invoked as FD& from a const call
// operator ([func.wrap.func.inv]).
enum class quals : unsigned char { none, c, lref, clref, rref, crref, function };

// `VT cv ref` (the is-callable-from check) and `VT inv-quals` (the invocation).
template <quals Q, class VT>
struct quals_of;
template <class VT>
struct quals_of<quals::none, VT> {
  using cvref = VT;
  using inv = VT&;
};
template <class VT>
struct quals_of<quals::c, VT> {
  using cvref = const VT;
  using inv = const VT&;
};
template <class VT>
struct quals_of<quals::lref, VT> {
  using cvref = VT&;
  using inv = VT&;
};
template <class VT>
struct quals_of<quals::clref, VT> {
  using cvref = const VT&;
  using inv = const VT&;
};
template <class VT>
struct quals_of<quals::rref, VT> {
  using cvref = VT&&;
  using inv = VT&&;
};
template <class VT>
struct quals_of<quals::crref, VT> {
  using cvref = const VT&&;
  using inv = const VT&&;
};
template <class VT>
struct quals_of<quals::function, VT> {
  using cvref = VT&;
  using inv = VT&;
};

template <bool N, class R, class F, class... A>
consteval bool invocable_r() {
  if constexpr (N)
    return std::is_nothrow_invocable_r_v<R, F, A...>;
  else
    return std::is_invocable_r_v<R, F, A...>;
}

// How an argument crosses the thunk boundary.
template <class A>
using param_t = std::conditional_t<std::is_scalar_v<A>, A, A&&>;

inline constexpr std::size_t small_size = 3 * sizeof(void*);
// fn_base's move assignment relies on no wrapper fitting in the buffer.
union storage {
  void* p;
  alignas(void*) unsigned char buf[small_size];
};

template <class VT>
inline constexpr bool is_small =
    sizeof(VT) <= small_size && alignof(VT) <= alignof(storage) && std::is_nothrow_move_constructible_v<VT>;

template <class VT>
[[gnu::always_inline]] inline VT* target(storage& s) noexcept {
  if constexpr (is_small<VT>)
    return std::launder(reinterpret_cast<VT*>(s.buf));
  else
    return static_cast<VT*>(s.p);
}

struct ops {
  void (*relocate)(storage& dst, storage& src) noexcept; // nullptr: copy the storage bytes
  void (*destroy)(storage&) noexcept;                     // nullptr: nothing to do
  void (*copy)(storage& dst, const storage& src);         // nullptr for move_only_function
  const void* tag;                                        // identifies the target type
  const std::type_info* type;                             // nullptr without RTTI
};

template <class T>
inline constexpr char type_tag = 0;

template <class VT>
struct handler {
  static void relocate(storage& d, storage& s) noexcept {
    VT* src = ::ycxx::detail::fw::target<VT>(s);
    ::new (static_cast<void*>(d.buf)) VT(static_cast<VT&&>(*src));
    src->~VT();
  }
  static void destroy(storage& s) noexcept {
    if constexpr (is_small<VT>)
      ::ycxx::detail::fw::target<VT>(s)->~VT();
    else
      delete ::ycxx::detail::fw::target<VT>(s);
  }
  static void copy(storage& d, const storage& s) {
    const VT& src = *::ycxx::detail::fw::target<VT>(const_cast<storage&>(s));
    if constexpr (is_small<VT>)
      ::new (static_cast<void*>(d.buf)) VT(src);
    else
      d.p = new VT(src);
  }
};

// Each entry names a handler member only when it is needed, so a move-only or immovable target
// never instantiates the copy or relocate code.
template <class VT>
consteval auto relocate_fn() {
  using fn = void (*)(storage&, storage&) noexcept;
  if constexpr (is_small<VT> && !std::is_trivially_copyable_v<VT>)
    return fn(&handler<VT>::relocate);
  else
    return fn(nullptr);
}
template <class VT>
consteval auto destroy_fn() {
  using fn = void (*)(storage&) noexcept;
  if constexpr (is_small<VT> && std::is_trivially_destructible_v<VT>)
    return fn(nullptr);
  else
    return fn(&handler<VT>::destroy);
}
template <class VT, bool Copy>
consteval auto copy_fn() {
  using fn = void (*)(storage&, const storage&);
  if constexpr (Copy)
    return fn(&handler<VT>::copy);
  else
    return fn(nullptr);
}

template <class VT, bool Copy>
inline constexpr ops ops_for = {relocate_fn<VT>(), destroy_fn<VT>(), copy_fn<VT, Copy>(), &type_tag<VT>,
                                ::ycxx::detail::type_id<VT>};

[[gnu::always_inline]] inline void relocate(const ops* op, storage& d, storage& s) noexcept {
  if (op->relocate)
    op->relocate(d, s);
  else
    __builtin_memcpy(&d, &s, sizeof(storage)); // implicitly creates the trivially copyable target
}

template <class VT, class Inv, bool N, class R, class... A>
R call_target(storage& s, param_t<A>... a) noexcept(N) {
  return ::ycxx::detail::invoke_r<R>(static_cast<Inv>(*::ycxx::detail::fw::target<VT>(s)),
                                     static_cast<param_t<A>&&>(a)...);
}
template <bool N, class R, class... A>
[[noreturn]] R call_empty(storage&, param_t<A>...) noexcept(N) {
  ::ycxx::detail::assertion_failed("std::move_only_function/copyable_function: called with no target");
}
template <class R, class... A>
[[noreturn]] R call_empty_function(storage&, param_t<A>...) {
  ::ycxx::detail::throw_bad_function_call();
}

struct empty_function_target {};

template <class T>
inline constexpr bool is_in_place_type = false;
template <class T>
inline constexpr bool is_in_place_type<std::in_place_type_t<T>> = true;

template <class T>
inline constexpr bool is_function_spec = false;
template <class S>
inline constexpr bool is_function_spec<std::function<S>> = true;
template <class T>
inline constexpr bool is_move_only_spec = false;
template <class... S>
inline constexpr bool is_move_only_spec<std::move_only_function<S...>> = true;
template <class T>
inline constexpr bool is_copyable_spec = false;
template <class... S>
inline constexpr bool is_copyable_spec<std::copyable_function<S...>> = true;

// Sources whose emptiness carries over ([func.wrap.func.con]/12.3, [func.wrap.move.ctor]/8.3,
// [func.wrap.copy.ctor]/10.3).
template <kind K, class T>
inline constexpr bool empty_carries =
    K == kind::function    ? is_function_spec<T>
    : K == kind::copyable  ? is_copyable_spec<T>
                           : is_move_only_spec<T> || is_copyable_spec<T>;

template <class VT>
inline constexpr bool is_nullable_pointer =
    (std::is_pointer_v<VT> && std::is_function_v<std::remove_pointer_t<VT>>) || std::is_member_pointer_v<VT>;

// ---- deduction-guide support ----
// R(G::*)(A...) cv &opt noexcept(E) -> R(A...) noexcept(E) ([func.wrap.func.con]/16.1,
// [func.wrap.ref.deduct]/5.1).
template <class M>
struct memfn_sig {};
template <class R, class G, class... A, bool E>
struct memfn_sig<R (G::*)(A...) noexcept(E)> {
  using type = R(A...) noexcept(E);
  using plain = R(A...);
};
template <class R, class G, class... A, bool E>
struct memfn_sig<R (G::*)(A...) const noexcept(E)> : memfn_sig<R (G::*)(A...) noexcept(E)> {};
template <class R, class G, class... A, bool E>
struct memfn_sig<R (G::*)(A...) volatile noexcept(E)> : memfn_sig<R (G::*)(A...) noexcept(E)> {};
template <class R, class G, class... A, bool E>
struct memfn_sig<R (G::*)(A...) const volatile noexcept(E)> : memfn_sig<R (G::*)(A...) noexcept(E)> {};
template <class R, class G, class... A, bool E>
struct memfn_sig<R (G::*)(A...) & noexcept(E)> : memfn_sig<R (G::*)(A...) noexcept(E)> {};
template <class R, class G, class... A, bool E>
struct memfn_sig<R (G::*)(A...) const & noexcept(E)> : memfn_sig<R (G::*)(A...) noexcept(E)> {};
template <class R, class G, class... A, bool E>
struct memfn_sig<R (G::*)(A...) volatile & noexcept(E)> : memfn_sig<R (G::*)(A...) noexcept(E)> {};
template <class R, class G, class... A, bool E>
struct memfn_sig<R (G::*)(A...) const volatile & noexcept(E)> : memfn_sig<R (G::*)(A...) noexcept(E)> {};

// A function pointer from &F::operator() comes from an explicit-object member function or a
// static one ([func.wrap.func.con]/16). A static operator() accepts every parameter as an
// argument; an explicit-object one takes its first parameter from the object expression, so
// called with all of them it has one argument too many.
template <class F, class R, class... A>
struct fnptr_sig {};
template <class F, class R, class... A>
  requires requires { std::declval<F&>().operator()(std::declval<A>()...); }
struct fnptr_sig<F, R, A...> {
  using plain = R(A...);
};
template <class F, class R, class G, class... A>
  requires(!requires { std::declval<F&>().operator()(std::declval<G>(), std::declval<A>()...); })
struct fnptr_sig<F, R, G, A...> {
  using plain = R(A...);
};

template <class F, class M>
struct call_op_sig {};
template <class F, class M>
  requires requires { typename memfn_sig<M>::plain; }
struct call_op_sig<F, M> {
  using type = typename memfn_sig<M>::plain;
};
template <class F, class R, class... A, bool E>
  requires requires { typename fnptr_sig<F, R, A...>::plain; }
struct call_op_sig<F, R (*)(A...) noexcept(E)> {
  using type = typename fnptr_sig<F, R, A...>::plain;
};

template <class F>
struct function_guide {};
template <class F>
  requires requires { &F::operator(); }
struct function_guide<F> : call_op_sig<F, decltype(&F::operator())> {};

// [func.wrap.ref.deduct]/5.
template <class F, class T>
struct fref_bound_sig {};
template <class F, class T>
  requires std::is_member_function_pointer_v<F> && requires { typename memfn_sig<F>::type; }
struct fref_bound_sig<F, T> {
  using type = typename memfn_sig<F>::type;
};
template <class M, class G, class T>
  requires std::is_object_v<M> && requires { typename std::invoke_result<M G::*, T&>::type; }
struct fref_bound_sig<M G::*, T> {
  using type = std::invoke_result_t<M G::*, T&>() noexcept;
};
template <class R, class G, class... A, bool E, class T>
struct fref_bound_sig<R (*)(G, A...) noexcept(E), T> {
  using type = R(A...) noexcept(E);
};

// ---- function_ref support ----
union bound_entity {
  const volatile void* obj;
  void (*fn)();
};

// is-convertible-from-specialization<F> for function_ref<R(A...) cv noexcept(N)>, where C is cv.
template <bool C, bool N, class Sig, class F>
inline constexpr bool fref_from_spec = false;
template <bool C, bool N, class R, class... A, bool N2>
inline constexpr bool fref_from_spec<C, N, R(A...), std::function_ref<R(A...) noexcept(N2)>> = (N2 || !N) && !C;
template <bool C, bool N, class R, class... A, bool N2>
inline constexpr bool fref_from_spec<C, N, R(A...), std::function_ref<R(A...) const noexcept(N2)>> = N2 || !N;

template <class T>
inline constexpr bool is_constant_wrapper = false;
template <auto X, class T>
inline constexpr bool is_constant_wrapper<std::constant_wrapper<X, T>> = true;

}} // namespace ycxx::detail::fw

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

// The owning wrappers. Self is the derived std:: class.
template <class Self, ::ycxx::detail::fw::kind K, ::ycxx::detail::fw::quals Q, bool N, class R, class... A>
class fn_base {
  template <class, ::ycxx::detail::fw::kind, ::ycxx::detail::fw::quals, bool, class, class...>
  friend class fn_base;
  using storage = ::ycxx::detail::fw::storage;
  using ops = ::ycxx::detail::fw::ops;
  using kind = ::ycxx::detail::fw::kind;
  using thunk_t = R (*)(storage&, ::ycxx::detail::fw::param_t<A>...) noexcept(N);

  static constexpr bool copyable = K != kind::move_only;

  static constexpr thunk_t empty_thunk() noexcept {
    if constexpr (K == kind::function)
      return &::ycxx::detail::fw::call_empty_function<R, A...>;
    else
      return &::ycxx::detail::fw::call_empty<N, R, A...>;
  }

  // is-callable-from<VT>
  template <class VT>
  static consteval bool callable_from() {
    using q = ::ycxx::detail::fw::quals_of<Q, VT>;
    return ::ycxx::detail::fw::invocable_r<N, R, typename q::cvref, A...>() &&
           ::ycxx::detail::fw::invocable_r<N, R, typename q::inv, A...>();
  }

  Self& self() noexcept { return static_cast<Self&>(*this); }

  // Another owning wrapper with the same R and argument passing ([func.wrap.general]/3: avoid double wrapping).
  // Its target, invoked through its own thunk, is what invoking it would do; is-callable-from
  // has already checked that our qualifiers may call it.
  static_assert(sizeof(storage) + 2 * sizeof(void*) > ::ycxx::detail::fw::small_size);

  // The thunks need only agree on how arguments travel: T and T&& both pass a class-type
  // argument as T&&, and [func.wrap.general]/2 lets the inner invocation alias it.
  template <class... A2>
  static constexpr bool same_thunk_args =
      std::is_same_v<void(::ycxx::detail::fw::param_t<A>...), void(::ycxx::detail::fw::param_t<A2>...)>;
  template <class S2, kind K2, ::ycxx::detail::fw::quals Q2, bool N2, class... A2>
    requires same_thunk_args<A2...>
  static S2* self_of(fn_base<S2, K2, Q2, N2, R, A2...>*);
  template <class S2, kind K2, ::ycxx::detail::fw::quals Q2, bool N2, class... A2>
    requires same_thunk_args<A2...>
  static fn_base<S2, K2, Q2, N2, R, A2...>* base_of(fn_base<S2, K2, Q2, N2, R, A2...>* p) noexcept {
    return p;
  }
  // Strengthened noexcept ([res.on.exception.handling]/5): nothing can throw when the target is
  // stored in place and constructed without throwing, or when another wrapper's target is taken.
  template <class VT, class... Args>
  static consteval bool ctor_noexcept() {
    if constexpr (sizeof...(Args) == 1 && adoptable<VT>())
      return (... && (std::is_rvalue_reference_v<Args&&> && !std::is_const_v<std::remove_reference_t<Args>>));
    else
      return ::ycxx::detail::fw::is_small<VT> && std::is_nothrow_constructible_v<VT, Args...>;
  }

  // Not for std::function: its target_type() and target() expose the target's type, which must
  // be the source wrapper ([func.wrap.func.con]/13, [func.wrap.func.targ]).
  template <class Src>
  static consteval bool adoptable() {
    if constexpr (K == kind::function)
      return false;
    else if constexpr (requires(Src* p) { fn_base::self_of(p); })
      return std::is_same_v<decltype(fn_base::self_of(static_cast<Src*>(nullptr))), Src*>;
    else
      return false;
  }

  template <class VT, class... Args>
  void emplace(Args&&... args) {
    if constexpr (::ycxx::detail::fw::is_small<VT>)
      ::new (static_cast<void*>(s_.buf)) VT(static_cast<Args&&>(args)...);
    else
      s_.p = new VT(static_cast<Args&&>(args)...);
    ops_ = &::ycxx::detail::fw::ops_for<VT, copyable>;
    call_ = &::ycxx::detail::fw::call_target<VT, typename ::ycxx::detail::fw::quals_of<Q, VT>::inv, N, R, A...>;
  }

  void take(fn_base& o) noexcept {
    if (o.ops_) {
      ::ycxx::detail::fw::relocate(o.ops_, s_, o.s_);
      ops_ = o.ops_;
      call_ = o.call_;
      o.ops_ = nullptr;
      o.call_ = empty_thunk();
    }
  }

  void reset() noexcept {
    if (ops_) {
      if (ops_->destroy)
        ops_->destroy(s_);
      ops_ = nullptr;
      call_ = empty_thunk();
    }
  }

  void swap_impl(fn_base& o) noexcept {
    if (this == __builtin_addressof(o))
      return;
    storage tmp;
    if (o.ops_)
      ::ycxx::detail::fw::relocate(o.ops_, tmp, o.s_);
    if (ops_)
      ::ycxx::detail::fw::relocate(ops_, o.s_, s_);
    if (o.ops_)
      ::ycxx::detail::fw::relocate(o.ops_, s_, tmp);
    const ops* op = ops_;
    ops_ = o.ops_;
    o.ops_ = op;
    thunk_t c = call_;
    call_ = o.call_;
    o.call_ = c;
  }

  template <class T>
  bool holds() const noexcept {
    if (!ops_)
      return false;
    if (ops_->tag == &::ycxx::detail::fw::type_tag<T>)
      return true;
    // Two copies of one table can exist across shared libraries.
    if constexpr (::ycxx::detail::cfg::rtti)
      return *ops_->type == *::ycxx::detail::type_id<T>;
    else
      return false;
  }

protected:
  // The buffer is not at offset 0: an empty target there could share its address with another
  // object of its type, such as an empty base of a class that has this wrapper as a member.
  thunk_t call_ = empty_thunk();
  const ops* ops_ = nullptr;
  mutable storage s_;

public:
  using result_type = R;

  // User-provided, as the wrappers' default constructors are in the draft, so `const function<F>
  // f;` is valid ([dcl.init.general]/8: s_ has no default member initializer).
  fn_base() noexcept {}
  fn_base(std::nullptr_t) noexcept {}
  fn_base(fn_base&& o) noexcept { take(o); }
  fn_base(const fn_base& o)
    requires copyable
  {
    if (o.ops_) {
      o.ops_->copy(s_, o.s_);
      ops_ = o.ops_;
      call_ = o.call_;
    }
  }

  template <class F, class VT = std::decay_t<F>>
    requires(!std::is_same_v<std::remove_cvref_t<F>, Self>) && (!std::is_same_v<std::remove_cvref_t<F>, fn_base>) &&
            (!::ycxx::detail::fw::is_in_place_type<std::remove_cvref_t<F>>) && (callable_from<VT>())
  fn_base(F&& f) noexcept(ctor_noexcept<VT, F>()) {
    static_assert(std::is_constructible_v<VT, F>, "std::function/move_only_function/copyable_function: Mandates: is_constructible_v<VT, F>");
    if constexpr (copyable)
      static_assert(std::is_copy_constructible_v<VT>, "std::function/move_only_function/copyable_function: Mandates: VT is copy constructible");
    if constexpr (std::is_constructible_v<VT, F> && (!copyable || std::is_copy_constructible_v<VT>)) {
      if constexpr (::ycxx::detail::fw::is_nullable_pointer<VT>) {
        if (f == nullptr)
          return;
      } else if constexpr (adoptable<VT>()) {
        auto* src = fn_base::base_of(const_cast<VT*>(__builtin_addressof(f)));
        if (src->ops_) {
          if constexpr (std::is_rvalue_reference_v<F&&> && !std::is_const_v<std::remove_reference_t<F>>)
            ::ycxx::detail::fw::relocate(src->ops_, s_, src->s_);
          else
            src->ops_->copy(s_, src->s_);
          ops_ = src->ops_;
          call_ = src->call_;
          if constexpr (std::is_rvalue_reference_v<F&&> && !std::is_const_v<std::remove_reference_t<F>>) {
            src->ops_ = nullptr;
            src->call_ = src->empty_thunk();
          }
          return;
        }
        if constexpr (::ycxx::detail::fw::empty_carries<K, VT>) {
          return;
        } else {
          // An empty std::function becomes a target of a move_only_function or
          // copyable_function: a stateless stand-in that throws as invoking it would.
          ops_ = &::ycxx::detail::fw::ops_for<::ycxx::detail::fw::empty_function_target, copyable>;
          call_ = &::ycxx::detail::fw::call_empty_function<R, A...>;
          return;
        }
      } else if constexpr (::ycxx::detail::fw::empty_carries<K, VT>) {
        if (!static_cast<bool>(f))
          return;
      }
      emplace<VT>(static_cast<F&&>(f));
    }
  }

  template <class T, class... Args, class VT = std::decay_t<T>>
    requires(K != kind::function) && std::is_constructible_v<VT, Args...> && (callable_from<VT>())
  explicit fn_base(std::in_place_type_t<T>, Args&&... args) noexcept(ctor_noexcept<VT, Args...>()) {
    static_assert(std::is_same_v<VT, T>, "std::function/move_only_function/copyable_function: Mandates: VT is the same type as T");
    if constexpr (copyable)
      static_assert(std::is_copy_constructible_v<VT>, "std::function/move_only_function/copyable_function: Mandates: VT is copy constructible");
    if constexpr (std::is_same_v<VT, T> && (!copyable || std::is_copy_constructible_v<VT>))
      emplace<VT>(static_cast<Args&&>(args)...);
  }
  template <class T, class U, class... Args, class VT = std::decay_t<T>>
    requires(K != kind::function) && std::is_constructible_v<VT, std::initializer_list<U>&, Args...> &&
            (callable_from<VT>())
  explicit fn_base(std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args) {
    static_assert(std::is_same_v<VT, T>, "std::function/move_only_function/copyable_function: Mandates: VT is the same type as T");
    if constexpr (copyable)
      static_assert(std::is_copy_constructible_v<VT>, "std::function/move_only_function/copyable_function: Mandates: VT is copy constructible");
    if constexpr (std::is_same_v<VT, T> && (!copyable || std::is_copy_constructible_v<VT>))
      emplace<VT>(il, static_cast<Args&&>(args)...);
  }

  // "Equivalent to: W(std::move(f)).swap(*this)": the old target is destroyed only after the
  // source's has been taken, since the source may live inside it. The old target is first moved
  // aside; one that can contain a wrapper (at least 40 bytes) is never in the 24-byte buffer, so
  // that move leaves it, and the source inside it, where they are. The source's target is
  // relocated once.
  fn_base& operator=(fn_base&& o) noexcept {
    if (this != __builtin_addressof(o)) {
      storage old;
      const ops* old_ops = ops_;
      if (old_ops)
        ::ycxx::detail::fw::relocate(old_ops, old, s_);
      ops_ = nullptr;
      call_ = empty_thunk();
      take(o);
      if (old_ops && old_ops->destroy)
        old_ops->destroy(old);
    }
    return *this;
  }
  // "Equivalent to: W(f).swap(*this)": self-assignment copies too. Releasing the old target and
  // taking the copy's is that swap with one relocation fewer.
  fn_base& operator=(const fn_base& o)
    requires copyable
  {
    fn_base tmp(o);
    reset();
    take(tmp);
    return *this;
  }
  Self& operator=(std::nullptr_t) noexcept {
    reset();
    return self();
  }
  template <class F>
    requires(!std::is_same_v<std::remove_cvref_t<F>, Self>) && (!std::is_same_v<std::remove_cvref_t<F>, fn_base>) &&
            (K == kind::function ? std::is_invocable_r_v<R, std::decay_t<F>&, A...>
                                 : std::is_constructible_v<Self, F>)
  Self& operator=(F&& f) {
    Self tmp(static_cast<F&&>(f));
    reset();
    take(tmp);
    return self();
  }

  ~fn_base() {
    if (ops_ && ops_->destroy)
      ops_->destroy(s_);
  }

  void swap(Self& other) noexcept { swap_impl(other); }
  explicit operator bool() const noexcept { return ops_ != nullptr; }

  // ---- [func.wrap.func.targ] ----
  const std::type_info& target_type() const noexcept
    requires(K == kind::function) && ::ycxx::detail::cfg::rtti
  {
    return ops_ ? *ops_->type : *::ycxx::detail::type_id<void>;
  }
  template <class T>
    requires(K == kind::function)
  T* target() noexcept {
    using U = std::remove_cv_t<T>;
    if constexpr (std::is_object_v<U> && !std::is_array_v<U>) {
      if (holds<U>())
        return ::ycxx::detail::fw::target<U>(s_);
    }
    return nullptr;
  }
  template <class T>
    requires(K == kind::function)
  const T* target() const noexcept {
    return const_cast<fn_base*>(this)->template target<T>();
  }

  friend void swap(Self& a, Self& b) noexcept { a.swap(b); }
  friend bool operator==(const Self& f, std::nullptr_t) noexcept { return !f; }
};

// function_ref. Self is the derived std:: class; C is the cv placeholder.
template <class Self, bool C, bool N, class R, class... A>
class fref_base {
  template <class, bool, bool, class, class...>
  friend class fref_base;

  using bound_entity = ::ycxx::detail::fw::bound_entity;
  using thunk_t = R (*)(bound_entity, ::ycxx::detail::fw::param_t<A>...) noexcept(N);
  template <class T>
  using cv = std::conditional_t<C, const T, T>;

  template <class... T>
  static consteval bool invocable_using() {
    return ::ycxx::detail::fw::invocable_r<N, R, T..., A...>();
  }

  template <class F>
  static R fn_thunk(bound_entity be, ::ycxx::detail::fw::param_t<A>... a) noexcept(N) {
    return ::ycxx::detail::invoke_r<R>(reinterpret_cast<F*>(be.fn), static_cast<::ycxx::detail::fw::param_t<A>&&>(a)...);
  }
  template <class T> // T is cv-qualified as stored
  static R obj_thunk(bound_entity be, ::ycxx::detail::fw::param_t<A>... a) noexcept(N) {
    return ::ycxx::detail::invoke_r<R>(*static_cast<T*>(const_cast<void*>(be.obj)),
                                       static_cast<::ycxx::detail::fw::param_t<A>&&>(a)...);
  }
  template <class CW>
  static R cw_thunk(bound_entity, ::ycxx::detail::fw::param_t<A>... a) noexcept(N) {
    return ::ycxx::detail::invoke_r<R>(CW::value, static_cast<::ycxx::detail::fw::param_t<A>&&>(a)...);
  }
  template <class CW, class T>
  static R cw_obj_thunk(bound_entity be, ::ycxx::detail::fw::param_t<A>... a) noexcept(N) {
    T* p;
    if constexpr (std::is_function_v<T>)
      p = reinterpret_cast<T*>(be.fn);
    else
      p = static_cast<T*>(const_cast<void*>(be.obj));
    return ::ycxx::detail::invoke_r<R>(CW::value, *p, static_cast<::ycxx::detail::fw::param_t<A>&&>(a)...);
  }
  template <class CW, class P>
  static R cw_ptr_thunk(bound_entity be, ::ycxx::detail::fw::param_t<A>... a) noexcept(N) {
    P p;
    if constexpr (std::is_function_v<std::remove_pointer_t<P>>)
      p = reinterpret_cast<P>(be.fn);
    else
      p = static_cast<P>(const_cast<void*>(be.obj));
    return ::ycxx::detail::invoke_r<R>(CW::value, p, static_cast<::ycxx::detail::fw::param_t<A>&&>(a)...);
  }

  template <class CW>
  static consteval void check_cw_not_null() {
    using F = typename CW::value_type;
    if constexpr (std::is_pointer_v<F> || std::is_member_pointer_v<F>)
      static_assert(CW::value != nullptr, "std::function_ref: Mandates: f.value != nullptr");
  }

  bound_entity be_;
  thunk_t thunk_;

public:
  // ---- [func.wrap.ref.ctor] ----
  template <class F>
    requires std::is_function_v<F> && (invocable_using<F>())
  fref_base(F* f) noexcept {
    ::ycxx::detail::precondition(f != nullptr, "std::function_ref: null function pointer");
    be_.fn = reinterpret_cast<void (*)()>(f);
    thunk_ = &fn_thunk<F>;
  }

  template <class F, class T = std::remove_reference_t<F>>
    requires(!std::is_same_v<std::remove_cvref_t<F>, Self>) && (!std::is_same_v<std::remove_cvref_t<F>, fref_base>) &&
            (!std::is_member_pointer_v<T>) &&
            (invocable_using<cv<T>&>())
  constexpr fref_base(F&& f) noexcept {
    if constexpr (::ycxx::detail::fw::fref_from_spec<C, N, R(A...), std::remove_cv_t<T>>) {
      be_ = f.be_;
      thunk_ = f.thunk_;
    } else {
      // (A function lvalue picks the F* constructor by partial ordering.)
      be_.obj = __builtin_addressof(f);
      thunk_ = &obj_thunk<cv<T>>;
    }
  }

  template <auto c, class F>
    requires(invocable_using<const F&>())
  constexpr fref_base(std::constant_wrapper<c, F>) noexcept {
    using CW = std::constant_wrapper<c, F>;
    check_cw_not_null<CW>();
    if constexpr (sizeof...(A) != 0)
      static_assert(!::ycxx::detail::cw_constant_call<CW, A...>,
                    "std::function_ref: Mandates: the call does not produce a constant_wrapper");
    be_.obj = nullptr;
    thunk_ = &cw_thunk<CW>;
  }

  template <auto c, class F, class U, class T = std::remove_reference_t<U>>
    requires(!std::is_rvalue_reference_v<U &&>) && (invocable_using<const F&, cv<T>&>())
  constexpr fref_base(std::constant_wrapper<c, F>, U&& obj) noexcept {
    using CW = std::constant_wrapper<c, F>;
    check_cw_not_null<CW>();
    if constexpr (std::is_function_v<T>)
      be_.fn = reinterpret_cast<void (*)()>(&obj);
    else
      be_.obj = __builtin_addressof(obj);
    thunk_ = &cw_obj_thunk<CW, cv<T>>;
  }

  template <auto c, class F, class T>
    requires(!C) && (invocable_using<const F&, T*>())
  constexpr fref_base(std::constant_wrapper<c, F>, T* obj) noexcept {
    using CW = std::constant_wrapper<c, F>;
    check_cw_not_null<CW>();
    if constexpr (std::is_member_pointer_v<F>)
      ::ycxx::detail::precondition(obj != nullptr, "std::function_ref: null object pointer");
    if constexpr (std::is_function_v<T>)
      be_.fn = reinterpret_cast<void (*)()>(obj);
    else
      be_.obj = obj;
    thunk_ = &cw_ptr_thunk<CW, T*>;
  }
  template <auto c, class F, class T>
    requires C && (invocable_using<const F&, const T*>())
  constexpr fref_base(std::constant_wrapper<c, F>, const T* obj) noexcept {
    using CW = std::constant_wrapper<c, F>;
    check_cw_not_null<CW>();
    if constexpr (std::is_member_pointer_v<F>)
      ::ycxx::detail::precondition(obj != nullptr, "std::function_ref: null object pointer");
    if constexpr (std::is_function_v<T>)
      be_.fn = reinterpret_cast<void (*)()>(obj);
    else
      be_.obj = obj;
    thunk_ = &cw_ptr_thunk<CW, const T*>;
  }

  constexpr fref_base(const fref_base&) noexcept = default;
  constexpr fref_base& operator=(const fref_base&) noexcept = default;

  // ---- [func.wrap.ref.inv] ----
  R operator()(A... a) const noexcept(N) { return thunk_(be_, static_cast<A&&>(a)...); }
};

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {

// ---- [func.wrap.func] ----
template <class R, class... A>
class function<R(A...)>
    : public ::ycxx::adl_free::fn_base<function<R(A...)>, ::ycxx::detail::fw::kind::function,
                                       ::ycxx::detail::fw::quals::function, false, R, A...> {
  using base = ::ycxx::adl_free::fn_base<function, ::ycxx::detail::fw::kind::function,
                                         ::ycxx::detail::fw::quals::function, false, R, A...>;

public:
  using base::base;
  using base::operator=;

  template <class F>
  function& operator=(reference_wrapper<F> f) noexcept {
    function(f).swap(*this);
    return *this;
  }

  R operator()(A... a) const { return this->call_(this->s_, static_cast<A&&>(a)...); }
};

template <class R, class... A>
function(R (*)(A...)) -> function<R(A...)>;
template <class F>
  requires requires { typename ::ycxx::detail::fw::function_guide<F>::type; }
function(F) -> function<typename ::ycxx::detail::fw::function_guide<F>::type>;

// ---- [func.wrap.move] / [func.wrap.copy] ----
// One partial specialization per cv/ref combination; noex is deduced.
template <class R, class... A, bool N>
class move_only_function<R(A...) noexcept(N)>
    : public ::ycxx::adl_free::fn_base<move_only_function<R(A...) noexcept(N)>, ::ycxx::detail::fw::kind::move_only,
                                       ::ycxx::detail::fw::quals::none, N, R, A...> {
  using base = typename move_only_function::fn_base;

public:
  using base::base;
  using base::operator=;
  R operator()(A... a) noexcept(N) { return this->call_(this->s_, static_cast<A&&>(a)...); }
};
template <class R, class... A, bool N>
class move_only_function<R(A...) const noexcept(N)>
    : public ::ycxx::adl_free::fn_base<move_only_function<R(A...) const noexcept(N)>,
                                       ::ycxx::detail::fw::kind::move_only, ::ycxx::detail::fw::quals::c, N, R, A...> {
  using base = typename move_only_function::fn_base;

public:
  using base::base;
  using base::operator=;
  R operator()(A... a) const noexcept(N) { return this->call_(this->s_, static_cast<A&&>(a)...); }
};
template <class R, class... A, bool N>
class move_only_function<R(A...) & noexcept(N)>
    : public ::ycxx::adl_free::fn_base<move_only_function<R(A...) & noexcept(N)>, ::ycxx::detail::fw::kind::move_only,
                                       ::ycxx::detail::fw::quals::lref, N, R, A...> {
  using base = typename move_only_function::fn_base;

public:
  using base::base;
  using base::operator=;
  R operator()(A... a) & noexcept(N) { return this->call_(this->s_, static_cast<A&&>(a)...); }
};
template <class R, class... A, bool N>
class move_only_function<R(A...) const & noexcept(N)>
    : public ::ycxx::adl_free::fn_base<move_only_function<R(A...) const & noexcept(N)>,
                                       ::ycxx::detail::fw::kind::move_only, ::ycxx::detail::fw::quals::clref, N, R,
                                       A...> {
  using base = typename move_only_function::fn_base;

public:
  using base::base;
  using base::operator=;
  R operator()(A... a) const & noexcept(N) { return this->call_(this->s_, static_cast<A&&>(a)...); }
};
template <class R, class... A, bool N>
class move_only_function<R(A...) && noexcept(N)>
    : public ::ycxx::adl_free::fn_base<move_only_function<R(A...) && noexcept(N)>,
                                       ::ycxx::detail::fw::kind::move_only, ::ycxx::detail::fw::quals::rref, N, R, A...> {
  using base = typename move_only_function::fn_base;

public:
  using base::base;
  using base::operator=;
  R operator()(A... a) && noexcept(N) { return this->call_(this->s_, static_cast<A&&>(a)...); }
};
template <class R, class... A, bool N>
class move_only_function<R(A...) const && noexcept(N)>
    : public ::ycxx::adl_free::fn_base<move_only_function<R(A...) const && noexcept(N)>,
                                       ::ycxx::detail::fw::kind::move_only, ::ycxx::detail::fw::quals::crref, N, R,
                                       A...> {
  using base = typename move_only_function::fn_base;

public:
  using base::base;
  using base::operator=;
  R operator()(A... a) const && noexcept(N) { return this->call_(this->s_, static_cast<A&&>(a)...); }
};

template <class R, class... A, bool N>
class copyable_function<R(A...) noexcept(N)>
    : public ::ycxx::adl_free::fn_base<copyable_function<R(A...) noexcept(N)>, ::ycxx::detail::fw::kind::copyable,
                                       ::ycxx::detail::fw::quals::none, N, R, A...> {
  using base = typename copyable_function::fn_base;

public:
  using base::base;
  using base::operator=;
  R operator()(A... a) noexcept(N) { return this->call_(this->s_, static_cast<A&&>(a)...); }
};
template <class R, class... A, bool N>
class copyable_function<R(A...) const noexcept(N)>
    : public ::ycxx::adl_free::fn_base<copyable_function<R(A...) const noexcept(N)>,
                                       ::ycxx::detail::fw::kind::copyable, ::ycxx::detail::fw::quals::c, N, R, A...> {
  using base = typename copyable_function::fn_base;

public:
  using base::base;
  using base::operator=;
  R operator()(A... a) const noexcept(N) { return this->call_(this->s_, static_cast<A&&>(a)...); }
};
template <class R, class... A, bool N>
class copyable_function<R(A...) & noexcept(N)>
    : public ::ycxx::adl_free::fn_base<copyable_function<R(A...) & noexcept(N)>, ::ycxx::detail::fw::kind::copyable,
                                       ::ycxx::detail::fw::quals::lref, N, R, A...> {
  using base = typename copyable_function::fn_base;

public:
  using base::base;
  using base::operator=;
  R operator()(A... a) & noexcept(N) { return this->call_(this->s_, static_cast<A&&>(a)...); }
};
template <class R, class... A, bool N>
class copyable_function<R(A...) const & noexcept(N)>
    : public ::ycxx::adl_free::fn_base<copyable_function<R(A...) const & noexcept(N)>,
                                       ::ycxx::detail::fw::kind::copyable, ::ycxx::detail::fw::quals::clref, N, R, A...> {
  using base = typename copyable_function::fn_base;

public:
  using base::base;
  using base::operator=;
  R operator()(A... a) const & noexcept(N) { return this->call_(this->s_, static_cast<A&&>(a)...); }
};
template <class R, class... A, bool N>
class copyable_function<R(A...) && noexcept(N)>
    : public ::ycxx::adl_free::fn_base<copyable_function<R(A...) && noexcept(N)>, ::ycxx::detail::fw::kind::copyable,
                                       ::ycxx::detail::fw::quals::rref, N, R, A...> {
  using base = typename copyable_function::fn_base;

public:
  using base::base;
  using base::operator=;
  R operator()(A... a) && noexcept(N) { return this->call_(this->s_, static_cast<A&&>(a)...); }
};
template <class R, class... A, bool N>
class copyable_function<R(A...) const && noexcept(N)>
    : public ::ycxx::adl_free::fn_base<copyable_function<R(A...) const && noexcept(N)>,
                                       ::ycxx::detail::fw::kind::copyable, ::ycxx::detail::fw::quals::crref, N, R,
                                       A...> {
  using base = typename copyable_function::fn_base;

public:
  using base::base;
  using base::operator=;
  R operator()(A... a) const && noexcept(N) { return this->call_(this->s_, static_cast<A&&>(a)...); }
};

// ---- [func.wrap.ref] ----
template <class R, class... A, bool N>
class function_ref<R(A...) noexcept(N)>
    : public ::ycxx::adl_free::fref_base<function_ref<R(A...) noexcept(N)>, false, N, R, A...> {
  using base = typename function_ref::fref_base;

public:
  using base::base;
  // Declared here, not in the base: a using-declaration would also bring in the base's copy
  // assignment, which competes with this class's for any argument convertible to both.
  template <class T>
    requires(!::ycxx::detail::fw::fref_from_spec<false, N, R(A...), T>) && (!is_pointer_v<T>) &&
            (!::ycxx::detail::fw::is_constant_wrapper<T>)
  function_ref& operator=(T) = delete;
};
template <class R, class... A, bool N>
class function_ref<R(A...) const noexcept(N)>
    : public ::ycxx::adl_free::fref_base<function_ref<R(A...) const noexcept(N)>, true, N, R, A...> {
  using base = typename function_ref::fref_base;

public:
  using base::base;
  // Declared here, not in the base: a using-declaration would also bring in the base's copy
  // assignment, which competes with this class's for any argument convertible to both.
  template <class T>
    requires(!::ycxx::detail::fw::fref_from_spec<true, N, R(A...), T>) && (!is_pointer_v<T>) &&
            (!::ycxx::detail::fw::is_constant_wrapper<T>)
  function_ref& operator=(T) = delete;
};

template <class F>
  requires is_function_v<F>
function_ref(F*) -> function_ref<F>;
template <auto c, class F0>
  requires is_function_v<remove_pointer_t<F0>>
function_ref(constant_wrapper<c, F0>) -> function_ref<remove_pointer_t<F0>>;
template <auto c, class F, class T>
  requires requires { typename ::ycxx::detail::fw::fref_bound_sig<F, T>::type; }
function_ref(constant_wrapper<c, F>, T&&) -> function_ref<typename ::ycxx::detail::fw::fref_bound_sig<F, T>::type>;

} // namespace std
