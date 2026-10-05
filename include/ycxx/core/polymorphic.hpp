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

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template <class T, class A>
struct poly_block {
  T* obj; // the owned object, as a T
  constexpr virtual poly_block* clone(A& a) const = 0;
  constexpr virtual poly_block* move_to(A& a) = 0;
  // Destroys the owned object with a, then the block itself, and frees it with a.
  constexpr virtual void dispose(A& a) noexcept = 0;

protected:
  constexpr poly_block() noexcept = default;
  constexpr ~poly_block() = default;
};

template <class T, class A, class U>
struct poly_block_for final : poly_block<T, A> {
  using traits = std::allocator_traits<A>;
  using block_alloc = typename traits::template rebind_alloc<poly_block_for>;
  using block_traits = std::allocator_traits<block_alloc>;

  union {
    U u;
  };
  constexpr poly_block_for() noexcept {}
  constexpr ~poly_block_for() {}

  // A new block owning a U constructed with args using a; nothing leaks if construction throws.
  template <class... Args>
  static constexpr poly_block_for* make(A& a, Args&&... args) {
    block_alloc ba(a);
    auto bp = block_traits::allocate(ba, 1);
    poly_block_for* b = std::to_address(bp);
    std::construct_at(b);
    ycxx::detail::rollback guard{[&] {
      std::destroy_at(b);
      block_traits::deallocate(ba, bp, 1);
    }};
    traits::construct(a, __builtin_addressof(b->u), static_cast<Args&&>(args)...);
    guard.release();
    b->obj = __builtin_addressof(b->u);
    return b;
  }

  constexpr poly_block<T, A>* clone(A& a) const override { return make(a, static_cast<const U&>(u)); }
  constexpr poly_block<T, A>* move_to(A& a) override { return make(a, static_cast<U&&>(u)); }
  constexpr void dispose(A& a) noexcept override {
    traits::destroy(a, __builtin_addressof(u));
    block_alloc ba(a);
    auto bp = ycxx::detail::to_alloc_pointer<typename block_traits::pointer>(this);
    poly_block_for* self = this;
    std::destroy_at(self);
    block_traits::deallocate(ba, bp, 1);
  }
};

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

template <class T, class Allocator = allocator<T>>
class polymorphic {
  static_assert(ycxx::detail::composite_value_ok<T>,
                "std::polymorphic: T must be a cv-unqualified object type that is not an array, in_place_t or "
                "a specialization of in_place_type_t");
  static_assert(is_same_v<typename allocator_traits<Allocator>::value_type, T>,
                "std::polymorphic: allocator_traits<Allocator>::value_type must be T");

  using traits = allocator_traits<Allocator>;
  using block = ycxx::detail::poly_block<T, Allocator>;
  template <class U>
  using block_for = ycxx::detail::poly_block_for<T, Allocator, U>;

public:
  using value_type = T;
  using allocator_type = Allocator;
  using pointer = typename traits::pointer;
  using const_pointer = typename traits::const_pointer;

private:
  block* b_ = nullptr;
  [[no_unique_address]] Allocator alloc_ = Allocator();

  template <class U, class... Args>
  constexpr void init(Args&&... args) {
    b_ = block_for<U>::make(alloc_, static_cast<Args&&>(args)...);
  }
  static constexpr void dispose(Allocator& a, block* b) noexcept {
    if (b != nullptr)
      b->dispose(a);
  }
  // [polymorphic.ctor]/12-22: the constraints shared by the converting and in-place constructors.
  template <class U, class... Args>
  static constexpr bool owned_ok =
      derived_from<U, T> && is_constructible_v<U, Args...> && is_copy_constructible_v<U>;

public:
  // ---- [polymorphic.ctor] ----
  constexpr explicit polymorphic()
    requires is_default_constructible_v<Allocator>
  {
    static_assert(is_default_constructible_v<T> && is_copy_constructible_v<T>,
                  "std::polymorphic(): T must be default and copy constructible");
    init<T>();
  }
  constexpr explicit polymorphic(allocator_arg_t, const Allocator& a) : alloc_(a) {
    static_assert(is_default_constructible_v<T> && is_copy_constructible_v<T>,
                  "std::polymorphic(allocator_arg_t, a): T must be default and copy constructible");
    init<T>();
  }
  constexpr polymorphic(const polymorphic& other)
      : alloc_(traits::select_on_container_copy_construction(other.alloc_)) {
    if (other.b_ != nullptr)
      b_ = other.b_->clone(alloc_);
  }
  constexpr polymorphic(allocator_arg_t, const Allocator& a, const polymorphic& other) : alloc_(a) {
    if (other.b_ != nullptr)
      b_ = other.b_->clone(alloc_);
  }
  constexpr polymorphic(polymorphic&& other) noexcept
      : b_(other.b_), alloc_(static_cast<Allocator&&>(other.alloc_)) {
    other.b_ = nullptr;
  }
  constexpr polymorphic(allocator_arg_t, const Allocator& a, polymorphic&& other) noexcept(
      traits::is_always_equal::value)
      : alloc_(a) {
    if (other.b_ == nullptr)
      return;
    if (traits::is_always_equal::value || alloc_ == other.alloc_) {
      b_ = other.b_;
      other.b_ = nullptr;
    } else {
      if constexpr (!traits::is_always_equal::value) {
        b_ = other.b_->move_to(alloc_);
        // Like indirect ([indirect.ctor]/16), other becomes valueless.
        dispose(other.alloc_, other.b_);
        other.b_ = nullptr;
      }
    }
  }
  template <class U = T>
    requires(!is_same_v<remove_cvref_t<U>, polymorphic>) && owned_ok<remove_cvref_t<U>, U> &&
            (!ycxx::detail::is_in_place_type<remove_cvref_t<U>>) && is_default_constructible_v<Allocator>
  constexpr explicit polymorphic(U&& u) {
    init<remove_cvref_t<U>>(static_cast<U&&>(u));
  }
  template <class U = T>
    requires(!is_same_v<remove_cvref_t<U>, polymorphic>) && owned_ok<remove_cvref_t<U>, U> &&
            (!ycxx::detail::is_in_place_type<remove_cvref_t<U>>)
  constexpr explicit polymorphic(allocator_arg_t, const Allocator& a, U&& u) : alloc_(a) {
    init<remove_cvref_t<U>>(static_cast<U&&>(u));
  }
  template <class U, class... Ts>
    requires is_same_v<remove_cvref_t<U>, U> && owned_ok<U, Ts...> && is_default_constructible_v<Allocator>
  constexpr explicit polymorphic(in_place_type_t<U>, Ts&&... ts) {
    init<U>(static_cast<Ts&&>(ts)...);
  }
  template <class U, class... Ts>
    requires is_same_v<remove_cvref_t<U>, U> && owned_ok<U, Ts...>
  constexpr explicit polymorphic(allocator_arg_t, const Allocator& a, in_place_type_t<U>, Ts&&... ts) : alloc_(a) {
    init<U>(static_cast<Ts&&>(ts)...);
  }
  template <class U, class I, class... Us>
    requires is_same_v<remove_cvref_t<U>, U> && owned_ok<U, initializer_list<I>&, Us...> &&
             is_default_constructible_v<Allocator>
  constexpr explicit polymorphic(in_place_type_t<U>, initializer_list<I> ilist, Us&&... us) {
    init<U>(ilist, static_cast<Us&&>(us)...);
  }
  template <class U, class I, class... Us>
    requires is_same_v<remove_cvref_t<U>, U> && owned_ok<U, initializer_list<I>&, Us...>
  constexpr explicit polymorphic(allocator_arg_t, const Allocator& a, in_place_type_t<U>, initializer_list<I> ilist,
                                 Us&&... us)
      : alloc_(a) {
    init<U>(ilist, static_cast<Us&&>(us)...);
  }

  // ---- [polymorphic.dtor] ----
  constexpr ~polymorphic() {
    static_assert(ycxx::detail::complete_type<T>,
                  "std::polymorphic: T must be complete where the destructor is used");
    dispose(alloc_, b_);
  }

  // ---- [polymorphic.assign] ----
  constexpr polymorphic& operator=(const polymorphic& other) {
    static_assert(ycxx::detail::complete_type<T>, "std::polymorphic: T must be complete for copy assignment");
    if (__builtin_addressof(other) == this)
      return *this;
    constexpr bool update = traits::propagate_on_container_copy_assignment::value;
    block* nb = nullptr;
    if (other.b_ != nullptr) {
      if constexpr (update) {
        Allocator a(other.alloc_);
        nb = other.b_->clone(a);
      } else {
        nb = other.b_->clone(alloc_);
      }
    }
    dispose(alloc_, b_);
    b_ = nb;
    if constexpr (update)
      alloc_ = other.alloc_;
    return *this;
  }
  constexpr polymorphic& operator=(polymorphic&& other) noexcept(
      traits::propagate_on_container_move_assignment::value || traits::is_always_equal::value) {
    constexpr bool update = traits::propagate_on_container_move_assignment::value;
    if constexpr (!update && !traits::is_always_equal::value)
      static_assert(ycxx::detail::complete_type<T>, "std::polymorphic: T must be complete for move assignment");
    if (__builtin_addressof(other) == this)
      return *this;
    if (other.b_ == nullptr) {
      dispose(alloc_, b_);
      b_ = nullptr;
    } else if (update || traits::is_always_equal::value || alloc_ == other.alloc_) {
      dispose(alloc_, b_);
      b_ = other.b_;
      other.b_ = nullptr;
    } else {
      if constexpr (!update && !traits::is_always_equal::value) {
        block* nb = other.b_->move_to(alloc_);
        dispose(alloc_, b_);
        b_ = nb;
        dispose(other.alloc_, other.b_);
        other.b_ = nullptr;
      }
    }
    if constexpr (update)
      alloc_ = other.alloc_;
    return *this;
  }

  // ---- [polymorphic.obs] ----
  constexpr const T& operator*() const noexcept {
    ycxx::detail::precondition(b_ != nullptr, "std::polymorphic::operator*: valueless");
    return *b_->obj;
  }
  constexpr T& operator*() noexcept {
    ycxx::detail::precondition(b_ != nullptr, "std::polymorphic::operator*: valueless");
    return *b_->obj;
  }
  constexpr const_pointer operator->() const noexcept {
    ycxx::detail::precondition(b_ != nullptr, "std::polymorphic::operator->: valueless");
    return ycxx::detail::to_alloc_pointer<const_pointer>(static_cast<const T*>(b_->obj));
  }
  constexpr pointer operator->() noexcept {
    ycxx::detail::precondition(b_ != nullptr, "std::polymorphic::operator->: valueless");
    return ycxx::detail::to_alloc_pointer<pointer>(b_->obj);
  }
  constexpr bool valueless_after_move() const noexcept { return b_ == nullptr; }
  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  // ---- [polymorphic.swap] ----
  constexpr void swap(polymorphic& other) noexcept(traits::propagate_on_container_swap::value ||
                                                  traits::is_always_equal::value) {
    if constexpr (traits::propagate_on_container_swap::value)
      ycxx::detail::swap_adl::do_swap(alloc_, other.alloc_);
    else
      ycxx::detail::precondition(traits::is_always_equal::value || alloc_ == other.alloc_,
                                 "std::polymorphic::swap: unequal allocators that do not propagate");
    block* t = b_;
    b_ = other.b_;
    other.b_ = t;
  }
  friend constexpr void swap(polymorphic& lhs, polymorphic& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
  }
};

} // namespace std
