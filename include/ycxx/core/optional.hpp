// libycxx core: <optional> ([optional]), including optional<T&> (C++26).
#pragma once

#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/utility_base.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/ranges_base.hpp>
#include <ycxx/core/range_access.hpp>
#include <ycxx/core/iterator_adaptors.hpp> // [iterator.range]/1: all of it, incl. rbegin/crend
#include <ycxx/core/format_kind.hpp>
#include <initializer_list>

namespace [[__gnu__::__visibility__("hidden")]] std {

// [optional.nullopt]: not default constructible, not an aggregate initialisable from {}.
struct nullopt_t {
  struct tag {
    explicit tag() = default;
  };
  constexpr explicit nullopt_t(tag) noexcept {}
  // [optional.nullopt]/2: models copyable and three_way_comparable<strong_ordering>.
  friend constexpr bool operator==(nullopt_t, nullopt_t) noexcept = default;
  friend constexpr strong_ordering operator<=>(nullopt_t, nullopt_t) noexcept = default;
};
inline constexpr nullopt_t nullopt{nullopt_t::tag{}};

// [optional.bad.access]
class bad_optional_access : public exception {
public:
  constexpr bad_optional_access() noexcept {}
  constexpr bad_optional_access(const bad_optional_access&) noexcept = default;
  constexpr bad_optional_access& operator=(const bad_optional_access&) noexcept = default;
  constexpr ~bad_optional_access() override {}
  constexpr const char* what() const noexcept override { return "bad optional access"; }
};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
[[noreturn]] [[__gnu__::__cold__]] constexpr void __throw_bad_optional_access() {
  ::__ycxx::__detail::__raise_with(ycxx_error_bad_optional_access, "std::bad_optional_access", [] { return std::bad_optional_access(); });
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp>
class optional;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Tp>
inline constexpr bool __is_optional = false;
template <class _Tp>
inline constexpr bool __is_optional<std::optional<_Tp>> = true;

// The Constraints of optional's equality operators, as default template arguments (see there).
template <bool _Cp>
struct __optional_check_t {};
template <>
struct __optional_check_t<true> {
  using type = void;
};
template <bool _Cp>
using __optional_check = typename __optional_check_t<_Cp>::type;
template <class _Tp, class _Up>
concept __optional_eq = requires(const _Tp& a, const _Up& b) {
  { a == b } -> std::convertible_to<bool>;
};
template <class _Tp, class _Up>
concept __optional_ne = requires(const _Tp& a, const _Up& b) {
  { a != b } -> std::convertible_to<bool>;
};

// optional<T&>::value_or's return type remove_cv_t<T>, named through U so that it is formed only
// when the member is used (never for the function and array types it is not declared for).
template <class _Tp, class _Up>
struct __optional_value_or {
  using type = std::remove_cv_t<_Tp>;
};

template <class _Tp>
concept __derived_from_optional = requires(const _Tp& t) { []<class _Up>(const std::optional<_Up>&) {}(t); };

template <class _Tp>
concept __valid_optional_type =
    (std::is_lvalue_reference_v<_Tp> || (std::is_object_v<_Tp> && !std::is_array_v<_Tp>)) &&
    !__is_same(std::remove_cvref_t<_Tp>, std::in_place_t) && !__is_same(std::remove_cvref_t<_Tp>, std::nullopt_t);

// Tag for constructing the contained value directly from an invocation (monadic transform).
struct __optional_invoke_tag {};

// Storage for optional<T>: a union with user-provided special members, so instantiating it never
// asks whether T is default constructible. (An anonymous union member would make Clang compute
// that while T may still be incomplete-in-context, e.g. a nested class whose default member
// initializers are parsed only when the enclosing class completes, and remember the answer.)
template <class _Sp>
union __optional_storage {
  char empty;
  _Sp __val;
  constexpr __optional_storage() noexcept : empty() {}
  template <class... _Args>
  constexpr explicit __optional_storage(std::in_place_t, _Args&&... __args) : __val(static_cast<_Args&&>(__args)...) {}
  template <class _Fp, class... _Args>
  constexpr __optional_storage(__optional_invoke_tag, _Fp&& __f, _Args&&... __args)
      : __val(::__ycxx::__detail::invoke(static_cast<_Fp&&>(__f), static_cast<_Args&&>(__args)...)) {}
  __optional_storage(const __optional_storage&) = default;
  __optional_storage(__optional_storage&&) = default;
  __optional_storage& operator=(const __optional_storage&) = default;
  __optional_storage& operator=(__optional_storage&&) = default;
  constexpr ~__optional_storage()
    requires std::is_trivially_destructible_v<_Sp>
  = default;
  constexpr ~__optional_storage() {}
};


}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp>
class optional {
  static_assert(__ycxx::__detail::__valid_optional_type<_Tp> && !is_reference_v<_Tp>,
                "optional<T>: T must be a complete non-array object type other than in_place_t/nullopt_t");
  static_assert(is_destructible_v<_Tp>, "optional<T>: T must be destructible");

  using __stored = remove_cv_t<_Tp>;
  __ycxx::__detail::__optional_storage<__stored> __u_;
  bool __engaged_;

  template <class _Up>
  friend class optional;

  template <class... _Args>
  constexpr void construct(_Args&&... __args) {
    std::construct_at(__builtin_addressof(__u_.__val), static_cast<_Args&&>(__args)...);
    __engaged_ = true;
  }
  constexpr void destroy() noexcept {
    if constexpr (!is_trivially_destructible_v<_Tp>) {
      __u_.__val.~__stored();
      // Clang 23 still takes the destroyed member for the union's active member during constant
      // evaluation (a constexpr optional<T> that was reset is "not initialized"), so 'empty' is
      // made active again.
      if consteval {
        std::construct_at(__builtin_addressof(__u_.empty));
      }
    }
    __engaged_ = false;
  }
  template <class _Opt>
  constexpr void __assign_from(_Opt&& __rhs) {
    if (__engaged_ && __rhs.has_value())
      static_cast<_Tp&>(__u_.__val) = *static_cast<_Opt&&>(__rhs);
    else if (__rhs.has_value())
      construct(*static_cast<_Opt&&>(__rhs));
    else if (__engaged_)
      destroy();
  }

public:
  using value_type = _Tp;
  using iterator = _Tp*;
  using const_iterator = const _Tp*;

  // ---- [optional.ctor] ----
  constexpr optional() noexcept : __u_(), __engaged_(false) {}
  constexpr optional(nullopt_t) noexcept : __u_(), __engaged_(false) {}

  constexpr optional(const optional&)
    requires is_copy_constructible_v<_Tp> && is_trivially_copy_constructible_v<_Tp>
  = default;
  constexpr optional(const optional& __rhs)
    requires is_copy_constructible_v<_Tp> && (!is_trivially_copy_constructible_v<_Tp>)
      : __u_(), __engaged_(false) {
    if (__rhs.__engaged_)
      construct(__rhs.__u_.__val);
  }
  // [optional.ctor]/7: "defined as deleted unless"; an explicitly deleted overload (rather than
  // none with satisfied constraints) keeps optional<T> trivially copyable on Clang.
  constexpr optional(const optional&)
    requires(!is_copy_constructible_v<_Tp>)
  = delete;
  constexpr optional(optional&&)
    requires is_move_constructible_v<_Tp> && is_trivially_move_constructible_v<_Tp>
  = default;
  constexpr optional(optional&& __rhs) noexcept(is_nothrow_move_constructible_v<_Tp>)
    requires is_move_constructible_v<_Tp> && (!is_trivially_move_constructible_v<_Tp>)
      : __u_(), __engaged_(false) {
    if (__rhs.__engaged_)
      construct(static_cast<__stored&&>(__rhs.__u_.__val));
  }

  template <class... _Args>
    requires is_constructible_v<_Tp, _Args...>
  constexpr explicit optional(in_place_t, _Args&&... __args) : __u_(in_place, static_cast<_Args&&>(__args)...), __engaged_(true) {}
  template <class _Up, class... _Args>
    requires is_constructible_v<_Tp, initializer_list<_Up>&, _Args...>
  constexpr explicit optional(in_place_t, initializer_list<_Up> il, _Args&&... __args)
      : __u_(in_place, il, static_cast<_Args&&>(__args)...), __engaged_(true) {}

  template <class _Up = remove_cv_t<_Tp>>
    requires(!is_same_v<remove_cvref_t<_Up>, in_place_t>) && (!is_same_v<remove_cvref_t<_Up>, optional>) &&
            (!is_same_v<remove_cv_t<_Tp>, bool> || !__ycxx::__detail::__is_optional<remove_cvref_t<_Up>>) &&
            is_constructible_v<_Tp, _Up>
  constexpr explicit(!is_convertible_v<_Up, _Tp>) optional(_Up&& __v) : __u_(in_place, static_cast<_Up&&>(__v)), __engaged_(true) {}

  // The converting members from optional<U> are never better than the copy and move members
  // when U is T; excluding that case first keeps the constraints from recursing for a T
  // constructible from anything (is_constructible_v<T, optional<T>&> would ask for them again).
  template <class _Up>
    requires(!is_same_v<_Up, _Tp>) && is_constructible_v<_Tp, const _Up&> &&
            (is_same_v<remove_cv_t<_Tp>, bool> || !__ycxx::__detail::__converts_from_any_cvref<_Tp, optional<_Up>>)
  constexpr explicit(!is_convertible_v<const _Up&, _Tp>) optional(const optional<_Up>& __rhs) : __u_(), __engaged_(false) {
    if (__rhs.has_value())
      construct(*__rhs);
  }
  template <class _Up>
    requires(!is_same_v<_Up, _Tp>) && is_constructible_v<_Tp, _Up> &&
            (is_same_v<remove_cv_t<_Tp>, bool> || !__ycxx::__detail::__converts_from_any_cvref<_Tp, optional<_Up>>)
  constexpr explicit(!is_convertible_v<_Up, _Tp>) optional(optional<_Up>&& __rhs) : __u_(), __engaged_(false) {
    if (__rhs.has_value())
      construct(*static_cast<optional<_Up>&&>(__rhs));
  }

private:
  template <class _Fp, class... _Args>
  constexpr optional(__ycxx::__detail::__optional_invoke_tag, _Fp&& __f, _Args&&... __args)
      : __u_(__ycxx::__detail::__optional_invoke_tag{}, static_cast<_Fp&&>(__f), static_cast<_Args&&>(__args)...), __engaged_(true) {}

public:

  // ---- [optional.dtor] ----
  constexpr ~optional()
    requires is_trivially_destructible_v<_Tp>
  = default;
  constexpr ~optional() {
    if (__engaged_)
      __u_.__val.~__stored();
  }

  // ---- [optional.assign] ----
  constexpr optional& operator=(nullopt_t) noexcept {
    reset();
    return *this;
  }
  constexpr optional& operator=(const optional&)
    requires is_copy_constructible_v<_Tp> && is_copy_assignable_v<_Tp> && is_trivially_copy_constructible_v<_Tp> &&
             is_trivially_copy_assignable_v<_Tp> && is_trivially_destructible_v<_Tp>
  = default;
  constexpr optional& operator=(const optional& __rhs)
    requires is_copy_constructible_v<_Tp> && is_copy_assignable_v<_Tp> &&
             (!(is_trivially_copy_constructible_v<_Tp> && is_trivially_copy_assignable_v<_Tp> &&
                is_trivially_destructible_v<_Tp>))
  {
    if (this != &__rhs)
      __assign_from(__rhs);
    return *this;
  }
  constexpr optional& operator=(const optional&)
    requires(!(is_copy_constructible_v<_Tp> && is_copy_assignable_v<_Tp>))
  = delete;
  constexpr optional& operator=(optional&&)
    requires is_move_constructible_v<_Tp> && is_move_assignable_v<_Tp> && is_trivially_move_constructible_v<_Tp> &&
             is_trivially_move_assignable_v<_Tp> && is_trivially_destructible_v<_Tp>
  = default;
  constexpr optional& operator=(optional&& __rhs) noexcept(is_nothrow_move_assignable_v<_Tp> &&
                                                         is_nothrow_move_constructible_v<_Tp>)
    requires is_move_constructible_v<_Tp> && is_move_assignable_v<_Tp> &&
             (!(is_trivially_move_constructible_v<_Tp> && is_trivially_move_assignable_v<_Tp> &&
                is_trivially_destructible_v<_Tp>))
  {
    __assign_from(static_cast<optional&&>(__rhs));
    return *this;
  }

  template <class _Up = remove_cv_t<_Tp>>
    requires(!is_same_v<remove_cvref_t<_Up>, optional>) && (!(is_scalar_v<_Tp> && is_same_v<_Tp, decay_t<_Up>>)) &&
            is_constructible_v<_Tp, _Up> && is_assignable_v<_Tp&, _Up>
  constexpr optional& operator=(_Up&& __v) {
    if (__engaged_)
      static_cast<_Tp&>(__u_.__val) = static_cast<_Up&&>(__v); // *val is an lvalue of type T, maybe const
    else
      construct(static_cast<_Up&&>(__v));
    return *this;
  }

  template <class _Up>
    requires(!is_same_v<_Up, _Tp>) && is_constructible_v<_Tp, const _Up&> && is_assignable_v<_Tp&, const _Up&> &&
             (!__ycxx::__detail::__converts_from_any_cvref<_Tp, optional<_Up>>) && (!is_assignable_v<_Tp&, optional<_Up>&>) &&
             (!is_assignable_v<_Tp&, optional<_Up> &&>) && (!is_assignable_v<_Tp&, const optional<_Up>&>) &&
             (!is_assignable_v<_Tp&, const optional<_Up> &&>)
  constexpr optional& operator=(const optional<_Up>& __rhs) {
    __assign_from(__rhs);
    return *this;
  }
  template <class _Up>
    requires(!is_same_v<_Up, _Tp>) && is_constructible_v<_Tp, _Up> && is_assignable_v<_Tp&, _Up> &&
             (!__ycxx::__detail::__converts_from_any_cvref<_Tp, optional<_Up>>) && (!is_assignable_v<_Tp&, optional<_Up>&>) &&
             (!is_assignable_v<_Tp&, optional<_Up> &&>) && (!is_assignable_v<_Tp&, const optional<_Up>&>) &&
             (!is_assignable_v<_Tp&, const optional<_Up> &&>)
  constexpr optional& operator=(optional<_Up>&& __rhs) {
    __assign_from(static_cast<optional<_Up>&&>(__rhs));
    return *this;
  }

  template <class... _Args>
    requires is_constructible_v<_Tp, _Args...>
  constexpr _Tp& emplace(_Args&&... __args) {
    reset();
    construct(static_cast<_Args&&>(__args)...);
    return __u_.__val;
  }
  template <class _Up, class... _Args>
    requires is_constructible_v<_Tp, initializer_list<_Up>&, _Args...>
  constexpr _Tp& emplace(initializer_list<_Up> il, _Args&&... __args) {
    reset();
    construct(il, static_cast<_Args&&>(__args)...);
    return __u_.__val;
  }

  // ---- [optional.swap] ----
  constexpr void swap(optional& __rhs) noexcept(is_nothrow_move_constructible_v<_Tp> && is_nothrow_swappable_v<_Tp>) {
    static_assert(is_move_constructible_v<_Tp>, "optional::swap: T must be move constructible");
    if (__engaged_ && __rhs.__engaged_) {
      __ycxx::__detail::__swap_adl::__do_swap(static_cast<_Tp&>(__u_.__val), static_cast<_Tp&>(__rhs.__u_.__val));
    } else if (__rhs.__engaged_) {
      construct(static_cast<__stored&&>(__rhs.__u_.__val));
      __rhs.destroy();
    } else if (__engaged_) {
      __rhs.construct(static_cast<__stored&&>(__u_.__val));
      destroy();
    }
  }

  // ---- [optional.iterators] ----
  constexpr iterator begin() noexcept { return __builtin_addressof(__u_.__val); }
  constexpr const_iterator begin() const noexcept { return __builtin_addressof(__u_.__val); }
  constexpr iterator end() noexcept { return begin() + __engaged_; }
  constexpr const_iterator end() const noexcept { return begin() + __engaged_; }

  // ---- [optional.observe] ----
  constexpr const _Tp* operator->() const noexcept {
    __ycxx::__detail::__precondition(__engaged_, "optional::operator->: no value");
    return __builtin_addressof(__u_.__val);
  }
  constexpr _Tp* operator->() noexcept {
    __ycxx::__detail::__precondition(__engaged_, "optional::operator->: no value");
    return __builtin_addressof(__u_.__val);
  }
  constexpr const _Tp& operator*() const& noexcept {
    __ycxx::__detail::__precondition(__engaged_, "optional::operator*: no value");
    return __u_.__val;
  }
  constexpr _Tp& operator*() & noexcept {
    __ycxx::__detail::__precondition(__engaged_, "optional::operator*: no value");
    return __u_.__val;
  }
  constexpr _Tp&& operator*() && noexcept {
    __ycxx::__detail::__precondition(__engaged_, "optional::operator*: no value");
    return static_cast<_Tp&&>(__u_.__val);
  }
  constexpr const _Tp&& operator*() const&& noexcept {
    __ycxx::__detail::__precondition(__engaged_, "optional::operator*: no value");
    return static_cast<const _Tp&&>(__u_.__val);
  }
  constexpr explicit operator bool() const noexcept { return __engaged_; }
  constexpr bool has_value() const noexcept { return __engaged_; }

  constexpr const _Tp& value() const& {
    if (!__engaged_)
      __ycxx::__detail::__throw_bad_optional_access();
    return __u_.__val;
  }
  constexpr _Tp& value() & {
    if (!__engaged_)
      __ycxx::__detail::__throw_bad_optional_access();
    return __u_.__val;
  }
  constexpr _Tp&& value() && {
    if (!__engaged_)
      __ycxx::__detail::__throw_bad_optional_access();
    return static_cast<_Tp&&>(__u_.__val);
  }
  constexpr const _Tp&& value() const&& {
    if (!__engaged_)
      __ycxx::__detail::__throw_bad_optional_access();
    return static_cast<const _Tp&&>(__u_.__val);
  }

  template <class _Up = remove_cv_t<_Tp>>
  constexpr _Tp value_or(_Up&& __v) const& {
    static_assert(is_copy_constructible_v<_Tp> && is_convertible_v<_Up&&, _Tp>, "optional::value_or: Mandates not met");
    return __engaged_ ? __u_.__val : static_cast<_Tp>(static_cast<_Up&&>(__v));
  }
  template <class _Up = remove_cv_t<_Tp>>
  constexpr _Tp value_or(_Up&& __v) && {
    static_assert(is_move_constructible_v<_Tp> && is_convertible_v<_Up&&, _Tp>, "optional::value_or: Mandates not met");
    return __engaged_ ? static_cast<_Tp&&>(__u_.__val) : static_cast<_Tp>(static_cast<_Up&&>(__v));
  }

  // ---- [optional.monadic] ----
  template <class _Fp>
  constexpr auto and_then(_Fp&& __f) & {
    return __and_then_impl(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto and_then(_Fp&& __f) && {
    return __and_then_impl(static_cast<optional&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto and_then(_Fp&& __f) const& {
    return __and_then_impl(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto and_then(_Fp&& __f) const&& {
    return __and_then_impl(static_cast<const optional&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto transform(_Fp&& __f) & {
    return __transform_impl(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto transform(_Fp&& __f) && {
    return __transform_impl(static_cast<optional&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto transform(_Fp&& __f) const& {
    return __transform_impl(*this, static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
  constexpr auto transform(_Fp&& __f) const&& {
    return __transform_impl(static_cast<const optional&&>(*this), static_cast<_Fp&&>(__f));
  }
  template <class _Fp>
    requires invocable<_Fp> && copy_constructible<_Tp>
  constexpr optional or_else(_Fp&& __f) const& {
    static_assert(is_same_v<remove_cvref_t<invoke_result_t<_Fp>>, optional>, "optional::or_else: F must return optional");
    if (__engaged_)
      return *this;
    return static_cast<_Fp&&>(__f)();
  }
  template <class _Fp>
    requires invocable<_Fp> && move_constructible<_Tp>
  constexpr optional or_else(_Fp&& __f) && {
    static_assert(is_same_v<remove_cvref_t<invoke_result_t<_Fp>>, optional>, "optional::or_else: F must return optional");
    if (__engaged_)
      return static_cast<optional&&>(*this);
    return static_cast<_Fp&&>(__f)();
  }

  // ---- [optional.mod] ----
  constexpr void reset() noexcept {
    if (__engaged_)
      destroy();
  }

private:
  template <class _Self, class _Fp>
  static constexpr auto __and_then_impl(_Self&& __self, _Fp&& __f) {
    using _Up = invoke_result_t<_Fp, decltype(*static_cast<_Self&&>(__self))>;
    static_assert(__ycxx::__detail::__is_optional<remove_cvref_t<_Up>>, "optional::and_then: F must return an optional");
    if (__self.__engaged_)
      return __ycxx::__detail::invoke(static_cast<_Fp&&>(__f), *static_cast<_Self&&>(__self));
    return remove_cvref_t<_Up>();
  }
  template <class _Self, class _Fp>
  static constexpr auto __transform_impl(_Self&& __self, _Fp&& __f) {
    using _Up = remove_cv_t<invoke_result_t<_Fp, decltype(*static_cast<_Self&&>(__self))>>;
    static_assert(__ycxx::__detail::__valid_optional_type<_Up>, "optional::transform: invalid result type");
    if (__self.__engaged_)
      return optional<_Up>(__ycxx::__detail::__optional_invoke_tag{}, static_cast<_Fp&&>(__f), *static_cast<_Self&&>(__self));
    return optional<_Up>();
  }
};

template <class _Tp>
optional(_Tp) -> optional<_Tp>;

// =============================================================================================
// optional<T&> ([optional.optional.ref])
// =============================================================================================
} // namespace std

// Base classes of std types live in __ycxx::__adl_free, a namespace that declares no functions:
// a base's namespace is an associated namespace for ADL ([basic.lookup.argdep]/3), so a
// __ycxx::__detail base would expose every internal function to lookup on the std type.
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// [optional.optional.ref.general]: optional<T&>::iterator exists only for object types other
// than arrays of unknown bound.
template <class _Tp>
struct __optional_ref_iterator {};
template <class _Tp>
  requires std::is_object_v<_Tp> && (!std::is_unbounded_array_v<_Tp>)
struct __optional_ref_iterator<_Tp> {
  using iterator = _Tp*;
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Tp>
class optional<_Tp&> : public __ycxx::__adl_free::__optional_ref_iterator<_Tp> {
  static_assert(__ycxx::__detail::__valid_optional_type<_Tp&>,
                "std::optional<T&>: remove_cvref_t<T> must not be in_place_t or nullopt_t");
  _Tp* __val_ = nullptr;

  template <class _Up>
  friend class optional;

  template <class _Up>
  constexpr void __convert_ref_init_val(_Up&& __u) {
    _Tp& r(static_cast<_Up&&>(__u));
    __val_ = __builtin_addressof(r);
  }

  template <class _Up>
  static constexpr bool __from_opt = !is_same_v<remove_cv_t<_Tp>, optional<_Up>> && !is_same_v<_Tp&, _Up>;

public:
  using value_type = _Tp;

  constexpr optional() noexcept = default;
  constexpr optional(nullopt_t) noexcept : optional() {}
  constexpr optional(const optional& __rhs) noexcept = default;

  template <class _Arg>
    requires is_constructible_v<_Tp&, _Arg> && (!reference_constructs_from_temporary_v<_Tp&, _Arg>)
  constexpr explicit optional(in_place_t, _Arg&& arg) {
    __convert_ref_init_val(static_cast<_Arg&&>(arg));
  }

  template <class _Up>
    requires(!is_same_v<remove_cvref_t<_Up>, optional>) && (!is_same_v<remove_cvref_t<_Up>, in_place_t>) &&
            is_constructible_v<_Tp&, _Up> && (!reference_constructs_from_temporary_v<_Tp&, _Up>)
  constexpr explicit(!is_convertible_v<_Up, _Tp&>) optional(_Up&& __u) noexcept(is_nothrow_constructible_v<_Tp&, _Up>) {
    __convert_ref_init_val(static_cast<_Up&&>(__u));
  }
  template <class _Up>
    requires(!is_same_v<remove_cvref_t<_Up>, optional>) && (!is_same_v<remove_cvref_t<_Up>, in_place_t>) &&
            is_constructible_v<_Tp&, _Up> && reference_constructs_from_temporary_v<_Tp&, _Up>
  constexpr explicit(!is_convertible_v<_Up, _Tp&>) optional(_Up&& __u) noexcept(is_nothrow_constructible_v<_Tp&, _Up>) = delete;

  template <class _Up>
    requires __from_opt<_Up> && is_constructible_v<_Tp&, _Up&> && (!reference_constructs_from_temporary_v<_Tp&, _Up&>)
  constexpr explicit(!is_convertible_v<_Up&, _Tp&>) optional(optional<_Up>& __rhs) noexcept(is_nothrow_constructible_v<_Tp&, _Up&>) {
    if (__rhs.has_value())
      __convert_ref_init_val(*__rhs);
  }
  template <class _Up>
    requires __from_opt<_Up> && is_constructible_v<_Tp&, _Up&> && reference_constructs_from_temporary_v<_Tp&, _Up&>
  constexpr explicit(!is_convertible_v<_Up&, _Tp&>) optional(optional<_Up>&) noexcept(is_nothrow_constructible_v<_Tp&, _Up&>) = delete;

  template <class _Up>
    requires __from_opt<_Up> && is_constructible_v<_Tp&, const _Up&> && (!reference_constructs_from_temporary_v<_Tp&, const _Up&>)
  constexpr explicit(!is_convertible_v<const _Up&, _Tp&>) optional(const optional<_Up>& __rhs) noexcept(
      is_nothrow_constructible_v<_Tp&, const _Up&>) {
    if (__rhs.has_value())
      __convert_ref_init_val(*__rhs);
  }
  template <class _Up>
    requires __from_opt<_Up> && is_constructible_v<_Tp&, const _Up&> && reference_constructs_from_temporary_v<_Tp&, const _Up&>
  constexpr explicit(!is_convertible_v<const _Up&, _Tp&>) optional(const optional<_Up>&) noexcept(
      is_nothrow_constructible_v<_Tp&, const _Up&>) = delete;

  template <class _Up>
    requires __from_opt<_Up> && is_constructible_v<_Tp&, _Up> && (!reference_constructs_from_temporary_v<_Tp&, _Up>)
  constexpr explicit(!is_convertible_v<_Up, _Tp&>) optional(optional<_Up>&& __rhs) noexcept(is_nothrow_constructible_v<_Tp&, _Up>) {
    if (__rhs.has_value())
      __convert_ref_init_val(*static_cast<optional<_Up>&&>(__rhs));
  }
  template <class _Up>
    requires __from_opt<_Up> && is_constructible_v<_Tp&, _Up> && reference_constructs_from_temporary_v<_Tp&, _Up>
  constexpr explicit(!is_convertible_v<_Up, _Tp&>) optional(optional<_Up>&&) noexcept(is_nothrow_constructible_v<_Tp&, _Up>) = delete;

  template <class _Up>
    requires __from_opt<_Up> && is_constructible_v<_Tp&, const _Up> && (!reference_constructs_from_temporary_v<_Tp&, const _Up>)
  constexpr explicit(!is_convertible_v<const _Up, _Tp&>) optional(const optional<_Up>&& __rhs) noexcept(
      is_nothrow_constructible_v<_Tp&, const _Up>) {
    if (__rhs.has_value())
      __convert_ref_init_val(*static_cast<const optional<_Up>&&>(__rhs));
  }
  template <class _Up>
    requires __from_opt<_Up> && is_constructible_v<_Tp&, const _Up> && reference_constructs_from_temporary_v<_Tp&, const _Up>
  constexpr explicit(!is_convertible_v<const _Up, _Tp&>) optional(const optional<_Up>&&) noexcept(
      is_nothrow_constructible_v<_Tp&, const _Up>) = delete;

private:
  template <class _Fp, class... _Args>
  constexpr optional(__ycxx::__detail::__optional_invoke_tag, _Fp&& __f, _Args&&... __args) {
    __convert_ref_init_val(__ycxx::__detail::invoke(static_cast<_Fp&&>(__f), static_cast<_Args&&>(__args)...));
  }

public:

  constexpr ~optional() = default;

  constexpr optional& operator=(nullopt_t) noexcept {
    __val_ = nullptr;
    return *this;
  }
  constexpr optional& operator=(const optional& __rhs) noexcept = default;

  template <class _Up>
    requires is_constructible_v<_Tp&, _Up> && (!reference_constructs_from_temporary_v<_Tp&, _Up>)
  constexpr _Tp& emplace(_Up&& __u) noexcept(is_nothrow_constructible_v<_Tp&, _Up>) {
    __convert_ref_init_val(static_cast<_Up&&>(__u));
    return *__val_;
  }

  constexpr void swap(optional& __rhs) noexcept {
    _Tp* __tmp = __val_;
    __val_ = __rhs.__val_;
    __rhs.__val_ = __tmp;
  }

  constexpr auto begin() const noexcept
    requires is_object_v<_Tp> && (!is_unbounded_array_v<_Tp>)
  {
    return static_cast<_Tp*>(__val_);
  }
  constexpr auto end() const noexcept
    requires is_object_v<_Tp> && (!is_unbounded_array_v<_Tp>)
  {
    return begin() + has_value();
  }

  constexpr _Tp* operator->() const noexcept {
    __ycxx::__detail::__precondition(__val_ != nullptr, "optional<T&>::operator->: no value");
    return __val_;
  }
  constexpr _Tp& operator*() const noexcept {
    __ycxx::__detail::__precondition(__val_ != nullptr, "optional<T&>::operator*: no value");
    return *__val_;
  }
  constexpr explicit operator bool() const noexcept { return __val_ != nullptr; }
  constexpr bool has_value() const noexcept { return __val_ != nullptr; }
  constexpr _Tp& value() const {
    if (!__val_)
      __ycxx::__detail::__throw_bad_optional_access();
    return *__val_;
  }
  // Not declared for array and non-object T ([optional.ref.observe]/12 leaves it unspecified).
  template <class _Up = remove_cv_t<_Tp>>
    requires is_object_v<_Tp> && (!is_array_v<_Tp>)
  constexpr typename __ycxx::__detail::__optional_value_or<_Tp, _Up>::type value_or(_Up&& __u) const {
    static_assert(is_constructible_v<remove_cv_t<_Tp>, _Tp&> && is_convertible_v<_Up, remove_cv_t<_Tp>>,
                  "optional<T&>::value_or: Mandates not met");
    return __val_ ? remove_cv_t<_Tp>(*__val_) : static_cast<remove_cv_t<_Tp>>(static_cast<_Up&&>(__u));
  }

  template <class _Fp>
  constexpr auto and_then(_Fp&& __f) const {
    using _Up = invoke_result_t<_Fp, _Tp&>;
    static_assert(__ycxx::__detail::__is_optional<remove_cvref_t<_Up>>, "optional::and_then: F must return an optional");
    if (__val_)
      return __ycxx::__detail::invoke(static_cast<_Fp&&>(__f), *__val_);
    return remove_cvref_t<_Up>();
  }
  template <class _Fp>
  constexpr optional<remove_cv_t<invoke_result_t<_Fp, _Tp&>>> transform(_Fp&& __f) const {
    using _Up = remove_cv_t<invoke_result_t<_Fp, _Tp&>>;
    static_assert(__ycxx::__detail::__valid_optional_type<_Up>, "optional::transform: invalid result type");
    if (__val_)
      return optional<_Up>(__ycxx::__detail::__optional_invoke_tag{}, static_cast<_Fp&&>(__f), *__val_);
    return optional<_Up>();
  }
  template <class _Fp>
    requires invocable<_Fp>
  constexpr optional or_else(_Fp&& __f) const {
    static_assert(is_same_v<remove_cvref_t<invoke_result_t<_Fp>>, optional>, "optional::or_else: F must return optional");
    if (__val_)
      return *__val_;
    return static_cast<_Fp&&>(__f)();
  }

  constexpr void reset() noexcept { __val_ = nullptr; }
};

template <class _Tp>
constexpr bool ranges::enable_view<optional<_Tp>> = true;
template <class _Tp>
constexpr range_format format_kind<optional<_Tp>> = range_format::disabled;
template <class _Tp>
constexpr bool ranges::enable_borrowed_range<optional<_Tp&>> = true;

// ---- [optional.relops] ----
// The equality operators: each operator!= corresponds to its operator== ([basic.scope.scope]/4:
// the same template-head, parameters and return type), so no operator== is a rewrite target
// ([over.match.oper]/4) and `__x != y` and the reversed `y == __x` are not formed from it, as the
// draft's declarations of both with one signature intend. Their differing Constraints are
// therefore checked in default template arguments (substituted in order, so a later check is
// not reached when an earlier one fails).
template <class _Tp, class _Up, class = __ycxx::__detail::__optional_check<__ycxx::__detail::__optional_eq<_Tp, _Up>>>
constexpr bool operator==(const optional<_Tp>& __x, const optional<_Up>& y) {
  if (__x.has_value() != y.has_value())
    return false;
  return !__x.has_value() || static_cast<bool>(*__x == *y);
}
template <class _Tp, class _Up, class = __ycxx::__detail::__optional_check<__ycxx::__detail::__optional_ne<_Tp, _Up>>>
constexpr bool operator!=(const optional<_Tp>& __x, const optional<_Up>& y) {
  if (__x.has_value() != y.has_value())
    return true;
  return __x.has_value() && static_cast<bool>(*__x != *y);
}
template <class _Tp, class _Up>
  requires requires(const _Tp& a, const _Up& b) {
    { a < b } -> convertible_to<bool>;
  }
constexpr bool operator<(const optional<_Tp>& __x, const optional<_Up>& y) {
  if (!y)
    return false;
  if (!__x)
    return true;
  return static_cast<bool>(*__x < *y);
}
template <class _Tp, class _Up>
  requires requires(const _Tp& a, const _Up& b) {
    { a > b } -> convertible_to<bool>;
  }
constexpr bool operator>(const optional<_Tp>& __x, const optional<_Up>& y) {
  if (!__x)
    return false;
  if (!y)
    return true;
  return static_cast<bool>(*__x > *y);
}
template <class _Tp, class _Up>
  requires requires(const _Tp& a, const _Up& b) {
    { a <= b } -> convertible_to<bool>;
  }
constexpr bool operator<=(const optional<_Tp>& __x, const optional<_Up>& y) {
  if (!__x)
    return true;
  if (!y)
    return false;
  return static_cast<bool>(*__x <= *y);
}
template <class _Tp, class _Up>
  requires requires(const _Tp& a, const _Up& b) {
    { a >= b } -> convertible_to<bool>;
  }
constexpr bool operator>=(const optional<_Tp>& __x, const optional<_Up>& y) {
  if (!y)
    return true;
  if (!__x)
    return false;
  return static_cast<bool>(*__x >= *y);
}
template <class _Tp, three_way_comparable_with<_Tp> _Up>
constexpr compare_three_way_result_t<_Tp, _Up> operator<=>(const optional<_Tp>& __x, const optional<_Up>& y) {
  if (__x && y)
    return *__x <=> *y;
  return __x.has_value() <=> y.has_value();
}

// ---- [optional.nullops] ----
template <class _Tp>
constexpr bool operator==(const optional<_Tp>& __x, nullopt_t) noexcept {
  return !__x;
}
template <class _Tp>
constexpr strong_ordering operator<=>(const optional<_Tp>& __x, nullopt_t) noexcept {
  return __x.has_value() <=> false;
}

// ---- [optional.comp.with.t] ----
template <class _Tp, class _Up, class = __ycxx::__detail::__optional_check<!__ycxx::__detail::__is_optional<_Up>>,
          class = __ycxx::__detail::__optional_check<__ycxx::__detail::__optional_eq<_Tp, _Up>>>
constexpr bool operator==(const optional<_Tp>& __x, const _Up& __v) {
  return __x.has_value() ? static_cast<bool>(*__x == __v) : false;
}
template <class _Tp, class _Up, class = __ycxx::__detail::__optional_check<!__ycxx::__detail::__is_optional<_Tp>>,
          class = __ycxx::__detail::__optional_check<__ycxx::__detail::__optional_eq<_Tp, _Up>>>
constexpr bool operator==(const _Tp& __v, const optional<_Up>& __x) {
  return __x.has_value() ? static_cast<bool>(__v == *__x) : false;
}
template <class _Tp, class _Up, class = __ycxx::__detail::__optional_check<!__ycxx::__detail::__is_optional<_Up>>,
          class = __ycxx::__detail::__optional_check<__ycxx::__detail::__optional_ne<_Tp, _Up>>>
constexpr bool operator!=(const optional<_Tp>& __x, const _Up& __v) {
  return __x.has_value() ? static_cast<bool>(*__x != __v) : true;
}
template <class _Tp, class _Up, class = __ycxx::__detail::__optional_check<!__ycxx::__detail::__is_optional<_Tp>>,
          class = __ycxx::__detail::__optional_check<__ycxx::__detail::__optional_ne<_Tp, _Up>>>
constexpr bool operator!=(const _Tp& __v, const optional<_Up>& __x) {
  return __x.has_value() ? static_cast<bool>(__v != *__x) : true;
}
template <class _Tp, class _Up>
  requires(!__ycxx::__detail::__is_optional<_Up>) && requires(const _Tp& a, const _Up& b) {
    { a < b } -> convertible_to<bool>;
  }
constexpr bool operator<(const optional<_Tp>& __x, const _Up& __v) {
  return __x.has_value() ? static_cast<bool>(*__x < __v) : true;
}
template <class _Tp, class _Up>
  requires(!__ycxx::__detail::__is_optional<_Tp>) && requires(const _Tp& a, const _Up& b) {
    { a < b } -> convertible_to<bool>;
  }
constexpr bool operator<(const _Tp& __v, const optional<_Up>& __x) {
  return __x.has_value() ? static_cast<bool>(__v < *__x) : false;
}
template <class _Tp, class _Up>
  requires(!__ycxx::__detail::__is_optional<_Up>) && requires(const _Tp& a, const _Up& b) {
    { a > b } -> convertible_to<bool>;
  }
constexpr bool operator>(const optional<_Tp>& __x, const _Up& __v) {
  return __x.has_value() ? static_cast<bool>(*__x > __v) : false;
}
template <class _Tp, class _Up>
  requires(!__ycxx::__detail::__is_optional<_Tp>) && requires(const _Tp& a, const _Up& b) {
    { a > b } -> convertible_to<bool>;
  }
constexpr bool operator>(const _Tp& __v, const optional<_Up>& __x) {
  return __x.has_value() ? static_cast<bool>(__v > *__x) : true;
}
template <class _Tp, class _Up>
  requires(!__ycxx::__detail::__is_optional<_Up>) && requires(const _Tp& a, const _Up& b) {
    { a <= b } -> convertible_to<bool>;
  }
constexpr bool operator<=(const optional<_Tp>& __x, const _Up& __v) {
  return __x.has_value() ? static_cast<bool>(*__x <= __v) : true;
}
template <class _Tp, class _Up>
  requires(!__ycxx::__detail::__is_optional<_Tp>) && requires(const _Tp& a, const _Up& b) {
    { a <= b } -> convertible_to<bool>;
  }
constexpr bool operator<=(const _Tp& __v, const optional<_Up>& __x) {
  return __x.has_value() ? static_cast<bool>(__v <= *__x) : false;
}
template <class _Tp, class _Up>
  requires(!__ycxx::__detail::__is_optional<_Up>) && requires(const _Tp& a, const _Up& b) {
    { a >= b } -> convertible_to<bool>;
  }
constexpr bool operator>=(const optional<_Tp>& __x, const _Up& __v) {
  return __x.has_value() ? static_cast<bool>(*__x >= __v) : false;
}
template <class _Tp, class _Up>
  requires(!__ycxx::__detail::__is_optional<_Tp>) && requires(const _Tp& a, const _Up& b) {
    { a >= b } -> convertible_to<bool>;
  }
constexpr bool operator>=(const _Tp& __v, const optional<_Up>& __x) {
  return __x.has_value() ? static_cast<bool>(__v >= *__x) : true;
}
// three_way_comparable<U> is part of three_way_comparable_with<T, U>; testing it first rejects
// a U without <=> before T's own comparisons are examined, which can depend on this operator.
template <class _Tp, class _Up>
  requires(!__ycxx::__detail::__derived_from_optional<_Up>) && three_way_comparable<_Up> && three_way_comparable_with<_Tp, _Up>
constexpr compare_three_way_result_t<_Tp, _Up> operator<=>(const optional<_Tp>& __x, const _Up& __v) {
  return __x.has_value() ? *__x <=> __v : strong_ordering::less;
}

// ---- [optional.specalg] ----
template <class _Tp>
  requires(is_reference_v<_Tp> || (is_move_constructible_v<_Tp> && is_swappable_v<_Tp>))
constexpr void swap(optional<_Tp>& __x, optional<_Tp>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}
// [optional.specalg]/3: not viable when called with an explicit template argument list beginning
// with a type; such an argument cannot match the leading int&... pack.
template <int&..., class _Tp>
constexpr optional<decay_t<_Tp>> make_optional(_Tp&& __v) {
  return optional<decay_t<_Tp>>(static_cast<_Tp&&>(__v));
}
template <class _Tp, class... _Args>
constexpr optional<_Tp> make_optional(_Args&&... __args) {
  return optional<_Tp>(in_place, static_cast<_Args&&>(__args)...);
}
template <class _Tp, class _Up, class... _Args>
constexpr optional<_Tp> make_optional(initializer_list<_Up> il, _Args&&... __args) {
  return optional<_Tp>(in_place, il, static_cast<_Args&&>(__args)...);
}

// ---- [optional.hash] ----
template <class _Tp>
  requires __ycxx::__detail::__hash_enabled<remove_const_t<_Tp>>
struct hash<optional<_Tp>> {
  constexpr size_t operator()(const optional<_Tp>& __o) const
      noexcept(noexcept(hash<remove_const_t<_Tp>>{}(declval<const remove_const_t<_Tp>&>()))) {
    return __o ? hash<remove_const_t<_Tp>>{}(*__o) : static_cast<size_t>(0x6e756c6cu);
  }
};

} // namespace std
