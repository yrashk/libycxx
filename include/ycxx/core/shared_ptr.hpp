// libycxx core: shared ownership ([util.sharedptr]): bad_weak_ptr, shared_ptr, weak_ptr,
// enable_shared_from_this, make_shared / allocate_shared (all forms), the pointer casts,
// get_deleter, owner_less / owner_hash / owner_equal and hash<shared_ptr>.
//
// Every owning shared_ptr points to a control block (__ycxx::__detail::__sp_block) holding the use
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

namespace [[__gnu__::__visibility__("hidden")]] std {

// [util.smartptr.weak.bad]
class bad_weak_ptr : public exception {
public:
  constexpr bad_weak_ptr() noexcept {}
  constexpr bad_weak_ptr(const bad_weak_ptr&) noexcept = default;
  constexpr bad_weak_ptr& operator=(const bad_weak_ptr&) noexcept = default;
  constexpr ~bad_weak_ptr() override {}
  constexpr const char* what() const noexcept override { return "bad_weak_ptr"; }
};

template <class _Tp>
class shared_ptr;
template <class _Tp>
class weak_ptr;
template <class _Tp>
class enable_shared_from_this;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

[[noreturn]] [[__gnu__::__cold__]] constexpr void __throw_bad_weak_ptr() {
  ::__ycxx::__detail::__raise_with(ycxx_error_bad_weak_ptr, "std::bad_weak_ptr", [] { return std::bad_weak_ptr(); });
}

// The address of sp_tag<D> identifies the deleter type D (get_deleter).
template <class _Dp>
inline constexpr char __sp_tag = 0;

// ---------------------------------------------------------------------------------------------
// Control blocks
// ---------------------------------------------------------------------------------------------
class __sp_block {
  long __shared_ = 1; // shared_ptr owners
  long __weak_ = 1;   // weak_ptrs, plus one while shared_ != 0

  // Destroys the owned object (use count reached zero).
  constexpr virtual void __dispose() noexcept = 0;

protected:
  // Destroys and deallocates the block (weak count reached zero). A function pointer rather
  // than a virtual function: its body rebinds the allocator to the block type, and Clang
  // instantiates constexpr virtual members while instantiating the class, so an allocator that
  // requires a complete value_type (libc++'s complete_type_allocator) would see the block
  // incomplete.
  using __destroy_fn = void (*)(__sp_block*) noexcept;
  __destroy_fn __destroy_;

  constexpr explicit __sp_block(__destroy_fn d) noexcept : __destroy_(d) {}
  constexpr ~__sp_block() = default;

public:
  __sp_block(const __sp_block&) = delete;
  __sp_block& operator=(const __sp_block&) = delete;

  // The stored deleter, if its type is the one `tag` identifies.
  constexpr virtual void* __deleter(const void* tag) noexcept {
    (void)tag;
    return nullptr;
  }

  // At run time the counts are atomic unless the process is single-threaded
  // (single_threaded.hpp). Increments are relaxed: a new reference is always made from an
  // existing one. A decrement releases and acquires (ref_release, single_threaded.hpp), so
  // everything the other owners did happens before the destruction.
  constexpr void __add_shared() noexcept {
    if consteval {
      ++__shared_;
    } else {
      if (::__ycxx::__detail::__single_threaded())
        ++__shared_;
      else
        __atomic_fetch_add(&__shared_, 1, __ATOMIC_RELAXED);
    }
  }
  // Takes a new shared reference unless the use count is already zero (weak_ptr::lock).
  constexpr bool __try_add_shared() noexcept {
    if consteval {
      if (__shared_ == 0)
        return false;
      ++__shared_;
      return true;
    } else {
      if (::__ycxx::__detail::__single_threaded()) {
        if (__shared_ == 0)
          return false;
        ++__shared_;
        return true;
      }
      long n = __atomic_load_n(&__shared_, __ATOMIC_RELAXED);
      while (n != 0)
        if (__atomic_compare_exchange_n(&__shared_, &n, n + 1, true, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED))
          return true;
      return false;
    }
  }
  constexpr void __release_shared() noexcept {
    long n;
    if consteval {
      n = --__shared_;
    } else {
      if (::__ycxx::__detail::__single_threaded()) {
        n = --__shared_;
      } else {
        n = __atomic_sub_fetch(&__shared_, 1, __ATOMIC_ACQ_REL);
      }
    }
    if (n == 0) {
      __dispose();
      __release_weak();
    }
  }
  constexpr void __add_weak() noexcept {
    if consteval {
      ++__weak_;
    } else {
      if (::__ycxx::__detail::__single_threaded())
        ++__weak_;
      else
        __atomic_fetch_add(&__weak_, 1, __ATOMIC_RELAXED);
    }
  }
  constexpr void __release_weak() noexcept {
    bool last;
    if consteval {
      last = --__weak_ == 0;
    } else {
      if (::__ycxx::__detail::__single_threaded()) {
        last = --__weak_ == 0;
      } else {
        // A count of one is the caller's own reference, and no other can appear: the use count
        // is zero (else it would hold one more), so neither a shared_ptr nor another weak_ptr
        // exists to make one. The usual case (no weak_ptr at all) then needs no atomic RMW.
        last = __atomic_load_n(&__weak_, __ATOMIC_ACQUIRE) == 1 || __atomic_sub_fetch(&__weak_, 1, __ATOMIC_ACQ_REL) == 0;
      }
    }
    if (last)
      __destroy_(this);
  }
  constexpr long use_count() const noexcept {
    if consteval {
      return __shared_;
    } else {
      return __atomic_load_n(&__shared_, __ATOMIC_RELAXED);
    }
  }
};

// Allocates one Block with a copy of `a` rebound to Block, and deallocates it again.
template <class _Block, class _Ap>
using __sp_block_alloc = typename std::allocator_traits<_Ap>::template rebind_alloc<_Block>;

template <class _Block, class _Ap>
constexpr _Block* __sp_allocate_block(const _Ap& a) {
  __sp_block_alloc<_Block, _Ap> __ba(a);
  return std::to_address(std::allocator_traits<__sp_block_alloc<_Block, _Ap>>::allocate(__ba, 1));
}
template <class _Block, class _Ap>
constexpr void __sp_deallocate_block(const _Ap& a, _Block* b) noexcept {
  using _BA = __sp_block_alloc<_Block, _Ap>;
  using _Ptr = typename std::allocator_traits<_BA>::pointer;
  _BA __ba(a);
  std::allocator_traits<_BA>::deallocate(__ba, std::pointer_traits<_Ptr>::pointer_to(*b), 1);
}

// The deleter used when a shared_ptr adopts a pointer without one ([util.smartptr.shared.const]
// /6): `delete p` or `delete[] p`. It is not a user-visible deleter, so get_deleter never finds it.
template <bool _Array>
struct __sp_default_delete {
  template <class _Pp>
  constexpr void operator()(_Pp* p) const noexcept {
    if constexpr (_Array)
      delete[] p;
    else
      delete p;
  }
};

// A pointer P, its deleter D and an allocator A (rebound to this block for deallocation).
template <class _Pp, class _Dp, class _Ap>
class __sp_ptr_block final : public __sp_block {
  _Pp __p_;
  [[no_unique_address]] _Dp __d_;
  [[no_unique_address]] _Ap __a_;

  constexpr void __dispose() noexcept override { __d_(__p_); }
  static constexpr void __destroy_self(__sp_block* b) noexcept {
    __sp_ptr_block* __self = static_cast<__sp_ptr_block*>(b);
    _Ap a(__self->__a_);
    std::destroy_at(__self);
    ::__ycxx::__detail::__sp_deallocate_block(a, __self);
  }

public:
  constexpr __sp_ptr_block(_Pp p, _Dp&& d, const _Ap& a) noexcept
      : __sp_block(&__destroy_self), __p_(p), __d_(static_cast<_Dp&&>(d)), __a_(a) {}
  constexpr ~__sp_ptr_block() = default;

  constexpr void* __deleter(const void* tag) noexcept override {
    if constexpr (std::is_same_v<_Dp, __sp_default_delete<true>> || std::is_same_v<_Dp, __sp_default_delete<false>>)
      return nullptr;
    else
      return tag == &__sp_tag<_Dp> ? __builtin_addressof(__d_) : nullptr;
  }
};

// Creates a block owning p with deleter d; if that fails, d(p) is called and the exception
// propagates ([util.smartptr.shared.const]/11).
template <class _Pp, class _Dp, class _Ap>
constexpr __sp_block* __sp_make_ptr_block(_Pp p, _Dp& d, const _Ap& a) {
  using _Block = __sp_ptr_block<_Pp, _Dp, _Ap>;
  if constexpr (__cfg::exceptions) {
    _Block* b;
    try {
      b = ::__ycxx::__detail::__sp_allocate_block<_Block>(a);
    } catch (...) {
      d(p);
      throw;
    }
    return std::construct_at(b, p, static_cast<_Dp&&>(d), a);
  } else {
    return std::construct_at(::__ycxx::__detail::__sp_allocate_block<_Block>(a), p, static_cast<_Dp&&>(d), a);
  }
}

// How make_shared & co. initialize each non-array subobject ([util.smartptr.shared.create]/7).
enum class __sp_init : unsigned char {
  value,     // U() or allocator construct(a, p)
  __overwrite, // default-initialized: ::new(pv) U
  fill,      // every element from *u (u has the element type)
  copy,      // element i from u[i]
};

// Constructs n elements of type E (cv-unqualified, possibly an array type) at p, in ascending
// order of address. ViaAlloc: through allocator_traits<A>::construct (allocate_shared),
// otherwise with placement new (make_shared, *_for_overwrite). On an exception the elements
// already constructed are destroyed in reverse order.
template <bool _ViaAlloc, class _Ap, class _Ep>
constexpr void __sp_destroy_n(_Ap& a, _Ep* p, std::size_t n) noexcept {
  while (n != 0) {
    --n;
    if constexpr (std::is_array_v<_Ep>)
      ::__ycxx::__detail::__sp_destroy_n<_ViaAlloc>(a, &p[n][0], std::extent_v<_Ep>);
    else if constexpr (_ViaAlloc)
      std::allocator_traits<_Ap>::destroy(a, p + n);
    else
      p[n].~_Ep();
  }
}

template <bool _ViaAlloc, class _Ap, class _Ep>
struct __sp_construct_guard {
  _Ap& a;
  _Ep* p;
  std::size_t* done;
  constexpr ~__sp_construct_guard() {
    if (done)
      ::__ycxx::__detail::__sp_destroy_n<_ViaAlloc>(a, p, *done);
  }
};

template <bool _ViaAlloc, __sp_init _How, class _Ap, class _Ep>
constexpr void __sp_construct_n(_Ap& a, _Ep* p, std::size_t n, const _Ep* __u) {
  std::size_t i = 0;
  __sp_construct_guard<_ViaAlloc, _Ap, _Ep> __g{a, p, &i};
  for (; i < n; ++i) {
    if constexpr (std::is_array_v<_Ep>) {
      constexpr std::size_t m = std::extent_v<_Ep>;
      if constexpr (std::is_trivially_default_constructible_v<_Ep> && std::is_trivially_destructible_v<_Ep>) {
        // Clang's constant evaluator does not let the element constructions below begin the
        // lifetime of the enclosing array p[i]; begin it first (no observable effect here).
        if consteval {
          ::new (static_cast<void*>(__builtin_addressof(p[i]))) _Ep;
        }
      }
      if constexpr (_How == __sp_init::fill)
        ::__ycxx::__detail::__sp_construct_n<_ViaAlloc, __sp_init::copy>(a, &p[i][0], m, &(*__u)[0]);
      else if constexpr (_How == __sp_init::copy)
        ::__ycxx::__detail::__sp_construct_n<_ViaAlloc, __sp_init::copy>(a, &p[i][0], m, &__u[i][0]);
      else
        ::__ycxx::__detail::__sp_construct_n<_ViaAlloc, _How>(a, &p[i][0], m,
                                                      static_cast<const std::remove_extent_t<_Ep>*>(nullptr));
    } else {
      void* __pv = __builtin_addressof(p[i]);
      if constexpr (_How == __sp_init::__overwrite)
        ::new (__pv) _Ep;
      else if constexpr (_How == __sp_init::value && _ViaAlloc)
        std::allocator_traits<_Ap>::construct(a, p + i);
      else if constexpr (_How == __sp_init::value)
        ::new (__pv) _Ep();
      else if constexpr (_ViaAlloc)
        std::allocator_traits<_Ap>::construct(a, p + i, _How == __sp_init::fill ? *__u : __u[i]);
      else
        ::new (__pv) _Ep(_How == __sp_init::fill ? *__u : __u[i]);
    }
  }
  __g.done = nullptr;
}

// make_shared / allocate_shared of a non-array T: the object lives in the block.
// A is the allocator rebound to remove_cv_t<T>; ViaAlloc as above.
template <class _Tp, class _Ap, bool _ViaAlloc>
class __sp_obj_block final : public __sp_block {
  using _Up = std::remove_cv_t<_Tp>;
  [[no_unique_address]] _Ap __a_;

public:
  union {
    _Up value;
  };

private:
  constexpr void __dispose() noexcept override {
    if constexpr (_ViaAlloc)
      std::allocator_traits<_Ap>::destroy(__a_, __builtin_addressof(value));
    else
      value.~_Up();
  }
  static constexpr void __destroy_self(__sp_block* b) noexcept {
    __sp_obj_block* __self = static_cast<__sp_obj_block*>(b);
    _Ap a(__self->__a_);
    std::destroy_at(__self);
    ::__ycxx::__detail::__sp_deallocate_block(a, __self);
  }

public:
  constexpr explicit __sp_obj_block(const _Ap& a) noexcept : __sp_block(&__destroy_self), __a_(a) {}
  constexpr ~__sp_obj_block() {}

  template <__sp_init _How, class... _Args>
  constexpr void construct(_Args&&... __args) {
    if constexpr (_How == __sp_init::__overwrite)
      ::new (static_cast<void*>(__builtin_addressof(value))) _Up;
    else if constexpr (_ViaAlloc)
      std::allocator_traits<_Ap>::construct(__a_, __builtin_addressof(value), static_cast<_Args&&>(__args)...);
    else
      ::new (static_cast<void*>(__builtin_addressof(value))) _Up(static_cast<_Args&&>(__args)...);
  }
};

// Deallocates a block whose construction of the owned object threw.
template <class _Block, class _Ap>
struct __sp_block_guard {
  _Block* b;
  const _Ap& a;
  constexpr ~__sp_block_guard() {
    if (b) {
      std::destroy_at(b);
      ::__ycxx::__detail::__sp_deallocate_block(a, b);
    }
  }
};

// make_shared / allocate_shared of an array: n elements of type E (cv-unqualified, possibly an
// array type). A is the allocator rebound to the scalar type remove_all_extents_t<E>.
template <class _Ep, class _Ap, bool _ViaAlloc>
class __sp_array_block final : public __sp_block {
  using _Sp = std::remove_all_extents_t<_Ep>;
  // At run time the block and its elements share one allocation of `__units` units of
  // max(alignof(block), alignof(E)) bytes; the elements start at `offset`.
  struct alignas(alignof(_Ep) > alignof(__sp_block) ? alignof(_Ep) : alignof(__sp_block)) __unit {
    unsigned char __bytes[alignof(_Ep) > alignof(__sp_block) ? alignof(_Ep) : alignof(__sp_block)];
  };
  using _UA = typename std::allocator_traits<_Ap>::template rebind_alloc<__unit>;
  using _EA = typename std::allocator_traits<_Ap>::template rebind_alloc<_Ep>;

  [[no_unique_address]] _Ap __a_;
  _Ep* __elems_;
  std::size_t __n_;

  static constexpr std::size_t offset() noexcept {
    return (sizeof(__sp_array_block) + alignof(_Ep) - 1) / alignof(_Ep) * alignof(_Ep);
  }
  static constexpr std::size_t __units(std::size_t n) noexcept {
    return (offset() + n * sizeof(_Ep) + sizeof(__unit) - 1) / sizeof(__unit);
  }

  constexpr void __dispose() noexcept override { ::__ycxx::__detail::__sp_destroy_n<_ViaAlloc>(__a_, __elems_, __n_); }
  static constexpr void __destroy_self(__sp_block* b) noexcept {
    __sp_array_block* __self = static_cast<__sp_array_block*>(b);
    _Ap a(__self->__a_);
    if consteval {
      _Ep* __y_elems = __self->__elems_;
      std::size_t n = __self->__n_;
      std::destroy_at(__self);
      _EA __ea(a); // the elements' storage is allocated even for n == 0
      std::allocator_traits<_EA>::deallocate(
          __ea, std::pointer_traits<typename std::allocator_traits<_EA>::pointer>::pointer_to(*__y_elems), n);
      ::__ycxx::__detail::__sp_deallocate_block(a, __self);
    } else {
      std::size_t count = __units(__self->__n_);
      __unit* __raw = reinterpret_cast<__unit*>(__self);
      std::destroy_at(__self);
      _UA __ua(a);
      std::allocator_traits<_UA>::deallocate(
          __ua, std::pointer_traits<typename std::allocator_traits<_UA>::pointer>::pointer_to(*__raw), count);
    }
  }

public:
  constexpr __sp_array_block(const _Ap& a, _Ep* __y_elems, std::size_t n) noexcept : __sp_block(&__destroy_self), __a_(a), __elems_(__y_elems), __n_(n) {}
  constexpr ~__sp_array_block() = default;

  constexpr _Ep* elements() const noexcept { return __elems_; }

  // Allocates the block and n elements and initializes the elements; on an exception nothing
  // is left allocated.
  template <__sp_init _How>
  static constexpr __sp_array_block* __create(const _Ap& a, std::size_t n, const _Ep* __u) {
    _Ap __a2(a);
    if consteval {
      _EA __ea(a);
      _Ep* __y_elems = std::to_address(std::allocator_traits<_EA>::allocate(__ea, n));
      __sp_array_block* b;
      {
        // Frees the elements' storage if the block allocation or an element constructor throws.
        struct __guard {
          _EA& __ea;
          _Ep* __y_elems;
          std::size_t n;
          constexpr ~__guard() {
            if (__y_elems)
              std::allocator_traits<_EA>::deallocate(
                  __ea, std::pointer_traits<typename std::allocator_traits<_EA>::pointer>::pointer_to(*__y_elems), n);
          }
        } __g{__ea, __y_elems, n};
        b = std::construct_at(::__ycxx::__detail::__sp_allocate_block<__sp_array_block>(a), a, __y_elems, n);
        __sp_block_guard<__sp_array_block, _Ap> __bg{b, a};
        ::__ycxx::__detail::__sp_construct_n<_ViaAlloc, _How>(__a2, __y_elems, n, __u);
        __bg.b = nullptr;
        __g.__y_elems = nullptr;
      }
      return b;
    } else {
      if (n > (static_cast<std::size_t>(-1) - offset() - sizeof(__unit)) / sizeof(_Ep))
        ::__ycxx::__detail::__throw_bad_array_new_length();
      _UA __ua(a);
      std::size_t count = __units(n);
      __unit* __raw = std::to_address(std::allocator_traits<_UA>::allocate(__ua, count));
      _Ep* __y_elems = reinterpret_cast<_Ep*>(reinterpret_cast<unsigned char*>(__raw) + offset());
      struct __guard {
        _UA& __ua;
        __unit* __raw;
        std::size_t count;
        constexpr ~__guard() {
          if (__raw)
            std::allocator_traits<_UA>::deallocate(
                __ua, std::pointer_traits<typename std::allocator_traits<_UA>::pointer>::pointer_to(*__raw), count);
        }
      } __g{__ua, __raw, count};
      ::__ycxx::__detail::__sp_construct_n<_ViaAlloc, _How>(__a2, __y_elems, n, __u);
      __g.__raw = nullptr;
      return ::new (static_cast<void*>(__raw)) __sp_array_block(a, __y_elems, n);
    }
  }
};

// "Y* is compatible with T*" ([util.smartptr.shared.general]/6).
template <class _Yp, class _Tp>
concept __sp_compatible = std::is_convertible_v<_Yp*, _Tp*> ||
                        (std::is_bounded_array_v<_Yp> && std::is_same_v<std::remove_extent_t<_Yp>[], std::remove_cv_t<_Tp>>);

// The pointer conversions the constructors from Y* accept ([util.smartptr.shared.const]/3, 9).
template <class _Yp, class _Tp>
concept __sp_convertible_ptr = (!std::is_array_v<_Tp> && std::is_convertible_v<_Yp*, _Tp*>) ||
                             (std::is_unbounded_array_v<_Tp> && std::is_convertible_v<_Yp (*)[], _Tp*>) ||
                             (std::is_bounded_array_v<_Tp> && std::is_convertible_v<_Yp (*)[std::extent_v<_Tp>], _Tp*>);

// A pointer to the unambiguous, accessible enable_shared_from_this base of *p, if there is one.
template <class _Xp>
constexpr const std::enable_shared_from_this<_Xp>* __sp_esft_base(const std::enable_shared_from_this<_Xp>* p) noexcept {
  return p;
}

// The library's access to shared_ptr's representation (defined after shared_ptr).
struct __sp_access;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [util.smartptr.shared]
template <class _Tp>
class shared_ptr {
public:
  using element_type = remove_extent_t<_Tp>;
  using weak_type = weak_ptr<_Tp>;

private:
  template <class>
  friend class shared_ptr;
  template <class>
  friend class weak_ptr;
  friend struct __ycxx::__detail::__sp_access;

  element_type* __ptr_ = nullptr;
  __ycxx::__detail::__sp_block* __ctrl_ = nullptr;

  // "enables shared_from_this with p" ([util.smartptr.shared.const]/1).
  template <class _Yp>
  constexpr void __enable_shared_from_this_with(_Yp* p) noexcept {
    if constexpr (requires { __ycxx::__detail::__sp_esft_base(p); }) {
      auto* base = __ycxx::__detail::__sp_esft_base(p);
      if (p != nullptr && base->__weak_this_.expired())
        base->__weak_this_ = shared_ptr<remove_cv_t<_Yp>>(*this, const_cast<remove_cv_t<_Yp>*>(p));
    }
  }

  template <class _Yp, class _Dp, class _Ap>
  constexpr void __adopt(_Yp* p, _Dp& d, const _Ap& a) {
    __ctrl_ = __ycxx::__detail::__sp_make_ptr_block(p, d, a);
    __ptr_ = p;
    if constexpr (!is_array_v<_Tp>)
      __enable_shared_from_this_with(p);
  }

public:
  // [util.smartptr.shared.const]
  constexpr shared_ptr() noexcept = default;
  constexpr shared_ptr(nullptr_t) noexcept : shared_ptr() {}

  template <class _Yp>
    requires __ycxx::__detail::__sp_convertible_ptr<_Yp, _Tp> &&
             ((is_array_v<_Tp> && requires(_Yp* p) { delete[] p; }) || (!is_array_v<_Tp> && requires(_Yp* p) { delete p; }))
  constexpr explicit shared_ptr(_Yp* p) {
    static_assert(requires { sizeof(_Yp); }, "std::shared_ptr(Y*): Y must be a complete type");
    __ycxx::__detail::__sp_default_delete<is_array_v<_Tp>> d;
    __adopt(p, d, allocator<int>());
  }
  template <class _Yp, class _Dp>
    requires __ycxx::__detail::__sp_convertible_ptr<_Yp, _Tp> && is_move_constructible_v<_Dp> &&
             requires(_Dp& d, _Yp* p) { d(p); }
  constexpr shared_ptr(_Yp* p, _Dp d) {
    __adopt(p, d, allocator<int>());
  }
  template <class _Yp, class _Dp, class _Ap>
    requires __ycxx::__detail::__sp_convertible_ptr<_Yp, _Tp> && is_move_constructible_v<_Dp> &&
             requires(_Dp& d, _Yp* p) { d(p); }
  constexpr shared_ptr(_Yp* p, _Dp d, _Ap a) {
    __adopt(p, d, a);
  }
  template <class _Dp>
    requires is_move_constructible_v<_Dp> && requires(_Dp& d) { d(nullptr); }
  constexpr shared_ptr(nullptr_t p, _Dp d) {
    __ctrl_ = __ycxx::__detail::__sp_make_ptr_block(p, d, allocator<int>());
  }
  template <class _Dp, class _Ap>
    requires is_move_constructible_v<_Dp> && requires(_Dp& d) { d(nullptr); }
  constexpr shared_ptr(nullptr_t p, _Dp d, _Ap a) {
    __ctrl_ = __ycxx::__detail::__sp_make_ptr_block(p, d, a);
  }

  // Aliasing constructors.
  template <class _Yp>
  constexpr shared_ptr(const shared_ptr<_Yp>& r, element_type* p) noexcept : __ptr_(p), __ctrl_(r.__ctrl_) {
    if (__ctrl_)
      __ctrl_->__add_shared();
  }
  template <class _Yp>
  constexpr shared_ptr(shared_ptr<_Yp>&& r, element_type* p) noexcept : __ptr_(p), __ctrl_(r.__ctrl_) {
    r.__ptr_ = nullptr;
    r.__ctrl_ = nullptr;
  }

  constexpr shared_ptr(const shared_ptr& r) noexcept : __ptr_(r.__ptr_), __ctrl_(r.__ctrl_) {
    if (__ctrl_)
      __ctrl_->__add_shared();
  }
  template <class _Yp>
    requires __ycxx::__detail::__sp_compatible<_Yp, _Tp>
  constexpr shared_ptr(const shared_ptr<_Yp>& r) noexcept : __ptr_(r.__ptr_), __ctrl_(r.__ctrl_) {
    if (__ctrl_)
      __ctrl_->__add_shared();
  }
  constexpr shared_ptr(shared_ptr&& r) noexcept : __ptr_(r.__ptr_), __ctrl_(r.__ctrl_) {
    r.__ptr_ = nullptr;
    r.__ctrl_ = nullptr;
  }
  template <class _Yp>
    requires __ycxx::__detail::__sp_compatible<_Yp, _Tp>
  constexpr shared_ptr(shared_ptr<_Yp>&& r) noexcept : __ptr_(r.__ptr_), __ctrl_(r.__ctrl_) {
    r.__ptr_ = nullptr;
    r.__ctrl_ = nullptr;
  }

  template <class _Yp>
    requires __ycxx::__detail::__sp_compatible<_Yp, _Tp>
  constexpr explicit shared_ptr(const weak_ptr<_Yp>& r) {
    if (!r.__ctrl_ || !r.__ctrl_->__try_add_shared())
      __ycxx::__detail::__throw_bad_weak_ptr();
    __ctrl_ = r.__ctrl_;
    __ptr_ = r.__ptr_;
  }

  template <class _Yp, class _Dp>
    requires __ycxx::__detail::__sp_compatible<_Yp, _Tp> &&
             is_convertible_v<typename unique_ptr<_Yp, _Dp>::pointer, element_type*> &&
             (is_reference_v<_Dp> || is_move_constructible_v<_Dp>) // implied by the Effects' shared_ptr(p, std::move(d))
  constexpr shared_ptr(unique_ptr<_Yp, _Dp>&& r) {
    if (!r.get())
      return;
    using _Pp = typename unique_ptr<_Yp, _Dp>::pointer;
    // The block is allocated before r gives up anything, so a failed allocation leaves r
    // untouched ("the constructor has no effect"); a reference deleter is held through
    // reference_wrapper ([util.smartptr.shared.const]/29).
    using _DS = conditional_t<is_reference_v<_Dp>, reference_wrapper<remove_reference_t<_Dp>>, _Dp>;
    using _Block = __ycxx::__detail::__sp_ptr_block<_Pp, _DS, allocator<int>>;
    _Block* b = __ycxx::__detail::__sp_allocate_block<_Block>(allocator<int>());
    _Pp p = r.get();
    if constexpr (is_reference_v<_Dp>)
      __ctrl_ = std::construct_at(b, p, _DS(r.get_deleter()), allocator<int>());
    else
      __ctrl_ = std::construct_at(b, p, static_cast<_Dp&&>(r.get_deleter()), allocator<int>());
    (void)r.release();
    __ptr_ = p;
    if constexpr (!is_array_v<_Tp> && is_same_v<_Pp, _Yp*>)
      __enable_shared_from_this_with(p);
  }

  // [util.smartptr.shared.dest]
  constexpr ~shared_ptr() {
    if (__ctrl_)
      __ctrl_->__release_shared();
  }

  // [util.smartptr.shared.assign]
  constexpr shared_ptr& operator=(const shared_ptr& r) noexcept {
    shared_ptr(r).swap(*this);
    return *this;
  }
  template <class _Yp>
    requires __ycxx::__detail::__sp_compatible<_Yp, _Tp>
  constexpr shared_ptr& operator=(const shared_ptr<_Yp>& r) noexcept {
    shared_ptr(r).swap(*this);
    return *this;
  }
  constexpr shared_ptr& operator=(shared_ptr&& r) noexcept {
    shared_ptr(static_cast<shared_ptr&&>(r)).swap(*this);
    return *this;
  }
  template <class _Yp>
    requires __ycxx::__detail::__sp_compatible<_Yp, _Tp>
  constexpr shared_ptr& operator=(shared_ptr<_Yp>&& r) noexcept {
    shared_ptr(static_cast<shared_ptr<_Yp>&&>(r)).swap(*this);
    return *this;
  }
  template <class _Yp, class _Dp>
    requires is_constructible_v<shared_ptr, unique_ptr<_Yp, _Dp>>
  constexpr shared_ptr& operator=(unique_ptr<_Yp, _Dp>&& r) {
    shared_ptr(static_cast<unique_ptr<_Yp, _Dp>&&>(r)).swap(*this);
    return *this;
  }

  // [util.smartptr.shared.mod]
  constexpr void swap(shared_ptr& r) noexcept {
    element_type* p = __ptr_;
    __ptr_ = r.__ptr_;
    r.__ptr_ = p;
    __ycxx::__detail::__sp_block* c = __ctrl_;
    __ctrl_ = r.__ctrl_;
    r.__ctrl_ = c;
  }
  constexpr void reset() noexcept { shared_ptr().swap(*this); }
  template <class _Yp>
    requires is_constructible_v<shared_ptr, _Yp*>
  constexpr void reset(_Yp* p) {
    shared_ptr(p).swap(*this);
  }
  template <class _Yp, class _Dp>
    requires is_constructible_v<shared_ptr, _Yp*, _Dp>
  constexpr void reset(_Yp* p, _Dp d) {
    shared_ptr(p, static_cast<_Dp&&>(d)).swap(*this);
  }
  template <class _Yp, class _Dp, class _Ap>
    requires is_constructible_v<shared_ptr, _Yp*, _Dp, _Ap>
  constexpr void reset(_Yp* p, _Dp d, _Ap a) {
    shared_ptr(p, static_cast<_Dp&&>(d), static_cast<_Ap&&>(a)).swap(*this);
  }

  // [util.smartptr.shared.obs]
  constexpr element_type* get() const noexcept { return __ptr_; }
  constexpr add_lvalue_reference_t<_Tp> operator*() const noexcept
    requires(!is_void_v<_Tp> && !is_array_v<_Tp>)
  {
    __ycxx::__detail::__precondition(__ptr_ != nullptr, "std::shared_ptr::operator*: null pointer");
    return *__ptr_;
  }
  constexpr _Tp* operator->() const noexcept
    requires(!is_array_v<_Tp>)
  {
    __ycxx::__detail::__precondition(__ptr_ != nullptr, "std::shared_ptr::operator->: null pointer");
    return __ptr_;
  }
  constexpr add_lvalue_reference_t<element_type> operator[](ptrdiff_t i) const noexcept // Throws: nothing
    requires is_array_v<_Tp>
  {
    __ycxx::__detail::__precondition(__ptr_ != nullptr, "std::shared_ptr::operator[]: null pointer");
    if constexpr (is_bounded_array_v<_Tp>)
      __ycxx::__detail::__precondition(i >= 0 && i < static_cast<ptrdiff_t>(extent_v<_Tp>),
                                 "std::shared_ptr::operator[]: index out of bounds");
    else
      __ycxx::__detail::__precondition(i >= 0, "std::shared_ptr::operator[]: negative index");
    return __ptr_[i];
  }
  constexpr long use_count() const noexcept { return __ctrl_ ? __ctrl_->use_count() : 0; }
  constexpr explicit operator bool() const noexcept { return __ptr_ != nullptr; }

  template <class _Up>
  bool owner_before(const shared_ptr<_Up>& b) const noexcept {
    return less<__ycxx::__detail::__sp_block*>()(__ctrl_, b.__ctrl_);
  }
  template <class _Up>
  bool owner_before(const weak_ptr<_Up>& b) const noexcept {
    return less<__ycxx::__detail::__sp_block*>()(__ctrl_, b.__ctrl_);
  }
  size_t owner_hash() const noexcept { return hash<__ycxx::__detail::__sp_block*>()(__ctrl_); }
  template <class _Up>
  constexpr bool owner_equal(const shared_ptr<_Up>& b) const noexcept {
    return __ctrl_ == b.__ctrl_;
  }
  template <class _Up>
  constexpr bool owner_equal(const weak_ptr<_Up>& b) const noexcept {
    return __ctrl_ == b.__ctrl_;
  }
};

template <class _Tp>
shared_ptr(weak_ptr<_Tp>) -> shared_ptr<_Tp>;
template <class _Tp, class _Dp>
shared_ptr(unique_ptr<_Tp, _Dp>) -> shared_ptr<_Tp>;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

struct __sp_access {
  // A shared_ptr taking over one already-counted reference to ctrl.
  template <class _Tp>
  static constexpr std::shared_ptr<_Tp> __adopt(typename std::shared_ptr<_Tp>::element_type* p, __sp_block* __ctrl) noexcept {
    std::shared_ptr<_Tp> r;
    r.__ptr_ = p;
    r.__ctrl_ = __ctrl;
    return r;
  }
  template <class _Tp>
  static constexpr __sp_block* __ctrl(const std::shared_ptr<_Tp>& p) noexcept {
    return p.__ctrl_;
  }
  // The stored pointer of a weak_ptr (atomic<weak_ptr<T>> compares it, [util.smartptr.atomic.weak]).
  template <class _Tp>
  static constexpr auto* __stored(const std::weak_ptr<_Tp>& __w) noexcept {
    return __w.__ptr_;
  }
  template <class _Tp>
  static constexpr void enable_shared_from_this(std::shared_ptr<_Tp>& r) noexcept {
    r.__enable_shared_from_this_with(r.__ptr_);
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [util.smartptr.weak]
template <class _Tp>
class weak_ptr {
public:
  using element_type = remove_extent_t<_Tp>;

private:
  template <class>
  friend class shared_ptr;
  template <class>
  friend class weak_ptr;
  friend struct __ycxx::__detail::__sp_access;

  element_type* __ptr_ = nullptr;
  __ycxx::__detail::__sp_block* __ctrl_ = nullptr;

  // The stored pointer of r converted to element_type*. Converting to a virtual base reads the
  // object, which may already be destroyed, so that conversion goes through lock().
  template <class _Yp>
  static constexpr element_type* __convert(const weak_ptr<_Yp>& r) noexcept {
    if constexpr (is_same_v<remove_cv_t<typename weak_ptr<_Yp>::element_type>, remove_cv_t<element_type>> ||
                  is_void_v<element_type>) {
      return r.__ptr_;
    } else if constexpr (is_virtual_base_of_v<remove_cv_t<element_type>,
                                              remove_cv_t<typename weak_ptr<_Yp>::element_type>>) {
      return r.lock().get();
    } else {
      return r.__ptr_;
    }
  }

public:
  // [util.smartptr.weak.const]
  constexpr weak_ptr() noexcept = default;
  template <class _Yp>
    requires __ycxx::__detail::__sp_compatible<_Yp, _Tp>
  constexpr weak_ptr(const shared_ptr<_Yp>& r) noexcept : __ptr_(r.__ptr_), __ctrl_(r.__ctrl_) {
    if (__ctrl_)
      __ctrl_->__add_weak();
  }
  constexpr weak_ptr(const weak_ptr& r) noexcept : __ptr_(r.__ptr_), __ctrl_(r.__ctrl_) {
    if (__ctrl_)
      __ctrl_->__add_weak();
  }
  template <class _Yp>
    requires __ycxx::__detail::__sp_compatible<_Yp, _Tp>
  constexpr weak_ptr(const weak_ptr<_Yp>& r) noexcept : __ptr_(__convert(r)), __ctrl_(r.__ctrl_) {
    if (__ctrl_)
      __ctrl_->__add_weak();
  }
  constexpr weak_ptr(weak_ptr&& r) noexcept : __ptr_(r.__ptr_), __ctrl_(r.__ctrl_) {
    r.__ptr_ = nullptr;
    r.__ctrl_ = nullptr;
  }
  template <class _Yp>
    requires __ycxx::__detail::__sp_compatible<_Yp, _Tp>
  constexpr weak_ptr(weak_ptr<_Yp>&& r) noexcept : __ptr_(__convert(r)), __ctrl_(r.__ctrl_) {
    r.__ptr_ = nullptr;
    r.__ctrl_ = nullptr;
  }

  // [util.smartptr.weak.dest]
  constexpr ~weak_ptr() {
    if (__ctrl_)
      __ctrl_->__release_weak();
  }

  // [util.smartptr.weak.assign]
  constexpr weak_ptr& operator=(const weak_ptr& r) noexcept {
    weak_ptr(r).swap(*this);
    return *this;
  }
  template <class _Yp>
    requires __ycxx::__detail::__sp_compatible<_Yp, _Tp>
  constexpr weak_ptr& operator=(const weak_ptr<_Yp>& r) noexcept {
    weak_ptr(r).swap(*this);
    return *this;
  }
  template <class _Yp>
    requires __ycxx::__detail::__sp_compatible<_Yp, _Tp>
  constexpr weak_ptr& operator=(const shared_ptr<_Yp>& r) noexcept {
    weak_ptr(r).swap(*this);
    return *this;
  }
  constexpr weak_ptr& operator=(weak_ptr&& r) noexcept {
    weak_ptr(static_cast<weak_ptr&&>(r)).swap(*this);
    return *this;
  }
  template <class _Yp>
    requires __ycxx::__detail::__sp_compatible<_Yp, _Tp>
  constexpr weak_ptr& operator=(weak_ptr<_Yp>&& r) noexcept {
    weak_ptr(static_cast<weak_ptr<_Yp>&&>(r)).swap(*this);
    return *this;
  }

  // [util.smartptr.weak.mod]
  constexpr void swap(weak_ptr& r) noexcept {
    element_type* p = __ptr_;
    __ptr_ = r.__ptr_;
    r.__ptr_ = p;
    __ycxx::__detail::__sp_block* c = __ctrl_;
    __ctrl_ = r.__ctrl_;
    r.__ctrl_ = c;
  }
  constexpr void reset() noexcept { weak_ptr().swap(*this); }

  // [util.smartptr.weak.obs]
  constexpr long use_count() const noexcept { return __ctrl_ ? __ctrl_->use_count() : 0; }
  constexpr bool expired() const noexcept { return use_count() == 0; }
  constexpr shared_ptr<_Tp> lock() const noexcept {
    if (__ctrl_ && __ctrl_->__try_add_shared())
      return __ycxx::__detail::__sp_access::__adopt<_Tp>(__ptr_, __ctrl_);
    return shared_ptr<_Tp>();
  }
  template <class _Up>
  bool owner_before(const shared_ptr<_Up>& b) const noexcept {
    return less<__ycxx::__detail::__sp_block*>()(__ctrl_, b.__ctrl_);
  }
  template <class _Up>
  bool owner_before(const weak_ptr<_Up>& b) const noexcept {
    return less<__ycxx::__detail::__sp_block*>()(__ctrl_, b.__ctrl_);
  }
  size_t owner_hash() const noexcept { return hash<__ycxx::__detail::__sp_block*>()(__ctrl_); }
  template <class _Up>
  constexpr bool owner_equal(const shared_ptr<_Up>& b) const noexcept {
    return __ctrl_ == b.__ctrl_;
  }
  template <class _Up>
  constexpr bool owner_equal(const weak_ptr<_Up>& b) const noexcept {
    return __ctrl_ == b.__ctrl_;
  }
};

template <class _Tp>
weak_ptr(shared_ptr<_Tp>) -> weak_ptr<_Tp>;

// [util.smartptr.weak.spec]
template <class _Tp>
constexpr void swap(weak_ptr<_Tp>& a, weak_ptr<_Tp>& b) noexcept {
  a.swap(b);
}

// [util.smartptr.enab]
template <class _Tp>
class enable_shared_from_this {
  template <class>
  friend class shared_ptr;

  mutable weak_ptr<_Tp> __weak_this_;

protected:
  constexpr enable_shared_from_this() noexcept {}
  constexpr enable_shared_from_this(const enable_shared_from_this&) noexcept {}
  constexpr enable_shared_from_this& operator=(const enable_shared_from_this&) noexcept { return *this; }
  constexpr ~enable_shared_from_this() {}

public:
  constexpr shared_ptr<_Tp> shared_from_this() { return shared_ptr<_Tp>(__weak_this_); }
  constexpr shared_ptr<const _Tp> shared_from_this() const { return shared_ptr<const _Tp>(__weak_this_); }
  constexpr weak_ptr<_Tp> weak_from_this() noexcept { return __weak_this_; }
  constexpr weak_ptr<const _Tp> weak_from_this() const noexcept { return __weak_this_; }
};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// [util.smartptr.shared.create]: the object form. A is the allocator the caller passed
// (std::allocator for make_shared); ViaAlloc selects allocator construct/destroy.
template <class _Tp, __sp_init _How, bool _ViaAlloc, class _Ap, class... _Args>
constexpr std::shared_ptr<_Tp> __sp_make_obj(const _Ap& a, _Args&&... __args) {
  using _Vp = std::remove_cv_t<_Tp>;
  using _VA = typename std::allocator_traits<_Ap>::template rebind_alloc<_Vp>;
  using _Block = __sp_obj_block<_Tp, _VA, _ViaAlloc>;
  _VA __va(a);
  _Block* b = std::construct_at(::__ycxx::__detail::__sp_allocate_block<_Block>(__va), __va);
  {
    __sp_block_guard<_Block, _VA> __g{b, __va};
    b->template construct<_How>(static_cast<_Args&&>(__args)...);
    __g.b = nullptr;
  }
  std::shared_ptr<_Tp> r = __sp_access::__adopt<_Tp>(__builtin_addressof(b->value), b);
  __sp_access::enable_shared_from_this(r);
  return r;
}

// The array forms: n elements of remove_extent_t<T>, from *u when How is fill.
template <class _Tp, __sp_init _How, bool _ViaAlloc, class _Ap>
constexpr std::shared_ptr<_Tp> __sp_make_array(const _Ap& a, std::size_t n, const std::remove_extent_t<_Tp>* __u) {
  using _Ep = std::remove_cv_t<std::remove_extent_t<_Tp>>;
  using _SA = typename std::allocator_traits<_Ap>::template rebind_alloc<std::remove_cv_t<std::remove_all_extents_t<_Tp>>>;
  using _Block = __sp_array_block<_Ep, _SA, _ViaAlloc>;
  _Block* b = _Block::template __create<_How>(_SA(a), n, __u);
  return __sp_access::__adopt<_Tp>(b->elements(), b);
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [util.smartptr.shared.create]
template <class _Tp, class... _Args>
  requires(!is_array_v<_Tp>)
constexpr shared_ptr<_Tp> make_shared(_Args&&... __args) {
  return __ycxx::__detail::__sp_make_obj<_Tp, __ycxx::__detail::__sp_init::value, false>(allocator<int>(),
                                                                           static_cast<_Args&&>(__args)...);
}
template <class _Tp, class _Ap, class... _Args>
  requires(!is_array_v<_Tp>)
constexpr shared_ptr<_Tp> allocate_shared(const _Ap& a, _Args&&... __args) {
  return __ycxx::__detail::__sp_make_obj<_Tp, __ycxx::__detail::__sp_init::value, true>(a, static_cast<_Args&&>(__args)...);
}

template <class _Tp>
  requires is_unbounded_array_v<_Tp>
constexpr shared_ptr<_Tp> make_shared(size_t _Np) {
  return __ycxx::__detail::__sp_make_array<_Tp, __ycxx::__detail::__sp_init::value, false>(allocator<int>(), _Np, nullptr);
}
template <class _Tp, class _Ap>
  requires is_unbounded_array_v<_Tp>
constexpr shared_ptr<_Tp> allocate_shared(const _Ap& a, size_t _Np) {
  return __ycxx::__detail::__sp_make_array<_Tp, __ycxx::__detail::__sp_init::value, true>(a, _Np, nullptr);
}
template <class _Tp>
  requires is_bounded_array_v<_Tp>
constexpr shared_ptr<_Tp> make_shared() {
  return __ycxx::__detail::__sp_make_array<_Tp, __ycxx::__detail::__sp_init::value, false>(allocator<int>(), extent_v<_Tp>, nullptr);
}
template <class _Tp, class _Ap>
  requires is_bounded_array_v<_Tp>
constexpr shared_ptr<_Tp> allocate_shared(const _Ap& a) {
  return __ycxx::__detail::__sp_make_array<_Tp, __ycxx::__detail::__sp_init::value, true>(a, extent_v<_Tp>, nullptr);
}
template <class _Tp>
  requires is_unbounded_array_v<_Tp>
constexpr shared_ptr<_Tp> make_shared(size_t _Np, const remove_extent_t<_Tp>& __u) {
  return __ycxx::__detail::__sp_make_array<_Tp, __ycxx::__detail::__sp_init::fill, false>(allocator<int>(), _Np,
                                                                            __builtin_addressof(__u));
}
template <class _Tp, class _Ap>
  requires is_unbounded_array_v<_Tp>
constexpr shared_ptr<_Tp> allocate_shared(const _Ap& a, size_t _Np, const remove_extent_t<_Tp>& __u) {
  return __ycxx::__detail::__sp_make_array<_Tp, __ycxx::__detail::__sp_init::fill, true>(a, _Np, __builtin_addressof(__u));
}
template <class _Tp>
  requires is_bounded_array_v<_Tp>
constexpr shared_ptr<_Tp> make_shared(const remove_extent_t<_Tp>& __u) {
  return __ycxx::__detail::__sp_make_array<_Tp, __ycxx::__detail::__sp_init::fill, false>(allocator<int>(), extent_v<_Tp>,
                                                                            __builtin_addressof(__u));
}
template <class _Tp, class _Ap>
  requires is_bounded_array_v<_Tp>
constexpr shared_ptr<_Tp> allocate_shared(const _Ap& a, const remove_extent_t<_Tp>& __u) {
  return __ycxx::__detail::__sp_make_array<_Tp, __ycxx::__detail::__sp_init::fill, true>(a, extent_v<_Tp>, __builtin_addressof(__u));
}
// The _for_overwrite forms default-initialize and destroy with ~U() even when an allocator
// supplies the storage ([util.smartptr.shared.create]/7.8, 7.11).
template <class _Tp>
  requires(!is_unbounded_array_v<_Tp>)
constexpr shared_ptr<_Tp> make_shared_for_overwrite() {
  if constexpr (is_array_v<_Tp>)
    return __ycxx::__detail::__sp_make_array<_Tp, __ycxx::__detail::__sp_init::__overwrite, false>(allocator<int>(), extent_v<_Tp>,
                                                                                   nullptr);
  else
    return __ycxx::__detail::__sp_make_obj<_Tp, __ycxx::__detail::__sp_init::__overwrite, false>(allocator<int>());
}
template <class _Tp, class _Ap>
  requires(!is_unbounded_array_v<_Tp>)
constexpr shared_ptr<_Tp> allocate_shared_for_overwrite(const _Ap& a) {
  if constexpr (is_array_v<_Tp>)
    return __ycxx::__detail::__sp_make_array<_Tp, __ycxx::__detail::__sp_init::__overwrite, false>(a, extent_v<_Tp>, nullptr);
  else
    return __ycxx::__detail::__sp_make_obj<_Tp, __ycxx::__detail::__sp_init::__overwrite, false>(a);
}
template <class _Tp>
  requires is_unbounded_array_v<_Tp>
constexpr shared_ptr<_Tp> make_shared_for_overwrite(size_t _Np) {
  return __ycxx::__detail::__sp_make_array<_Tp, __ycxx::__detail::__sp_init::__overwrite, false>(allocator<int>(), _Np, nullptr);
}
template <class _Tp, class _Ap>
  requires is_unbounded_array_v<_Tp>
constexpr shared_ptr<_Tp> allocate_shared_for_overwrite(const _Ap& a, size_t _Np) {
  return __ycxx::__detail::__sp_make_array<_Tp, __ycxx::__detail::__sp_init::__overwrite, false>(a, _Np, nullptr);
}

// [util.smartptr.shared.cmp]
template <class _Tp, class _Up>
constexpr bool operator==(const shared_ptr<_Tp>& a, const shared_ptr<_Up>& b) noexcept {
  return a.get() == b.get();
}
template <class _Tp>
constexpr bool operator==(const shared_ptr<_Tp>& a, nullptr_t) noexcept {
  return !a;
}
template <class _Tp, class _Up>
constexpr strong_ordering operator<=>(const shared_ptr<_Tp>& a, const shared_ptr<_Up>& b) noexcept {
  return compare_three_way()(a.get(), b.get());
}
template <class _Tp>
constexpr strong_ordering operator<=>(const shared_ptr<_Tp>& a, nullptr_t) noexcept {
  return compare_three_way()(a.get(), static_cast<typename shared_ptr<_Tp>::element_type*>(nullptr));
}

// [util.smartptr.shared.spec]
template <class _Tp>
constexpr void swap(shared_ptr<_Tp>& a, shared_ptr<_Tp>& b) noexcept {
  a.swap(b);
}

// [util.smartptr.shared.cast]
template <class _Tp, class _Up>
constexpr shared_ptr<_Tp> static_pointer_cast(const shared_ptr<_Up>& r) noexcept {
  return shared_ptr<_Tp>(r, static_cast<typename shared_ptr<_Tp>::element_type*>(r.get()));
}
template <class _Tp, class _Up>
constexpr shared_ptr<_Tp> static_pointer_cast(shared_ptr<_Up>&& r) noexcept {
  auto* p = static_cast<typename shared_ptr<_Tp>::element_type*>(r.get());
  return shared_ptr<_Tp>(static_cast<shared_ptr<_Up>&&>(r), p);
}
template <class _Tp, class _Up>
constexpr shared_ptr<_Tp> dynamic_pointer_cast(const shared_ptr<_Up>& r) noexcept {
  if (auto* p = dynamic_cast<typename shared_ptr<_Tp>::element_type*>(r.get()))
    return shared_ptr<_Tp>(r, p);
  return shared_ptr<_Tp>();
}
template <class _Tp, class _Up>
constexpr shared_ptr<_Tp> dynamic_pointer_cast(shared_ptr<_Up>&& r) noexcept {
  if (auto* p = dynamic_cast<typename shared_ptr<_Tp>::element_type*>(r.get()))
    return shared_ptr<_Tp>(static_cast<shared_ptr<_Up>&&>(r), p);
  return shared_ptr<_Tp>();
}
template <class _Tp, class _Up>
constexpr shared_ptr<_Tp> const_pointer_cast(const shared_ptr<_Up>& r) noexcept {
  return shared_ptr<_Tp>(r, const_cast<typename shared_ptr<_Tp>::element_type*>(r.get()));
}
template <class _Tp, class _Up>
constexpr shared_ptr<_Tp> const_pointer_cast(shared_ptr<_Up>&& r) noexcept {
  auto* p = const_cast<typename shared_ptr<_Tp>::element_type*>(r.get());
  return shared_ptr<_Tp>(static_cast<shared_ptr<_Up>&&>(r), p);
}
template <class _Tp, class _Up>
shared_ptr<_Tp> reinterpret_pointer_cast(const shared_ptr<_Up>& r) noexcept {
  return shared_ptr<_Tp>(r, reinterpret_cast<typename shared_ptr<_Tp>::element_type*>(r.get()));
}
template <class _Tp, class _Up>
shared_ptr<_Tp> reinterpret_pointer_cast(shared_ptr<_Up>&& r) noexcept {
  auto* p = reinterpret_cast<typename shared_ptr<_Tp>::element_type*>(r.get());
  return shared_ptr<_Tp>(static_cast<shared_ptr<_Up>&&>(r), p);
}

// [util.smartptr.getdeleter]
template <class _Dp, class _Tp>
constexpr _Dp* get_deleter(const shared_ptr<_Tp>& p) noexcept {
  __ycxx::__detail::__sp_block* c = __ycxx::__detail::__sp_access::__ctrl(p);
  return c ? static_cast<_Dp*>(c->__deleter(&__ycxx::__detail::__sp_tag<remove_cv_t<_Dp>>)) : nullptr;
}

// [util.smartptr.ownerless]
template <class _Tp = void>
struct owner_less;
template <class _Tp>
struct owner_less<shared_ptr<_Tp>> {
  bool operator()(const shared_ptr<_Tp>& __x, const shared_ptr<_Tp>& y) const noexcept { return __x.owner_before(y); }
  bool operator()(const shared_ptr<_Tp>& __x, const weak_ptr<_Tp>& y) const noexcept { return __x.owner_before(y); }
  bool operator()(const weak_ptr<_Tp>& __x, const shared_ptr<_Tp>& y) const noexcept { return __x.owner_before(y); }
};
template <class _Tp>
struct owner_less<weak_ptr<_Tp>> {
  bool operator()(const weak_ptr<_Tp>& __x, const weak_ptr<_Tp>& y) const noexcept { return __x.owner_before(y); }
  bool operator()(const shared_ptr<_Tp>& __x, const weak_ptr<_Tp>& y) const noexcept { return __x.owner_before(y); }
  bool operator()(const weak_ptr<_Tp>& __x, const shared_ptr<_Tp>& y) const noexcept { return __x.owner_before(y); }
};
template <>
struct owner_less<void> {
  template <class _Tp, class _Up>
  bool operator()(const shared_ptr<_Tp>& __x, const shared_ptr<_Up>& y) const noexcept {
    return __x.owner_before(y);
  }
  template <class _Tp, class _Up>
  bool operator()(const shared_ptr<_Tp>& __x, const weak_ptr<_Up>& y) const noexcept {
    return __x.owner_before(y);
  }
  template <class _Tp, class _Up>
  bool operator()(const weak_ptr<_Tp>& __x, const shared_ptr<_Up>& y) const noexcept {
    return __x.owner_before(y);
  }
  template <class _Tp, class _Up>
  bool operator()(const weak_ptr<_Tp>& __x, const weak_ptr<_Up>& y) const noexcept {
    return __x.owner_before(y);
  }
  using is_transparent = void;
};

// [util.smartptr.owner.hash]
struct owner_hash {
  template <class _Tp>
  size_t operator()(const shared_ptr<_Tp>& __x) const noexcept {
    return __x.owner_hash();
  }
  template <class _Tp>
  size_t operator()(const weak_ptr<_Tp>& __x) const noexcept {
    return __x.owner_hash();
  }
  using is_transparent = void;
};

// [util.smartptr.owner.equal]
struct owner_equal {
  template <class _Tp, class _Up>
  constexpr bool operator()(const shared_ptr<_Tp>& __x, const shared_ptr<_Up>& y) const noexcept {
    return __x.owner_equal(y);
  }
  template <class _Tp, class _Up>
  constexpr bool operator()(const shared_ptr<_Tp>& __x, const weak_ptr<_Up>& y) const noexcept {
    return __x.owner_equal(y);
  }
  template <class _Tp, class _Up>
  constexpr bool operator()(const weak_ptr<_Tp>& __x, const shared_ptr<_Up>& y) const noexcept {
    return __x.owner_equal(y);
  }
  template <class _Tp, class _Up>
  constexpr bool operator()(const weak_ptr<_Tp>& __x, const weak_ptr<_Up>& y) const noexcept {
    return __x.owner_equal(y);
  }
  using is_transparent = void;
};

// [util.smartptr.hash]
template <class _Tp>
struct hash<shared_ptr<_Tp>> {
  size_t operator()(const shared_ptr<_Tp>& p) const noexcept {
    return hash<typename shared_ptr<_Tp>::element_type*>()(p.get());
  }
};

// [util.smartptr.shared.io]: written against the declaration of basic_ostream (unique_ptr.hpp
// includes it); usable once <ostream> is included.
template <class _Ep, class _Tp, class _Yp>
basic_ostream<_Ep, _Tp>& operator<<(basic_ostream<_Ep, _Tp>& __os, const shared_ptr<_Yp>& p) {
  __os << p.get();
  return __os;
}

} // namespace std
