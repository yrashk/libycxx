// libycxx core: polymorphic ([polymorphic], P3019), a value-semantic owner of one object of T or
// of a type derived from T, allocated through an allocator.
//
// The owned object of type U lives in a block (poly_block_for<T, Allocator, U>) allocated with
// the allocator rebound to the block type; the object itself is constructed and destroyed with
// allocator_traits<Allocator>::construct/destroy ([polymorphic.general]/3). The block's virtual
// functions copy, move and destroy the object as its own type U, so copies never slice and the
// destructor of T need not be virtual. A null block pointer is the valueless state. Everything
// is constexpr (constexpr virtual functions).
#pragma once

#include <ycxx/core/indirect.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Tp, class _Ap>
struct __poly_block {
  _Tp* __obj; // the owned object, as a T
  constexpr virtual __poly_block* __clone(_Ap& a) const = 0;
  constexpr virtual __poly_block* __move_to(_Ap& a) = 0;
  // Destroys the owned object with a, then the block itself, and frees it with a.
  constexpr virtual void __dispose(_Ap& a) noexcept = 0;

protected:
  constexpr __poly_block() noexcept = default;
  constexpr ~__poly_block() = default;
};

template <class _Tp, class _Ap, class _Up>
struct __poly_block_for final : __poly_block<_Tp, _Ap> {
  using __traits = std::allocator_traits<_Ap>;
  using __block_alloc = typename __traits::template rebind_alloc<__poly_block_for>;
  using __block_traits = std::allocator_traits<__block_alloc>;

  union {
    _Up __u;
  };
  constexpr __poly_block_for() noexcept {}
  constexpr ~__poly_block_for() {}

  // A new block owning a U constructed with args using a; nothing leaks if construction throws.
  template <class... _Args>
  static constexpr __poly_block_for* __make(_Ap& a, _Args&&... __args) {
    __block_alloc __ba(a);
    auto __bp = __block_traits::allocate(__ba, 1);
    __poly_block_for* b = std::to_address(__bp);
    std::construct_at(b);
    __ycxx::__detail::__rollback __guard{[&] {
      std::destroy_at(b);
      __block_traits::deallocate(__ba, __bp, 1);
    }};
    __traits::construct(a, __builtin_addressof(b->__u), static_cast<_Args&&>(__args)...);
    __guard.release();
    b->__obj = __builtin_addressof(b->__u);
    return b;
  }

  constexpr __poly_block<_Tp, _Ap>* __clone(_Ap& a) const override { return __make(a, static_cast<const _Up&>(__u)); }
  constexpr __poly_block<_Tp, _Ap>* __move_to(_Ap& a) override { return __make(a, static_cast<_Up&&>(__u)); }
  constexpr void __dispose(_Ap& a) noexcept override {
    __traits::destroy(a, __builtin_addressof(__u));
    __block_alloc __ba(a);
    auto __bp = __ycxx::__detail::__to_alloc_pointer<typename __block_traits::pointer>(this);
    __poly_block_for* __self = this;
    std::destroy_at(__self);
    __block_traits::deallocate(__ba, __bp, 1);
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp, class _Allocator = allocator<_Tp>>
class polymorphic {
  static_assert(__ycxx::__detail::__composite_value_ok<_Tp>,
                "std::polymorphic: T must be a cv-unqualified object type that is not an array, in_place_t or "
                "a specialization of in_place_type_t");
  static_assert(is_same_v<typename allocator_traits<_Allocator>::value_type, _Tp>,
                "std::polymorphic: allocator_traits<Allocator>::value_type must be T");

  using __traits = allocator_traits<_Allocator>;
  using block = __ycxx::__detail::__poly_block<_Tp, _Allocator>;
  template <class _Up>
  using __block_for = __ycxx::__detail::__poly_block_for<_Tp, _Allocator, _Up>;

public:
  using value_type = _Tp;
  using allocator_type = _Allocator;
  using pointer = typename __traits::pointer;
  using const_pointer = typename __traits::const_pointer;

private:
  block* __b_ = nullptr;
  [[no_unique_address]] _Allocator __alloc_ = _Allocator();

  template <class _Up, class... _Args>
  constexpr void init(_Args&&... __args) {
    __b_ = __block_for<_Up>::__make(__alloc_, static_cast<_Args&&>(__args)...);
  }
  static constexpr void __dispose(_Allocator& a, block* b) noexcept {
    if (b != nullptr)
      b->__dispose(a);
  }
  // [polymorphic.ctor]/12-22: the constraints shared by the converting and in-place constructors.
  template <class _Up, class... _Args>
  static constexpr bool __owned_ok =
      derived_from<_Up, _Tp> && is_constructible_v<_Up, _Args...> && is_copy_constructible_v<_Up>;

public:
  // ---- [polymorphic.ctor] ----
  constexpr explicit polymorphic()
    requires is_default_constructible_v<_Allocator>
  {
    static_assert(is_default_constructible_v<_Tp> && is_copy_constructible_v<_Tp>,
                  "std::polymorphic(): T must be default and copy constructible");
    init<_Tp>();
  }
  constexpr explicit polymorphic(allocator_arg_t, const _Allocator& a) : __alloc_(a) {
    static_assert(is_default_constructible_v<_Tp> && is_copy_constructible_v<_Tp>,
                  "std::polymorphic(allocator_arg_t, a): T must be default and copy constructible");
    init<_Tp>();
  }
  constexpr polymorphic(const polymorphic& other)
      : __alloc_(__traits::select_on_container_copy_construction(other.__alloc_)) {
    if (other.__b_ != nullptr)
      __b_ = other.__b_->__clone(__alloc_);
  }
  constexpr polymorphic(allocator_arg_t, const _Allocator& a, const polymorphic& other) : __alloc_(a) {
    if (other.__b_ != nullptr)
      __b_ = other.__b_->__clone(__alloc_);
  }
  constexpr polymorphic(polymorphic&& other) noexcept
      : __b_(other.__b_), __alloc_(static_cast<_Allocator&&>(other.__alloc_)) {
    other.__b_ = nullptr;
  }
  constexpr polymorphic(allocator_arg_t, const _Allocator& a, polymorphic&& other) noexcept(
      __traits::is_always_equal::value)
      : __alloc_(a) {
    if (other.__b_ == nullptr)
      return;
    if (__traits::is_always_equal::value || __alloc_ == other.__alloc_) {
      __b_ = other.__b_;
      other.__b_ = nullptr;
    } else {
      if constexpr (!__traits::is_always_equal::value) {
        __b_ = other.__b_->__move_to(__alloc_);
        // Like indirect ([indirect.ctor]/16), other becomes valueless.
        __dispose(other.__alloc_, other.__b_);
        other.__b_ = nullptr;
      }
    }
  }
  template <class _Up = _Tp>
    requires(!is_same_v<remove_cvref_t<_Up>, polymorphic>) && __owned_ok<remove_cvref_t<_Up>, _Up> &&
            (!__ycxx::__detail::__is_in_place_type<remove_cvref_t<_Up>>) && is_default_constructible_v<_Allocator>
  constexpr explicit polymorphic(_Up&& __u) {
    init<remove_cvref_t<_Up>>(static_cast<_Up&&>(__u));
  }
  template <class _Up = _Tp>
    requires(!is_same_v<remove_cvref_t<_Up>, polymorphic>) && __owned_ok<remove_cvref_t<_Up>, _Up> &&
            (!__ycxx::__detail::__is_in_place_type<remove_cvref_t<_Up>>)
  constexpr explicit polymorphic(allocator_arg_t, const _Allocator& a, _Up&& __u) : __alloc_(a) {
    init<remove_cvref_t<_Up>>(static_cast<_Up&&>(__u));
  }
  template <class _Up, class... _Ts>
    requires is_same_v<remove_cvref_t<_Up>, _Up> && __owned_ok<_Up, _Ts...> && is_default_constructible_v<_Allocator>
  constexpr explicit polymorphic(in_place_type_t<_Up>, _Ts&&... __ts) {
    init<_Up>(static_cast<_Ts&&>(__ts)...);
  }
  template <class _Up, class... _Ts>
    requires is_same_v<remove_cvref_t<_Up>, _Up> && __owned_ok<_Up, _Ts...>
  constexpr explicit polymorphic(allocator_arg_t, const _Allocator& a, in_place_type_t<_Up>, _Ts&&... __ts) : __alloc_(a) {
    init<_Up>(static_cast<_Ts&&>(__ts)...);
  }
  template <class _Up, class _Ip, class... _Us>
    requires is_same_v<remove_cvref_t<_Up>, _Up> && __owned_ok<_Up, initializer_list<_Ip>&, _Us...> &&
             is_default_constructible_v<_Allocator>
  constexpr explicit polymorphic(in_place_type_t<_Up>, initializer_list<_Ip> __ilist, _Us&&... us) {
    init<_Up>(__ilist, static_cast<_Us&&>(us)...);
  }
  template <class _Up, class _Ip, class... _Us>
    requires is_same_v<remove_cvref_t<_Up>, _Up> && __owned_ok<_Up, initializer_list<_Ip>&, _Us...>
  constexpr explicit polymorphic(allocator_arg_t, const _Allocator& a, in_place_type_t<_Up>, initializer_list<_Ip> __ilist,
                                 _Us&&... us)
      : __alloc_(a) {
    init<_Up>(__ilist, static_cast<_Us&&>(us)...);
  }

  // ---- [polymorphic.dtor] ----
  constexpr ~polymorphic() {
    static_assert(__ycxx::__detail::__complete_type<_Tp>,
                  "std::polymorphic: T must be complete where the destructor is used");
    __dispose(__alloc_, __b_);
  }

  // ---- [polymorphic.assign] ----
  constexpr polymorphic& operator=(const polymorphic& other) {
    static_assert(__ycxx::__detail::__complete_type<_Tp>, "std::polymorphic: T must be complete for copy assignment");
    if (__builtin_addressof(other) == this)
      return *this;
    constexpr bool __update = __traits::propagate_on_container_copy_assignment::value;
    block* __nb = nullptr;
    if (other.__b_ != nullptr) {
      if constexpr (__update) {
        _Allocator a(other.__alloc_);
        __nb = other.__b_->__clone(a);
      } else {
        __nb = other.__b_->__clone(__alloc_);
      }
    }
    __dispose(__alloc_, __b_);
    __b_ = __nb;
    if constexpr (__update)
      __alloc_ = other.__alloc_;
    return *this;
  }
  constexpr polymorphic& operator=(polymorphic&& other) noexcept(
      __traits::propagate_on_container_move_assignment::value || __traits::is_always_equal::value) {
    constexpr bool __update = __traits::propagate_on_container_move_assignment::value;
    if constexpr (!__update && !__traits::is_always_equal::value)
      static_assert(__ycxx::__detail::__complete_type<_Tp>, "std::polymorphic: T must be complete for move assignment");
    if (__builtin_addressof(other) == this)
      return *this;
    if (other.__b_ == nullptr) {
      __dispose(__alloc_, __b_);
      __b_ = nullptr;
    } else if (__update || __traits::is_always_equal::value || __alloc_ == other.__alloc_) {
      __dispose(__alloc_, __b_);
      __b_ = other.__b_;
      other.__b_ = nullptr;
    } else {
      if constexpr (!__update && !__traits::is_always_equal::value) {
        block* __nb = other.__b_->__move_to(__alloc_);
        __dispose(__alloc_, __b_);
        __b_ = __nb;
        __dispose(other.__alloc_, other.__b_);
        other.__b_ = nullptr;
      }
    }
    // "Replaced with a copy of the allocator in other" ([indirect.assign]/7, [polymorphic.assign]/7):
    // by move assignment: an allocator whose propagate_on_container_move_assignment is true need
    // only be Cpp17MoveAssignable, not Cpp17CopyAssignable ([allocator.requirements.general]).
    if constexpr (__update)
      __alloc_ = static_cast<_Allocator&&>(other.__alloc_);
    return *this;
  }

  // ---- [polymorphic.obs] ----
  constexpr const _Tp& operator*() const noexcept {
    __ycxx::__detail::__precondition(__b_ != nullptr, "std::polymorphic::operator*: valueless");
    return *__b_->__obj;
  }
  constexpr _Tp& operator*() noexcept {
    __ycxx::__detail::__precondition(__b_ != nullptr, "std::polymorphic::operator*: valueless");
    return *__b_->__obj;
  }
  constexpr const_pointer operator->() const noexcept {
    __ycxx::__detail::__precondition(__b_ != nullptr, "std::polymorphic::operator->: valueless");
    return __ycxx::__detail::__to_alloc_pointer<const_pointer>(static_cast<const _Tp*>(__b_->__obj));
  }
  constexpr pointer operator->() noexcept {
    __ycxx::__detail::__precondition(__b_ != nullptr, "std::polymorphic::operator->: valueless");
    return __ycxx::__detail::__to_alloc_pointer<pointer>(__b_->__obj);
  }
  constexpr bool valueless_after_move() const noexcept { return __b_ == nullptr; }
  constexpr allocator_type get_allocator() const noexcept { return __alloc_; }

  // ---- [polymorphic.swap] ----
  constexpr void swap(polymorphic& other) noexcept(__traits::propagate_on_container_swap::value ||
                                                  __traits::is_always_equal::value) {
    if constexpr (__traits::propagate_on_container_swap::value)
      __ycxx::__detail::__swap_adl::__do_swap(__alloc_, other.__alloc_);
    else
      __ycxx::__detail::__precondition(__traits::is_always_equal::value || __alloc_ == other.__alloc_,
                                 "std::polymorphic::swap: unequal allocators that do not propagate");
    block* t = __b_;
    __b_ = other.__b_;
    other.__b_ = t;
  }
  friend constexpr void swap(polymorphic& __lhs, polymorphic& __rhs) noexcept(noexcept(__lhs.swap(__rhs))) {
    __lhs.swap(__rhs);
  }
};

} // namespace std
