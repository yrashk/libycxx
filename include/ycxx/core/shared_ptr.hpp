// libycxx core: shared ownership ([util.sharedptr]): bad_weak_ptr, shared_ptr, weak_ptr,
// enable_shared_from_this, make_shared / allocate_shared (all forms), the pointer casts,
// get_deleter, owner_less / owner_hash / owner_equal and hash<shared_ptr>.
//
// Every owning shared_ptr points to a control block (ycxx::detail::sp_block) holding the use
// count and the weak count (the number of weak_ptrs, plus one while the use count is nonzero).
// The counts are updated with the compiler's __atomic builtins at run time (plain arithmetic while
// the process is single-threaded) and with plain arithmetic during constant evaluation. Three block kinds exist:
//   - sp_ptr_block: an adopted pointer, its deleter and an allocator (the constructors);
//   - sp_obj_block: the object itself, in a union member (make_shared for non-arrays);
//   - sp_array_block: an array; at run time the elements follow the block in the same
//     allocation, during constant evaluation they are allocated separately (make_shared for
//     arrays).
// get_deleter identifies the deleter type by the address of a per-type tag variable, so it
// needs no RTTI and works in constant evaluation (the same caveat as std::any applies across
// shared libraries built with hidden visibility).
//
// atomic<shared_ptr<T>> / atomic<weak_ptr<T>> are in atomic_smart_ptr.hpp.
#pragma once

#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/unique_ptr.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/exception_base.hpp>
#include <ycxx/core/single_threaded.hpp>

namespace [[gnu::visibility("hidden")]] std {

// [util.smartptr.weak.bad]
class bad_weak_ptr : public exception {
public:
  constexpr bad_weak_ptr() noexcept {}
  constexpr bad_weak_ptr(const bad_weak_ptr&) noexcept = default;
  constexpr bad_weak_ptr& operator=(const bad_weak_ptr&) noexcept = default;
  constexpr ~bad_weak_ptr() override {}
  constexpr const char* what() const noexcept override { return "bad_weak_ptr"; }
};

template <class T>
class shared_ptr;
template <class T>
class weak_ptr;
template <class T>
class enable_shared_from_this;

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

[[noreturn]] [[gnu::cold]] constexpr void throw_bad_weak_ptr() {
  ::ycxx::detail::raise_with(ycxx_error_bad_weak_ptr, "std::bad_weak_ptr", [] { return std::bad_weak_ptr(); });
}

// The address of sp_tag<D> identifies the deleter type D (get_deleter).
template <class D>
inline constexpr char sp_tag = 0;

// ---------------------------------------------------------------------------------------------
// Control blocks
// ---------------------------------------------------------------------------------------------
class sp_block {
  long shared_ = 1; // shared_ptr owners
  long weak_ = 1;   // weak_ptrs, plus one while shared_ != 0

  // Destroys the owned object (use count reached zero).
  constexpr virtual void dispose() noexcept = 0;

protected:
  // Destroys and deallocates the block (weak count reached zero). A function pointer rather
  // than a virtual function: its body rebinds the allocator to the block type, and Clang
  // instantiates constexpr virtual members while instantiating the class, so an allocator that
  // requires a complete value_type (libc++'s complete_type_allocator) would see the block
  // incomplete.
  using destroy_fn = void (*)(sp_block*) noexcept;
  destroy_fn destroy_;

  constexpr explicit sp_block(destroy_fn d) noexcept : destroy_(d) {}
  constexpr ~sp_block() = default;

public:
  sp_block(const sp_block&) = delete;
  sp_block& operator=(const sp_block&) = delete;

  // The stored deleter, if its type is the one `tag` identifies.
  constexpr virtual void* deleter(const void* tag) noexcept {
    (void)tag;
    return nullptr;
  }

  // At run time the counts are atomic unless the process is single-threaded
  // (single_threaded.hpp). Increments are relaxed: a new reference is always made from an
  // existing one. A decrement releases and acquires (ref_release, single_threaded.hpp), so
  // everything the other owners did happens before the destruction.
  constexpr void add_shared() noexcept {
    if consteval {
      ++shared_;
    } else {
      if (::ycxx::detail::single_threaded())
        ++shared_;
      else
        __atomic_fetch_add(&shared_, 1, __ATOMIC_RELAXED);
    }
  }
  // Takes a new shared reference unless the use count is already zero (weak_ptr::lock).
  constexpr bool try_add_shared() noexcept {
    if consteval {
      if (shared_ == 0)
        return false;
      ++shared_;
      return true;
    } else {
      if (::ycxx::detail::single_threaded()) {
        if (shared_ == 0)
          return false;
        ++shared_;
        return true;
      }
      long n = __atomic_load_n(&shared_, __ATOMIC_RELAXED);
      while (n != 0)
        if (__atomic_compare_exchange_n(&shared_, &n, n + 1, true, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
          return true;
      return false;
    }
  }
  constexpr void release_shared() noexcept {
    long n;
    if consteval {
      n = --shared_;
    } else {
      if (::ycxx::detail::single_threaded()) {
        n = --shared_;
      } else {
        n = __atomic_sub_fetch(&shared_, 1, __ATOMIC_ACQ_REL);
      }
    }
    if (n == 0) {
      dispose();
      release_weak();
    }
  }
  constexpr void add_weak() noexcept {
    if consteval {
      ++weak_;
    } else {
      if (::ycxx::detail::single_threaded())
        ++weak_;
      else
        __atomic_fetch_add(&weak_, 1, __ATOMIC_RELAXED);
    }
  }
  constexpr void release_weak() noexcept {
    bool last;
    if consteval {
      last = --weak_ == 0;
    } else {
      if (::ycxx::detail::single_threaded()) {
        last = --weak_ == 0;
      } else {
        // A count of one is the caller's own reference, and no other can appear: the use count
        // is zero (else it would hold one more), so neither a shared_ptr nor another weak_ptr
        // exists to make one. The usual case (no weak_ptr at all) then needs no atomic RMW.
        last = __atomic_load_n(&weak_, __ATOMIC_ACQUIRE) == 1 || __atomic_sub_fetch(&weak_, 1, __ATOMIC_ACQ_REL) == 0;
      }
    }
    if (last)
      destroy_(this);
  }
  constexpr long use_count() const noexcept {
    if consteval {
      return shared_;
    } else {
      return __atomic_load_n(&shared_, __ATOMIC_RELAXED);
    }
  }
};

// Allocates one Block with a copy of `a` rebound to Block, and deallocates it again.
template <class Block, class A>
using sp_block_alloc = typename std::allocator_traits<A>::template rebind_alloc<Block>;

template <class Block, class A>
constexpr Block* sp_allocate_block(const A& a) {
  sp_block_alloc<Block, A> ba(a);
  return std::to_address(std::allocator_traits<sp_block_alloc<Block, A>>::allocate(ba, 1));
}
template <class Block, class A>
constexpr void sp_deallocate_block(const A& a, Block* b) noexcept {
  using BA = sp_block_alloc<Block, A>;
  using Ptr = typename std::allocator_traits<BA>::pointer;
  BA ba(a);
  std::allocator_traits<BA>::deallocate(ba, std::pointer_traits<Ptr>::pointer_to(*b), 1);
}

// The deleter used when a shared_ptr adopts a pointer without one ([util.smartptr.shared.const]
// /6): `delete p` or `delete[] p`. It is not a user-visible deleter, so get_deleter never finds it.
template <bool Array>
struct sp_default_delete {
  template <class P>
  constexpr void operator()(P* p) const noexcept {
    if constexpr (Array)
      delete[] p;
    else
      delete p;
  }
};

// A pointer P, its deleter D and an allocator A (rebound to this block for deallocation).
template <class P, class D, class A>
class sp_ptr_block final : public sp_block {
  P p_;
  [[no_unique_address]] D d_;
  [[no_unique_address]] A a_;

  constexpr void dispose() noexcept override { d_(p_); }
  static constexpr void destroy_self(sp_block* b) noexcept {
    sp_ptr_block* self = static_cast<sp_ptr_block*>(b);
    A a(self->a_);
    std::destroy_at(self);
    ::ycxx::detail::sp_deallocate_block(a, self);
  }

public:
  constexpr sp_ptr_block(P p, D&& d, const A& a) noexcept
      : sp_block(&destroy_self), p_(p), d_(static_cast<D&&>(d)), a_(a) {}
  constexpr ~sp_ptr_block() = default;

  constexpr void* deleter(const void* tag) noexcept override {
    if constexpr (std::is_same_v<D, sp_default_delete<true>> || std::is_same_v<D, sp_default_delete<false>>)
      return nullptr;
    else
      return tag == &sp_tag<D> ? __builtin_addressof(d_) : nullptr;
  }
};

// Creates a block owning p with deleter d; if that fails, d(p) is called and the exception
// propagates ([util.smartptr.shared.const]/11).
template <class P, class D, class A>
constexpr sp_block* sp_make_ptr_block(P p, D& d, const A& a) {
  using Block = sp_ptr_block<P, D, A>;
  if constexpr (cfg::exceptions) {
    Block* b;
    try {
      b = ::ycxx::detail::sp_allocate_block<Block>(a);
    } catch (...) {
      d(p);
      throw;
    }
    return std::construct_at(b, p, static_cast<D&&>(d), a);
  } else {
    return std::construct_at(::ycxx::detail::sp_allocate_block<Block>(a), p, static_cast<D&&>(d), a);
  }
}

// How make_shared & co. initialize each non-array subobject ([util.smartptr.shared.create]/7).
enum class sp_init : unsigned char {
  value,     // U() or allocator construct(a, p)
  overwrite, // default-initialized: ::new(pv) U
  fill,      // every element from *u (u has the element type)
  copy,      // element i from u[i]
};

// Constructs n elements of type E (cv-unqualified, possibly an array type) at p, in ascending
// order of address. ViaAlloc: through allocator_traits<A>::construct (allocate_shared),
// otherwise with placement new (make_shared, *_for_overwrite). On an exception the elements
// already constructed are destroyed in reverse order.
template <bool ViaAlloc, class A, class E>
constexpr void sp_destroy_n(A& a, E* p, std::size_t n) noexcept {
  while (n != 0) {
    --n;
    if constexpr (std::is_array_v<E>)
      ::ycxx::detail::sp_destroy_n<ViaAlloc>(a, &p[n][0], std::extent_v<E>);
    else if constexpr (ViaAlloc)
      std::allocator_traits<A>::destroy(a, p + n);
    else
      p[n].~E();
  }
}

template <bool ViaAlloc, class A, class E>
struct sp_construct_guard {
  A& a;
  E* p;
  std::size_t* done;
  constexpr ~sp_construct_guard() {
    if (done)
      ::ycxx::detail::sp_destroy_n<ViaAlloc>(a, p, *done);
  }
};

template <bool ViaAlloc, sp_init How, class A, class E>
constexpr void sp_construct_n(A& a, E* p, std::size_t n, const E* u) {
  std::size_t i = 0;
  sp_construct_guard<ViaAlloc, A, E> g{a, p, &i};
  for (; i < n; ++i) {
    if constexpr (std::is_array_v<E>) {
      constexpr std::size_t m = std::extent_v<E>;
      if constexpr (std::is_trivially_default_constructible_v<E> && std::is_trivially_destructible_v<E>) {
        // Clang's constant evaluator does not let the element constructions below begin the
        // lifetime of the enclosing array p[i]; begin it first (no observable effect here).
        if consteval {
          ::new (static_cast<void*>(__builtin_addressof(p[i]))) E;
        }
      }
      if constexpr (How == sp_init::fill)
        ::ycxx::detail::sp_construct_n<ViaAlloc, sp_init::copy>(a, &p[i][0], m, &(*u)[0]);
      else if constexpr (How == sp_init::copy)
        ::ycxx::detail::sp_construct_n<ViaAlloc, sp_init::copy>(a, &p[i][0], m, &u[i][0]);
      else
        ::ycxx::detail::sp_construct_n<ViaAlloc, How>(a, &p[i][0], m,
                                                      static_cast<const std::remove_extent_t<E>*>(nullptr));
    } else {
      void* pv = __builtin_addressof(p[i]);
      if constexpr (How == sp_init::overwrite)
        ::new (pv) E;
      else if constexpr (How == sp_init::value && ViaAlloc)
        std::allocator_traits<A>::construct(a, p + i);
      else if constexpr (How == sp_init::value)
        ::new (pv) E();
      else if constexpr (ViaAlloc)
        std::allocator_traits<A>::construct(a, p + i, How == sp_init::fill ? *u : u[i]);
      else
        ::new (pv) E(How == sp_init::fill ? *u : u[i]);
    }
  }
  g.done = nullptr;
}

// make_shared / allocate_shared of a non-array T: the object lives in the block.
// A is the allocator rebound to remove_cv_t<T>; ViaAlloc as above.
template <class T, class A, bool ViaAlloc>
class sp_obj_block final : public sp_block {
  using U = std::remove_cv_t<T>;
  [[no_unique_address]] A a_;

public:
  union {
    U value;
  };

private:
  constexpr void dispose() noexcept override {
    if constexpr (ViaAlloc)
      std::allocator_traits<A>::destroy(a_, __builtin_addressof(value));
    else
      value.~U();
  }
  static constexpr void destroy_self(sp_block* b) noexcept {
    sp_obj_block* self = static_cast<sp_obj_block*>(b);
    A a(self->a_);
    std::destroy_at(self);
    ::ycxx::detail::sp_deallocate_block(a, self);
  }

public:
  constexpr explicit sp_obj_block(const A& a) noexcept : sp_block(&destroy_self), a_(a) {}
  constexpr ~sp_obj_block() {}

  template <sp_init How, class... Args>
  constexpr void construct(Args&&... args) {
    if constexpr (How == sp_init::overwrite)
      ::new (static_cast<void*>(__builtin_addressof(value))) U;
    else if constexpr (ViaAlloc)
      std::allocator_traits<A>::construct(a_, __builtin_addressof(value), static_cast<Args&&>(args)...);
    else
      ::new (static_cast<void*>(__builtin_addressof(value))) U(static_cast<Args&&>(args)...);
  }
};

// Deallocates a block whose construction of the owned object threw.
template <class Block, class A>
struct sp_block_guard {
  Block* b;
  const A& a;
  constexpr ~sp_block_guard() {
    if (b) {
      std::destroy_at(b);
      ::ycxx::detail::sp_deallocate_block(a, b);
    }
  }
};

// make_shared / allocate_shared of an array: n elements of type E (cv-unqualified, possibly an
// array type). A is the allocator rebound to the scalar type remove_all_extents_t<E>.
template <class E, class A, bool ViaAlloc>
class sp_array_block final : public sp_block {
  using S = std::remove_all_extents_t<E>;
  // At run time the block and its elements share one allocation of `units` units of
  // max(alignof(block), alignof(E)) bytes; the elements start at `offset`.
  struct alignas(alignof(E) > alignof(sp_block) ? alignof(E) : alignof(sp_block)) unit {
    unsigned char bytes[alignof(E) > alignof(sp_block) ? alignof(E) : alignof(sp_block)];
  };
  using UA = typename std::allocator_traits<A>::template rebind_alloc<unit>;
  using EA = typename std::allocator_traits<A>::template rebind_alloc<E>;

  [[no_unique_address]] A a_;
  E* elems_;
  std::size_t n_;

  static constexpr std::size_t offset() noexcept {
    return (sizeof(sp_array_block) + alignof(E) - 1) / alignof(E) * alignof(E);
  }
  static constexpr std::size_t units(std::size_t n) noexcept {
    return (offset() + n * sizeof(E) + sizeof(unit) - 1) / sizeof(unit);
  }

  constexpr void dispose() noexcept override { ::ycxx::detail::sp_destroy_n<ViaAlloc>(a_, elems_, n_); }
  static constexpr void destroy_self(sp_block* b) noexcept {
    sp_array_block* self = static_cast<sp_array_block*>(b);
    A a(self->a_);
    if consteval {
      E* elems = self->elems_;
      std::size_t n = self->n_;
      std::destroy_at(self);
      EA ea(a); // the elements' storage is allocated even for n == 0
      std::allocator_traits<EA>::deallocate(
          ea, std::pointer_traits<typename std::allocator_traits<EA>::pointer>::pointer_to(*elems), n);
      ::ycxx::detail::sp_deallocate_block(a, self);
    } else {
      std::size_t count = units(self->n_);
      unit* raw = reinterpret_cast<unit*>(self);
      std::destroy_at(self);
      UA ua(a);
      std::allocator_traits<UA>::deallocate(
          ua, std::pointer_traits<typename std::allocator_traits<UA>::pointer>::pointer_to(*raw), count);
    }
  }

public:
  constexpr sp_array_block(const A& a, E* elems, std::size_t n) noexcept : sp_block(&destroy_self), a_(a), elems_(elems), n_(n) {}
  constexpr ~sp_array_block() = default;

  constexpr E* elements() const noexcept { return elems_; }

  // Allocates the block and n elements and initializes the elements; on an exception nothing
  // is left allocated.
  template <sp_init How>
  static constexpr sp_array_block* create(const A& a, std::size_t n, const E* u) {
    A a2(a);
    if consteval {
      EA ea(a);
      E* elems = std::to_address(std::allocator_traits<EA>::allocate(ea, n));
      sp_array_block* b;
      {
        // Frees the elements' storage if the block allocation or an element constructor throws.
        struct guard {
          EA& ea;
          E* elems;
          std::size_t n;
          constexpr ~guard() {
            if (elems)
              std::allocator_traits<EA>::deallocate(
                  ea, std::pointer_traits<typename std::allocator_traits<EA>::pointer>::pointer_to(*elems), n);
          }
        } g{ea, elems, n};
        b = std::construct_at(::ycxx::detail::sp_allocate_block<sp_array_block>(a), a, elems, n);
        sp_block_guard<sp_array_block, A> bg{b, a};
        ::ycxx::detail::sp_construct_n<ViaAlloc, How>(a2, elems, n, u);
        bg.b = nullptr;
        g.elems = nullptr;
      }
      return b;
    } else {
      if (n > (static_cast<std::size_t>(-1) - offset() - sizeof(unit)) / sizeof(E))
        ::ycxx::detail::throw_bad_array_new_length();
      UA ua(a);
      std::size_t count = units(n);
      unit* raw = std::to_address(std::allocator_traits<UA>::allocate(ua, count));
      E* elems = reinterpret_cast<E*>(reinterpret_cast<unsigned char*>(raw) + offset());
      struct guard {
        UA& ua;
        unit* raw;
        std::size_t count;
        constexpr ~guard() {
          if (raw)
            std::allocator_traits<UA>::deallocate(
                ua, std::pointer_traits<typename std::allocator_traits<UA>::pointer>::pointer_to(*raw), count);
        }
      } g{ua, raw, count};
      ::ycxx::detail::sp_construct_n<ViaAlloc, How>(a2, elems, n, u);
      g.raw = nullptr;
      return ::new (static_cast<void*>(raw)) sp_array_block(a, elems, n);
    }
  }
};

// "Y* is compatible with T*" ([util.smartptr.shared.general]/6).
template <class Y, class T>
concept sp_compatible = std::is_convertible_v<Y*, T*> ||
                        (std::is_bounded_array_v<Y> && std::is_same_v<std::remove_extent_t<Y>[], std::remove_cv_t<T>>);

// The pointer conversions the constructors from Y* accept ([util.smartptr.shared.const]/3, 9).
template <class Y, class T>
concept sp_convertible_ptr = (!std::is_array_v<T> && std::is_convertible_v<Y*, T*>) ||
                             (std::is_unbounded_array_v<T> && std::is_convertible_v<Y (*)[], T*>) ||
                             (std::is_bounded_array_v<T> && std::is_convertible_v<Y (*)[std::extent_v<T>], T*>);

// A pointer to the unambiguous, accessible enable_shared_from_this base of *p, if there is one.
template <class X>
constexpr const std::enable_shared_from_this<X>* sp_esft_base(const std::enable_shared_from_this<X>* p) noexcept {
  return p;
}

// The library's access to shared_ptr's representation (defined after shared_ptr).
struct sp_access;

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [util.smartptr.shared]
template <class T>
class shared_ptr {
public:
  using element_type = remove_extent_t<T>;
  using weak_type = weak_ptr<T>;

private:
  template <class>
  friend class shared_ptr;
  template <class>
  friend class weak_ptr;
  friend struct ycxx::detail::sp_access;

  element_type* ptr_ = nullptr;
  ycxx::detail::sp_block* ctrl_ = nullptr;

  // "enables shared_from_this with p" ([util.smartptr.shared.const]/1).
  template <class Y>
  constexpr void enable_shared_from_this_with(Y* p) noexcept {
    if constexpr (requires { ycxx::detail::sp_esft_base(p); }) {
      auto* base = ycxx::detail::sp_esft_base(p);
      if (p != nullptr && base->weak_this_.expired())
        base->weak_this_ = shared_ptr<remove_cv_t<Y>>(*this, const_cast<remove_cv_t<Y>*>(p));
    }
  }

  template <class Y, class D, class A>
  constexpr void adopt(Y* p, D& d, const A& a) {
    ctrl_ = ycxx::detail::sp_make_ptr_block(p, d, a);
    ptr_ = p;
    if constexpr (!is_array_v<T>)
      enable_shared_from_this_with(p);
  }

public:
  // [util.smartptr.shared.const]
  constexpr shared_ptr() noexcept = default;
  constexpr shared_ptr(nullptr_t) noexcept : shared_ptr() {}

  template <class Y>
    requires ycxx::detail::sp_convertible_ptr<Y, T> &&
             ((is_array_v<T> && requires(Y* p) { delete[] p; }) || (!is_array_v<T> && requires(Y* p) { delete p; }))
  constexpr explicit shared_ptr(Y* p) {
    static_assert(requires { sizeof(Y); }, "std::shared_ptr(Y*): Y must be a complete type");
    ycxx::detail::sp_default_delete<is_array_v<T>> d;
    adopt(p, d, allocator<int>());
  }
  template <class Y, class D>
    requires ycxx::detail::sp_convertible_ptr<Y, T> && is_move_constructible_v<D> &&
             requires(D& d, Y* p) { d(p); }
  constexpr shared_ptr(Y* p, D d) {
    adopt(p, d, allocator<int>());
  }
  template <class Y, class D, class A>
    requires ycxx::detail::sp_convertible_ptr<Y, T> && is_move_constructible_v<D> &&
             requires(D& d, Y* p) { d(p); }
  constexpr shared_ptr(Y* p, D d, A a) {
    adopt(p, d, a);
  }
  template <class D>
    requires is_move_constructible_v<D> && requires(D& d) { d(nullptr); }
  constexpr shared_ptr(nullptr_t p, D d) {
    ctrl_ = ycxx::detail::sp_make_ptr_block(p, d, allocator<int>());
  }
  template <class D, class A>
    requires is_move_constructible_v<D> && requires(D& d) { d(nullptr); }
  constexpr shared_ptr(nullptr_t p, D d, A a) {
    ctrl_ = ycxx::detail::sp_make_ptr_block(p, d, a);
  }

  // Aliasing constructors.
  template <class Y>
  constexpr shared_ptr(const shared_ptr<Y>& r, element_type* p) noexcept : ptr_(p), ctrl_(r.ctrl_) {
    if (ctrl_)
      ctrl_->add_shared();
  }
  template <class Y>
  constexpr shared_ptr(shared_ptr<Y>&& r, element_type* p) noexcept : ptr_(p), ctrl_(r.ctrl_) {
    r.ptr_ = nullptr;
    r.ctrl_ = nullptr;
  }

  constexpr shared_ptr(const shared_ptr& r) noexcept : ptr_(r.ptr_), ctrl_(r.ctrl_) {
    if (ctrl_)
      ctrl_->add_shared();
  }
  template <class Y>
    requires ycxx::detail::sp_compatible<Y, T>
  constexpr shared_ptr(const shared_ptr<Y>& r) noexcept : ptr_(r.ptr_), ctrl_(r.ctrl_) {
    if (ctrl_)
      ctrl_->add_shared();
  }
  constexpr shared_ptr(shared_ptr&& r) noexcept : ptr_(r.ptr_), ctrl_(r.ctrl_) {
    r.ptr_ = nullptr;
    r.ctrl_ = nullptr;
  }
  template <class Y>
    requires ycxx::detail::sp_compatible<Y, T>
  constexpr shared_ptr(shared_ptr<Y>&& r) noexcept : ptr_(r.ptr_), ctrl_(r.ctrl_) {
    r.ptr_ = nullptr;
    r.ctrl_ = nullptr;
  }

  template <class Y>
    requires ycxx::detail::sp_compatible<Y, T>
  constexpr explicit shared_ptr(const weak_ptr<Y>& r) {
    if (!r.ctrl_ || !r.ctrl_->try_add_shared())
      ycxx::detail::throw_bad_weak_ptr();
    ctrl_ = r.ctrl_;
    ptr_ = r.ptr_;
  }

  template <class Y, class D>
    requires ycxx::detail::sp_compatible<Y, T> &&
             is_convertible_v<typename unique_ptr<Y, D>::pointer, element_type*> &&
             (is_reference_v<D> || is_move_constructible_v<D>) // implied by the Effects' shared_ptr(p, std::move(d))
  constexpr shared_ptr(unique_ptr<Y, D>&& r) {
    if (!r.get())
      return;
    using P = typename unique_ptr<Y, D>::pointer;
    // The block is allocated before r gives up anything, so a failed allocation leaves r
    // untouched ("the constructor has no effect"); a reference deleter is held through
    // reference_wrapper ([util.smartptr.shared.const]/29).
    using DS = conditional_t<is_reference_v<D>, reference_wrapper<remove_reference_t<D>>, D>;
    using Block = ycxx::detail::sp_ptr_block<P, DS, allocator<int>>;
    Block* b = ycxx::detail::sp_allocate_block<Block>(allocator<int>());
    P p = r.get();
    if constexpr (is_reference_v<D>)
      ctrl_ = std::construct_at(b, p, DS(r.get_deleter()), allocator<int>());
    else
      ctrl_ = std::construct_at(b, p, static_cast<D&&>(r.get_deleter()), allocator<int>());
    (void)r.release();
    ptr_ = p;
    if constexpr (!is_array_v<T> && is_same_v<P, Y*>)
      enable_shared_from_this_with(p);
  }

  // [util.smartptr.shared.dest]
  constexpr ~shared_ptr() {
    if (ctrl_)
      ctrl_->release_shared();
  }

  // [util.smartptr.shared.assign]
  constexpr shared_ptr& operator=(const shared_ptr& r) noexcept {
    shared_ptr(r).swap(*this);
    return *this;
  }
  template <class Y>
    requires ycxx::detail::sp_compatible<Y, T>
  constexpr shared_ptr& operator=(const shared_ptr<Y>& r) noexcept {
    shared_ptr(r).swap(*this);
    return *this;
  }
  constexpr shared_ptr& operator=(shared_ptr&& r) noexcept {
    shared_ptr(static_cast<shared_ptr&&>(r)).swap(*this);
    return *this;
  }
  template <class Y>
    requires ycxx::detail::sp_compatible<Y, T>
  constexpr shared_ptr& operator=(shared_ptr<Y>&& r) noexcept {
    shared_ptr(static_cast<shared_ptr<Y>&&>(r)).swap(*this);
    return *this;
  }
  template <class Y, class D>
    requires is_constructible_v<shared_ptr, unique_ptr<Y, D>>
  constexpr shared_ptr& operator=(unique_ptr<Y, D>&& r) {
    shared_ptr(static_cast<unique_ptr<Y, D>&&>(r)).swap(*this);
    return *this;
  }

  // [util.smartptr.shared.mod]
  constexpr void swap(shared_ptr& r) noexcept {
    element_type* p = ptr_;
    ptr_ = r.ptr_;
    r.ptr_ = p;
    ycxx::detail::sp_block* c = ctrl_;
    ctrl_ = r.ctrl_;
    r.ctrl_ = c;
  }
  constexpr void reset() noexcept { shared_ptr().swap(*this); }
  template <class Y>
    requires is_constructible_v<shared_ptr, Y*>
  constexpr void reset(Y* p) {
    shared_ptr(p).swap(*this);
  }
  template <class Y, class D>
    requires is_constructible_v<shared_ptr, Y*, D>
  constexpr void reset(Y* p, D d) {
    shared_ptr(p, static_cast<D&&>(d)).swap(*this);
  }
  template <class Y, class D, class A>
    requires is_constructible_v<shared_ptr, Y*, D, A>
  constexpr void reset(Y* p, D d, A a) {
    shared_ptr(p, static_cast<D&&>(d), static_cast<A&&>(a)).swap(*this);
  }

  // [util.smartptr.shared.obs]
  constexpr element_type* get() const noexcept { return ptr_; }
  constexpr add_lvalue_reference_t<T> operator*() const noexcept
    requires(!is_void_v<T> && !is_array_v<T>)
  {
    ycxx::detail::precondition(ptr_ != nullptr, "std::shared_ptr::operator*: null pointer");
    return *ptr_;
  }
  constexpr T* operator->() const noexcept
    requires(!is_array_v<T>)
  {
    ycxx::detail::precondition(ptr_ != nullptr, "std::shared_ptr::operator->: null pointer");
    return ptr_;
  }
  constexpr add_lvalue_reference_t<element_type> operator[](ptrdiff_t i) const noexcept // Throws: nothing
    requires is_array_v<T>
  {
    ycxx::detail::precondition(ptr_ != nullptr, "std::shared_ptr::operator[]: null pointer");
    if constexpr (is_bounded_array_v<T>)
      ycxx::detail::precondition(i >= 0 && i < static_cast<ptrdiff_t>(extent_v<T>),
                                 "std::shared_ptr::operator[]: index out of bounds");
    else
      ycxx::detail::precondition(i >= 0, "std::shared_ptr::operator[]: negative index");
    return ptr_[i];
  }
  constexpr long use_count() const noexcept { return ctrl_ ? ctrl_->use_count() : 0; }
  constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }

  template <class U>
  bool owner_before(const shared_ptr<U>& b) const noexcept {
    return less<ycxx::detail::sp_block*>()(ctrl_, b.ctrl_);
  }
  template <class U>
  bool owner_before(const weak_ptr<U>& b) const noexcept {
    return less<ycxx::detail::sp_block*>()(ctrl_, b.ctrl_);
  }
  size_t owner_hash() const noexcept { return hash<ycxx::detail::sp_block*>()(ctrl_); }
  template <class U>
  constexpr bool owner_equal(const shared_ptr<U>& b) const noexcept {
    return ctrl_ == b.ctrl_;
  }
  template <class U>
  constexpr bool owner_equal(const weak_ptr<U>& b) const noexcept {
    return ctrl_ == b.ctrl_;
  }
};

template <class T>
shared_ptr(weak_ptr<T>) -> shared_ptr<T>;
template <class T, class D>
shared_ptr(unique_ptr<T, D>) -> shared_ptr<T>;

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

struct sp_access {
  // A shared_ptr taking over one already-counted reference to ctrl.
  template <class T>
  static constexpr std::shared_ptr<T> adopt(typename std::shared_ptr<T>::element_type* p, sp_block* ctrl) noexcept {
    std::shared_ptr<T> r;
    r.ptr_ = p;
    r.ctrl_ = ctrl;
    return r;
  }
  template <class T>
  static constexpr sp_block* ctrl(const std::shared_ptr<T>& p) noexcept {
    return p.ctrl_;
  }
  // The stored pointer of a weak_ptr (atomic<weak_ptr<T>> compares it, [util.smartptr.atomic.weak]).
  template <class T>
  static constexpr auto* stored(const std::weak_ptr<T>& w) noexcept {
    return w.ptr_;
  }
  template <class T>
  static constexpr void enable_shared_from_this(std::shared_ptr<T>& r) noexcept {
    r.enable_shared_from_this_with(r.ptr_);
  }
};

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [util.smartptr.weak]
template <class T>
class weak_ptr {
public:
  using element_type = remove_extent_t<T>;

private:
  template <class>
  friend class shared_ptr;
  template <class>
  friend class weak_ptr;
  friend struct ycxx::detail::sp_access;

  element_type* ptr_ = nullptr;
  ycxx::detail::sp_block* ctrl_ = nullptr;

  // The stored pointer of r converted to element_type*. Converting to a virtual base reads the
  // object, which may already be destroyed, so that conversion goes through lock().
  template <class Y>
  static constexpr element_type* convert(const weak_ptr<Y>& r) noexcept {
    if constexpr (is_same_v<remove_cv_t<typename weak_ptr<Y>::element_type>, remove_cv_t<element_type>> ||
                  is_void_v<element_type>) {
      return r.ptr_;
    } else if constexpr (is_virtual_base_of_v<remove_cv_t<element_type>,
                                              remove_cv_t<typename weak_ptr<Y>::element_type>>) {
      return r.lock().get();
    } else {
      return r.ptr_;
    }
  }

public:
  // [util.smartptr.weak.const]
  constexpr weak_ptr() noexcept = default;
  template <class Y>
    requires ycxx::detail::sp_compatible<Y, T>
  constexpr weak_ptr(const shared_ptr<Y>& r) noexcept : ptr_(r.ptr_), ctrl_(r.ctrl_) {
    if (ctrl_)
      ctrl_->add_weak();
  }
  constexpr weak_ptr(const weak_ptr& r) noexcept : ptr_(r.ptr_), ctrl_(r.ctrl_) {
    if (ctrl_)
      ctrl_->add_weak();
  }
  template <class Y>
    requires ycxx::detail::sp_compatible<Y, T>
  constexpr weak_ptr(const weak_ptr<Y>& r) noexcept : ptr_(convert(r)), ctrl_(r.ctrl_) {
    if (ctrl_)
      ctrl_->add_weak();
  }
  constexpr weak_ptr(weak_ptr&& r) noexcept : ptr_(r.ptr_), ctrl_(r.ctrl_) {
    r.ptr_ = nullptr;
    r.ctrl_ = nullptr;
  }
  template <class Y>
    requires ycxx::detail::sp_compatible<Y, T>
  constexpr weak_ptr(weak_ptr<Y>&& r) noexcept : ptr_(convert(r)), ctrl_(r.ctrl_) {
    r.ptr_ = nullptr;
    r.ctrl_ = nullptr;
  }

  // [util.smartptr.weak.dest]
  constexpr ~weak_ptr() {
    if (ctrl_)
      ctrl_->release_weak();
  }

  // [util.smartptr.weak.assign]
  constexpr weak_ptr& operator=(const weak_ptr& r) noexcept {
    weak_ptr(r).swap(*this);
    return *this;
  }
  template <class Y>
    requires ycxx::detail::sp_compatible<Y, T>
  constexpr weak_ptr& operator=(const weak_ptr<Y>& r) noexcept {
    weak_ptr(r).swap(*this);
    return *this;
  }
  template <class Y>
    requires ycxx::detail::sp_compatible<Y, T>
  constexpr weak_ptr& operator=(const shared_ptr<Y>& r) noexcept {
    weak_ptr(r).swap(*this);
    return *this;
  }
  constexpr weak_ptr& operator=(weak_ptr&& r) noexcept {
    weak_ptr(static_cast<weak_ptr&&>(r)).swap(*this);
    return *this;
  }
  template <class Y>
    requires ycxx::detail::sp_compatible<Y, T>
  constexpr weak_ptr& operator=(weak_ptr<Y>&& r) noexcept {
    weak_ptr(static_cast<weak_ptr<Y>&&>(r)).swap(*this);
    return *this;
  }

  // [util.smartptr.weak.mod]
  constexpr void swap(weak_ptr& r) noexcept {
    element_type* p = ptr_;
    ptr_ = r.ptr_;
    r.ptr_ = p;
    ycxx::detail::sp_block* c = ctrl_;
    ctrl_ = r.ctrl_;
    r.ctrl_ = c;
  }
  constexpr void reset() noexcept { weak_ptr().swap(*this); }

  // [util.smartptr.weak.obs]
  constexpr long use_count() const noexcept { return ctrl_ ? ctrl_->use_count() : 0; }
  constexpr bool expired() const noexcept { return use_count() == 0; }
  constexpr shared_ptr<T> lock() const noexcept {
    if (ctrl_ && ctrl_->try_add_shared())
      return ycxx::detail::sp_access::adopt<T>(ptr_, ctrl_);
    return shared_ptr<T>();
  }
  template <class U>
  bool owner_before(const shared_ptr<U>& b) const noexcept {
    return less<ycxx::detail::sp_block*>()(ctrl_, b.ctrl_);
  }
  template <class U>
  bool owner_before(const weak_ptr<U>& b) const noexcept {
    return less<ycxx::detail::sp_block*>()(ctrl_, b.ctrl_);
  }
  size_t owner_hash() const noexcept { return hash<ycxx::detail::sp_block*>()(ctrl_); }
  template <class U>
  constexpr bool owner_equal(const shared_ptr<U>& b) const noexcept {
    return ctrl_ == b.ctrl_;
  }
  template <class U>
  constexpr bool owner_equal(const weak_ptr<U>& b) const noexcept {
    return ctrl_ == b.ctrl_;
  }
};

template <class T>
weak_ptr(shared_ptr<T>) -> weak_ptr<T>;

// [util.smartptr.weak.spec]
template <class T>
constexpr void swap(weak_ptr<T>& a, weak_ptr<T>& b) noexcept {
  a.swap(b);
}

// [util.smartptr.enab]
template <class T>
class enable_shared_from_this {
  template <class>
  friend class shared_ptr;

  mutable weak_ptr<T> weak_this_;

protected:
  constexpr enable_shared_from_this() noexcept {}
  constexpr enable_shared_from_this(const enable_shared_from_this&) noexcept {}
  constexpr enable_shared_from_this& operator=(const enable_shared_from_this&) noexcept { return *this; }
  constexpr ~enable_shared_from_this() {}

public:
  constexpr shared_ptr<T> shared_from_this() { return shared_ptr<T>(weak_this_); }
  constexpr shared_ptr<const T> shared_from_this() const { return shared_ptr<const T>(weak_this_); }
  constexpr weak_ptr<T> weak_from_this() noexcept { return weak_this_; }
  constexpr weak_ptr<const T> weak_from_this() const noexcept { return weak_this_; }
};

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// [util.smartptr.shared.create]: the object form. A is the allocator the caller passed
// (std::allocator for make_shared); ViaAlloc selects allocator construct/destroy.
template <class T, sp_init How, bool ViaAlloc, class A, class... Args>
constexpr std::shared_ptr<T> sp_make_obj(const A& a, Args&&... args) {
  using V = std::remove_cv_t<T>;
  using VA = typename std::allocator_traits<A>::template rebind_alloc<V>;
  using Block = sp_obj_block<T, VA, ViaAlloc>;
  VA va(a);
  Block* b = std::construct_at(::ycxx::detail::sp_allocate_block<Block>(va), va);
  {
    sp_block_guard<Block, VA> g{b, va};
    b->template construct<How>(static_cast<Args&&>(args)...);
    g.b = nullptr;
  }
  std::shared_ptr<T> r = sp_access::adopt<T>(__builtin_addressof(b->value), b);
  sp_access::enable_shared_from_this(r);
  return r;
}

// The array forms: n elements of remove_extent_t<T>, from *u when How is fill.
template <class T, sp_init How, bool ViaAlloc, class A>
constexpr std::shared_ptr<T> sp_make_array(const A& a, std::size_t n, const std::remove_extent_t<T>* u) {
  using E = std::remove_cv_t<std::remove_extent_t<T>>;
  using SA = typename std::allocator_traits<A>::template rebind_alloc<std::remove_cv_t<std::remove_all_extents_t<T>>>;
  using Block = sp_array_block<E, SA, ViaAlloc>;
  Block* b = Block::template create<How>(SA(a), n, u);
  return sp_access::adopt<T>(b->elements(), b);
}

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

// [util.smartptr.shared.create]
template <class T, class... Args>
  requires(!is_array_v<T>)
constexpr shared_ptr<T> make_shared(Args&&... args) {
  return ycxx::detail::sp_make_obj<T, ycxx::detail::sp_init::value, false>(allocator<int>(),
                                                                           static_cast<Args&&>(args)...);
}
template <class T, class A, class... Args>
  requires(!is_array_v<T>)
constexpr shared_ptr<T> allocate_shared(const A& a, Args&&... args) {
  return ycxx::detail::sp_make_obj<T, ycxx::detail::sp_init::value, true>(a, static_cast<Args&&>(args)...);
}

template <class T>
  requires is_unbounded_array_v<T>
constexpr shared_ptr<T> make_shared(size_t N) {
  return ycxx::detail::sp_make_array<T, ycxx::detail::sp_init::value, false>(allocator<int>(), N, nullptr);
}
template <class T, class A>
  requires is_unbounded_array_v<T>
constexpr shared_ptr<T> allocate_shared(const A& a, size_t N) {
  return ycxx::detail::sp_make_array<T, ycxx::detail::sp_init::value, true>(a, N, nullptr);
}
template <class T>
  requires is_bounded_array_v<T>
constexpr shared_ptr<T> make_shared() {
  return ycxx::detail::sp_make_array<T, ycxx::detail::sp_init::value, false>(allocator<int>(), extent_v<T>, nullptr);
}
template <class T, class A>
  requires is_bounded_array_v<T>
constexpr shared_ptr<T> allocate_shared(const A& a) {
  return ycxx::detail::sp_make_array<T, ycxx::detail::sp_init::value, true>(a, extent_v<T>, nullptr);
}
template <class T>
  requires is_unbounded_array_v<T>
constexpr shared_ptr<T> make_shared(size_t N, const remove_extent_t<T>& u) {
  return ycxx::detail::sp_make_array<T, ycxx::detail::sp_init::fill, false>(allocator<int>(), N,
                                                                            __builtin_addressof(u));
}
template <class T, class A>
  requires is_unbounded_array_v<T>
constexpr shared_ptr<T> allocate_shared(const A& a, size_t N, const remove_extent_t<T>& u) {
  return ycxx::detail::sp_make_array<T, ycxx::detail::sp_init::fill, true>(a, N, __builtin_addressof(u));
}
template <class T>
  requires is_bounded_array_v<T>
constexpr shared_ptr<T> make_shared(const remove_extent_t<T>& u) {
  return ycxx::detail::sp_make_array<T, ycxx::detail::sp_init::fill, false>(allocator<int>(), extent_v<T>,
                                                                            __builtin_addressof(u));
}
template <class T, class A>
  requires is_bounded_array_v<T>
constexpr shared_ptr<T> allocate_shared(const A& a, const remove_extent_t<T>& u) {
  return ycxx::detail::sp_make_array<T, ycxx::detail::sp_init::fill, true>(a, extent_v<T>, __builtin_addressof(u));
}
// The _for_overwrite forms default-initialize and destroy with ~U() even when an allocator
// supplies the storage ([util.smartptr.shared.create]/7.8, 7.11).
template <class T>
  requires(!is_unbounded_array_v<T>)
constexpr shared_ptr<T> make_shared_for_overwrite() {
  if constexpr (is_array_v<T>)
    return ycxx::detail::sp_make_array<T, ycxx::detail::sp_init::overwrite, false>(allocator<int>(), extent_v<T>,
                                                                                   nullptr);
  else
    return ycxx::detail::sp_make_obj<T, ycxx::detail::sp_init::overwrite, false>(allocator<int>());
}
template <class T, class A>
  requires(!is_unbounded_array_v<T>)
constexpr shared_ptr<T> allocate_shared_for_overwrite(const A& a) {
  if constexpr (is_array_v<T>)
    return ycxx::detail::sp_make_array<T, ycxx::detail::sp_init::overwrite, false>(a, extent_v<T>, nullptr);
  else
    return ycxx::detail::sp_make_obj<T, ycxx::detail::sp_init::overwrite, false>(a);
}
template <class T>
  requires is_unbounded_array_v<T>
constexpr shared_ptr<T> make_shared_for_overwrite(size_t N) {
  return ycxx::detail::sp_make_array<T, ycxx::detail::sp_init::overwrite, false>(allocator<int>(), N, nullptr);
}
template <class T, class A>
  requires is_unbounded_array_v<T>
constexpr shared_ptr<T> allocate_shared_for_overwrite(const A& a, size_t N) {
  return ycxx::detail::sp_make_array<T, ycxx::detail::sp_init::overwrite, false>(a, N, nullptr);
}

// [util.smartptr.shared.cmp]
template <class T, class U>
constexpr bool operator==(const shared_ptr<T>& a, const shared_ptr<U>& b) noexcept {
  return a.get() == b.get();
}
template <class T>
constexpr bool operator==(const shared_ptr<T>& a, nullptr_t) noexcept {
  return !a;
}
template <class T, class U>
constexpr strong_ordering operator<=>(const shared_ptr<T>& a, const shared_ptr<U>& b) noexcept {
  return compare_three_way()(a.get(), b.get());
}
template <class T>
constexpr strong_ordering operator<=>(const shared_ptr<T>& a, nullptr_t) noexcept {
  return compare_three_way()(a.get(), static_cast<typename shared_ptr<T>::element_type*>(nullptr));
}

// [util.smartptr.shared.spec]
template <class T>
constexpr void swap(shared_ptr<T>& a, shared_ptr<T>& b) noexcept {
  a.swap(b);
}

// [util.smartptr.shared.cast]
template <class T, class U>
constexpr shared_ptr<T> static_pointer_cast(const shared_ptr<U>& r) noexcept {
  return shared_ptr<T>(r, static_cast<typename shared_ptr<T>::element_type*>(r.get()));
}
template <class T, class U>
constexpr shared_ptr<T> static_pointer_cast(shared_ptr<U>&& r) noexcept {
  auto* p = static_cast<typename shared_ptr<T>::element_type*>(r.get());
  return shared_ptr<T>(static_cast<shared_ptr<U>&&>(r), p);
}
template <class T, class U>
constexpr shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U>& r) noexcept {
  if (auto* p = dynamic_cast<typename shared_ptr<T>::element_type*>(r.get()))
    return shared_ptr<T>(r, p);
  return shared_ptr<T>();
}
template <class T, class U>
constexpr shared_ptr<T> dynamic_pointer_cast(shared_ptr<U>&& r) noexcept {
  if (auto* p = dynamic_cast<typename shared_ptr<T>::element_type*>(r.get()))
    return shared_ptr<T>(static_cast<shared_ptr<U>&&>(r), p);
  return shared_ptr<T>();
}
template <class T, class U>
constexpr shared_ptr<T> const_pointer_cast(const shared_ptr<U>& r) noexcept {
  return shared_ptr<T>(r, const_cast<typename shared_ptr<T>::element_type*>(r.get()));
}
template <class T, class U>
constexpr shared_ptr<T> const_pointer_cast(shared_ptr<U>&& r) noexcept {
  auto* p = const_cast<typename shared_ptr<T>::element_type*>(r.get());
  return shared_ptr<T>(static_cast<shared_ptr<U>&&>(r), p);
}
template <class T, class U>
shared_ptr<T> reinterpret_pointer_cast(const shared_ptr<U>& r) noexcept {
  return shared_ptr<T>(r, reinterpret_cast<typename shared_ptr<T>::element_type*>(r.get()));
}
template <class T, class U>
shared_ptr<T> reinterpret_pointer_cast(shared_ptr<U>&& r) noexcept {
  auto* p = reinterpret_cast<typename shared_ptr<T>::element_type*>(r.get());
  return shared_ptr<T>(static_cast<shared_ptr<U>&&>(r), p);
}

// [util.smartptr.getdeleter]
template <class D, class T>
constexpr D* get_deleter(const shared_ptr<T>& p) noexcept {
  ycxx::detail::sp_block* c = ycxx::detail::sp_access::ctrl(p);
  return c ? static_cast<D*>(c->deleter(&ycxx::detail::sp_tag<remove_cv_t<D>>)) : nullptr;
}

// [util.smartptr.ownerless]
template <class T = void>
struct owner_less;
template <class T>
struct owner_less<shared_ptr<T>> {
  bool operator()(const shared_ptr<T>& x, const shared_ptr<T>& y) const noexcept { return x.owner_before(y); }
  bool operator()(const shared_ptr<T>& x, const weak_ptr<T>& y) const noexcept { return x.owner_before(y); }
  bool operator()(const weak_ptr<T>& x, const shared_ptr<T>& y) const noexcept { return x.owner_before(y); }
};
template <class T>
struct owner_less<weak_ptr<T>> {
  bool operator()(const weak_ptr<T>& x, const weak_ptr<T>& y) const noexcept { return x.owner_before(y); }
  bool operator()(const shared_ptr<T>& x, const weak_ptr<T>& y) const noexcept { return x.owner_before(y); }
  bool operator()(const weak_ptr<T>& x, const shared_ptr<T>& y) const noexcept { return x.owner_before(y); }
};
template <>
struct owner_less<void> {
  template <class T, class U>
  bool operator()(const shared_ptr<T>& x, const shared_ptr<U>& y) const noexcept {
    return x.owner_before(y);
  }
  template <class T, class U>
  bool operator()(const shared_ptr<T>& x, const weak_ptr<U>& y) const noexcept {
    return x.owner_before(y);
  }
  template <class T, class U>
  bool operator()(const weak_ptr<T>& x, const shared_ptr<U>& y) const noexcept {
    return x.owner_before(y);
  }
  template <class T, class U>
  bool operator()(const weak_ptr<T>& x, const weak_ptr<U>& y) const noexcept {
    return x.owner_before(y);
  }
  using is_transparent = void;
};

// [util.smartptr.owner.hash]
struct owner_hash {
  template <class T>
  size_t operator()(const shared_ptr<T>& x) const noexcept {
    return x.owner_hash();
  }
  template <class T>
  size_t operator()(const weak_ptr<T>& x) const noexcept {
    return x.owner_hash();
  }
  using is_transparent = void;
};

// [util.smartptr.owner.equal]
struct owner_equal {
  template <class T, class U>
  constexpr bool operator()(const shared_ptr<T>& x, const shared_ptr<U>& y) const noexcept {
    return x.owner_equal(y);
  }
  template <class T, class U>
  constexpr bool operator()(const shared_ptr<T>& x, const weak_ptr<U>& y) const noexcept {
    return x.owner_equal(y);
  }
  template <class T, class U>
  constexpr bool operator()(const weak_ptr<T>& x, const shared_ptr<U>& y) const noexcept {
    return x.owner_equal(y);
  }
  template <class T, class U>
  constexpr bool operator()(const weak_ptr<T>& x, const weak_ptr<U>& y) const noexcept {
    return x.owner_equal(y);
  }
  using is_transparent = void;
};

// [util.smartptr.hash]
template <class T>
struct hash<shared_ptr<T>> {
  size_t operator()(const shared_ptr<T>& p) const noexcept {
    return hash<typename shared_ptr<T>::element_type*>()(p.get());
  }
};

// [util.smartptr.shared.io]: written against the declaration of basic_ostream (unique_ptr.hpp
// includes it); usable once <ostream> is included.
template <class E, class T, class Y>
basic_ostream<E, T>& operator<<(basic_ostream<E, T>& os, const shared_ptr<Y>& p) {
  os << p.get();
  return os;
}

} // namespace std
