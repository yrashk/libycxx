// libycxx core: indirect ([indirect], P3019), a value-semantic owner of one object of type T
// allocated through an allocator.
//
// The owned object is held through allocator_traits<Allocator>::pointer; a null pointer is the
// valueless state. T may be incomplete until a member that needs it (the destructor, a
// constructor, an assignment) is instantiated.
#pragma once

#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/hash.hpp>
#include <initializer_list>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/utility_base.hpp>
#include <ycxx/core/error.hpp>

namespace [[gnu::visibility("hidden")]] std {
template <class T, class Allocator>
class indirect;
}

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template <class T>
inline constexpr bool is_indirect = false;
template <class T, class A>
inline constexpr bool is_indirect<std::indirect<T, A>> = true;

// [indirect.general]/5, [polymorphic.general]/5: the value types indirect and polymorphic reject.
template <class T>
inline constexpr bool composite_value_ok = std::is_object_v<T> && !std::is_array_v<T> &&
                                           !std::is_same_v<T, std::in_place_t> && !is_in_place_type<T> &&
                                           std::is_same_v<T, std::remove_cv_t<T>>;

// T is a complete type (for the Mandates that require one).
template <class T>
concept complete_type = requires { sizeof(T); };

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

template <class T, class Allocator = allocator<T>>
class indirect {
  static_assert(ycxx::detail::composite_value_ok<T>,
                "std::indirect: T must be a cv-unqualified object type that is not an array, in_place_t or "
                "a specialization of in_place_type_t");
  static_assert(is_same_v<typename allocator_traits<Allocator>::value_type, T>,
                "std::indirect: allocator_traits<Allocator>::value_type must be T");

  using traits = allocator_traits<Allocator>;

public:
  using value_type = T;
  using allocator_type = Allocator;
  using pointer = typename traits::pointer;
  using const_pointer = typename traits::const_pointer;

private:
  pointer p_ = nullptr;
  [[no_unique_address]] Allocator alloc_ = Allocator();

  template <class U, class AA>
  friend class indirect;

  // A new owned object constructed with args using a; nothing leaks if construction throws.
  template <class... Args>
  static constexpr pointer make(Allocator& a, Args&&... args) {
    pointer np = traits::allocate(a, 1);
    ycxx::detail::rollback guard{[&] { traits::deallocate(a, np, 1); }};
    traits::construct(a, std::to_address(np), static_cast<Args&&>(args)...);
    guard.release();
    return np;
  }
  // Destroys and frees the owned object q, which was allocated with a (or an equal allocator).
  static constexpr void dispose(Allocator& a, pointer q) noexcept {
    if (q != nullptr) {
      traits::destroy(a, std::to_address(q));
      traits::deallocate(a, q, 1);
    }
  }
  // An owned object for a constructor that was given (or defaulted) the allocator.
  template <class... Args>
  constexpr void init(Args&&... args) {
    p_ = make(alloc_, static_cast<Args&&>(args)...);
  }

public:
  // ---- [indirect.ctor] ----
  constexpr explicit indirect()
    requires is_default_constructible_v<Allocator>
  {
    static_assert(is_default_constructible_v<T>, "std::indirect(): T must be default constructible");
    init();
  }
  constexpr explicit indirect(allocator_arg_t, const Allocator& a) : alloc_(a) {
    static_assert(is_default_constructible_v<T>, "std::indirect(allocator_arg_t, a): T must be default constructible");
    init();
  }
  constexpr indirect(const indirect& other) : alloc_(traits::select_on_container_copy_construction(other.alloc_)) {
    static_assert(is_copy_constructible_v<T>, "std::indirect: T must be copy constructible");
    if (other.p_ != nullptr)
      init(*other.p_);
  }
  constexpr indirect(allocator_arg_t, const Allocator& a, const indirect& other) : alloc_(a) {
    static_assert(is_copy_constructible_v<T>, "std::indirect: T must be copy constructible");
    if (other.p_ != nullptr)
      init(*other.p_);
  }
  constexpr indirect(indirect&& other) noexcept : p_(other.p_), alloc_(static_cast<Allocator&&>(other.alloc_)) {
    other.p_ = nullptr;
  }
  constexpr indirect(allocator_arg_t, const Allocator& a, indirect&& other) noexcept(traits::is_always_equal::value)
      : alloc_(a) {
    if constexpr (!traits::is_always_equal::value)
      static_assert(ycxx::detail::complete_type<T>,
                    "std::indirect: T must be complete for the allocator-extended move constructor");
    if (other.p_ == nullptr)
      return;
    if constexpr (traits::is_always_equal::value) {
      p_ = other.p_;
      other.p_ = nullptr;
    } else {
      if (alloc_ == other.alloc_) {
        p_ = other.p_;
        other.p_ = nullptr;
      } else {
        init(static_cast<T&&>(*other.p_));
        // [indirect.ctor]/16 Postconditions: other is valueless.
        dispose(other.alloc_, other.p_);
        other.p_ = nullptr;
      }
    }
  }
  template <class U = T>
    requires(!is_same_v<remove_cvref_t<U>, indirect>) && (!is_same_v<remove_cvref_t<U>, in_place_t>) &&
            is_constructible_v<T, U> && is_default_constructible_v<Allocator>
  constexpr explicit indirect(U&& u) {
    init(static_cast<U&&>(u));
  }
  template <class U = T>
    requires(!is_same_v<remove_cvref_t<U>, indirect>) && (!is_same_v<remove_cvref_t<U>, in_place_t>) &&
            is_constructible_v<T, U>
  constexpr explicit indirect(allocator_arg_t, const Allocator& a, U&& u) : alloc_(a) {
    init(static_cast<U&&>(u));
  }
  template <class... Us>
    requires is_constructible_v<T, Us...> && is_default_constructible_v<Allocator>
  constexpr explicit indirect(in_place_t, Us&&... us) {
    init(static_cast<Us&&>(us)...);
  }
  template <class... Us>
    requires is_constructible_v<T, Us...>
  constexpr explicit indirect(allocator_arg_t, const Allocator& a, in_place_t, Us&&... us) : alloc_(a) {
    init(static_cast<Us&&>(us)...);
  }
  template <class I, class... Us>
    requires is_constructible_v<T, initializer_list<I>&, Us...> && is_default_constructible_v<Allocator>
  constexpr explicit indirect(in_place_t, initializer_list<I> ilist, Us&&... us) {
    init(ilist, static_cast<Us&&>(us)...);
  }
  template <class I, class... Us>
    requires is_constructible_v<T, initializer_list<I>&, Us...>
  constexpr explicit indirect(allocator_arg_t, const Allocator& a, in_place_t, initializer_list<I> ilist, Us&&... us)
      : alloc_(a) {
    init(ilist, static_cast<Us&&>(us)...);
  }

  // ---- [indirect.dtor] ----
  constexpr ~indirect() {
    static_assert(ycxx::detail::complete_type<T>, "std::indirect: T must be complete where the destructor is used");
    dispose(alloc_, p_);
  }

  // ---- [indirect.assign] ----
  constexpr indirect& operator=(const indirect& other) {
    static_assert(is_copy_assignable_v<T> && is_copy_constructible_v<T>,
                  "std::indirect: copy assignment needs a copy-assignable and copy-constructible T");
    if (__builtin_addressof(other) == this)
      return *this;
    constexpr bool update = traits::propagate_on_container_copy_assignment::value;
    if (other.p_ == nullptr) {
      dispose(alloc_, p_);
      p_ = nullptr;
    } else if (p_ != nullptr && alloc_ == other.alloc_) {
      *p_ = *other.p_;
    } else {
      pointer np;
      if constexpr (update) {
        Allocator a(other.alloc_);
        np = make(a, *other.p_);
      } else {
        np = make(alloc_, *other.p_);
      }
      dispose(alloc_, p_);
      p_ = np;
    }
    if constexpr (update)
      alloc_ = other.alloc_;
    return *this;
  }
  constexpr indirect& operator=(indirect&& other) noexcept(traits::propagate_on_container_move_assignment::value ||
                                                          traits::is_always_equal::value) {
    constexpr bool update = traits::propagate_on_container_move_assignment::value;
    if constexpr (!update && !traits::is_always_equal::value)
      static_assert(is_move_constructible_v<T>, "std::indirect: move assignment needs a move-constructible T");
    if (__builtin_addressof(other) == this)
      return *this;
    if (other.p_ == nullptr) {
      dispose(alloc_, p_);
      p_ = nullptr;
    } else if (update || traits::is_always_equal::value || alloc_ == other.alloc_) {
      dispose(alloc_, p_);
      p_ = other.p_;
      other.p_ = nullptr;
    } else {
      if constexpr (!update && !traits::is_always_equal::value) {
        pointer np = make(alloc_, static_cast<T&&>(*other.p_));
        dispose(alloc_, p_);
        p_ = np;
        dispose(other.alloc_, other.p_); // [indirect.assign]/7 Postconditions: other is valueless.
        other.p_ = nullptr;
      }
    }
    // "Replaced with a copy of the allocator in other" ([indirect.assign]/7, [polymorphic.assign]/7):
    // by move assignment: an allocator whose propagate_on_container_move_assignment is true need
    // only be Cpp17MoveAssignable, not Cpp17CopyAssignable ([allocator.requirements.general]).
    if constexpr (update)
      alloc_ = static_cast<Allocator&&>(other.alloc_);
    return *this;
  }
  template <class U = T>
    requires(!is_same_v<remove_cvref_t<U>, indirect>) && is_constructible_v<T, U> && is_assignable_v<T&, U>
  constexpr indirect& operator=(U&& u) {
    if (p_ == nullptr)
      init(static_cast<U&&>(u));
    else
      *p_ = static_cast<U&&>(u);
    return *this;
  }

  // ---- [indirect.obs] ----
  constexpr const T& operator*() const& noexcept {
    ycxx::detail::precondition(p_ != nullptr, "std::indirect::operator*: valueless");
    return *p_;
  }
  constexpr T& operator*() & noexcept {
    ycxx::detail::precondition(p_ != nullptr, "std::indirect::operator*: valueless");
    return *p_;
  }
  constexpr const T&& operator*() const&& noexcept {
    ycxx::detail::precondition(p_ != nullptr, "std::indirect::operator*: valueless");
    return static_cast<const T&&>(*p_);
  }
  constexpr T&& operator*() && noexcept {
    ycxx::detail::precondition(p_ != nullptr, "std::indirect::operator*: valueless");
    return static_cast<T&&>(*p_);
  }
  constexpr const_pointer operator->() const noexcept {
    ycxx::detail::precondition(p_ != nullptr, "std::indirect::operator->: valueless");
    return p_;
  }
  constexpr pointer operator->() noexcept {
    ycxx::detail::precondition(p_ != nullptr, "std::indirect::operator->: valueless");
    return p_;
  }
  constexpr bool valueless_after_move() const noexcept { return p_ == nullptr; }
  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  // ---- [indirect.swap] ----
  constexpr void swap(indirect& other) noexcept(traits::propagate_on_container_swap::value ||
                                               traits::is_always_equal::value) {
    if constexpr (traits::propagate_on_container_swap::value)
      ycxx::detail::swap_adl::do_swap(alloc_, other.alloc_);
    else
      ycxx::detail::precondition(traits::is_always_equal::value || alloc_ == other.alloc_,
                                 "std::indirect::swap: unequal allocators that do not propagate");
    pointer t = p_;
    p_ = other.p_;
    other.p_ = t;
  }
  friend constexpr void swap(indirect& lhs, indirect& rhs) noexcept(noexcept(lhs.swap(rhs))) { lhs.swap(rhs); }

  // ---- [indirect.relops] ----
  template <class U, class AA>
  friend constexpr bool operator==(const indirect& lhs, const indirect<U, AA>& rhs) noexcept(noexcept(bool(*lhs ==
                                                                                                          *rhs))) {
    static_assert(requires { static_cast<bool>(*lhs == *rhs); },
                  "std::indirect: operator== needs *lhs == *rhs convertible to bool");
    if (lhs.p_ == nullptr || rhs.p_ == nullptr)
      return (lhs.p_ == nullptr) == (rhs.p_ == nullptr);
    return static_cast<bool>(*lhs.p_ == *rhs.p_);
  }
  template <class U, class AA>
  friend constexpr auto operator<=>(const indirect& lhs, const indirect<U, AA>& rhs)
      -> ycxx::detail::synth_three_way_result<T, U> {
    if (lhs.p_ == nullptr || rhs.p_ == nullptr)
      return !(lhs.p_ == nullptr) <=> !(rhs.p_ == nullptr);
    return ycxx::detail::synth_three_way(*lhs.p_, *rhs.p_);
  }

  // ---- [indirect.comp.with.t] ----
  // Not for U an indirect: the operators above are more specialized and are chosen for those
  // anyway, and leaving them out keeps the return type's synth-three-way from recursing.
  template <class U>
    requires(!ycxx::detail::is_indirect<U>)
  friend constexpr bool operator==(const indirect& lhs, const U& rhs) noexcept(noexcept(bool(*lhs == rhs))) {
    static_assert(requires { static_cast<bool>(*lhs == rhs); },
                  "std::indirect: operator== needs *lhs == rhs convertible to bool");
    if (lhs.p_ == nullptr)
      return false;
    return static_cast<bool>(*lhs.p_ == rhs);
  }
  template <class U>
    requires(!ycxx::detail::is_indirect<U>)
  friend constexpr auto operator<=>(const indirect& lhs, const U& rhs) -> ycxx::detail::synth_three_way_result<T, U> {
    if (lhs.p_ == nullptr)
      return strong_ordering::less;
    return ycxx::detail::synth_three_way(*lhs.p_, rhs);
  }
};

template <class Value>
indirect(Value) -> indirect<Value>;
template <class Allocator, class Value>
indirect(allocator_arg_t, Allocator, Value)
    -> indirect<Value, typename allocator_traits<Allocator>::template rebind_alloc<Value>>;

// [indirect.hash]: enabled iff hash<T> is; a valueless object hashes to 0.
template <class T, class Allocator>
  requires ycxx::detail::hash_enabled<T>
struct hash<indirect<T, Allocator>> {
  constexpr size_t operator()(const indirect<T, Allocator>& i) const {
    return i.valueless_after_move() ? size_t(0) : hash<T>()(*i);
  }
};

} // namespace std
