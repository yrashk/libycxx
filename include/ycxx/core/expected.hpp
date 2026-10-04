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

namespace std {

// ---- [expected.unexpected] ----
template <class E>
class unexpected;

} // namespace std

namespace ycxx::detail {
template <class T>
inline constexpr bool is_unexpected = false;
template <class E>
inline constexpr bool is_unexpected<std::unexpected<E>> = true;

// [expected.un.general]/2: a valid template argument for unexpected.
template <class E>
concept valid_unexpected_arg =
    std::is_object_v<E> && !std::is_array_v<E> && !is_unexpected<E> && std::is_same_v<E, std::remove_cv_t<E>>;

// "a == b is well-formed and its result is convertible to bool": implicit conversion only
// (LWG4366), so results are converted by initialization, never static_cast.
template <class A, class B>
concept eq_to_bool = requires(const A& a, const B& b) { requires std::is_convertible_v<decltype(a == b), bool>; };
constexpr bool implicit_bool(bool b) noexcept { return b; }
} // namespace ycxx::detail

namespace std {

template <class E>
class unexpected {
  static_assert(ycxx::detail::valid_unexpected_arg<E>,
                "std::unexpected: E must be a non-array, non-cv object type that is not a specialization of unexpected");
  E unex_;

  template <class>
  friend class unexpected;

public:
  constexpr unexpected(const unexpected&) = default;
  constexpr unexpected(unexpected&&) = default;
  template <class Err = E>
    requires(!is_same_v<remove_cvref_t<Err>, unexpected>) && (!is_same_v<remove_cvref_t<Err>, in_place_t>) &&
            is_constructible_v<E, Err>
  constexpr explicit unexpected(Err&& e) : unex_(static_cast<Err&&>(e)) {}
  template <class... Args>
    requires is_constructible_v<E, Args...>
  constexpr explicit unexpected(in_place_t, Args&&... args) : unex_(static_cast<Args&&>(args)...) {}
  template <class U, class... Args>
    requires is_constructible_v<E, initializer_list<U>&, Args...>
  constexpr explicit unexpected(in_place_t, initializer_list<U> il, Args&&... args)
      : unex_(il, static_cast<Args&&>(args)...) {}

  constexpr unexpected& operator=(const unexpected&) = default;
  constexpr unexpected& operator=(unexpected&&) = default;

  constexpr const E& error() const& noexcept { return unex_; }
  constexpr E& error() & noexcept { return unex_; }
  constexpr const E&& error() const&& noexcept { return static_cast<const E&&>(unex_); }
  constexpr E&& error() && noexcept { return static_cast<E&&>(unex_); }

  constexpr void swap(unexpected& other) noexcept(is_nothrow_swappable_v<E>) {
    static_assert(is_swappable_v<E>, "std::unexpected::swap: E must be swappable");
    ycxx::detail::swap_adl::do_swap(unex_, other.unex_);
  }

  template <class E2>
  friend constexpr bool operator==(const unexpected& x, const unexpected<E2>& y) {
    static_assert(ycxx::detail::eq_to_bool<E, E2>,
                  "std::unexpected: x.error() == y.error() must be well-formed and convertible to bool");
    return ycxx::detail::implicit_bool(x.error() == y.error());
  }
  friend constexpr void swap(unexpected& x, unexpected& y) noexcept(noexcept(x.swap(y)))
    requires is_swappable_v<E>
  {
    x.swap(y);
  }
};

template <class E>
unexpected(E) -> unexpected<E>;

// ---- [expected.bad.void], [expected.bad] ----
template <class E>
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

template <class E>
class bad_expected_access : public bad_expected_access<void> {
  E unex_;

public:
  constexpr explicit bad_expected_access(E e) : unex_(static_cast<E&&>(e)) {}
  constexpr const char* what() const noexcept override { return "bad access to std::expected without a value"; }
  constexpr E& error() & noexcept { return unex_; }
  constexpr const E& error() const& noexcept { return unex_; }
  constexpr E&& error() && noexcept { return static_cast<E&&>(unex_); }
  constexpr const E&& error() const&& noexcept { return static_cast<const E&&>(unex_); }
};

struct unexpect_t {
  explicit unexpect_t() = default;
};
inline constexpr unexpect_t unexpect{};

template <class T, class E>
class expected;

} // namespace std

namespace ycxx::detail {

template <class T>
inline constexpr bool is_expected = false;
template <class T, class E>
inline constexpr bool is_expected<std::expected<T, E>> = true;

// [expected.object.general]/2
template <class T>
concept valid_expected_value =
    std::is_void_v<T> ||
    (std::is_object_v<T> && !std::is_array_v<T> && !std::is_same_v<std::remove_cv_t<T>, std::in_place_t> &&
     !std::is_same_v<std::remove_cv_t<T>, std::unexpect_t> && !is_unexpected<std::remove_cv_t<T>>);

// Stands in for the value member when T is void, so both class templates share one storage.
struct expected_void_value {};
template <class T>
using expected_value_t = std::conditional_t<std::is_void_v<T>, expected_void_value, std::remove_cv_t<T>>;

// Tags for building the value or the error directly from an invocation (monadic transforms),
// so non-movable results work.
struct expected_invoke_val_tag {};
struct expected_invoke_err_tag {};

template <class V, class E>
union expected_union {
  V val;
  E unex;

  template <class... Args>
  constexpr explicit expected_union(std::in_place_t, Args&&... args) : val(static_cast<Args&&>(args)...) {}
  template <class... Args>
  constexpr explicit expected_union(std::unexpect_t, Args&&... args) : unex(static_cast<Args&&>(args)...) {}
  template <class F, class... Args>
  constexpr expected_union(expected_invoke_val_tag, F&& f, Args&&... args)
      : val(::ycxx::detail::invoke(static_cast<F&&>(f), static_cast<Args&&>(args)...)) {}
  template <class F, class... Args>
  constexpr expected_union(expected_invoke_err_tag, F&& f, Args&&... args)
      : unex(::ycxx::detail::invoke(static_cast<F&&>(f), static_cast<Args&&>(args)...)) {}

  expected_union(const expected_union&) = default;
  expected_union(expected_union&&) = default;
  expected_union& operator=(const expected_union&) = default;
  expected_union& operator=(expected_union&&) = default;
  constexpr ~expected_union()
    requires std::is_trivially_destructible_v<V> && std::is_trivially_destructible_v<E>
  = default;
  constexpr ~expected_union() {}
};

// reinit-expected ([expected.object.assign]/1)
template <class T, class U, class... Args>
constexpr void reinit_expected(T& newval, U& oldval, Args&&... args) {
  if constexpr (std::is_nothrow_constructible_v<T, Args...>) {
    std::destroy_at(__builtin_addressof(oldval));
    std::construct_at(__builtin_addressof(newval), static_cast<Args&&>(args)...);
  } else if constexpr (std::is_nothrow_move_constructible_v<T>) {
    T tmp(static_cast<Args&&>(args)...);
    std::destroy_at(__builtin_addressof(oldval));
    std::construct_at(__builtin_addressof(newval), static_cast<T&&>(tmp));
  } else {
    U tmp(static_cast<U&&>(oldval));
    std::destroy_at(__builtin_addressof(oldval));
    if constexpr (cfg::exceptions) {
      try {
        std::construct_at(__builtin_addressof(newval), static_cast<Args&&>(args)...);
      } catch (...) {
        std::construct_at(__builtin_addressof(oldval), static_cast<U&&>(tmp));
        throw;
      }
    } else {
      std::construct_at(__builtin_addressof(newval), static_cast<Args&&>(args)...);
    }
  }
}

// Constraints shared by the converting constructors from expected<U, G>
// ([expected.object.cons]/18, [expected.void.cons]/13). UF/GF carry the source qualification.
template <class E, class U, class G>
concept expected_unexpected_not_from =
    !std::is_constructible_v<std::unexpected<E>, std::expected<U, G>&> &&
    !std::is_constructible_v<std::unexpected<E>, std::expected<U, G>> &&
    !std::is_constructible_v<std::unexpected<E>, const std::expected<U, G>&> &&
    !std::is_constructible_v<std::unexpected<E>, const std::expected<U, G>>;
// The same-type case is handled by the copy/move constructors; excluding it first keeps their
// own constructibility checks from re-entering this concept.
template <class T, class E, class U, class G, class UF, class GF>
concept expected_converts_from =
    !(std::is_same_v<T, U> && std::is_same_v<E, G>) && std::is_constructible_v<T, UF> && std::is_constructible_v<E, GF> &&
    (std::is_same_v<std::remove_cv_t<T>, bool> || !converts_from_any_cvref<T, std::expected<U, G>>) &&
    expected_unexpected_not_from<E, U, G>;
template <class T, class E, class U, class G, class GF>
concept expected_void_converts_from = !(std::is_same_v<T, U> && std::is_same_v<E, G>) && std::is_constructible_v<E, GF> &&
                                      expected_unexpected_not_from<E, U, G>;

// Common machinery of expected<T, E> and expected<void, E>: storage, lifetime, assignment.
template <class T, class E>
class expected_base {
protected:
  using V = expected_value_t<T>;
  using storage = expected_union<V, E>;

  storage u_;
  bool has_val_;

  static constexpr bool is_void = std::is_void_v<T>;
  // Value-side traits; the empty stand-in for void is trivially everything.
  static constexpr bool trivial_dtor = std::is_trivially_destructible_v<V> && std::is_trivially_destructible_v<E>;

  template <class... Args>
  constexpr explicit expected_base(std::in_place_t t, Args&&... args)
      : u_(t, static_cast<Args&&>(args)...), has_val_(true) {}
  template <class... Args>
  constexpr explicit expected_base(std::unexpect_t t, Args&&... args)
      : u_(t, static_cast<Args&&>(args)...), has_val_(false) {}
  template <class F, class... Args>
  constexpr expected_base(expected_invoke_val_tag t, F&& f, Args&&... args)
      : u_(t, static_cast<F&&>(f), static_cast<Args&&>(args)...), has_val_(true) {}
  template <class F, class... Args>
  constexpr expected_base(expected_invoke_err_tag t, F&& f, Args&&... args)
      : u_(t, static_cast<F&&>(f), static_cast<Args&&>(args)...), has_val_(false) {}
  // From another expected (same or converting): rhs is any expected<U, G> cvref; its value is
  // forwarded with rhs's qualification.
  struct from_other_tag {};
  template <class R>
  constexpr expected_base(from_other_tag, R&& rhs)
      : u_(rhs.has_value() ? make_value(static_cast<R&&>(rhs)) : storage(std::unexpect, static_cast<R&&>(rhs).error())),
        has_val_(rhs.has_value()) {}

  expected_base(const expected_base&) = default;
  expected_base(expected_base&&) = default;
  expected_base& operator=(const expected_base&) = default;
  expected_base& operator=(expected_base&&) = default;
  constexpr ~expected_base()
    requires trivial_dtor
  = default;
  constexpr ~expected_base() { destroy(); }

  template <class R>
  static constexpr storage make_value(R&& rhs) {
    if constexpr (is_void)
      return storage(std::in_place);
    else
      return storage(std::in_place, *static_cast<R&&>(rhs));
  }

  constexpr void destroy() noexcept {
    if (has_val_)
      std::destroy_at(__builtin_addressof(u_.val));
    else
      std::destroy_at(__builtin_addressof(u_.unex));
  }

  // Copy / move assignment ([expected.object.assign]/2,7, [expected.void.assign]/1,6). R is
  // const expected_base& or expected_base&& (the derived class converts to its private base).
  template <class R>
  constexpr void assign_from(R&& rhs) {
    using EF = std::conditional_t<std::is_lvalue_reference_v<R>, const E&, E&&>;
    if (has_val_ && rhs.has_val_) {
      if constexpr (!is_void) {
        using VF = std::conditional_t<std::is_lvalue_reference_v<R>, const V&, V&&>;
        u_.val = static_cast<VF>(rhs.u_.val);
      }
    } else if (has_val_) {
      if constexpr (is_void)
        std::construct_at(__builtin_addressof(u_.unex), static_cast<EF>(rhs.u_.unex));
      else
        reinit_expected(u_.unex, u_.val, static_cast<EF>(rhs.u_.unex));
    } else if (rhs.has_val_) {
      if constexpr (is_void) {
        std::destroy_at(__builtin_addressof(u_.unex));
        std::construct_at(__builtin_addressof(u_.val));
      } else {
        using VF = std::conditional_t<std::is_lvalue_reference_v<R>, const V&, V&&>;
        reinit_expected(u_.val, u_.unex, static_cast<VF>(rhs.u_.val));
      }
    } else {
      u_.unex = static_cast<EF>(rhs.u_.unex);
    }
    has_val_ = rhs.has_val_;
  }

  // Assignment from unexpected<G> ([expected.object.assign]/16, [expected.void.assign]/12).
  template <class GF>
  constexpr void assign_error(GF&& g) {
    if (has_val_) {
      if constexpr (is_void)
        std::construct_at(__builtin_addressof(u_.unex), static_cast<GF&&>(g));
      else
        reinit_expected(u_.unex, u_.val, static_cast<GF&&>(g));
      has_val_ = false;
    } else {
      u_.unex = static_cast<GF&&>(g);
    }
  }

public:
  constexpr explicit operator bool() const noexcept { return has_val_; }
  constexpr bool has_value() const noexcept { return has_val_; }
  constexpr bool has_error() const noexcept { return !has_val_; }

  constexpr const E& error() const& noexcept {
    precondition(!has_val_, "std::expected::error: has_value() is true");
    return u_.unex;
  }
  constexpr E& error() & noexcept {
    precondition(!has_val_, "std::expected::error: has_value() is true");
    return u_.unex;
  }
  constexpr const E&& error() const&& noexcept {
    precondition(!has_val_, "std::expected::error: has_value() is true");
    return static_cast<const E&&>(u_.unex);
  }
  constexpr E&& error() && noexcept {
    precondition(!has_val_, "std::expected::error: has_value() is true");
    return static_cast<E&&>(u_.unex);
  }

  template <class G = E>
  constexpr E error_or(G&& e) const& {
    static_assert(std::is_copy_constructible_v<E> && std::is_convertible_v<G, E>,
                  "std::expected::error_or: E must be copy constructible and G convertible to E");
    if (has_val_)
      return static_cast<G&&>(e);
    return u_.unex;
  }
  template <class G = E>
  constexpr E error_or(G&& e) && {
    static_assert(std::is_move_constructible_v<E> && std::is_convertible_v<G, E>,
                  "std::expected::error_or: E must be move constructible and G convertible to E");
    if (has_val_)
      return static_cast<G&&>(e);
    return static_cast<E&&>(u_.unex);
  }
};

} // namespace ycxx::detail

namespace std {

// =============================================================================================
// [expected.expected]
// =============================================================================================
template <class T, class E>
class expected : private ycxx::detail::expected_base<T, E> {
  static_assert(ycxx::detail::valid_expected_value<T>,
                "std::expected: T must be void or a non-array object type other than in_place_t, unexpect_t "
                "and specializations of unexpected");
  static_assert(ycxx::detail::valid_unexpected_arg<E>, "std::expected: E must be a valid argument for unexpected");

  using base = ycxx::detail::expected_base<T, E>;
  using typename base::storage;
  using typename base::V;
  using base::u_;
  using base::has_val_;
  using from_other = typename base::from_other_tag;

  template <class, class>
  friend class expected;

  template <class Tag, class F, class... Args>
    requires is_same_v<Tag, ycxx::detail::expected_invoke_val_tag> || is_same_v<Tag, ycxx::detail::expected_invoke_err_tag>
  constexpr expected(Tag t, F&& f, Args&&... args) : base(t, static_cast<F&&>(f), static_cast<Args&&>(args)...) {}

  static constexpr bool copy_ok = is_copy_constructible_v<T> && is_copy_constructible_v<E>;
  static constexpr bool move_ok = is_move_constructible_v<T> && is_move_constructible_v<E>;
  static constexpr bool copy_assign_ok = is_copy_assignable_v<T> && is_copy_constructible_v<T> &&
                                         is_copy_assignable_v<E> && is_copy_constructible_v<E> &&
                                         (is_nothrow_move_constructible_v<T> || is_nothrow_move_constructible_v<E>);
  static constexpr bool move_assign_ok = is_move_constructible_v<T> && is_move_assignable_v<T> &&
                                         is_move_constructible_v<E> && is_move_assignable_v<E> &&
                                         (is_nothrow_move_constructible_v<T> || is_nothrow_move_constructible_v<E>);
  // Exception specifications. Written on the defaulted (trivial) members too: a defaulted
  // member's implicit specification would follow the union and always be noexcept. The copy
  // operations' specifications are a permitted strengthening ([res.on.exception.handling]/5).
  static constexpr bool nothrow_copy = is_nothrow_copy_constructible_v<T> && is_nothrow_copy_constructible_v<E>;
  static constexpr bool nothrow_move = is_nothrow_move_constructible_v<T> && is_nothrow_move_constructible_v<E>;
  static constexpr bool nothrow_copy_assign = nothrow_copy && is_nothrow_copy_assignable_v<T> && is_nothrow_copy_assignable_v<E>;
  static constexpr bool nothrow_move_assign = nothrow_move && is_nothrow_move_assignable_v<T> && is_nothrow_move_assignable_v<E>;
  static constexpr bool trivial_copy = is_trivially_copy_constructible_v<T> && is_trivially_copy_constructible_v<E>;
  static constexpr bool trivial_move = is_trivially_move_constructible_v<T> && is_trivially_move_constructible_v<E>;
  static constexpr bool trivial_copy_assign =
      is_trivially_copy_constructible_v<T> && is_trivially_copy_assignable_v<T> && is_trivially_destructible_v<T> &&
      is_trivially_copy_constructible_v<E> && is_trivially_copy_assignable_v<E> && is_trivially_destructible_v<E>;
  static constexpr bool trivial_move_assign =
      is_trivially_move_constructible_v<T> && is_trivially_move_assignable_v<T> && is_trivially_destructible_v<T> &&
      is_trivially_move_constructible_v<E> && is_trivially_move_assignable_v<E> && is_trivially_destructible_v<E>;

public:
  using value_type = T;
  using error_type = E;
  using unexpected_type = unexpected<E>;
  template <class U>
  using rebind = expected<U, error_type>;

  using base::operator bool;
  using base::has_value;
  using base::has_error;
  using base::error;
  using base::error_or;

  // ---- [expected.object.cons] ----
  constexpr expected()
    requires is_default_constructible_v<T>
      : base(in_place) {}

  constexpr expected(const expected&) noexcept(nothrow_copy)
    requires copy_ok && trivial_copy
  = default;
  constexpr expected(const expected& rhs) noexcept(nothrow_copy)
    requires copy_ok && (!trivial_copy)
      : base(from_other{}, rhs) {}
  constexpr expected(const expected&)
    requires(!copy_ok)
  = delete;

  constexpr expected(expected&&) noexcept(nothrow_move)
    requires move_ok && trivial_move
  = default;
  constexpr expected(expected&& rhs) noexcept(nothrow_move)
    requires move_ok && (!trivial_move)
      : base(from_other{}, static_cast<expected&&>(rhs)) {}

  template <class U, class G>
    requires(!is_void_v<U>) && ycxx::detail::expected_converts_from<T, E, U, G, const U&, const G&>
  constexpr explicit(!is_convertible_v<const U&, T> || !is_convertible_v<const G&, E>)
      expected(const expected<U, G>& rhs)
      : base(from_other{}, rhs) {}
  template <class U, class G>
    requires(!is_void_v<U>) && ycxx::detail::expected_converts_from<T, E, U, G, U, G>
  constexpr explicit(!is_convertible_v<U, T> || !is_convertible_v<G, E>) expected(expected<U, G>&& rhs)
      : base(from_other{}, static_cast<expected<U, G>&&>(rhs)) {}

  template <class U = remove_cv_t<T>>
    requires(!is_same_v<remove_cvref_t<U>, in_place_t>) && (!is_same_v<remove_cvref_t<U>, expected>) &&
            (!is_same_v<remove_cvref_t<U>, unexpect_t>) && (!ycxx::detail::is_unexpected<remove_cvref_t<U>>) &&
            is_constructible_v<T, U> &&
            (!is_same_v<remove_cv_t<T>, bool> || !ycxx::detail::is_expected<remove_cvref_t<U>>)
  constexpr explicit(!is_convertible_v<U, T>) expected(U&& v) : base(in_place, static_cast<U&&>(v)) {}

  template <class G>
    requires is_constructible_v<E, const G&>
  constexpr explicit(!is_convertible_v<const G&, E>) expected(const unexpected<G>& e) : base(unexpect, e.error()) {}
  template <class G>
    requires is_constructible_v<E, G>
  constexpr explicit(!is_convertible_v<G, E>) expected(unexpected<G>&& e)
      : base(unexpect, static_cast<unexpected<G>&&>(e).error()) {}

  template <class... Args>
    requires is_constructible_v<T, Args...>
  constexpr explicit expected(in_place_t, Args&&... args) : base(in_place, static_cast<Args&&>(args)...) {}
  template <class U, class... Args>
    requires is_constructible_v<T, initializer_list<U>&, Args...>
  constexpr explicit expected(in_place_t, initializer_list<U> il, Args&&... args)
      : base(in_place, il, static_cast<Args&&>(args)...) {}
  template <class... Args>
    requires is_constructible_v<E, Args...>
  constexpr explicit expected(unexpect_t, Args&&... args) : base(unexpect, static_cast<Args&&>(args)...) {}
  template <class U, class... Args>
    requires is_constructible_v<E, initializer_list<U>&, Args...>
  constexpr explicit expected(unexpect_t, initializer_list<U> il, Args&&... args)
      : base(unexpect, il, static_cast<Args&&>(args)...) {}

  // ---- [expected.object.dtor]: base's destructor ----

  // ---- [expected.object.assign] ----
  constexpr expected& operator=(const expected&) noexcept(nothrow_copy_assign)
    requires copy_assign_ok && trivial_copy_assign
  = default;
  constexpr expected& operator=(const expected& rhs) noexcept(nothrow_copy_assign)
    requires copy_assign_ok && (!trivial_copy_assign)
  {
    this->assign_from(static_cast<const base&>(rhs));
    return *this;
  }
  constexpr expected& operator=(const expected&)
    requires(!copy_assign_ok)
  = delete;

  constexpr expected& operator=(expected&&) noexcept(nothrow_move_assign)
    requires move_assign_ok && trivial_move_assign
  = default;
  constexpr expected& operator=(expected&& rhs) noexcept(nothrow_move_assign)
    requires move_assign_ok && (!trivial_move_assign)
  {
    this->assign_from(static_cast<base&&>(rhs));
    return *this;
  }

  template <class U = remove_cv_t<T>>
    requires(!is_same_v<expected, remove_cvref_t<U>>) && (!ycxx::detail::is_unexpected<remove_cvref_t<U>>) &&
            is_constructible_v<T, U> && is_assignable_v<T&, U> &&
            (is_nothrow_constructible_v<T, U> || is_nothrow_move_constructible_v<T> ||
             is_nothrow_move_constructible_v<E>)
  constexpr expected& operator=(U&& v) {
    if (has_val_) {
      u_.val = static_cast<U&&>(v);
    } else {
      ycxx::detail::reinit_expected(u_.val, u_.unex, static_cast<U&&>(v));
      has_val_ = true;
    }
    return *this;
  }

  template <class G>
    requires is_constructible_v<E, const G&> && is_assignable_v<E&, const G&> &&
             (is_nothrow_constructible_v<E, const G&> || is_nothrow_move_constructible_v<T> ||
              is_nothrow_move_constructible_v<E>)
  constexpr expected& operator=(const unexpected<G>& e) {
    this->assign_error(e.error());
    return *this;
  }
  template <class G>
    requires is_constructible_v<E, G> && is_assignable_v<E&, G> &&
             (is_nothrow_constructible_v<E, G> || is_nothrow_move_constructible_v<T> ||
              is_nothrow_move_constructible_v<E>)
  constexpr expected& operator=(unexpected<G>&& e) {
    this->assign_error(static_cast<unexpected<G>&&>(e).error());
    return *this;
  }

  template <class... Args>
    requires is_nothrow_constructible_v<T, Args...>
  constexpr T& emplace(Args&&... args) noexcept {
    this->destroy();
    has_val_ = true;
    return *std::construct_at(__builtin_addressof(u_.val), static_cast<Args&&>(args)...);
  }
  template <class U, class... Args>
    requires is_nothrow_constructible_v<T, initializer_list<U>&, Args...>
  constexpr T& emplace(initializer_list<U> il, Args&&... args) noexcept {
    this->destroy();
    has_val_ = true;
    return *std::construct_at(__builtin_addressof(u_.val), il, static_cast<Args&&>(args)...);
  }

  // ---- [expected.object.swap] ----
  constexpr void swap(expected& rhs) noexcept(is_nothrow_move_constructible_v<T> && is_nothrow_swappable_v<T> &&
                                              is_nothrow_move_constructible_v<E> && is_nothrow_swappable_v<E>)
    requires is_swappable_v<T> && is_swappable_v<E> && is_move_constructible_v<T> && is_move_constructible_v<E> &&
             (is_nothrow_move_constructible_v<T> || is_nothrow_move_constructible_v<E>)
  {
    if (has_val_ && rhs.has_val_) {
      ycxx::detail::swap_adl::do_swap(u_.val, rhs.u_.val);
    } else if (!has_val_ && !rhs.has_val_) {
      ycxx::detail::swap_adl::do_swap(u_.unex, rhs.u_.unex);
    } else if (!has_val_) {
      rhs.swap(*this);
    } else {
      swap_value_with_error(rhs);
    }
  }
  friend constexpr void swap(expected& x, expected& y) noexcept(noexcept(x.swap(y)))
    requires requires { x.swap(y); }
  {
    x.swap(y);
  }

  // ---- [expected.object.obs] ----
  constexpr const T* operator->() const noexcept {
    ycxx::detail::precondition(has_val_, "std::expected::operator->: no value");
    return __builtin_addressof(u_.val);
  }
  constexpr T* operator->() noexcept {
    ycxx::detail::precondition(has_val_, "std::expected::operator->: no value");
    return __builtin_addressof(u_.val);
  }
  constexpr const T& operator*() const& noexcept {
    ycxx::detail::precondition(has_val_, "std::expected::operator*: no value");
    return u_.val;
  }
  constexpr T& operator*() & noexcept {
    ycxx::detail::precondition(has_val_, "std::expected::operator*: no value");
    return u_.val;
  }
  constexpr const T&& operator*() const&& noexcept {
    ycxx::detail::precondition(has_val_, "std::expected::operator*: no value");
    return static_cast<const T&&>(u_.val);
  }
  constexpr T&& operator*() && noexcept {
    ycxx::detail::precondition(has_val_, "std::expected::operator*: no value");
    return static_cast<T&&>(u_.val);
  }

  constexpr const T& value() const& {
    static_assert(is_copy_constructible_v<E>, "std::expected::value: E must be copy constructible");
    if (!has_val_)
      throw_bad_access(u_.unex);
    return u_.val;
  }
  constexpr T& value() & {
    static_assert(is_copy_constructible_v<E>, "std::expected::value: E must be copy constructible");
    if (!has_val_)
      throw_bad_access(static_cast<const E&>(u_.unex));
    return u_.val;
  }
  constexpr const T&& value() const&& {
    static_assert(is_copy_constructible_v<E> && is_constructible_v<E, const E&&>,
                  "std::expected::value: E must be copy constructible and constructible from std::move(error())");
    if (!has_val_)
      throw_bad_access(static_cast<const E&&>(u_.unex));
    return static_cast<const T&&>(u_.val);
  }
  constexpr T&& value() && {
    static_assert(is_copy_constructible_v<E> && is_constructible_v<E, E&&>,
                  "std::expected::value: E must be copy constructible and constructible from std::move(error())");
    if (!has_val_)
      throw_bad_access(static_cast<E&&>(u_.unex));
    return static_cast<T&&>(u_.val);
  }

  template <class U = remove_cv_t<T>>
  constexpr T value_or(U&& v) const& {
    static_assert(is_copy_constructible_v<T> && is_convertible_v<U, T>,
                  "std::expected::value_or: T must be copy constructible and U convertible to T");
    return has_val_ ? u_.val : static_cast<T>(static_cast<U&&>(v));
  }
  template <class U = remove_cv_t<T>>
  constexpr T value_or(U&& v) && {
    static_assert(is_move_constructible_v<T> && is_convertible_v<U, T>,
                  "std::expected::value_or: T must be move constructible and U convertible to T");
    return has_val_ ? static_cast<T&&>(u_.val) : static_cast<T>(static_cast<U&&>(v));
  }

  // ---- [expected.object.monadic] ----
  // Four overloads each (not one explicit-object member): when an rvalue's && overload is not
  // viable, overload resolution must fall back to const && (LWG3877). The *_impl helpers take
  // the qualified expected type as Self; decltype((val)) / decltype(error()) is forward_like<Self>.
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<expected&, E>>
  constexpr auto and_then(F&& f) & {
    return and_then_impl<expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<const expected&, E>>
  constexpr auto and_then(F&& f) const& {
    return and_then_impl<const expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<expected, E>>
  constexpr auto and_then(F&& f) && {
    return and_then_impl<expected>(static_cast<expected&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<const expected, E>>
  constexpr auto and_then(F&& f) const&& {
    return and_then_impl<const expected>(static_cast<const expected&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<T, ycxx::detail::forward_like_t<expected&, V>>
  constexpr auto or_else(F&& f) & {
    return or_else_impl<expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<T, ycxx::detail::forward_like_t<const expected&, V>>
  constexpr auto or_else(F&& f) const& {
    return or_else_impl<const expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<T, ycxx::detail::forward_like_t<expected, V>>
  constexpr auto or_else(F&& f) && {
    return or_else_impl<expected>(static_cast<expected&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<T, ycxx::detail::forward_like_t<const expected, V>>
  constexpr auto or_else(F&& f) const&& {
    return or_else_impl<const expected>(static_cast<const expected&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<expected&, E>>
  constexpr auto transform(F&& f) & {
    return transform_impl<expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<const expected&, E>>
  constexpr auto transform(F&& f) const& {
    return transform_impl<const expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<expected, E>>
  constexpr auto transform(F&& f) && {
    return transform_impl<expected>(static_cast<expected&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<const expected, E>>
  constexpr auto transform(F&& f) const&& {
    return transform_impl<const expected>(static_cast<const expected&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<T, ycxx::detail::forward_like_t<expected&, V>>
  constexpr auto transform_error(F&& f) & {
    return transform_error_impl<expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<T, ycxx::detail::forward_like_t<const expected&, V>>
  constexpr auto transform_error(F&& f) const& {
    return transform_error_impl<const expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<T, ycxx::detail::forward_like_t<expected, V>>
  constexpr auto transform_error(F&& f) && {
    return transform_error_impl<expected>(static_cast<expected&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<T, ycxx::detail::forward_like_t<const expected, V>>
  constexpr auto transform_error(F&& f) const&& {
    return transform_error_impl<const expected>(static_cast<const expected&&>(*this), static_cast<F&&>(f));
  }

  // ---- [expected.object.eq] ----
  template <class T2, class E2>
    requires(!is_void_v<T2>) && ycxx::detail::eq_to_bool<T, T2> && ycxx::detail::eq_to_bool<E, E2>
  friend constexpr bool operator==(const expected& x, const expected<T2, E2>& y) {
    if (x.has_value() != y.has_value())
      return false;
    return x.has_value() ? ycxx::detail::implicit_bool(*x == *y) : ycxx::detail::implicit_bool(x.error() == y.error());
  }
  // The left operand is deduced (and must be this expected or derived from it) so that other types
  // never convert to expected here; with a plain `const expected&` parameter, checking the
  // constraint for e.g. int == pair<int, expected<int, int>> found via ADL re-enters itself.
  template <class X, class T2>
    requires derived_from<X, expected> && (!ycxx::detail::is_expected<T2>) && ycxx::detail::eq_to_bool<T, T2>
  friend constexpr bool operator==(const X& xd, const T2& v) {
    const expected& x = xd;
    return x.has_value() && ycxx::detail::implicit_bool(*x == v);
  }
  template <class E2>
    requires ycxx::detail::eq_to_bool<E, E2>
  friend constexpr bool operator==(const expected& x, const unexpected<E2>& e) {
    return !x.has_value() && ycxx::detail::implicit_bool(x.error() == e.error());
  }

private:
  // Monadic operations ([expected.object.monadic]); Self is the qualified expected type.
  template <class Self, class F>
  static constexpr auto and_then_impl(Self&& s, F&& f) {
    using U = remove_cvref_t<invoke_result_t<F, ycxx::detail::forward_like_t<Self, V>>>;
    static_assert(ycxx::detail::is_expected<U>, "std::expected::and_then: F must return a specialization of expected");
    static_assert(is_same_v<typename U::error_type, E>, "std::expected::and_then: F must return the same error_type");
    if (s.has_val_)
      return ::ycxx::detail::invoke(static_cast<F&&>(f), std::forward_like<Self>(s.u_.val));
    return U(unexpect, std::forward_like<Self>(s.u_.unex));
  }
  template <class Self, class F>
  static constexpr auto or_else_impl(Self&& s, F&& f) {
    using G = remove_cvref_t<invoke_result_t<F, ycxx::detail::forward_like_t<Self, E>>>;
    static_assert(ycxx::detail::is_expected<G>, "std::expected::or_else: F must return a specialization of expected");
    static_assert(is_same_v<typename G::value_type, T>, "std::expected::or_else: F must return the same value_type");
    if (s.has_val_)
      return G(in_place, std::forward_like<Self>(s.u_.val));
    return ::ycxx::detail::invoke(static_cast<F&&>(f), std::forward_like<Self>(s.u_.unex));
  }
  template <class Self, class F>
  static constexpr auto transform_impl(Self&& s, F&& f) {
    using U = remove_cv_t<invoke_result_t<F, ycxx::detail::forward_like_t<Self, V>>>;
    static_assert(ycxx::detail::valid_expected_value<U>, "std::expected::transform: invalid result type");
    using R = expected<U, E>;
    if (!s.has_val_)
      return R(unexpect, std::forward_like<Self>(s.u_.unex));
    if constexpr (is_void_v<U>) {
      ::ycxx::detail::invoke(static_cast<F&&>(f), std::forward_like<Self>(s.u_.val));
      return R();
    } else {
      return R(ycxx::detail::expected_invoke_val_tag{}, static_cast<F&&>(f), std::forward_like<Self>(s.u_.val));
    }
  }
  template <class Self, class F>
  static constexpr auto transform_error_impl(Self&& s, F&& f) {
    using G = remove_cv_t<invoke_result_t<F, ycxx::detail::forward_like_t<Self, E>>>;
    static_assert(ycxx::detail::valid_unexpected_arg<G>, "std::expected::transform_error: invalid error type");
    using R = expected<T, G>;
    if (s.has_val_)
      return R(in_place, std::forward_like<Self>(s.u_.val));
    return R(ycxx::detail::expected_invoke_err_tag{}, static_cast<F&&>(f), std::forward_like<Self>(s.u_.unex));
  }

  template <class EF>
  [[noreturn]] static constexpr void throw_bad_access(EF&& e) {
    ycxx::detail::raise_with(ycxx_error_bad_expected_access, "std::bad_expected_access",
                             [&] { return bad_expected_access<E>(static_cast<EF&&>(e)); });
  }

  // Table 72, the case this->has_value() && !rhs.has_value().
  constexpr void swap_value_with_error(expected& rhs) {
    if constexpr (is_nothrow_move_constructible_v<E>) {
      E tmp(static_cast<E&&>(rhs.u_.unex));
      std::destroy_at(__builtin_addressof(rhs.u_.unex));
      auto body = [&] {
        std::construct_at(__builtin_addressof(rhs.u_.val), static_cast<V&&>(u_.val));
        std::destroy_at(__builtin_addressof(u_.val));
        std::construct_at(__builtin_addressof(u_.unex), static_cast<E&&>(tmp));
      };
      if constexpr (ycxx::detail::cfg::exceptions) {
        try {
          body();
        } catch (...) {
          std::construct_at(__builtin_addressof(rhs.u_.unex), static_cast<E&&>(tmp));
          throw;
        }
      } else {
        body();
      }
    } else {
      V tmp(static_cast<V&&>(u_.val));
      std::destroy_at(__builtin_addressof(u_.val));
      auto body = [&] {
        std::construct_at(__builtin_addressof(u_.unex), static_cast<E&&>(rhs.u_.unex));
        std::destroy_at(__builtin_addressof(rhs.u_.unex));
        std::construct_at(__builtin_addressof(rhs.u_.val), static_cast<V&&>(tmp));
      };
      if constexpr (ycxx::detail::cfg::exceptions) {
        try {
          body();
        } catch (...) {
          std::construct_at(__builtin_addressof(u_.val), static_cast<V&&>(tmp));
          throw;
        }
      } else {
        body();
      }
    }
    has_val_ = false;
    rhs.has_val_ = true;
  }
};

// =============================================================================================
// [expected.void]
// =============================================================================================
template <class T, class E>
  requires is_void_v<T>
class expected<T, E> : private ycxx::detail::expected_base<T, E> {
  static_assert(ycxx::detail::valid_unexpected_arg<E>, "std::expected: E must be a valid argument for unexpected");

  using base = ycxx::detail::expected_base<T, E>;
  using typename base::storage;
  using base::u_;
  using base::has_val_;
  using from_other = typename base::from_other_tag;

  template <class, class>
  friend class expected;

  template <class Tag, class F, class... Args>
    requires is_same_v<Tag, ycxx::detail::expected_invoke_val_tag> || is_same_v<Tag, ycxx::detail::expected_invoke_err_tag>
  constexpr expected(Tag t, F&& f, Args&&... args) : base(t, static_cast<F&&>(f), static_cast<Args&&>(args)...) {}

  // See the primary template for why the defaulted members carry exception specifications.
  static constexpr bool nothrow_copy = is_nothrow_copy_constructible_v<E>;
  static constexpr bool nothrow_move = is_nothrow_move_constructible_v<E>;
  static constexpr bool nothrow_copy_assign = nothrow_copy && is_nothrow_copy_assignable_v<E>;
  static constexpr bool nothrow_move_assign = nothrow_move && is_nothrow_move_assignable_v<E>;
  static constexpr bool copy_assign_ok = is_copy_assignable_v<E> && is_copy_constructible_v<E>;
  static constexpr bool move_assign_ok = is_move_constructible_v<E> && is_move_assignable_v<E>;
  static constexpr bool trivial_copy_assign =
      is_trivially_copy_constructible_v<E> && is_trivially_copy_assignable_v<E> && is_trivially_destructible_v<E>;
  static constexpr bool trivial_move_assign =
      is_trivially_move_constructible_v<E> && is_trivially_move_assignable_v<E> && is_trivially_destructible_v<E>;

public:
  using value_type = T;
  using error_type = E;
  using unexpected_type = unexpected<E>;
  template <class U>
  using rebind = expected<U, error_type>;

  using base::operator bool;
  using base::has_value;
  using base::has_error;
  using base::error;
  using base::error_or;

  // ---- [expected.void.cons] ----
  constexpr expected() noexcept : base(in_place) {}

  constexpr expected(const expected&) noexcept(nothrow_copy)
    requires is_copy_constructible_v<E> && is_trivially_copy_constructible_v<E>
  = default;
  constexpr expected(const expected& rhs) noexcept(nothrow_copy)
    requires is_copy_constructible_v<E> && (!is_trivially_copy_constructible_v<E>)
      : base(from_other{}, rhs) {}
  constexpr expected(const expected&)
    requires(!is_copy_constructible_v<E>)
  = delete;

  constexpr expected(expected&&) noexcept(nothrow_move)
    requires is_move_constructible_v<E> && is_trivially_move_constructible_v<E>
  = default;
  constexpr expected(expected&& rhs) noexcept(nothrow_move)
    requires is_move_constructible_v<E> && (!is_trivially_move_constructible_v<E>)
      : base(from_other{}, static_cast<expected&&>(rhs)) {}

  template <class U, class G>
    requires is_void_v<U> && ycxx::detail::expected_void_converts_from<T, E, U, G, const G&>
  constexpr explicit(!is_convertible_v<const G&, E>) expected(const expected<U, G>& rhs) : base(from_other{}, rhs) {}
  template <class U, class G>
    requires is_void_v<U> && ycxx::detail::expected_void_converts_from<T, E, U, G, G>
  constexpr explicit(!is_convertible_v<G, E>) expected(expected<U, G>&& rhs)
      : base(from_other{}, static_cast<expected<U, G>&&>(rhs)) {}

  template <class G>
    requires is_constructible_v<E, const G&>
  constexpr explicit(!is_convertible_v<const G&, E>) expected(const unexpected<G>& e) : base(unexpect, e.error()) {}
  template <class G>
    requires is_constructible_v<E, G>
  constexpr explicit(!is_convertible_v<G, E>) expected(unexpected<G>&& e)
      : base(unexpect, static_cast<unexpected<G>&&>(e).error()) {}

  constexpr explicit expected(in_place_t) noexcept : base(in_place) {}
  template <class... Args>
    requires is_constructible_v<E, Args...>
  constexpr explicit expected(unexpect_t, Args&&... args) : base(unexpect, static_cast<Args&&>(args)...) {}
  template <class U, class... Args>
    requires is_constructible_v<E, initializer_list<U>&, Args...>
  constexpr explicit expected(unexpect_t, initializer_list<U> il, Args&&... args)
      : base(unexpect, il, static_cast<Args&&>(args)...) {}

  // ---- [expected.void.assign] ----
  constexpr expected& operator=(const expected&) noexcept(nothrow_copy_assign)
    requires copy_assign_ok && trivial_copy_assign
  = default;
  constexpr expected& operator=(const expected& rhs) noexcept(nothrow_copy_assign)
    requires copy_assign_ok && (!trivial_copy_assign)
  {
    this->assign_from(static_cast<const base&>(rhs));
    return *this;
  }
  constexpr expected& operator=(const expected&)
    requires(!copy_assign_ok)
  = delete;

  constexpr expected& operator=(expected&&) noexcept(nothrow_move_assign)
    requires move_assign_ok && trivial_move_assign
  = default;
  constexpr expected& operator=(expected&& rhs) noexcept(nothrow_move_assign)
    requires move_assign_ok && (!trivial_move_assign)
  {
    this->assign_from(static_cast<base&&>(rhs));
    return *this;
  }

  template <class G>
    requires is_constructible_v<E, const G&> && is_assignable_v<E&, const G&>
  constexpr expected& operator=(const unexpected<G>& e) {
    this->assign_error(e.error());
    return *this;
  }
  template <class G>
    requires is_constructible_v<E, G> && is_assignable_v<E&, G>
  constexpr expected& operator=(unexpected<G>&& e) {
    this->assign_error(static_cast<unexpected<G>&&>(e).error());
    return *this;
  }

  constexpr void emplace() noexcept {
    if (!has_val_) {
      std::destroy_at(__builtin_addressof(u_.unex));
      std::construct_at(__builtin_addressof(u_.val));
      has_val_ = true;
    }
  }

  // ---- [expected.void.swap] ----
  constexpr void swap(expected& rhs) noexcept(is_nothrow_move_constructible_v<E> && is_nothrow_swappable_v<E>)
    requires is_swappable_v<E> && is_move_constructible_v<E>
  {
    if (has_val_ && rhs.has_val_)
      return;
    if (!has_val_ && !rhs.has_val_) {
      ycxx::detail::swap_adl::do_swap(u_.unex, rhs.u_.unex);
    } else if (!has_val_) {
      rhs.swap(*this);
    } else {
      std::construct_at(__builtin_addressof(u_.unex), static_cast<E&&>(rhs.u_.unex));
      std::destroy_at(__builtin_addressof(rhs.u_.unex));
      std::construct_at(__builtin_addressof(rhs.u_.val));
      has_val_ = false;
      rhs.has_val_ = true;
    }
  }
  friend constexpr void swap(expected& x, expected& y) noexcept(noexcept(x.swap(y)))
    requires requires { x.swap(y); }
  {
    x.swap(y);
  }

  // ---- [expected.void.obs] ----
  constexpr void operator*() const noexcept {
    ycxx::detail::precondition(has_val_, "std::expected::operator*: no value");
  }
  constexpr void value() const& {
    static_assert(is_copy_constructible_v<E>, "std::expected::value: E must be copy constructible");
    if (!has_val_)
      throw_bad_access(u_.unex);
  }
  constexpr void value() && {
    static_assert(is_copy_constructible_v<E> && is_move_constructible_v<E>,
                  "std::expected::value: E must be copy and move constructible");
    if (!has_val_)
      throw_bad_access(static_cast<E&&>(u_.unex));
  }

  // ---- [expected.void.monadic] ----
  // Four overloads each (not one explicit-object member): when an rvalue's && overload is not
  // viable, overload resolution must fall back to const && (LWG3877). The *_impl helpers take
  // the qualified expected type as Self; decltype((val)) / decltype(error()) is forward_like<Self>.
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<expected&, E>>
  constexpr auto and_then(F&& f) & {
    return and_then_impl<expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<const expected&, E>>
  constexpr auto and_then(F&& f) const& {
    return and_then_impl<const expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<expected, E>>
  constexpr auto and_then(F&& f) && {
    return and_then_impl<expected>(static_cast<expected&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<const expected, E>>
  constexpr auto and_then(F&& f) const&& {
    return and_then_impl<const expected>(static_cast<const expected&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto or_else(F&& f) & {
    return or_else_impl<expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto or_else(F&& f) const& {
    return or_else_impl<const expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto or_else(F&& f) && {
    return or_else_impl<expected>(static_cast<expected&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto or_else(F&& f) const&& {
    return or_else_impl<const expected>(static_cast<const expected&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<expected&, E>>
  constexpr auto transform(F&& f) & {
    return transform_impl<expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<const expected&, E>>
  constexpr auto transform(F&& f) const& {
    return transform_impl<const expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<expected, E>>
  constexpr auto transform(F&& f) && {
    return transform_impl<expected>(static_cast<expected&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
    requires is_constructible_v<E, ycxx::detail::forward_like_t<const expected, E>>
  constexpr auto transform(F&& f) const&& {
    return transform_impl<const expected>(static_cast<const expected&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto transform_error(F&& f) & {
    return transform_error_impl<expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto transform_error(F&& f) const& {
    return transform_error_impl<const expected&>(*this, static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto transform_error(F&& f) && {
    return transform_error_impl<expected>(static_cast<expected&&>(*this), static_cast<F&&>(f));
  }
  template <class F>
  constexpr auto transform_error(F&& f) const&& {
    return transform_error_impl<const expected>(static_cast<const expected&&>(*this), static_cast<F&&>(f));
  }

  // ---- [expected.void.eq] ----
  template <class T2, class E2>
    requires is_void_v<T2> && ycxx::detail::eq_to_bool<E, E2>
  friend constexpr bool operator==(const expected& x, const expected<T2, E2>& y) {
    if (x.has_value() != y.has_value())
      return false;
    return x.has_value() || ycxx::detail::implicit_bool(x.error() == y.error());
  }
  template <class E2>
    requires ycxx::detail::eq_to_bool<E, E2>
  friend constexpr bool operator==(const expected& x, const unexpected<E2>& e) {
    return !x.has_value() && ycxx::detail::implicit_bool(x.error() == e.error());
  }

private:
  // Monadic operations ([expected.void.monadic]); Self is the qualified expected type.
  template <class Self, class F>
  static constexpr auto and_then_impl(Self&& s, F&& f) {
    using U = remove_cvref_t<invoke_result_t<F>>;
    static_assert(ycxx::detail::is_expected<U>, "std::expected::and_then: F must return a specialization of expected");
    static_assert(is_same_v<typename U::error_type, E>, "std::expected::and_then: F must return the same error_type");
    if (s.has_val_)
      return ::ycxx::detail::invoke(static_cast<F&&>(f));
    return U(unexpect, std::forward_like<Self>(s.u_.unex));
  }
  template <class Self, class F>
  static constexpr auto or_else_impl(Self&& s, F&& f) {
    using G = remove_cvref_t<invoke_result_t<F, ycxx::detail::forward_like_t<Self, E>>>;
    static_assert(ycxx::detail::is_expected<G>, "std::expected::or_else: F must return a specialization of expected");
    static_assert(is_same_v<typename G::value_type, T>, "std::expected::or_else: F must return the same value_type");
    if (s.has_val_)
      return G();
    return ::ycxx::detail::invoke(static_cast<F&&>(f), std::forward_like<Self>(s.u_.unex));
  }
  template <class Self, class F>
  static constexpr auto transform_impl(Self&& s, F&& f) {
    using U = remove_cv_t<invoke_result_t<F>>;
    static_assert(ycxx::detail::valid_expected_value<U>, "std::expected::transform: invalid result type");
    using R = expected<U, E>;
    if (!s.has_val_)
      return R(unexpect, std::forward_like<Self>(s.u_.unex));
    if constexpr (is_void_v<U>) {
      ::ycxx::detail::invoke(static_cast<F&&>(f));
      return R();
    } else {
      return R(ycxx::detail::expected_invoke_val_tag{}, static_cast<F&&>(f));
    }
  }
  template <class Self, class F>
  static constexpr auto transform_error_impl(Self&& s, F&& f) {
    using G = remove_cv_t<invoke_result_t<F, ycxx::detail::forward_like_t<Self, E>>>;
    static_assert(ycxx::detail::valid_unexpected_arg<G>, "std::expected::transform_error: invalid error type");
    using R = expected<T, G>;
    if (s.has_val_)
      return R();
    return R(ycxx::detail::expected_invoke_err_tag{}, static_cast<F&&>(f), std::forward_like<Self>(s.u_.unex));
  }

  template <class EF>
  [[noreturn]] static constexpr void throw_bad_access(EF&& e) {
    ycxx::detail::raise_with(ycxx_error_bad_expected_access, "std::bad_expected_access",
                             [&] { return bad_expected_access<E>(static_cast<EF&&>(e)); });
  }
};

} // namespace std
