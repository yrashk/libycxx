// libycxx core: <optional> ([optional]), including optional<T&> (C++26).
#pragma once

#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/utility_base.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/ranges_base.hpp>
#include <ycxx/core/range_access.hpp>
#include <initializer_list>

namespace std {

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
  bad_optional_access() noexcept = default;
  bad_optional_access(const bad_optional_access&) noexcept = default;
  bad_optional_access& operator=(const bad_optional_access&) noexcept = default;
  const char* what() const noexcept override { return "bad optional access"; }
};

template <class T>
class optional;

} // namespace std

namespace ycxx::detail {

template <class T>
inline constexpr bool is_optional = false;
template <class T>
inline constexpr bool is_optional<std::optional<T>> = true;

template <class T>
concept derived_from_optional = requires(const T& t) { []<class U>(const std::optional<U>&) {}(t); };

template <class T>
concept valid_optional_type =
    (std::is_lvalue_reference_v<T> || (std::is_object_v<T> && !std::is_array_v<T>)) &&
    !__is_same(std::remove_cvref_t<T>, std::in_place_t) && !__is_same(std::remove_cvref_t<T>, std::nullopt_t);

// Tag for constructing the contained value directly from an invocation (monadic transform).
struct optional_invoke_tag {};

// Storage for optional<T>: a union with user-provided special members, so instantiating it never
// asks whether T is default constructible. (An anonymous union member would make Clang compute
// that while T may still be incomplete-in-context, e.g. a nested class whose default member
// initializers are parsed only when the enclosing class completes, and remember the answer.)
template <class S>
union optional_storage {
  char empty;
  S val;
  constexpr optional_storage() noexcept : empty() {}
  template <class... Args>
  constexpr explicit optional_storage(std::in_place_t, Args&&... args) : val(static_cast<Args&&>(args)...) {}
  template <class F, class... Args>
  constexpr optional_storage(optional_invoke_tag, F&& f, Args&&... args)
      : val(::ycxx::detail::invoke(static_cast<F&&>(f), static_cast<Args&&>(args)...)) {}
  optional_storage(const optional_storage&) = default;
  optional_storage(optional_storage&&) = default;
  optional_storage& operator=(const optional_storage&) = default;
  optional_storage& operator=(optional_storage&&) = default;
  constexpr ~optional_storage()
    requires std::is_trivially_destructible_v<S>
  = default;
  constexpr ~optional_storage() {}
};


} // namespace ycxx::detail

namespace std {

template <class T>
class optional {
  static_assert(ycxx::detail::valid_optional_type<T> && !is_reference_v<T>,
                "optional<T>: T must be a complete non-array object type other than in_place_t/nullopt_t");
  static_assert(is_destructible_v<T>, "optional<T>: T must be destructible");

  using stored = remove_cv_t<T>;
  ycxx::detail::optional_storage<stored> u_;
  bool engaged_;

  template <class U>
  friend class optional;

  template <class... Args>
  constexpr void construct(Args&&... args) {
    std::construct_at(__builtin_addressof(u_.val), static_cast<Args&&>(args)...);
    engaged_ = true;
  }
  constexpr void destroy() noexcept {
    if constexpr (!is_trivially_destructible_v<T>)
      u_.val.~stored();
    engaged_ = false;
  }
  template <class Opt>
  constexpr void assign_from(Opt&& rhs) {
    if (engaged_ && rhs.has_value())
      u_.val = *static_cast<Opt&&>(rhs);
    else if (rhs.has_value())
      construct(*static_cast<Opt&&>(rhs));
    else if (engaged_)
      destroy();
  }

public:
  using value_type = T;
  using iterator = T*;
  using const_iterator = const T*;

  // ---- [optional.ctor] ----
  constexpr optional() noexcept : u_(), engaged_(false) {}
  constexpr optional(nullopt_t) noexcept : u_(), engaged_(false) {}

  constexpr optional(const optional&)
    requires is_copy_constructible_v<T> && is_trivially_copy_constructible_v<T>
  = default;
  constexpr optional(const optional& rhs)
    requires is_copy_constructible_v<T> && (!is_trivially_copy_constructible_v<T>)
      : u_(), engaged_(false) {
    if (rhs.engaged_)
      construct(rhs.u_.val);
  }
  // [optional.ctor]/7: "defined as deleted unless"; an explicitly deleted overload (rather than
  // none with satisfied constraints) keeps optional<T> trivially copyable on Clang.
  constexpr optional(const optional&)
    requires(!is_copy_constructible_v<T>)
  = delete;
  constexpr optional(optional&&)
    requires is_move_constructible_v<T> && is_trivially_move_constructible_v<T>
  = default;
  constexpr optional(optional&& rhs) noexcept(is_nothrow_move_constructible_v<T>)
    requires is_move_constructible_v<T> && (!is_trivially_move_constructible_v<T>)
      : u_(), engaged_(false) {
    if (rhs.engaged_)
      construct(static_cast<stored&&>(rhs.u_.val));
  }

  template <class... Args>
    requires is_constructible_v<T, Args...>
  constexpr explicit optional(in_place_t, Args&&... args) : u_(in_place, static_cast<Args&&>(args)...), engaged_(true) {}
  template <class U, class... Args>
    requires is_constructible_v<T, initializer_list<U>&, Args...>
  constexpr explicit optional(in_place_t, initializer_list<U> il, Args&&... args)
      : u_(in_place, il, static_cast<Args&&>(args)...), engaged_(true) {}

  template <class U = remove_cv_t<T>>
    requires is_constructible_v<T, U> && (!is_same_v<remove_cvref_t<U>, in_place_t>) &&
             (!is_same_v<remove_cvref_t<U>, optional>) &&
             (!is_same_v<remove_cv_t<T>, bool> || !ycxx::detail::is_optional<remove_cvref_t<U>>)
  constexpr explicit(!is_convertible_v<U, T>) optional(U&& v) : u_(in_place, static_cast<U&&>(v)), engaged_(true) {}

  template <class U>
    requires is_constructible_v<T, const U&> &&
             (is_same_v<remove_cv_t<T>, bool> || !ycxx::detail::converts_from_any_cvref<T, optional<U>>)
  constexpr explicit(!is_convertible_v<const U&, T>) optional(const optional<U>& rhs) : u_(), engaged_(false) {
    if (rhs.has_value())
      construct(*rhs);
  }
  template <class U>
    requires is_constructible_v<T, U> &&
             (is_same_v<remove_cv_t<T>, bool> || !ycxx::detail::converts_from_any_cvref<T, optional<U>>)
  constexpr explicit(!is_convertible_v<U, T>) optional(optional<U>&& rhs) : u_(), engaged_(false) {
    if (rhs.has_value())
      construct(*static_cast<optional<U>&&>(rhs));
  }

private:
  template <class F, class... Args>
  constexpr optional(ycxx::detail::optional_invoke_tag, F&& f, Args&&... args)
      : u_(ycxx::detail::optional_invoke_tag{}, static_cast<F&&>(f), static_cast<Args&&>(args)...), engaged_(true) {}

public:

  // ---- [optional.dtor] ----
  constexpr ~optional()
    requires is_trivially_destructible_v<T>
  = default;
  constexpr ~optional() {
    if (engaged_)
      u_.val.~stored();
  }

  // ---- [optional.assign] ----
  constexpr optional& operator=(nullopt_t) noexcept {
    reset();
    return *this;
  }
  constexpr optional& operator=(const optional&)
    requires is_copy_constructible_v<T> && is_copy_assignable_v<T> && is_trivially_copy_constructible_v<T> &&
             is_trivially_copy_assignable_v<T> && is_trivially_destructible_v<T>
  = default;
  constexpr optional& operator=(const optional& rhs)
    requires is_copy_constructible_v<T> && is_copy_assignable_v<T> &&
             (!(is_trivially_copy_constructible_v<T> && is_trivially_copy_assignable_v<T> &&
                is_trivially_destructible_v<T>))
  {
    if (this != &rhs)
      assign_from(rhs);
    return *this;
  }
  constexpr optional& operator=(const optional&)
    requires(!(is_copy_constructible_v<T> && is_copy_assignable_v<T>))
  = delete;
  constexpr optional& operator=(optional&&)
    requires is_move_constructible_v<T> && is_move_assignable_v<T> && is_trivially_move_constructible_v<T> &&
             is_trivially_move_assignable_v<T> && is_trivially_destructible_v<T>
  = default;
  constexpr optional& operator=(optional&& rhs) noexcept(is_nothrow_move_assignable_v<T> &&
                                                         is_nothrow_move_constructible_v<T>)
    requires is_move_constructible_v<T> && is_move_assignable_v<T> &&
             (!(is_trivially_move_constructible_v<T> && is_trivially_move_assignable_v<T> &&
                is_trivially_destructible_v<T>))
  {
    assign_from(static_cast<optional&&>(rhs));
    return *this;
  }

  template <class U = remove_cv_t<T>>
    requires(!is_same_v<remove_cvref_t<U>, optional>) && (!(is_scalar_v<T> && is_same_v<T, decay_t<U>>)) &&
            is_constructible_v<T, U> && is_assignable_v<T&, U>
  constexpr optional& operator=(U&& v) {
    if (engaged_)
      u_.val = static_cast<U&&>(v);
    else
      construct(static_cast<U&&>(v));
    return *this;
  }

  template <class U>
    requires is_constructible_v<T, const U&> && is_assignable_v<T&, const U&> &&
             (!ycxx::detail::converts_from_any_cvref<T, optional<U>>) && (!is_assignable_v<T&, optional<U>&>) &&
             (!is_assignable_v<T&, optional<U> &&>) && (!is_assignable_v<T&, const optional<U>&>) &&
             (!is_assignable_v<T&, const optional<U> &&>)
  constexpr optional& operator=(const optional<U>& rhs) {
    assign_from(rhs);
    return *this;
  }
  template <class U>
    requires is_constructible_v<T, U> && is_assignable_v<T&, U> &&
             (!ycxx::detail::converts_from_any_cvref<T, optional<U>>) && (!is_assignable_v<T&, optional<U>&>) &&
             (!is_assignable_v<T&, optional<U> &&>) && (!is_assignable_v<T&, const optional<U>&>) &&
             (!is_assignable_v<T&, const optional<U> &&>)
  constexpr optional& operator=(optional<U>&& rhs) {
    assign_from(static_cast<optional<U>&&>(rhs));
    return *this;
  }

  template <class... Args>
    requires is_constructible_v<T, Args...>
  constexpr T& emplace(Args&&... args) {
    reset();
    construct(static_cast<Args&&>(args)...);
    return u_.val;
  }
  template <class U, class... Args>
    requires is_constructible_v<T, initializer_list<U>&, Args...>
  constexpr T& emplace(initializer_list<U> il, Args&&... args) {
    reset();
    construct(il, static_cast<Args&&>(args)...);
    return u_.val;
  }

  // ---- [optional.swap] ----
  constexpr void swap(optional& rhs) noexcept(is_nothrow_move_constructible_v<T> && is_nothrow_swappable_v<T>) {
    static_assert(is_move_constructible_v<T>, "optional::swap: T must be move constructible");
    if (engaged_ && rhs.engaged_) {
      ycxx::detail::swap_adl::do_swap(u_.val, rhs.u_.val);
    } else if (rhs.engaged_) {
      construct(static_cast<stored&&>(rhs.u_.val));
      rhs.destroy();
    } else if (engaged_) {
      rhs.construct(static_cast<stored&&>(u_.val));
      destroy();
    }
  }

  // ---- [optional.iterators] ----
  constexpr iterator begin() noexcept { return __builtin_addressof(u_.val); }
  constexpr const_iterator begin() const noexcept { return __builtin_addressof(u_.val); }
  constexpr iterator end() noexcept { return begin() + engaged_; }
  constexpr const_iterator end() const noexcept { return begin() + engaged_; }

  // ---- [optional.observe] ----
  constexpr const T* operator->() const noexcept {
    ycxx::detail::precondition(engaged_, "optional::operator->: no value");
    return __builtin_addressof(u_.val);
  }
  constexpr T* operator->() noexcept {
    ycxx::detail::precondition(engaged_, "optional::operator->: no value");
    return __builtin_addressof(u_.val);
  }
  constexpr const T& operator*() const& noexcept {
    ycxx::detail::precondition(engaged_, "optional::operator*: no value");
    return u_.val;
  }
  constexpr T& operator*() & noexcept {
    ycxx::detail::precondition(engaged_, "optional::operator*: no value");
    return u_.val;
  }
  constexpr T&& operator*() && noexcept {
    ycxx::detail::precondition(engaged_, "optional::operator*: no value");
    return static_cast<T&&>(u_.val);
  }
  constexpr const T&& operator*() const&& noexcept {
    ycxx::detail::precondition(engaged_, "optional::operator*: no value");
    return static_cast<const T&&>(u_.val);
  }
  constexpr explicit operator bool() const noexcept { return engaged_; }
  constexpr bool has_value() const noexcept { return engaged_; }

  constexpr const T& value() const& {
    if (!engaged_)
      ycxx::detail::throw_bad_optional_access();
    return u_.val;
  }
  constexpr T& value() & {
    if (!engaged_)
      ycxx::detail::throw_bad_optional_access();
    return u_.val;
  }
  constexpr T&& value() && {
    if (!engaged_)
      ycxx::detail::throw_bad_optional_access();
    return static_cast<T&&>(u_.val);
  }
  constexpr const T&& value() const&& {
    if (!engaged_)
      ycxx::detail::throw_bad_optional_access();
    return static_cast<const T&&>(u_.val);
  }

  template <class U = remove_cv_t<T>>
  constexpr T value_or(U&& v) const& {
    static_assert(is_copy_constructible_v<T> && is_convertible_v<U&&, T>, "optional::value_or: Mandates not met");
    return engaged_ ? u_.val : static_cast<T>(static_cast<U&&>(v));
  }
  template <class U = remove_cv_t<T>>
  constexpr T value_or(U&& v) && {
    static_assert(is_move_constructible_v<T> && is_convertible_v<U&&, T>, "optional::value_or: Mandates not met");
    return engaged_ ? static_cast<T&&>(u_.val) : static_cast<T>(static_cast<U&&>(v));
  }

  // ---- [optional.monadic] ----
  template <class F>
  constexpr auto and_then(F&& f) & {
    return and_then_impl(*this, static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto and_then(F&& f) && {
    return and_then_impl(static_cast<optional&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto and_then(F&& f) const& {
    return and_then_impl(*this, static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto and_then(F&& f) const&& {
    return and_then_impl(static_cast<const optional&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto transform(F&& f) & {
    return transform_impl(*this, static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto transform(F&& f) && {
    return transform_impl(static_cast<optional&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto transform(F&& f) const& {
    return transform_impl(*this, static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto transform(F&& f) const&& {
    return transform_impl(static_cast<const optional&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
    requires invocable<F> && copy_constructible<T>
  constexpr optional or_else(F&& f) const& {
    static_assert(is_same_v<remove_cvref_t<invoke_result_t<F>>, optional>, "optional::or_else: F must return optional");
    if (engaged_)
      return *this;
    return static_cast<F&&>(f)();
  }
  template <class F>
    requires invocable<F> && move_constructible<T>
  constexpr optional or_else(F&& f) && {
    static_assert(is_same_v<remove_cvref_t<invoke_result_t<F>>, optional>, "optional::or_else: F must return optional");
    if (engaged_)
      return static_cast<optional&&>(*this);
    return static_cast<F&&>(f)();
  }

  // ---- [optional.mod] ----
  constexpr void reset() noexcept {
    if (engaged_)
      destroy();
  }

private:
  template <class Self, class F>
  static constexpr auto and_then_impl(Self&& self, F&& f) {
    using U = invoke_result_t<F, decltype(*static_cast<Self&&>(self))>;
    static_assert(ycxx::detail::is_optional<remove_cvref_t<U>>, "optional::and_then: F must return an optional");
    if (self.engaged_)
      return ycxx::detail::invoke(static_cast<F&&>(f), *static_cast<Self&&>(self));
    return remove_cvref_t<U>();
  }
  template <class Self, class F>
  static constexpr auto transform_impl(Self&& self, F&& f) {
    using U = remove_cv_t<invoke_result_t<F, decltype(*static_cast<Self&&>(self))>>;
    static_assert(ycxx::detail::valid_optional_type<U>, "optional::transform: invalid result type");
    if (self.engaged_)
      return optional<U>(ycxx::detail::optional_invoke_tag{}, static_cast<F&&>(f), *static_cast<Self&&>(self));
    return optional<U>();
  }
};

template <class T>
optional(T) -> optional<T>;

// =============================================================================================
// optional<T&> ([optional.optional.ref])
// =============================================================================================
} // namespace std

// Base classes of std types live in ycxx::adl_free, a namespace that declares no functions:
// a base's namespace is an associated namespace for ADL ([basic.lookup.argdep]/3), so a
// ycxx::detail base would expose every internal function to lookup on the std type.
namespace ycxx::adl_free {
// [optional.optional.ref.general]: optional<T&>::iterator exists only for object types other
// than arrays of unknown bound.
template <class T>
struct optional_ref_iterator {};
template <class T>
  requires std::is_object_v<T> && (!std::is_unbounded_array_v<T>)
struct optional_ref_iterator<T> {
  using iterator = T*;
};
} // namespace ycxx::adl_free

namespace std {
template <class T>
class optional<T&> : public ycxx::adl_free::optional_ref_iterator<T> {
  static_assert(ycxx::detail::valid_optional_type<T&>,
                "std::optional<T&>: remove_cvref_t<T> must not be in_place_t or nullopt_t");
  T* val_ = nullptr;

  template <class U>
  friend class optional;

  template <class U>
  constexpr void convert_ref_init_val(U&& u) {
    T& r(static_cast<U&&>(u));
    val_ = __builtin_addressof(r);
  }

  template <class U>
  static constexpr bool from_opt = !is_same_v<remove_cv_t<T>, optional<U>> && !is_same_v<T&, U>;

public:
  using value_type = T;

  constexpr optional() noexcept = default;
  constexpr optional(nullopt_t) noexcept : optional() {}
  constexpr optional(const optional& rhs) noexcept = default;

  template <class Arg>
    requires is_constructible_v<T&, Arg> && (!reference_constructs_from_temporary_v<T&, Arg>)
  constexpr explicit optional(in_place_t, Arg&& arg) {
    convert_ref_init_val(static_cast<Arg&&>(arg));
  }

  template <class U>
    requires(!is_same_v<remove_cvref_t<U>, optional>) && (!is_same_v<remove_cvref_t<U>, in_place_t>) &&
            is_constructible_v<T&, U> && (!reference_constructs_from_temporary_v<T&, U>)
  constexpr explicit(!is_convertible_v<U, T&>) optional(U&& u) noexcept(is_nothrow_constructible_v<T&, U>) {
    convert_ref_init_val(static_cast<U&&>(u));
  }
  template <class U>
    requires(!is_same_v<remove_cvref_t<U>, optional>) && (!is_same_v<remove_cvref_t<U>, in_place_t>) &&
            is_constructible_v<T&, U> && reference_constructs_from_temporary_v<T&, U>
  constexpr explicit(!is_convertible_v<U, T&>) optional(U&& u) noexcept(is_nothrow_constructible_v<T&, U>) = delete;

  template <class U>
    requires from_opt<U> && is_constructible_v<T&, U&> && (!reference_constructs_from_temporary_v<T&, U&>)
  constexpr explicit(!is_convertible_v<U&, T&>) optional(optional<U>& rhs) noexcept(is_nothrow_constructible_v<T&, U&>) {
    if (rhs.has_value())
      convert_ref_init_val(*rhs);
  }
  template <class U>
    requires from_opt<U> && is_constructible_v<T&, U&> && reference_constructs_from_temporary_v<T&, U&>
  constexpr explicit(!is_convertible_v<U&, T&>) optional(optional<U>&) noexcept(is_nothrow_constructible_v<T&, U&>) = delete;

  template <class U>
    requires from_opt<U> && is_constructible_v<T&, const U&> && (!reference_constructs_from_temporary_v<T&, const U&>)
  constexpr explicit(!is_convertible_v<const U&, T&>) optional(const optional<U>& rhs) noexcept(
      is_nothrow_constructible_v<T&, const U&>) {
    if (rhs.has_value())
      convert_ref_init_val(*rhs);
  }
  template <class U>
    requires from_opt<U> && is_constructible_v<T&, const U&> && reference_constructs_from_temporary_v<T&, const U&>
  constexpr explicit(!is_convertible_v<const U&, T&>) optional(const optional<U>&) noexcept(
      is_nothrow_constructible_v<T&, const U&>) = delete;

  template <class U>
    requires from_opt<U> && is_constructible_v<T&, U> && (!reference_constructs_from_temporary_v<T&, U>)
  constexpr explicit(!is_convertible_v<U, T&>) optional(optional<U>&& rhs) noexcept(is_nothrow_constructible_v<T&, U>) {
    if (rhs.has_value())
      convert_ref_init_val(*static_cast<optional<U>&&>(rhs));
  }
  template <class U>
    requires from_opt<U> && is_constructible_v<T&, U> && reference_constructs_from_temporary_v<T&, U>
  constexpr explicit(!is_convertible_v<U, T&>) optional(optional<U>&&) noexcept(is_nothrow_constructible_v<T&, U>) = delete;

  template <class U>
    requires from_opt<U> && is_constructible_v<T&, const U> && (!reference_constructs_from_temporary_v<T&, const U>)
  constexpr explicit(!is_convertible_v<const U, T&>) optional(const optional<U>&& rhs) noexcept(
      is_nothrow_constructible_v<T&, const U>) {
    if (rhs.has_value())
      convert_ref_init_val(*static_cast<const optional<U>&&>(rhs));
  }
  template <class U>
    requires from_opt<U> && is_constructible_v<T&, const U> && reference_constructs_from_temporary_v<T&, const U>
  constexpr explicit(!is_convertible_v<const U, T&>) optional(const optional<U>&&) noexcept(
      is_nothrow_constructible_v<T&, const U>) = delete;

private:
  template <class F, class... Args>
  constexpr optional(ycxx::detail::optional_invoke_tag, F&& f, Args&&... args) {
    convert_ref_init_val(ycxx::detail::invoke(static_cast<F&&>(f), static_cast<Args&&>(args)...));
  }

public:

  constexpr ~optional() = default;

  constexpr optional& operator=(nullopt_t) noexcept {
    val_ = nullptr;
    return *this;
  }
  constexpr optional& operator=(const optional& rhs) noexcept = default;

  template <class U>
    requires is_constructible_v<T&, U> && (!reference_constructs_from_temporary_v<T&, U>)
  constexpr T& emplace(U&& u) noexcept(is_nothrow_constructible_v<T&, U>) {
    convert_ref_init_val(static_cast<U&&>(u));
    return *val_;
  }

  constexpr void swap(optional& rhs) noexcept {
    T* tmp = val_;
    val_ = rhs.val_;
    rhs.val_ = tmp;
  }

  constexpr auto begin() const noexcept
    requires is_object_v<T> && (!is_unbounded_array_v<T>)
  {
    return static_cast<T*>(val_);
  }
  constexpr auto end() const noexcept
    requires is_object_v<T> && (!is_unbounded_array_v<T>)
  {
    return begin() + has_value();
  }

  constexpr T* operator->() const noexcept {
    ycxx::detail::precondition(val_ != nullptr, "optional<T&>::operator->: no value");
    return val_;
  }
  constexpr T& operator*() const noexcept {
    ycxx::detail::precondition(val_ != nullptr, "optional<T&>::operator*: no value");
    return *val_;
  }
  constexpr explicit operator bool() const noexcept { return val_ != nullptr; }
  constexpr bool has_value() const noexcept { return val_ != nullptr; }
  constexpr T& value() const {
    if (!val_)
      ycxx::detail::throw_bad_optional_access();
    return *val_;
  }
  // Return type is unspecified for array and non-object T ([optional.ref.observe]/12).
  template <class U = remove_cv_t<T>>
    requires is_object_v<T> && (!is_array_v<T>)
  constexpr auto value_or(U&& u) const {
    static_assert(is_constructible_v<remove_cv_t<T>, T&> && is_convertible_v<U, remove_cv_t<T>>,
                  "optional<T&>::value_or: Mandates not met");
    return val_ ? remove_cv_t<T>(*val_) : static_cast<remove_cv_t<T>>(static_cast<U&&>(u));
  }

  template <class F>
  constexpr auto and_then(F&& f) const {
    using U = invoke_result_t<F, T&>;
    static_assert(ycxx::detail::is_optional<remove_cvref_t<U>>, "optional::and_then: F must return an optional");
    if (val_)
      return ycxx::detail::invoke(static_cast<F&&>(f), *val_);
    return remove_cvref_t<U>();
  }
  template <class F>
  constexpr optional<remove_cv_t<invoke_result_t<F, T&>>> transform(F&& f) const {
    using U = remove_cv_t<invoke_result_t<F, T&>>;
    static_assert(ycxx::detail::valid_optional_type<U>, "optional::transform: invalid result type");
    if (val_)
      return optional<U>(ycxx::detail::optional_invoke_tag{}, static_cast<F&&>(f), *val_);
    return optional<U>();
  }
  template <class F>
    requires invocable<F>
  constexpr optional or_else(F&& f) const {
    static_assert(is_same_v<remove_cvref_t<invoke_result_t<F>>, optional>, "optional::or_else: F must return optional");
    if (val_)
      return *val_;
    return static_cast<F&&>(f)();
  }

  constexpr void reset() noexcept { val_ = nullptr; }
};

template <class T>
constexpr bool ranges::enable_view<optional<T>> = true;
template <class T>
constexpr bool ranges::enable_borrowed_range<optional<T&>> = true;

// ---- [optional.relops] ----
template <class T, class U>
  requires requires(const T& a, const U& b) {
    { a == b } -> convertible_to<bool>;
  }
constexpr bool operator==(const optional<T>& x, const optional<U>& y) {
  if (x.has_value() != y.has_value())
    return false;
  return !x.has_value() || static_cast<bool>(*x == *y);
}
template <class T, class U>
  requires requires(const T& a, const U& b) {
    { a != b } -> convertible_to<bool>;
  }
constexpr bool operator!=(const optional<T>& x, const optional<U>& y) {
  if (x.has_value() != y.has_value())
    return true;
  return x.has_value() && static_cast<bool>(*x != *y);
}
template <class T, class U>
  requires requires(const T& a, const U& b) {
    { a < b } -> convertible_to<bool>;
  }
constexpr bool operator<(const optional<T>& x, const optional<U>& y) {
  if (!y)
    return false;
  if (!x)
    return true;
  return static_cast<bool>(*x < *y);
}
template <class T, class U>
  requires requires(const T& a, const U& b) {
    { a > b } -> convertible_to<bool>;
  }
constexpr bool operator>(const optional<T>& x, const optional<U>& y) {
  if (!x)
    return false;
  if (!y)
    return true;
  return static_cast<bool>(*x > *y);
}
template <class T, class U>
  requires requires(const T& a, const U& b) {
    { a <= b } -> convertible_to<bool>;
  }
constexpr bool operator<=(const optional<T>& x, const optional<U>& y) {
  if (!x)
    return true;
  if (!y)
    return false;
  return static_cast<bool>(*x <= *y);
}
template <class T, class U>
  requires requires(const T& a, const U& b) {
    { a >= b } -> convertible_to<bool>;
  }
constexpr bool operator>=(const optional<T>& x, const optional<U>& y) {
  if (!y)
    return true;
  if (!x)
    return false;
  return static_cast<bool>(*x >= *y);
}
template <class T, three_way_comparable_with<T> U>
constexpr compare_three_way_result_t<T, U> operator<=>(const optional<T>& x, const optional<U>& y) {
  if (x && y)
    return *x <=> *y;
  return x.has_value() <=> y.has_value();
}

// ---- [optional.nullops] ----
template <class T>
constexpr bool operator==(const optional<T>& x, nullopt_t) noexcept {
  return !x;
}
template <class T>
constexpr strong_ordering operator<=>(const optional<T>& x, nullopt_t) noexcept {
  return x.has_value() <=> false;
}

// ---- [optional.comp.with.t] ----
template <class T, class U>
  requires(!ycxx::detail::is_optional<U>) && requires(const T& a, const U& b) {
    { a == b } -> convertible_to<bool>;
  }
constexpr bool operator==(const optional<T>& x, const U& v) {
  return x.has_value() ? static_cast<bool>(*x == v) : false;
}
template <class T, class U>
  requires(!ycxx::detail::is_optional<T>) && requires(const T& a, const U& b) {
    { a == b } -> convertible_to<bool>;
  }
constexpr bool operator==(const T& v, const optional<U>& x) {
  return x.has_value() ? static_cast<bool>(v == *x) : false;
}
template <class T, class U>
  requires(!ycxx::detail::is_optional<U>) && requires(const T& a, const U& b) {
    { a != b } -> convertible_to<bool>;
  }
constexpr bool operator!=(const optional<T>& x, const U& v) {
  return x.has_value() ? static_cast<bool>(*x != v) : true;
}
template <class T, class U>
  requires(!ycxx::detail::is_optional<T>) && requires(const T& a, const U& b) {
    { a != b } -> convertible_to<bool>;
  }
constexpr bool operator!=(const T& v, const optional<U>& x) {
  return x.has_value() ? static_cast<bool>(v != *x) : true;
}
template <class T, class U>
  requires(!ycxx::detail::is_optional<U>) && requires(const T& a, const U& b) {
    { a < b } -> convertible_to<bool>;
  }
constexpr bool operator<(const optional<T>& x, const U& v) {
  return x.has_value() ? static_cast<bool>(*x < v) : true;
}
template <class T, class U>
  requires(!ycxx::detail::is_optional<T>) && requires(const T& a, const U& b) {
    { a < b } -> convertible_to<bool>;
  }
constexpr bool operator<(const T& v, const optional<U>& x) {
  return x.has_value() ? static_cast<bool>(v < *x) : false;
}
template <class T, class U>
  requires(!ycxx::detail::is_optional<U>) && requires(const T& a, const U& b) {
    { a > b } -> convertible_to<bool>;
  }
constexpr bool operator>(const optional<T>& x, const U& v) {
  return x.has_value() ? static_cast<bool>(*x > v) : false;
}
template <class T, class U>
  requires(!ycxx::detail::is_optional<T>) && requires(const T& a, const U& b) {
    { a > b } -> convertible_to<bool>;
  }
constexpr bool operator>(const T& v, const optional<U>& x) {
  return x.has_value() ? static_cast<bool>(v > *x) : true;
}
template <class T, class U>
  requires(!ycxx::detail::is_optional<U>) && requires(const T& a, const U& b) {
    { a <= b } -> convertible_to<bool>;
  }
constexpr bool operator<=(const optional<T>& x, const U& v) {
  return x.has_value() ? static_cast<bool>(*x <= v) : true;
}
template <class T, class U>
  requires(!ycxx::detail::is_optional<T>) && requires(const T& a, const U& b) {
    { a <= b } -> convertible_to<bool>;
  }
constexpr bool operator<=(const T& v, const optional<U>& x) {
  return x.has_value() ? static_cast<bool>(v <= *x) : false;
}
template <class T, class U>
  requires(!ycxx::detail::is_optional<U>) && requires(const T& a, const U& b) {
    { a >= b } -> convertible_to<bool>;
  }
constexpr bool operator>=(const optional<T>& x, const U& v) {
  return x.has_value() ? static_cast<bool>(*x >= v) : false;
}
template <class T, class U>
  requires(!ycxx::detail::is_optional<T>) && requires(const T& a, const U& b) {
    { a >= b } -> convertible_to<bool>;
  }
constexpr bool operator>=(const T& v, const optional<U>& x) {
  return x.has_value() ? static_cast<bool>(v >= *x) : true;
}
template <class T, class U>
  requires(!ycxx::detail::derived_from_optional<U>) && three_way_comparable_with<T, U>
constexpr compare_three_way_result_t<T, U> operator<=>(const optional<T>& x, const U& v) {
  return x.has_value() ? *x <=> v : strong_ordering::less;
}

// ---- [optional.specalg] ----
template <class T>
  requires(is_reference_v<T> || (is_move_constructible_v<T> && is_swappable_v<T>))
constexpr void swap(optional<T>& x, optional<T>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}
// [optional.specalg]/3: not viable when called with an explicit template argument list beginning
// with a type; such an argument cannot match the leading int&... pack.
template <int&..., class T>
constexpr optional<decay_t<T>> make_optional(T&& v) {
  return optional<decay_t<T>>(static_cast<T&&>(v));
}
template <class T, class... Args>
constexpr optional<T> make_optional(Args&&... args) {
  return optional<T>(in_place, static_cast<Args&&>(args)...);
}
template <class T, class U, class... Args>
constexpr optional<T> make_optional(initializer_list<U> il, Args&&... args) {
  return optional<T>(in_place, il, static_cast<Args&&>(args)...);
}

// ---- [optional.hash] ----
template <class T>
  requires ycxx::detail::hash_enabled<remove_const_t<T>>
struct hash<optional<T>> {
  constexpr size_t operator()(const optional<T>& o) const
      noexcept(noexcept(hash<remove_const_t<T>>{}(declval<const remove_const_t<T>&>()))) {
    return o ? hash<remove_const_t<T>>{}(*o) : static_cast<size_t>(0x6e756c6cu);
  }
};

} // namespace std
