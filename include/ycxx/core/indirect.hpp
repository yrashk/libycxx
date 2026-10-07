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
#include <ycxx/core/memory_resource_fwd.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Tp, class _Allocator>
class indirect;
}

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Tp>
inline constexpr bool __is_indirect = false;
template <class _Tp, class _Ap>
inline constexpr bool __is_indirect<std::indirect<_Tp, _Ap>> = true;

// [indirect.general]/5, [polymorphic.general]/5: the value types indirect and polymorphic reject.
template <class _Tp>
inline constexpr bool __composite_value_ok = std::is_object_v<_Tp> && !std::is_array_v<_Tp> &&
                                           !std::is_same_v<_Tp, std::in_place_t> && !__is_in_place_type<_Tp> &&
                                           std::is_same_v<_Tp, std::remove_cv_t<_Tp>>;

// T is a complete type (for the Mandates that require one).
template <class _Tp>
concept __complete_type = requires { sizeof(_Tp); };

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp, class _Allocator = allocator<_Tp>>
class indirect {
  static_assert(__ycxx::__detail::__composite_value_ok<_Tp>,
                "std::indirect: T must be a cv-unqualified object type that is not an array, in_place_t or "
                "a specialization of in_place_type_t");
  static_assert(is_same_v<typename allocator_traits<_Allocator>::value_type, _Tp>,
                "std::indirect: allocator_traits<Allocator>::value_type must be T");

  using __traits = allocator_traits<_Allocator>;

public:
  using value_type = _Tp;
  using allocator_type = _Allocator;
  using pointer = typename __traits::pointer;
  using const_pointer = typename __traits::const_pointer;

private:
  pointer __p_ = nullptr;
  [[no_unique_address]] _Allocator __alloc_ = _Allocator();

  template <class _Up, class _AA>
  friend class indirect;

  // A new owned object constructed with args using a; nothing leaks if construction throws.
  template <class... _Args>
  static constexpr pointer __make(_Allocator& a, _Args&&... __args) {
    pointer __np = __traits::allocate(a, 1);
    __ycxx::__detail::__rollback __guard{[&] { __traits::deallocate(a, __np, 1); }};
    __traits::construct(a, std::to_address(__np), static_cast<_Args&&>(__args)...);
    __guard.release();
    return __np;
  }
  // Destroys and frees the owned object q, which was allocated with a (or an equal allocator).
  static constexpr void __dispose(_Allocator& a, pointer __q) noexcept {
    if (__q != nullptr) {
      __traits::destroy(a, std::to_address(__q));
      __traits::deallocate(a, __q, 1);
    }
  }
  // An owned object for a constructor that was given (or defaulted) the allocator.
  template <class... _Args>
  constexpr void init(_Args&&... __args) {
    __p_ = __make(__alloc_, static_cast<_Args&&>(__args)...);
  }

public:
  // ---- [indirect.ctor] ----
  constexpr explicit indirect()
    requires is_default_constructible_v<_Allocator>
  {
    static_assert(is_default_constructible_v<_Tp>, "std::indirect(): T must be default constructible");
    init();
  }
  constexpr explicit indirect(allocator_arg_t, const _Allocator& a) : __alloc_(a) {
    static_assert(is_default_constructible_v<_Tp>, "std::indirect(allocator_arg_t, a): T must be default constructible");
    init();
  }
  constexpr indirect(const indirect& other) : __alloc_(__traits::select_on_container_copy_construction(other.__alloc_)) {
    static_assert(is_copy_constructible_v<_Tp>, "std::indirect: T must be copy constructible");
    if (other.__p_ != nullptr)
      init(*other.__p_);
  }
  constexpr indirect(allocator_arg_t, const _Allocator& a, const indirect& other) : __alloc_(a) {
    static_assert(is_copy_constructible_v<_Tp>, "std::indirect: T must be copy constructible");
    if (other.__p_ != nullptr)
      init(*other.__p_);
  }
  constexpr indirect(indirect&& other) noexcept : __p_(other.__p_), __alloc_(static_cast<_Allocator&&>(other.__alloc_)) {
    other.__p_ = nullptr;
  }
  constexpr indirect(allocator_arg_t, const _Allocator& a, indirect&& other) noexcept(__traits::is_always_equal::value)
      : __alloc_(a) {
    if constexpr (!__traits::is_always_equal::value)
      static_assert(__ycxx::__detail::__complete_type<_Tp>,
                    "std::indirect: T must be complete for the allocator-extended move constructor");
    if (other.__p_ == nullptr)
      return;
    if constexpr (__traits::is_always_equal::value) {
      __p_ = other.__p_;
      other.__p_ = nullptr;
    } else {
      if (__alloc_ == other.__alloc_) {
        __p_ = other.__p_;
        other.__p_ = nullptr;
      } else {
        init(static_cast<_Tp&&>(*other.__p_));
        // [indirect.ctor]/16 Postconditions: other is valueless.
        __dispose(other.__alloc_, other.__p_);
        other.__p_ = nullptr;
      }
    }
  }
  template <class _Up = _Tp>
    requires(!is_same_v<remove_cvref_t<_Up>, indirect>) && (!is_same_v<remove_cvref_t<_Up>, in_place_t>) &&
            is_constructible_v<_Tp, _Up> && is_default_constructible_v<_Allocator>
  constexpr explicit indirect(_Up&& __u) {
    init(static_cast<_Up&&>(__u));
  }
  template <class _Up = _Tp>
    requires(!is_same_v<remove_cvref_t<_Up>, indirect>) && (!is_same_v<remove_cvref_t<_Up>, in_place_t>) &&
            is_constructible_v<_Tp, _Up>
  constexpr explicit indirect(allocator_arg_t, const _Allocator& a, _Up&& __u) : __alloc_(a) {
    init(static_cast<_Up&&>(__u));
  }
  template <class... _Us>
    requires is_constructible_v<_Tp, _Us...> && is_default_constructible_v<_Allocator>
  constexpr explicit indirect(in_place_t, _Us&&... us) {
    init(static_cast<_Us&&>(us)...);
  }
  template <class... _Us>
    requires is_constructible_v<_Tp, _Us...>
  constexpr explicit indirect(allocator_arg_t, const _Allocator& a, in_place_t, _Us&&... us) : __alloc_(a) {
    init(static_cast<_Us&&>(us)...);
  }
  template <class _Ip, class... _Us>
    requires is_constructible_v<_Tp, initializer_list<_Ip>&, _Us...> && is_default_constructible_v<_Allocator>
  constexpr explicit indirect(in_place_t, initializer_list<_Ip> __ilist, _Us&&... us) {
    init(__ilist, static_cast<_Us&&>(us)...);
  }
  template <class _Ip, class... _Us>
    requires is_constructible_v<_Tp, initializer_list<_Ip>&, _Us...>
  constexpr explicit indirect(allocator_arg_t, const _Allocator& a, in_place_t, initializer_list<_Ip> __ilist, _Us&&... us)
      : __alloc_(a) {
    init(__ilist, static_cast<_Us&&>(us)...);
  }

  // ---- [indirect.dtor] ----
  constexpr ~indirect() {
    static_assert(__ycxx::__detail::__complete_type<_Tp>, "std::indirect: T must be complete where the destructor is used");
    __dispose(__alloc_, __p_);
  }

  // ---- [indirect.assign] ----
  constexpr indirect& operator=(const indirect& other) {
    static_assert(is_copy_assignable_v<_Tp> && is_copy_constructible_v<_Tp>,
                  "std::indirect: copy assignment needs a copy-assignable and copy-constructible T");
    if (__builtin_addressof(other) == this)
      return *this;
    constexpr bool __update = __traits::propagate_on_container_copy_assignment::value;
    if (other.__p_ == nullptr) {
      __dispose(__alloc_, __p_);
      __p_ = nullptr;
    } else if (__p_ != nullptr && __alloc_ == other.__alloc_) {
      *__p_ = *other.__p_;
    } else {
      pointer __np;
      if constexpr (__update) {
        _Allocator a(other.__alloc_);
        __np = __make(a, *other.__p_);
      } else {
        __np = __make(__alloc_, *other.__p_);
      }
      __dispose(__alloc_, __p_);
      __p_ = __np;
    }
    if constexpr (__update)
      __alloc_ = other.__alloc_;
    return *this;
  }
  constexpr indirect& operator=(indirect&& other) noexcept(__traits::propagate_on_container_move_assignment::value ||
                                                          __traits::is_always_equal::value) {
    constexpr bool __update = __traits::propagate_on_container_move_assignment::value;
    if constexpr (!__update && !__traits::is_always_equal::value)
      static_assert(is_move_constructible_v<_Tp>, "std::indirect: move assignment needs a move-constructible T");
    if (__builtin_addressof(other) == this)
      return *this;
    if (other.__p_ == nullptr) {
      __dispose(__alloc_, __p_);
      __p_ = nullptr;
    } else if (__update || __traits::is_always_equal::value || __alloc_ == other.__alloc_) {
      __dispose(__alloc_, __p_);
      __p_ = other.__p_;
      other.__p_ = nullptr;
    } else {
      if constexpr (!__update && !__traits::is_always_equal::value) {
        pointer __np = __make(__alloc_, static_cast<_Tp&&>(*other.__p_));
        __dispose(__alloc_, __p_);
        __p_ = __np;
        __dispose(other.__alloc_, other.__p_); // [indirect.assign]/7 Postconditions: other is valueless.
        other.__p_ = nullptr;
      }
    }
    // "Replaced with a copy of the allocator in other" ([indirect.assign]/7, [polymorphic.assign]/7):
    // by move assignment: an allocator whose propagate_on_container_move_assignment is true need
    // only be Cpp17MoveAssignable, not Cpp17CopyAssignable ([allocator.requirements.general]).
    if constexpr (__update)
      __alloc_ = static_cast<_Allocator&&>(other.__alloc_);
    return *this;
  }
  template <class _Up = _Tp>
    requires(!is_same_v<remove_cvref_t<_Up>, indirect>) && is_constructible_v<_Tp, _Up> && is_assignable_v<_Tp&, _Up>
  constexpr indirect& operator=(_Up&& __u) {
    if (__p_ == nullptr)
      init(static_cast<_Up&&>(__u));
    else
      *__p_ = static_cast<_Up&&>(__u);
    return *this;
  }

  // ---- [indirect.obs] ----
  constexpr const _Tp& operator*() const& noexcept {
    __ycxx::__detail::__precondition(__p_ != nullptr, "std::indirect::operator*: valueless");
    return *__p_;
  }
  constexpr _Tp& operator*() & noexcept {
    __ycxx::__detail::__precondition(__p_ != nullptr, "std::indirect::operator*: valueless");
    return *__p_;
  }
  constexpr const _Tp&& operator*() const&& noexcept {
    __ycxx::__detail::__precondition(__p_ != nullptr, "std::indirect::operator*: valueless");
    return static_cast<const _Tp&&>(*__p_);
  }
  constexpr _Tp&& operator*() && noexcept {
    __ycxx::__detail::__precondition(__p_ != nullptr, "std::indirect::operator*: valueless");
    return static_cast<_Tp&&>(*__p_);
  }
  constexpr const_pointer operator->() const noexcept {
    __ycxx::__detail::__precondition(__p_ != nullptr, "std::indirect::operator->: valueless");
    return __p_;
  }
  constexpr pointer operator->() noexcept {
    __ycxx::__detail::__precondition(__p_ != nullptr, "std::indirect::operator->: valueless");
    return __p_;
  }
  constexpr bool valueless_after_move() const noexcept { return __p_ == nullptr; }
  constexpr allocator_type get_allocator() const noexcept { return __alloc_; }

  // ---- [indirect.swap] ----
  constexpr void swap(indirect& other) noexcept(__traits::propagate_on_container_swap::value ||
                                               __traits::is_always_equal::value) {
    if constexpr (__traits::propagate_on_container_swap::value)
      __ycxx::__detail::__swap_adl::__do_swap(__alloc_, other.__alloc_);
    else
      __ycxx::__detail::__precondition(__traits::is_always_equal::value || __alloc_ == other.__alloc_,
                                 "std::indirect::swap: unequal allocators that do not propagate");
    pointer t = __p_;
    __p_ = other.__p_;
    other.__p_ = t;
  }
  friend constexpr void swap(indirect& __lhs, indirect& __rhs) noexcept(noexcept(__lhs.swap(__rhs))) { __lhs.swap(__rhs); }

  // ---- [indirect.relops] ----
  template <class _Up, class _AA>
  friend constexpr bool operator==(const indirect& __lhs, const indirect<_Up, _AA>& __rhs) noexcept(noexcept(bool(*__lhs ==
                                                                                                          *__rhs))) {
    static_assert(requires { static_cast<bool>(*__lhs == *__rhs); },
                  "std::indirect: operator== needs *lhs == *rhs convertible to bool");
    if (__lhs.__p_ == nullptr || __rhs.__p_ == nullptr)
      return (__lhs.__p_ == nullptr) == (__rhs.__p_ == nullptr);
    return static_cast<bool>(*__lhs.__p_ == *__rhs.__p_);
  }
  template <class _Up, class _AA>
  friend constexpr auto operator<=>(const indirect& __lhs, const indirect<_Up, _AA>& __rhs)
      -> __ycxx::__detail::__synth_three_way_result<_Tp, _Up> {
    if (__lhs.__p_ == nullptr || __rhs.__p_ == nullptr)
      return !(__lhs.__p_ == nullptr) <=> !(__rhs.__p_ == nullptr);
    return __ycxx::__detail::__synth_three_way(*__lhs.__p_, *__rhs.__p_);
  }

  // ---- [indirect.comp.with.t] ----
  // Not for U an indirect: the operators above are more specialized and are chosen for those
  // anyway, and leaving them out keeps the return type's synth-three-way from recursing.
  template <class _Up>
    requires(!__ycxx::__detail::__is_indirect<_Up>)
  friend constexpr bool operator==(const indirect& __lhs, const _Up& __rhs) noexcept(noexcept(bool(*__lhs == __rhs))) {
    static_assert(requires { static_cast<bool>(*__lhs == __rhs); },
                  "std::indirect: operator== needs *lhs == rhs convertible to bool");
    if (__lhs.__p_ == nullptr)
      return false;
    return static_cast<bool>(*__lhs.__p_ == __rhs);
  }
  template <class _Up>
    requires(!__ycxx::__detail::__is_indirect<_Up>)
  friend constexpr auto operator<=>(const indirect& __lhs, const _Up& __rhs) -> __ycxx::__detail::__synth_three_way_result<_Tp, _Up> {
    if (__lhs.__p_ == nullptr)
      return strong_ordering::less;
    return __ycxx::__detail::__synth_three_way(*__lhs.__p_, __rhs);
  }
};

template <class _Value>
indirect(_Value) -> indirect<_Value>;
template <class _Allocator, class _Value>
indirect(allocator_arg_t, _Allocator, _Value)
    -> indirect<_Value, typename allocator_traits<_Allocator>::template rebind_alloc<_Value>>;

// [indirect.hash]: enabled iff hash<T> is; a valueless object hashes to 0.
template <class _Tp, class _Allocator>
  requires __ycxx::__detail::__hash_enabled<_Tp>
struct hash<indirect<_Tp, _Allocator>> {
  constexpr size_t operator()(const indirect<_Tp, _Allocator>& i) const {
    return i.valueless_after_move() ? size_t(0) : hash<_Tp>()(*i);
  }
};

namespace pmr {
template <class _Tp>
using indirect = std::indirect<_Tp, polymorphic_allocator<_Tp>>;
} // namespace pmr

} // namespace std
