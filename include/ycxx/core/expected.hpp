// libycxx core: <expected> ([expected]).
//
// Storage: a union of the value (or an empty stand-in for void) and the error, plus a flag.
// Special members are conditionally trivial through constrained defaulted overloads, with
// explicitly deleted twins where the draft says "defined as deleted unless" (an overload set
// with no viable member would make Clang treat the class as not trivially copyable).
#pragma once

#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/utility_base.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/invoke.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/exception_base.hpp>
#include <initializer_list>

namespace [[__gnu__::__visibility__("hidden")]] std {

// ---- [expected.unexpected] ----
template <class _Ep>
class unexpected;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp>
inline constexpr bool __is_unexpected = false;
template <class _Ep>
inline constexpr bool __is_unexpected<std::unexpected<_Ep>> = true;

// [expected.un.general]/2: a valid template argument for unexpected.
template <class _Ep>
concept __valid_unexpected_arg =
    std::is_object_v<_Ep> && !std::is_array_v<_Ep> && !__is_unexpected<_Ep> && std::is_same_v<_Ep, std::remove_cv_t<_Ep>>;

// "a == b is well-formed and its result is convertible to bool": implicit conversion only
// (LWG4366), so results are converted by initialization, never static_cast.
template <class _Ap, class _Bp>
concept __eq_to_bool = requires(const _Ap& a, const _Bp& b) { requires std::is_convertible_v<decltype(a == b), bool>; };
constexpr bool __implicit_bool(bool b) noexcept { return b; }
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Ep>
class unexpected {
  static_assert(__ycxx::__detail::__valid_unexpected_arg<_Ep>,
                "std::unexpected: E must be a non-array, non-cv object type that is not a specialization of unexpected");
  _Ep __unex_;

  template <class>
  friend class unexpected;

public:
  constexpr unexpected(const unexpected&) = default;
  constexpr unexpected(unexpected&&) = default;
  template <class _Err = _Ep>
    requires(!is_same_v<remove_cvref_t<_Err>, unexpected>) && (!is_same_v<remove_cvref_t<_Err>, in_place_t>) &&
            is_constructible_v<_Ep, _Err>
  constexpr explicit unexpected(_Err&& e) : __unex_(static_cast<_Err&&>(e)) {}
  template <class... _Args>
    requires is_constructible_v<_Ep, _Args...>
  constexpr explicit unexpected(in_place_t, _Args&&... __args) : __unex_(static_cast<_Args&&>(__args)...) {}
  template <class _Up, class... _Args>
    requires is_constructible_v<_Ep, initializer_list<_Up>&, _Args...>
  constexpr explicit unexpected(in_place_t, initializer_list<_Up> il, _Args&&... __args)
      : __unex_(il, static_cast<_Args&&>(__args)...) {}

  constexpr unexpected& operator=(const unexpected&) = default;
  constexpr unexpected& operator=(unexpected&&) = default;

  constexpr const _Ep& error() const& noexcept { return __unex_; }
  constexpr _Ep& error() & noexcept { return __unex_; }
  constexpr const _Ep&& error() const&& noexcept { return static_cast<const _Ep&&>(__unex_); }
  constexpr _Ep&& error() && noexcept { return static_cast<_Ep&&>(__unex_); }

  constexpr void swap(unexpected& other) noexcept(is_nothrow_swappable_v<_Ep>) {
    static_assert(is_swappable_v<_Ep>, "std::unexpected::swap: E must be swappable");
    __ycxx::__detail::__swap_adl::__do_swap(__unex_, other.__unex_);
  }

  template <class _E2>
  friend constexpr bool operator==(const unexpected& __x, const unexpected<_E2>& y) {
    static_assert(__ycxx::__detail::__eq_to_bool<_Ep, _E2>,
                  "std::unexpected: x.error() == y.error() must be well-formed and convertible to bool");
    return __ycxx::__detail::__implicit_bool(__x.error() == y.error());
  }
  friend constexpr void swap(unexpected& __x, unexpected& y) noexcept(noexcept(__x.swap(y)))
    requires is_swappable_v<_Ep>
  {
    __x.swap(y);
  }
};

template <class _Ep>
unexpected(_Ep) -> unexpected<_Ep>;

// ---- [expected.bad.void], [expected.bad] ----
template <class _Ep>
class bad_expected_access;

template <>
class bad_expected_access<void> : public exception {
protected:
  constexpr bad_expected_access() noexcept {}
  constexpr bad_expected_access(const bad_expected_access&) noexcept = default;
  constexpr bad_expected_access(bad_expected_access&&) noexcept = default;
  constexpr bad_expected_access& operator=(const bad_expected_access&) noexcept = default;
  constexpr bad_expected_access& operator=(bad_expected_access&&) noexcept = default;
  constexpr ~bad_expected_access() override {}

public:
  constexpr const char* what() const noexcept override { return "bad access to std::expected without a value"; }
};

template <class _Ep>
class bad_expected_access : public bad_expected_access<void> {
  _Ep __unex_;

public:
  constexpr explicit bad_expected_access(_Ep e) : __unex_(static_cast<_Ep&&>(e)) {}
  constexpr const char* what() const noexcept override { return "bad access to std::expected without a value"; }
  constexpr _Ep& error() & noexcept { return __unex_; }
  constexpr const _Ep& error() const& noexcept { return __unex_; }
  constexpr _Ep&& error() && noexcept { return static_cast<_Ep&&>(__unex_); }
  constexpr const _Ep&& error() const&& noexcept { return static_cast<const _Ep&&>(__unex_); }
};

struct unexpect_t {
  explicit unexpect_t() = default;
};
inline constexpr unexpect_t unexpect{};

template <class _Tp, class _Ep>
class expected;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Tp>
inline constexpr bool __is_expected = false;
template <class _Tp, class _Ep>
inline constexpr bool __is_expected<std::expected<_Tp, _Ep>> = true;

// Mandates of the monadic operations, as concepts that are simply false (not ill-formed) for
// results that are not expected specializations.
template <class _Up, class _Ep>
concept __expected_with_error = __is_expected<_Up> && std::is_same_v<typename _Up::error_type, _Ep>;
template <class _Gp, class _Tp>
concept __expected_with_value = __is_expected<_Gp> && std::is_same_v<typename _Gp::value_type, _Tp>;

// [expected.object.general]/2
template <class _Tp>
concept __valid_expected_value =
    std::is_void_v<_Tp> ||
    (std::is_object_v<_Tp> && !std::is_array_v<_Tp> && !std::is_same_v<std::remove_cv_t<_Tp>, std::in_place_t> &&
     !std::is_same_v<std::remove_cv_t<_Tp>, std::unexpect_t> && !__is_unexpected<std::remove_cv_t<_Tp>>);

// Stands in for the value member when T is void, so both class templates share one storage.
struct __expected_void_value {};
template <class _Tp>
using __expected_value_t = std::conditional_t<std::is_void_v<_Tp>, __expected_void_value, std::remove_cv_t<_Tp>>;

// Tags for building the value or the error directly from an invocation (monadic transforms),
// so non-movable results work.
struct __expected_invoke_val_tag {};
struct __expected_invoke_err_tag {};

template <class _Vp, class _Ep>
union __expected_union {
  _Vp __val;
  _Ep __unex;

  template <class... _Args>
  constexpr explicit __expected_union(std::in_place_t, _Args&&... __args) : __val(static_cast<_Args&&>(__args)...) {}
  template <class... _Args>
  constexpr explicit __expected_union(std::unexpect_t, _Args&&... __args) : __unex(static_cast<_Args&&>(__args)...) {}
  template <class _Fp, class... _Args>
  constexpr __expected_union(__expected_invoke_val_tag, _Fp&& __f, _Args&&... __args)
      : __val(::__ycxx::__detail::invoke(static_cast<_Fp&&>(__f), static_cast<_Args&&>(__args)...)) {}
  template <class _Fp, class... _Args>
  constexpr __expected_union(__expected_invoke_err_tag, _Fp&& __f, _Args&&... __args)
      : __unex(::__ycxx::__detail::invoke(static_cast<_Fp&&>(__f), static_cast<_Args&&>(__args)...)) {}

  __expected_union(const __expected_union&) = default;
  __expected_union(__expected_union&&) = default;
  __expected_union& operator=(const __expected_union&) = default;
  __expected_union& operator=(__expected_union&&) = default;
  constexpr ~__expected_union()
    requires std::is_trivially_destructible_v<_Vp> && std::is_trivially_destructible_v<_Ep>
  = default;
  constexpr ~__expected_union() {}
};

// reinit-expected ([expected.object.assign]/1)
template <class _Tp, class _Up, class... _Args>
constexpr void __reinit_expected(_Tp& __newval, _Up& __oldval, _Args&&... __args) {
  if constexpr (std::is_nothrow_constructible_v<_Tp, _Args...>) {
    std::destroy_at(__builtin_addressof(__oldval));
    std::construct_at(__builtin_addressof(__newval), static_cast<_Args&&>(__args)...);
  } else if constexpr (std::is_nothrow_move_constructible_v<_Tp>) {
    _Tp __tmp(static_cast<_Args&&>(__args)...);
    std::destroy_at(__builtin_addressof(__oldval));
    std::construct_at(__builtin_addressof(__newval), static_cast<_Tp&&>(__tmp));
  } else {
    _Up __tmp(static_cast<_Up&&>(__oldval));
    std::destroy_at(__builtin_addressof(__oldval));
    if constexpr (__cfg::exceptions) {
      try {
        std::construct_at(__builtin_addressof(__newval), static_cast<_Args&&>(__args)...);
      } catch (...) {
        std::construct_at(__builtin_addressof(__oldval), static_cast<_Up&&>(__tmp));
        throw;
      }
    } else {
      std::construct_at(__builtin_addressof(__newval), static_cast<_Args&&>(__args)...);
    }
  }
}

// Constraints shared by the converting constructors from expected<U, G>
// ([expected.object.cons]/18, [expected.void.cons]/13). UF/GF carry the source qualification.
template <class _Ep, class _Up, class _Gp>
concept __expected_unexpected_not_from =
    !std::is_constructible_v<std::unexpected<_Ep>, std::expected<_Up, _Gp>&> &&
    !std::is_constructible_v<std::unexpected<_Ep>, std::expected<_Up, _Gp>> &&
    !std::is_constructible_v<std::unexpected<_Ep>, const std::expected<_Up, _Gp>&> &&
    !std::is_constructible_v<std::unexpected<_Ep>, const std::expected<_Up, _Gp>>;
// The same-type case is handled by the copy/move constructors; excluding it first keeps their
// own constructibility checks from re-entering this concept.
template <class _Tp, class _Ep, class _Up, class _Gp, class _UF, class _GF>
concept __expected_converts_from =
    !(std::is_same_v<_Tp, _Up> && std::is_same_v<_Ep, _Gp>) && std::is_constructible_v<_Tp, _UF> && std::is_constructible_v<_Ep, _GF> &&
    (std::is_same_v<std::remove_cv_t<_Tp>, bool> || !__converts_from_any_cvref<_Tp, std::expected<_Up, _Gp>>) &&
    __expected_unexpected_not_from<_Ep, _Up, _Gp>;
template <class _Tp, class _Ep, class _Up, class _Gp, class _GF>
concept __expected_void_converts_from = !(std::is_same_v<_Tp, _Up> && std::is_same_v<_Ep, _Gp>) && std::is_constructible_v<_Ep, _GF> &&
                                      __expected_unexpected_not_from<_Ep, _Up, _Gp>;

}} // namespace __ycxx::__detail

// Base classes of std types live in __ycxx::__adl_free, a namespace that declares no functions:
// a base's namespace is an associated namespace for ADL ([basic.lookup.argdep]/3), so a
// __ycxx::__detail base would expose every internal function to lookup on the std type.
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// Common machinery of expected<T, E> and expected<void, E>: storage, lifetime, assignment.
template <class _Tp, class _Ep>
class __expected_base {
protected:
  using _Vp = __ycxx::__detail::__expected_value_t<_Tp>;
  using __storage = __ycxx::__detail::__expected_union<_Vp, _Ep>;

  __storage __u_;
  bool __has_val_;

  static constexpr bool is_void = std::is_void_v<_Tp>;
  // Value-side traits; the empty stand-in for void is trivially everything.
  static constexpr bool __trivial_dtor = std::is_trivially_destructible_v<_Vp> && std::is_trivially_destructible_v<_Ep>;

  template <class... _Args>
  constexpr explicit __expected_base(std::in_place_t t, _Args&&... __args)
      : __u_(t, static_cast<_Args&&>(__args)...), __has_val_(true) {}
  template <class... _Args>
  constexpr explicit __expected_base(std::unexpect_t t, _Args&&... __args)
      : __u_(t, static_cast<_Args&&>(__args)...), __has_val_(false) {}
  template <class _Fp, class... _Args>
  constexpr __expected_base(__ycxx::__detail::__expected_invoke_val_tag t, _Fp&& __f, _Args&&... __args)
      : __u_(t, static_cast<_Fp&&>(__f), static_cast<_Args&&>(__args)...), __has_val_(true) {}
  template <class _Fp, class... _Args>
  constexpr __expected_base(__ycxx::__detail::__expected_invoke_err_tag t, _Fp&& __f, _Args&&... __args)
      : __u_(t, static_cast<_Fp&&>(__f), static_cast<_Args&&>(__args)...), __has_val_(false) {}
  // From another expected (same or converting): rhs is any expected<U, G> cvref; its value is
  // forwarded with rhs's qualification.
  struct __from_other_tag {};
  template <class _Rp>
  constexpr __expected_base(__from_other_tag, _Rp&& __rhs)
      : __u_(__rhs.has_value() ? __make_value(static_cast<_Rp&&>(__rhs)) : __storage(std::unexpect, static_cast<_Rp&&>(__rhs).error())),
        __has_val_(__rhs.has_value()) {}

  __expected_base(const __expected_base&) = default;
  __expected_base(__expected_base&&) = default;
  __expected_base& operator=(const __expected_base&) = default;
  __expected_base& operator=(__expected_base&&) = default;
  constexpr ~__expected_base()
    requires __trivial_dtor
  = default;
  constexpr ~__expected_base() { destroy(); }

  template <class _Rp>
  static constexpr __storage __make_value(_Rp&& __rhs) {
    if constexpr (is_void)
      return __storage(std::in_place);
    else
      return __storage(std::in_place, *static_cast<_Rp&&>(__rhs));
  }

  constexpr void destroy() noexcept {
    if (__has_val_)
      std::destroy_at(__builtin_addressof(__u_.__val));
    else
      std::destroy_at(__builtin_addressof(__u_.__unex));
  }

  // Copy / move assignment ([expected.object.assign]/2,7, [expected.void.assign]/1,6). R is
  // const expected_base& or expected_base&& (the derived class converts to its private base).
  template <class _Rp>
  constexpr void __assign_from(_Rp&& __rhs) {
    using _EF = std::conditional_t<std::is_lvalue_reference_v<_Rp>, const _Ep&, _Ep&&>;
    if (__has_val_ && __rhs.__has_val_) {
      if constexpr (!is_void) {
        using _VF = std::conditional_t<std::is_lvalue_reference_v<_Rp>, const _Vp&, _Vp&&>;
        __u_.__val = static_cast<_VF>(__rhs.__u_.__val);
      }
    } else if (__has_val_) {
      if constexpr (is_void)
        std::construct_at(__builtin_addressof(__u_.__unex), static_cast<_EF>(__rhs.__u_.__unex));
      else
        ::__ycxx::__detail::__reinit_expected(__u_.__unex, __u_.__val, static_cast<_EF>(__rhs.__u_.__unex));
    } else if (__rhs.__has_val_) {
      if constexpr (is_void) {
        std::destroy_at(__builtin_addressof(__u_.__unex));
        std::construct_at(__builtin_addressof(__u_.__val));
      } else {
        using _VF = std::conditional_t<std::is_lvalue_reference_v<_Rp>, const _Vp&, _Vp&&>;
        ::__ycxx::__detail::__reinit_expected(__u_.__val, __u_.__unex, static_cast<_VF>(__rhs.__u_.__val));
      }
    } else {
      __u_.__unex = static_cast<_EF>(__rhs.__u_.__unex);
    }
    __has_val_ = __rhs.__has_val_;
  }

  // Assignment from unexpected<G> ([expected.object.assign]/16, [expected.void.assign]/12).
  template <class _GF>
  constexpr void __assign_error(_GF&& __g) {
    if (__has_val_) {
      if constexpr (is_void)
        std::construct_at(__builtin_addressof(__u_.__unex), static_cast<_GF&&>(__g));
      else
        ::__ycxx::__detail::__reinit_expected(__u_.__unex, __u_.__val, static_cast<_GF&&>(__g));
      __has_val_ = false;
    } else {
      __u_.__unex = static_cast<_GF&&>(__g);
    }
  }

public:
  constexpr explicit operator bool() const noexcept { return __has_val_; }
  constexpr bool has_value() const noexcept { return __has_val_; }
  constexpr bool has_error() const noexcept { return !__has_val_; }

  constexpr const _Ep& error() const& noexcept {
    __ycxx::__detail::__precondition(!__has_val_, "std::expected::error: has_value() is true");
    return __u_.__unex;
  }
  constexpr _Ep& error() & noexcept {
    __ycxx::__detail::__precondition(!__has_val_, "std::expected::error: has_value() is true");
    return __u_.__unex;
  }
  constexpr const _Ep&& error() const&& noexcept {
    __ycxx::__detail::__precondition(!__has_val_, "std::expected::error: has_value() is true");
    return static_cast<const _Ep&&>(__u_.__unex);
  }
  constexpr _Ep&& error() && noexcept {
    __ycxx::__detail::__precondition(!__has_val_, "std::expected::error: has_value() is true");
    return static_cast<_Ep&&>(__u_.__unex);
  }

  template <class _Gp = _Ep>
  constexpr _Ep error_or(_Gp&& e) const& {
    static_assert(std::is_copy_constructible_v<_Ep> && std::is_convertible_v<_Gp, _Ep>,
                  "std::expected::error_or: E must be copy constructible and G convertible to E");
    if (__has_val_)
      return static_cast<_Gp&&>(e);
    return __u_.__unex;
  }
  template <class _Gp = _Ep>
  constexpr _Ep error_or(_Gp&& e) && {
    static_assert(std::is_move_constructible_v<_Ep> && std::is_convertible_v<_Gp, _Ep>,
                  "std::expected::error_or: E must be move constructible and G convertible to E");
    if (__has_val_)
      return static_cast<_Gp&&>(e);
    return static_cast<_Ep&&>(__u_.__unex);
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

// =============================================================================================
// [expected.expected]
// =============================================================================================
template <class _Tp, class _Ep>
class expected : private __ycxx::__adl_free::__expected_base<_Tp, _Ep> {
  static_assert(__ycxx::__detail::__valid_expected_value<_Tp>,
                "std::expected: T must be void or a non-array object type other than in_place_t, unexpect_t "
                "and specializations of unexpected");
  static_assert(__ycxx::__detail::__valid_unexpected_arg<_Ep>, "std::expected: E must be a valid argument for unexpected");

  using base = __ycxx::__adl_free::__expected_base<_Tp, _Ep>;
  using typename base::__storage;
  using typename base::_Vp;
  using base::__u_;
  using base::__has_val_;
  using __from_other = typename base::__from_other_tag;

  template <class, class>
  friend class expected;

  template <class _Tag, class _Fp, class... _Args>
    requires is_same_v<_Tag, __ycxx::__detail::__expected_invoke_val_tag> || is_same_v<_Tag, __ycxx::__detail::__expected_invoke_err_tag>
  constexpr expected(_Tag t, _Fp&& __f, _Args&&... __args) : base(t, static_cast<_Fp&&>(__f), static_cast<_Args&&>(__args)...) {}

  static constexpr bool __copy_ok = is_copy_constructible_v<_Tp> && is_copy_constructible_v<_Ep>;
  static constexpr bool __move_ok = is_move_constructible_v<_Tp> && is_move_constructible_v<_Ep>;
  static constexpr bool __copy_assign_ok = is_copy_assignable_v<_Tp> && is_copy_constructible_v<_Tp> &&
                                         is_copy_assignable_v<_Ep> && is_copy_constructible_v<_Ep> &&
                                         (is_nothrow_move_constructible_v<_Tp> || is_nothrow_move_constructible_v<_Ep>);
  static constexpr bool __move_assign_ok = is_move_constructible_v<_Tp> && is_move_assignable_v<_Tp> &&
                                         is_move_constructible_v<_Ep> && is_move_assignable_v<_Ep> &&
                                         (is_nothrow_move_constructible_v<_Tp> || is_nothrow_move_constructible_v<_Ep>);
  // Exception specifications. Written on the defaulted (trivial) members too: a defaulted
  // member's implicit specification would follow the union and always be noexcept. The copy
  // operations' specifications are a permitted strengthening ([res.on.exception.handling]/5).
  static constexpr bool __nothrow_copy = is_nothrow_copy_constructible_v<_Tp> && is_nothrow_copy_constructible_v<_Ep>;
  static constexpr bool __nothrow_move = is_nothrow_move_constructible_v<_Tp> && is_nothrow_move_constructible_v<_Ep>;
  static constexpr bool __nothrow_copy_assign = __nothrow_copy && is_nothrow_copy_assignable_v<_Tp> && is_nothrow_copy_assignable_v<_Ep>;
  static constexpr bool __nothrow_move_assign = __nothrow_move && is_nothrow_move_assignable_v<_Tp> && is_nothrow_move_assignable_v<_Ep>;
  static constexpr bool __trivial_copy = is_trivially_copy_constructible_v<_Tp> && is_trivially_copy_constructible_v<_Ep>;
  static constexpr bool __trivial_move = is_trivially_move_constructible_v<_Tp> && is_trivially_move_constructible_v<_Ep>;
  static constexpr bool __trivial_copy_assign =
      is_trivially_copy_constructible_v<_Tp> && is_trivially_copy_assignable_v<_Tp> && is_trivially_destructible_v<_Tp> &&
      is_trivially_copy_constructible_v<_Ep> && is_trivially_copy_assignable_v<_Ep> && is_trivially_destructible_v<_Ep>;
  static constexpr bool __trivial_move_assign =
      is_trivially_move_constructible_v<_Tp> && is_trivially_move_assignable_v<_Tp> && is_trivially_destructible_v<_Tp> &&
      is_trivially_move_constructible_v<_Ep> && is_trivially_move_assignable_v<_Ep> && is_trivially_destructible_v<_Ep>;

public:
  using value_type = _Tp;
  using error_type = _Ep;
  using unexpected_type = unexpected<_Ep>;
  template <class _Up>
  using rebind = expected<_Up, error_type>;

  using base::operator bool;
  using base::has_value;
  using base::has_error;
  using base::error;
  using base::error_or;

  // ---- [expected.object.cons] ----
  constexpr expected()
    requires is_default_constructible_v<_Tp>
      : base(in_place) {}

  constexpr expected(const expected&) noexcept(__nothrow_copy)
    requires __copy_ok && __trivial_copy
  = default;
  constexpr expected(const expected& __rhs) noexcept(__nothrow_copy)
    requires __copy_ok && (!__trivial_copy)
      : base(__from_other{}, __rhs) {}
  constexpr expected(const expected&)
    requires(!__copy_ok)
  = delete;

  constexpr expected(expected&&) noexcept(__nothrow_move)
    requires __move_ok && __trivial_move
  = default;
  constexpr expected(expected&& __rhs) noexcept(__nothrow_move)
    requires __move_ok && (!__trivial_move)
      : base(__from_other{}, static_cast<expected&&>(__rhs)) {}

  template <class _Up, class _Gp>
    requires(!is_void_v<_Up>) && __ycxx::__detail::__expected_converts_from<_Tp, _Ep, _Up, _Gp, const _Up&, const _Gp&>
  constexpr explicit(!is_convertible_v<const _Up&, _Tp> || !is_convertible_v<const _Gp&, _Ep>)
      expected(const expected<_Up, _Gp>& __rhs)
      : base(__from_other{}, __rhs) {}
  template <class _Up, class _Gp>
    requires(!is_void_v<_Up>) && __ycxx::__detail::__expected_converts_from<_Tp, _Ep, _Up, _Gp, _Up, _Gp>
  constexpr explicit(!is_convertible_v<_Up, _Tp> || !is_convertible_v<_Gp, _Ep>) expected(expected<_Up, _Gp>&& __rhs)
      : base(__from_other{}, static_cast<expected<_Up, _Gp>&&>(__rhs)) {}

  template <class _Up = remove_cv_t<_Tp>>
    requires(!is_same_v<remove_cvref_t<_Up>, in_place_t>) && (!is_same_v<remove_cvref_t<_Up>, expected>) &&
            (!is_same_v<remove_cvref_t<_Up>, unexpect_t>) && (!__ycxx::__detail::__is_unexpected<remove_cvref_t<_Up>>) &&
            is_constructible_v<_Tp, _Up> &&
            (!is_same_v<remove_cv_t<_Tp>, bool> || !__ycxx::__detail::__is_expected<remove_cvref_t<_Up>>)
  constexpr explicit(!is_convertible_v<_Up, _Tp>) expected(_Up&& __v) : base(in_place, static_cast<_Up&&>(__v)) {}

  template <class _Gp>
    requires is_constructible_v<_Ep, const _Gp&>
  constexpr explicit(!is_convertible_v<const _Gp&, _Ep>) expected(const unexpected<_Gp>& e) : base(unexpect, e.error()) {}
  template <class _Gp>
    requires is_constructible_v<_Ep, _Gp>
  constexpr explicit(!is_convertible_v<_Gp, _Ep>) expected(unexpected<_Gp>&& e)
      : base(unexpect, static_cast<unexpected<_Gp>&&>(e).error()) {}

  template <class... _Args>
    requires is_constructible_v<_Tp, _Args...>
  constexpr explicit expected(in_place_t, _Args&&... __args) : base(in_place, static_cast<_Args&&>(__args)...) {}
  template <class _Up, class... _Args>
    requires is_constructible_v<_Tp, initializer_list<_Up>&, _Args...>
  constexpr explicit expected(in_place_t, initializer_list<_Up> il, _Args&&... __args)
      : base(in_place, il, static_cast<_Args&&>(__args)...) {}
  template <class... _Args>
    requires is_constructible_v<_Ep, _Args...>
  constexpr explicit expected(unexpect_t, _Args&&... __args) : base(unexpect, static_cast<_Args&&>(__args)...) {}
  template <class _Up, class... _Args>
    requires is_constructible_v<_Ep, initializer_list<_Up>&, _Args...>
  constexpr explicit expected(unexpect_t, initializer_list<_Up> il, _Args&&... __args)
      : base(unexpect, il, static_cast<_Args&&>(__args)...) {}

  // ---- [expected.object.dtor]: base's destructor ----

  // ---- [expected.object.assign] ----
  constexpr expected& operator=(const expected&) noexcept(__nothrow_copy_assign)
    requires __copy_assign_ok && __trivial_copy_assign
  = default;
  constexpr expected& operator=(const expected& __rhs) noexcept(__nothrow_copy_assign)
    requires __copy_assign_ok && (!__trivial_copy_assign)
  {
    this->__assign_from(static_cast<const base&>(__rhs));
    return *this;
  }
  constexpr expected& operator=(const expected&)
    requires(!__copy_assign_ok)
  = delete;

  constexpr expected& operator=(expected&&) noexcept(__nothrow_move_assign)
    requires __move_assign_ok && __trivial_move_assign
  = default;
  constexpr expected& operator=(expected&& __rhs) noexcept(__nothrow_move_assign)
    requires __move_assign_ok && (!__trivial_move_assign)
  {
    this->__assign_from(static_cast<base&&>(__rhs));
    return *this;
  }

  template <class _Up = remove_cv_t<_Tp>>
    requires(!is_same_v<expected, remove_cvref_t<_Up>>) && (!__ycxx::__detail::__is_unexpected<remove_cvref_t<_Up>>) &&
            is_constructible_v<_Tp, _Up> && is_assignable_v<_Tp&, _Up> &&
            (is_nothrow_constructible_v<_Tp, _Up> || is_nothrow_move_constructible_v<_Tp> ||
             is_nothrow_move_constructible_v<_Ep>)
  constexpr expected& operator=(_Up&& __v) {
    if (__has_val_) {
      __u_.__val = static_cast<_Up&&>(__v);
    } else {
      __ycxx::__detail::__reinit_expected(__u_.__val, __u_.__unex, static_cast<_Up&&>(__v));
      __has_val_ = true;
    }
    return *this;
  }

  template <class _Gp>
    requires is_constructible_v<_Ep, const _Gp&> && is_assignable_v<_Ep&, const _Gp&> &&
             (is_nothrow_constructible_v<_Ep, const _Gp&> || is_nothrow_move_constructible_v<_Tp> ||
              is_nothrow_move_constructible_v<_Ep>)
  constexpr expected& operator=(const unexpected<_Gp>& e) {
    this->__assign_error(e.error());
    return *this;
  }
  template <class _Gp>
    requires is_constructible_v<_Ep, _Gp> && is_assignable_v<_Ep&, _Gp> &&
             (is_nothrow_constructible_v<_Ep, _Gp> || is_nothrow_move_constructible_v<_Tp> ||
              is_nothrow_move_constructible_v<_Ep>)
  constexpr expected& operator=(unexpected<_Gp>&& e) {
    this->__assign_error(static_cast<unexpected<_Gp>&&>(e).error());
    return *this;
  }

  template <class... _Args>
    requires is_nothrow_constructible_v<_Tp, _Args...>
  constexpr _Tp& emplace(_Args&&... __args) noexcept {
    this->destroy();
    __has_val_ = true;
    return *std::construct_at(__builtin_addressof(__u_.__val), static_cast<_Args&&>(__args)...);
  }
  template <class _Up, class... _Args>
    requires is_nothrow_constructible_v<_Tp, initializer_list<_Up>&, _Args...>
  constexpr _Tp& emplace(initializer_list<_Up> il, _Args&&... __args) noexcept {
    this->destroy();
    __has_val_ = true;
    return *std::construct_at(__builtin_addressof(__u_.__val), il, static_cast<_Args&&>(__args)...);
  }

  // ---- [expected.object.swap] ----
  constexpr void swap(expected& __rhs) noexcept(is_nothrow_move_constructible_v<_Tp> && is_nothrow_swappable_v<_Tp> &&
                                              is_nothrow_move_constructible_v<_Ep> && is_nothrow_swappable_v<_Ep>)
    requires is_swappable_v<_Tp> && is_swappable_v<_Ep> && is_move_constructible_v<_Tp> && is_move_constructible_v<_Ep> &&
             (is_nothrow_move_constructible_v<_Tp> || is_nothrow_move_constructible_v<_Ep>)
  {
    if (__has_val_ && __rhs.__has_val_) {
      __ycxx::__detail::__swap_adl::__do_swap(__u_.__val, __rhs.__u_.__val);
    } else if (!__has_val_ && !__rhs.__has_val_) {
      __ycxx::__detail::__swap_adl::__do_swap(__u_.__unex, __rhs.__u_.__unex);
    } else if (!__has_val_) {
      __rhs.swap(*this);
    } else {
      __swap_value_with_error(__rhs);
    }
  }
  friend constexpr void swap(expected& __x, expected& y) noexcept(noexcept(__x.swap(y)))
    requires requires { __x.swap(y); }
  {
    __x.swap(y);
  }

  // ---- [expected.object.obs] ----
  constexpr const _Tp* operator->() const noexcept {
    __ycxx::__detail::__precondition(__has_val_, "std::expected::operator->: no value");
    return __builtin_addressof(__u_.__val);
  }
  constexpr _Tp* operator->() noexcept {
    __ycxx::__detail::__precondition(__has_val_, "std::expected::operator->: no value");
    return __builtin_addressof(__u_.__val);
  }
  constexpr const _Tp& operator*() const& noexcept {
    __ycxx::__detail::__precondition(__has_val_, "std::expected::operator*: no value");
    return __u_.__val;
  }
  constexpr _Tp& operator*() & noexcept {
    __ycxx::__detail::__precondition(__has_val_, "std::expected::operator*: no value");
    return __u_.__val;
  }
  constexpr const _Tp&& operator*() const&& noexcept {
    __ycxx::__detail::__precondition(__has_val_, "std::expected::operator*: no value");
    return static_cast<const _Tp&&>(__u_.__val);
  }
  constexpr _Tp&& operator*() && noexcept {
    __ycxx::__detail::__precondition(__has_val_, "std::expected::operator*: no value");
    return static_cast<_Tp&&>(__u_.__val);
  }

  constexpr const _Tp& value() const& {
    static_assert(is_copy_constructible_v<_Ep>, "std::expected::value: E must be copy constructible");
    if (!__has_val_)
      __throw_bad_access(__u_.__unex);
    return __u_.__val;
  }
  constexpr _Tp& value() & {
    static_assert(is_copy_constructible_v<_Ep>, "std::expected::value: E must be copy constructible");
    if (!__has_val_)
      __throw_bad_access(static_cast<const _Ep&>(__u_.__unex));
    return __u_.__val;
  }
  constexpr const _Tp&& value() const&& {
    static_assert(is_copy_constructible_v<_Ep> && is_constructible_v<_Ep, const _Ep&&>,
                  "std::expected::value: E must be copy constructible and constructible from std::move(error())");
    if (!__has_val_)
      __throw_bad_access(static_cast<const _Ep&&>(__u_.__unex));
    return static_cast<const _Tp&&>(__u_.__val);
  }
  constexpr _Tp&& value() && {
    static_assert(is_copy_constructible_v<_Ep> && is_constructible_v<_Ep, _Ep&&>,
                  "std::expected::value: E must be copy constructible and constructible from std::move(error())");
    if (!__has_val_)
      __throw_bad_access(static_cast<_Ep&&>(__u_.__unex));
    return static_cast<_Tp&&>(__u_.__val);
  }

  template <class _Up = remove_cv_t<_Tp>>
  constexpr _Tp value_or(_Up&& __v) const& {
    static_assert(is_copy_constructible_v<_Tp> && is_convertible_v<_Up, _Tp>,
                  "std::expected::value_or: T must be copy constructible and U convertible to T");
    return __has_val_ ? __u_.__val : static_cast<_Tp>(static_cast<_Up&&>(__v));
  }
  template <class _Up = remove_cv_t<_Tp>>
  constexpr _Tp value_or(_Up&& __v) && {
    static_assert(is_move_constructible_v<_Tp> && is_convertible_v<_Up, _Tp>,
                  "std::expected::value_or: T must be move constructible and U convertible to T");
    return __has_val_ ? static_cast<_Tp&&>(__u_.__val) : static_cast<_Tp>(static_cast<_Up&&>(__v));
  }

  // ---- [expected.object.monadic] ----
  // Four overloads each (not one explicit-object member): when an rvalue's && overload is not
  // viable, overload resolution must fall back to const && (LWG3877). The *_impl helpers take
  // the qualified expected type as Self; decltype((val)) / decltype(error()) is forward_like<Self>.
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<expected&, _Ep>>
  constexpr auto and_then(_Fp&& __f) & {
    return __and_then_impl<expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<const expected&, _Ep>>
  constexpr auto and_then(_Fp&& __f) const& {
    return __and_then_impl<const expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<expected, _Ep>>
  constexpr auto and_then(_Fp&& __f) && {
    return __and_then_impl<expected>(static_cast<expected&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<const expected, _Ep>>
  constexpr auto and_then(_Fp&& __f) const&& {
    return __and_then_impl<const expected>(static_cast<const expected&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Tp, __ycxx::__detail::__forward_like_t<expected&, _Vp>>
  constexpr auto or_else(_Fp&& __f) & {
    return __or_else_impl<expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Tp, __ycxx::__detail::__forward_like_t<const expected&, _Vp>>
  constexpr auto or_else(_Fp&& __f) const& {
    return __or_else_impl<const expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Tp, __ycxx::__detail::__forward_like_t<expected, _Vp>>
  constexpr auto or_else(_Fp&& __f) && {
    return __or_else_impl<expected>(static_cast<expected&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Tp, __ycxx::__detail::__forward_like_t<const expected, _Vp>>
  constexpr auto or_else(_Fp&& __f) const&& {
    return __or_else_impl<const expected>(static_cast<const expected&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<expected&, _Ep>>
  constexpr auto transform(_Fp&& __f) & {
    return __transform_impl<expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<const expected&, _Ep>>
  constexpr auto transform(_Fp&& __f) const& {
    return __transform_impl<const expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<expected, _Ep>>
  constexpr auto transform(_Fp&& __f) && {
    return __transform_impl<expected>(static_cast<expected&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<const expected, _Ep>>
  constexpr auto transform(_Fp&& __f) const&& {
    return __transform_impl<const expected>(static_cast<const expected&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Tp, __ycxx::__detail::__forward_like_t<expected&, _Vp>>
  constexpr auto transform_error(_Fp&& __f) & {
    return __transform_error_impl<expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Tp, __ycxx::__detail::__forward_like_t<const expected&, _Vp>>
  constexpr auto transform_error(_Fp&& __f) const& {
    return __transform_error_impl<const expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Tp, __ycxx::__detail::__forward_like_t<expected, _Vp>>
  constexpr auto transform_error(_Fp&& __f) && {
    return __transform_error_impl<expected>(static_cast<expected&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Tp, __ycxx::__detail::__forward_like_t<const expected, _Vp>>
  constexpr auto transform_error(_Fp&& __f) const&& {
    return __transform_error_impl<const expected>(static_cast<const expected&&>(*this), static_cast<_Fp&&>(__f));
  }

  // ---- [expected.object.eq] ----
  template <class _T2, class _E2>
    requires(!is_void_v<_T2>) && __ycxx::__detail::__eq_to_bool<_Tp, _T2> && __ycxx::__detail::__eq_to_bool<_Ep, _E2>
  friend constexpr bool operator==(const expected& __x, const expected<_T2, _E2>& y) {
    if (__x.has_value() != y.has_value())
      return false;
    return __x.has_value() ? __ycxx::__detail::__implicit_bool(*__x == *y) : __ycxx::__detail::__implicit_bool(__x.error() == y.error());
  }
  // The left operand is deduced (and must be this expected or derived from it) so that other types
  // never convert to expected here; with a plain `const expected&` parameter, checking the
  // constraint for e.g. int == pair<int, expected<int, int>> found via ADL re-enters itself.
  template <class _Xp, class _T2>
    requires derived_from<_Xp, expected> && (!__ycxx::__detail::__is_expected<_T2>) && __ycxx::__detail::__eq_to_bool<_Tp, _T2>
  friend constexpr bool operator==(const _Xp& __xd, const _T2& __v) {
    const expected& __x = __xd;
    return __x.has_value() && __ycxx::__detail::__implicit_bool(*__x == __v);
  }
  template <class _E2>
    requires __ycxx::__detail::__eq_to_bool<_Ep, _E2>
  friend constexpr bool operator==(const expected& __x, const unexpected<_E2>& e) {
    return !__x.has_value() && __ycxx::__detail::__implicit_bool(__x.error() == e.error());
  }

private:
  // Monadic operations ([expected.object.monadic]); Self is the qualified expected type.
  template <class _Self, class _Fp>
  static constexpr auto __and_then_impl(_Self&& s, _Fp&& __f) {
    using _Up = remove_cvref_t<invoke_result_t<_Fp, __ycxx::__detail::__forward_like_t<_Self, _Vp>>>;
    static_assert(__ycxx::__detail::__expected_with_error<_Up, _Ep>,
                  "std::expected::and_then: F must return a specialization of expected with the same error_type");
    if constexpr (!__ycxx::__detail::__expected_with_error<_Up, _Ep>)
      return; // no follow-on errors after the Mandates failure
    else {
      if (s.__has_val_)
        return ::__ycxx::__detail::invoke(static_cast<_Fp&&>(__f), std::forward_like<_Self>(s.__u_.__val));
      return _Up(unexpect, std::forward_like<_Self>(s.__u_.__unex));
    }
  }
  template <class _Self, class _Fp>
  static constexpr auto __or_else_impl(_Self&& s, _Fp&& __f) {
    using _Gp = remove_cvref_t<invoke_result_t<_Fp, __ycxx::__detail::__forward_like_t<_Self, _Ep>>>;
    static_assert(__ycxx::__detail::__expected_with_value<_Gp, _Tp>,
                  "std::expected::or_else: F must return a specialization of expected with the same value_type");
    if constexpr (!__ycxx::__detail::__expected_with_value<_Gp, _Tp>)
      return;
    else {
      if (s.__has_val_)
        return _Gp(in_place, std::forward_like<_Self>(s.__u_.__val));
      return ::__ycxx::__detail::invoke(static_cast<_Fp&&>(__f), std::forward_like<_Self>(s.__u_.__unex));
    }
  }
  template <class _Self, class _Fp>
  static constexpr auto __transform_impl(_Self&& s, _Fp&& __f) {
    using _Up = remove_cv_t<invoke_result_t<_Fp, __ycxx::__detail::__forward_like_t<_Self, _Vp>>>;
    static_assert(__ycxx::__detail::__valid_expected_value<_Up>, "std::expected::transform: invalid result type");
    if constexpr (!__ycxx::__detail::__valid_expected_value<_Up>)
      return;
    else {
      using _Rp = expected<_Up, _Ep>;
      if (!s.__has_val_)
        return _Rp(unexpect, std::forward_like<_Self>(s.__u_.__unex));
      if constexpr (is_void_v<_Up>) {
        ::__ycxx::__detail::invoke(static_cast<_Fp&&>(__f), std::forward_like<_Self>(s.__u_.__val));
        return _Rp();
      } else {
        return _Rp(__ycxx::__detail::__expected_invoke_val_tag{}, static_cast<_Fp&&>(__f), std::forward_like<_Self>(s.__u_.__val));
      }
    }
  }
  template <class _Self, class _Fp>
  static constexpr auto __transform_error_impl(_Self&& s, _Fp&& __f) {
    using _Gp = remove_cv_t<invoke_result_t<_Fp, __ycxx::__detail::__forward_like_t<_Self, _Ep>>>;
    static_assert(__ycxx::__detail::__valid_unexpected_arg<_Gp>, "std::expected::transform_error: invalid error type");
    if constexpr (!__ycxx::__detail::__valid_unexpected_arg<_Gp>)
      return;
    else {
      using _Rp = expected<_Tp, _Gp>;
      if (s.__has_val_)
        return _Rp(in_place, std::forward_like<_Self>(s.__u_.__val));
      return _Rp(__ycxx::__detail::__expected_invoke_err_tag{}, static_cast<_Fp&&>(__f), std::forward_like<_Self>(s.__u_.__unex));
    }
  }

  template <class _EF>
  [[noreturn]] static constexpr void __throw_bad_access(_EF&& e) {
    __ycxx::__detail::__raise_with(ycxx_error_bad_expected_access, "std::bad_expected_access",
                             [&] { return bad_expected_access<_Ep>(static_cast<_EF&&>(e)); });
  }

  // Table 72, the case this->has_value() && !rhs.has_value().
  constexpr void __swap_value_with_error(expected& __rhs) {
    if constexpr (is_nothrow_move_constructible_v<_Ep>) {
      _Ep __tmp(static_cast<_Ep&&>(__rhs.__u_.__unex));
      std::destroy_at(__builtin_addressof(__rhs.__u_.__unex));
      auto __body = [&] {
        std::construct_at(__builtin_addressof(__rhs.__u_.__val), static_cast<_Vp&&>(__u_.__val));
        std::destroy_at(__builtin_addressof(__u_.__val));
        std::construct_at(__builtin_addressof(__u_.__unex), static_cast<_Ep&&>(__tmp));
      };
      if constexpr (__ycxx::__detail::__cfg::exceptions) {
        try {
          __body();
        } catch (...) {
          std::construct_at(__builtin_addressof(__rhs.__u_.__unex), static_cast<_Ep&&>(__tmp));
          throw;
        }
      } else {
        __body();
      }
    } else {
      _Vp __tmp(static_cast<_Vp&&>(__u_.__val));
      std::destroy_at(__builtin_addressof(__u_.__val));
      auto __body = [&] {
        std::construct_at(__builtin_addressof(__u_.__unex), static_cast<_Ep&&>(__rhs.__u_.__unex));
        std::destroy_at(__builtin_addressof(__rhs.__u_.__unex));
        std::construct_at(__builtin_addressof(__rhs.__u_.__val), static_cast<_Vp&&>(__tmp));
      };
      if constexpr (__ycxx::__detail::__cfg::exceptions) {
        try {
          __body();
        } catch (...) {
          std::construct_at(__builtin_addressof(__u_.__val), static_cast<_Vp&&>(__tmp));
          throw;
        }
      } else {
        __body();
      }
    }
    __has_val_ = false;
    __rhs.__has_val_ = true;
  }
};

// =============================================================================================
// [expected.void]
// =============================================================================================
template <class _Tp, class _Ep>
  requires is_void_v<_Tp>
class expected<_Tp, _Ep> : private __ycxx::__adl_free::__expected_base<_Tp, _Ep> {
  static_assert(__ycxx::__detail::__valid_unexpected_arg<_Ep>, "std::expected: E must be a valid argument for unexpected");

  using base = __ycxx::__adl_free::__expected_base<_Tp, _Ep>;
  using typename base::__storage;
  using base::__u_;
  using base::__has_val_;
  using __from_other = typename base::__from_other_tag;

  template <class, class>
  friend class expected;

  template <class _Tag, class _Fp, class... _Args>
    requires is_same_v<_Tag, __ycxx::__detail::__expected_invoke_val_tag> || is_same_v<_Tag, __ycxx::__detail::__expected_invoke_err_tag>
  constexpr expected(_Tag t, _Fp&& __f, _Args&&... __args) : base(t, static_cast<_Fp&&>(__f), static_cast<_Args&&>(__args)...) {}

  // See the primary template for why the defaulted members carry exception specifications.
  static constexpr bool __nothrow_copy = is_nothrow_copy_constructible_v<_Ep>;
  static constexpr bool __nothrow_move = is_nothrow_move_constructible_v<_Ep>;
  static constexpr bool __nothrow_copy_assign = __nothrow_copy && is_nothrow_copy_assignable_v<_Ep>;
  static constexpr bool __nothrow_move_assign = __nothrow_move && is_nothrow_move_assignable_v<_Ep>;
  static constexpr bool __copy_assign_ok = is_copy_assignable_v<_Ep> && is_copy_constructible_v<_Ep>;
  static constexpr bool __move_assign_ok = is_move_constructible_v<_Ep> && is_move_assignable_v<_Ep>;
  static constexpr bool __trivial_copy_assign =
      is_trivially_copy_constructible_v<_Ep> && is_trivially_copy_assignable_v<_Ep> && is_trivially_destructible_v<_Ep>;
  static constexpr bool __trivial_move_assign =
      is_trivially_move_constructible_v<_Ep> && is_trivially_move_assignable_v<_Ep> && is_trivially_destructible_v<_Ep>;

public:
  using value_type = _Tp;
  using error_type = _Ep;
  using unexpected_type = unexpected<_Ep>;
  template <class _Up>
  using rebind = expected<_Up, error_type>;

  using base::operator bool;
  using base::has_value;
  using base::has_error;
  using base::error;
  using base::error_or;

  // ---- [expected.void.cons] ----
  constexpr expected() noexcept : base(in_place) {}

  constexpr expected(const expected&) noexcept(__nothrow_copy)
    requires is_copy_constructible_v<_Ep> && is_trivially_copy_constructible_v<_Ep>
  = default;
  constexpr expected(const expected& __rhs) noexcept(__nothrow_copy)
    requires is_copy_constructible_v<_Ep> && (!is_trivially_copy_constructible_v<_Ep>)
      : base(__from_other{}, __rhs) {}
  constexpr expected(const expected&)
    requires(!is_copy_constructible_v<_Ep>)
  = delete;

  constexpr expected(expected&&) noexcept(__nothrow_move)
    requires is_move_constructible_v<_Ep> && is_trivially_move_constructible_v<_Ep>
  = default;
  constexpr expected(expected&& __rhs) noexcept(__nothrow_move)
    requires is_move_constructible_v<_Ep> && (!is_trivially_move_constructible_v<_Ep>)
      : base(__from_other{}, static_cast<expected&&>(__rhs)) {}

  template <class _Up, class _Gp>
    requires is_void_v<_Up> && __ycxx::__detail::__expected_void_converts_from<_Tp, _Ep, _Up, _Gp, const _Gp&>
  constexpr explicit(!is_convertible_v<const _Gp&, _Ep>) expected(const expected<_Up, _Gp>& __rhs) : base(__from_other{}, __rhs) {}
  template <class _Up, class _Gp>
    requires is_void_v<_Up> && __ycxx::__detail::__expected_void_converts_from<_Tp, _Ep, _Up, _Gp, _Gp>
  constexpr explicit(!is_convertible_v<_Gp, _Ep>) expected(expected<_Up, _Gp>&& __rhs)
      : base(__from_other{}, static_cast<expected<_Up, _Gp>&&>(__rhs)) {}

  template <class _Gp>
    requires is_constructible_v<_Ep, const _Gp&>
  constexpr explicit(!is_convertible_v<const _Gp&, _Ep>) expected(const unexpected<_Gp>& e) : base(unexpect, e.error()) {}
  template <class _Gp>
    requires is_constructible_v<_Ep, _Gp>
  constexpr explicit(!is_convertible_v<_Gp, _Ep>) expected(unexpected<_Gp>&& e)
      : base(unexpect, static_cast<unexpected<_Gp>&&>(e).error()) {}

  constexpr explicit expected(in_place_t) noexcept : base(in_place) {}
  template <class... _Args>
    requires is_constructible_v<_Ep, _Args...>
  constexpr explicit expected(unexpect_t, _Args&&... __args) : base(unexpect, static_cast<_Args&&>(__args)...) {}
  template <class _Up, class... _Args>
    requires is_constructible_v<_Ep, initializer_list<_Up>&, _Args...>
  constexpr explicit expected(unexpect_t, initializer_list<_Up> il, _Args&&... __args)
      : base(unexpect, il, static_cast<_Args&&>(__args)...) {}

  // ---- [expected.void.assign] ----
  constexpr expected& operator=(const expected&) noexcept(__nothrow_copy_assign)
    requires __copy_assign_ok && __trivial_copy_assign
  = default;
  constexpr expected& operator=(const expected& __rhs) noexcept(__nothrow_copy_assign)
    requires __copy_assign_ok && (!__trivial_copy_assign)
  {
    this->__assign_from(static_cast<const base&>(__rhs));
    return *this;
  }
  constexpr expected& operator=(const expected&)
    requires(!__copy_assign_ok)
  = delete;

  constexpr expected& operator=(expected&&) noexcept(__nothrow_move_assign)
    requires __move_assign_ok && __trivial_move_assign
  = default;
  constexpr expected& operator=(expected&& __rhs) noexcept(__nothrow_move_assign)
    requires __move_assign_ok && (!__trivial_move_assign)
  {
    this->__assign_from(static_cast<base&&>(__rhs));
    return *this;
  }

  template <class _Gp>
    requires is_constructible_v<_Ep, const _Gp&> && is_assignable_v<_Ep&, const _Gp&>
  constexpr expected& operator=(const unexpected<_Gp>& e) {
    this->__assign_error(e.error());
    return *this;
  }
  template <class _Gp>
    requires is_constructible_v<_Ep, _Gp> && is_assignable_v<_Ep&, _Gp>
  constexpr expected& operator=(unexpected<_Gp>&& e) {
    this->__assign_error(static_cast<unexpected<_Gp>&&>(e).error());
    return *this;
  }

  constexpr void emplace() noexcept {
    if (!__has_val_) {
      std::destroy_at(__builtin_addressof(__u_.__unex));
      std::construct_at(__builtin_addressof(__u_.__val));
      __has_val_ = true;
    }
  }

  // ---- [expected.void.swap] ----
  constexpr void swap(expected& __rhs) noexcept(is_nothrow_move_constructible_v<_Ep> && is_nothrow_swappable_v<_Ep>)
    requires is_swappable_v<_Ep> && is_move_constructible_v<_Ep>
  {
    if (__has_val_ && __rhs.__has_val_)
      return;
    if (!__has_val_ && !__rhs.__has_val_) {
      __ycxx::__detail::__swap_adl::__do_swap(__u_.__unex, __rhs.__u_.__unex);
    } else if (!__has_val_) {
      __rhs.swap(*this);
    } else {
      std::construct_at(__builtin_addressof(__u_.__unex), static_cast<_Ep&&>(__rhs.__u_.__unex));
      std::destroy_at(__builtin_addressof(__rhs.__u_.__unex));
      std::construct_at(__builtin_addressof(__rhs.__u_.__val));
      __has_val_ = false;
      __rhs.__has_val_ = true;
    }
  }
  friend constexpr void swap(expected& __x, expected& y) noexcept(noexcept(__x.swap(y)))
    requires requires { __x.swap(y); }
  {
    __x.swap(y);
  }

  // ---- [expected.void.obs] ----
  constexpr void operator*() const noexcept {
    __ycxx::__detail::__precondition(__has_val_, "std::expected::operator*: no value");
  }
  constexpr void value() const& {
    static_assert(is_copy_constructible_v<_Ep>, "std::expected::value: E must be copy constructible");
    if (!__has_val_)
      __throw_bad_access(__u_.__unex);
  }
  constexpr void value() && {
    static_assert(is_copy_constructible_v<_Ep> && is_move_constructible_v<_Ep>,
                  "std::expected::value: E must be copy and move constructible");
    if (!__has_val_)
      __throw_bad_access(static_cast<_Ep&&>(__u_.__unex));
  }

  // ---- [expected.void.monadic] ----
  // Four overloads each (not one explicit-object member): when an rvalue's && overload is not
  // viable, overload resolution must fall back to const && (LWG3877). The *_impl helpers take
  // the qualified expected type as Self; decltype((val)) / decltype(error()) is forward_like<Self>.
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<expected&, _Ep>>
  constexpr auto and_then(_Fp&& __f) & {
    return __and_then_impl<expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<const expected&, _Ep>>
  constexpr auto and_then(_Fp&& __f) const& {
    return __and_then_impl<const expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<expected, _Ep>>
  constexpr auto and_then(_Fp&& __f) && {
    return __and_then_impl<expected>(static_cast<expected&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<const expected, _Ep>>
  constexpr auto and_then(_Fp&& __f) const&& {
    return __and_then_impl<const expected>(static_cast<const expected&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto or_else(_Fp&& __f) & {
    return __or_else_impl<expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto or_else(_Fp&& __f) const& {
    return __or_else_impl<const expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto or_else(_Fp&& __f) && {
    return __or_else_impl<expected>(static_cast<expected&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto or_else(_Fp&& __f) const&& {
    return __or_else_impl<const expected>(static_cast<const expected&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<expected&, _Ep>>
  constexpr auto transform(_Fp&& __f) & {
    return __transform_impl<expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<const expected&, _Ep>>
  constexpr auto transform(_Fp&& __f) const& {
    return __transform_impl<const expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<expected, _Ep>>
  constexpr auto transform(_Fp&& __f) && {
    return __transform_impl<expected>(static_cast<expected&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires is_constructible_v<_Ep, __ycxx::__detail::__forward_like_t<const expected, _Ep>>
  constexpr auto transform(_Fp&& __f) const&& {
    return __transform_impl<const expected>(static_cast<const expected&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto transform_error(_Fp&& __f) & {
    return __transform_error_impl<expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto transform_error(_Fp&& __f) const& {
    return __transform_error_impl<const expected&>(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto transform_error(_Fp&& __f) && {
    return __transform_error_impl<expected>(static_cast<expected&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto transform_error(_Fp&& __f) const&& {
    return __transform_error_impl<const expected>(static_cast<const expected&&>(*this), static_cast<_Fp&&>(__f));
  }

  // ---- [expected.void.eq] ----
  template <class _T2, class _E2>
    requires is_void_v<_T2> && __ycxx::__detail::__eq_to_bool<_Ep, _E2>
  friend constexpr bool operator==(const expected& __x, const expected<_T2, _E2>& y) {
    if (__x.has_value() != y.has_value())
      return false;
    return __x.has_value() || __ycxx::__detail::__implicit_bool(__x.error() == y.error());
  }
  template <class _E2>
    requires __ycxx::__detail::__eq_to_bool<_Ep, _E2>
  friend constexpr bool operator==(const expected& __x, const unexpected<_E2>& e) {
    return !__x.has_value() && __ycxx::__detail::__implicit_bool(__x.error() == e.error());
  }

private:
  // Monadic operations ([expected.void.monadic]); Self is the qualified expected type.
  template <class _Self, class _Fp>
  static constexpr auto __and_then_impl(_Self&& s, _Fp&& __f) {
    using _Up = remove_cvref_t<invoke_result_t<_Fp>>;
    static_assert(__ycxx::__detail::__expected_with_error<_Up, _Ep>,
                  "std::expected::and_then: F must return a specialization of expected with the same error_type");
    if constexpr (!__ycxx::__detail::__expected_with_error<_Up, _Ep>)
      return; // no follow-on errors after the Mandates failure
    else {
      if (s.__has_val_)
        return ::__ycxx::__detail::invoke(static_cast<_Fp&&>(__f));
      return _Up(unexpect, std::forward_like<_Self>(s.__u_.__unex));
    }
  }
  template <class _Self, class _Fp>
  static constexpr auto __or_else_impl(_Self&& s, _Fp&& __f) {
    using _Gp = remove_cvref_t<invoke_result_t<_Fp, __ycxx::__detail::__forward_like_t<_Self, _Ep>>>;
    static_assert(__ycxx::__detail::__expected_with_value<_Gp, _Tp>,
                  "std::expected::or_else: F must return a specialization of expected with the same value_type");
    if constexpr (!__ycxx::__detail::__expected_with_value<_Gp, _Tp>)
      return;
    else {
      if (s.__has_val_)
        return _Gp();
      return ::__ycxx::__detail::invoke(static_cast<_Fp&&>(__f), std::forward_like<_Self>(s.__u_.__unex));
    }
  }
  template <class _Self, class _Fp>
  static constexpr auto __transform_impl(_Self&& s, _Fp&& __f) {
    using _Up = remove_cv_t<invoke_result_t<_Fp>>;
    static_assert(__ycxx::__detail::__valid_expected_value<_Up>, "std::expected::transform: invalid result type");
    if constexpr (!__ycxx::__detail::__valid_expected_value<_Up>)
      return;
    else {
      using _Rp = expected<_Up, _Ep>;
      if (!s.__has_val_)
        return _Rp(unexpect, std::forward_like<_Self>(s.__u_.__unex));
      if constexpr (is_void_v<_Up>) {
        ::__ycxx::__detail::invoke(static_cast<_Fp&&>(__f));
        return _Rp();
      } else {
        return _Rp(__ycxx::__detail::__expected_invoke_val_tag{}, static_cast<_Fp&&>(__f));
      }
    }
  }
  template <class _Self, class _Fp>
  static constexpr auto __transform_error_impl(_Self&& s, _Fp&& __f) {
    using _Gp = remove_cv_t<invoke_result_t<_Fp, __ycxx::__detail::__forward_like_t<_Self, _Ep>>>;
    static_assert(__ycxx::__detail::__valid_unexpected_arg<_Gp>, "std::expected::transform_error: invalid error type");
    if constexpr (!__ycxx::__detail::__valid_unexpected_arg<_Gp>)
      return;
    else {
      using _Rp = expected<_Tp, _Gp>;
      if (s.__has_val_)
        return _Rp();
      return _Rp(__ycxx::__detail::__expected_invoke_err_tag{}, static_cast<_Fp&&>(__f), std::forward_like<_Self>(s.__u_.__unex));
    }
  }

  template <class _EF>
  [[noreturn]] static constexpr void __throw_bad_access(_EF&& e) {
    __ycxx::__detail::__raise_with(ycxx_error_bad_expected_access, "std::bad_expected_access",
                             [&] { return bad_expected_access<_Ep>(static_cast<_EF&&>(e)); });
  }
};

} // namespace std
