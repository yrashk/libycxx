// libycxx core: <memory> primitives -- pointer_traits, to_address, alignment, explicit lifetime
// management, construct_at/destroy, allocator, allocator_traits, uses_allocator.
#pragma once

#include <ycxx/core/type_traits.hpp>
#include <ycxx/core/new.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/error.hpp>

// ---------------------------------------------------------------------------------------------
// [pointer.traits]
// ---------------------------------------------------------------------------------------------
namespace ycxx::detail {

// First template argument of a template specialization, and rebinding of it.
template <class T>
struct first_template_arg {};
template <template <class, class...> class Tmpl, class T, class... Rest>
struct first_template_arg<Tmpl<T, Rest...>> {
  using type = T;
};
template <class T, class U>
struct rebind_first {};
template <template <class, class...> class Tmpl, class T, class... Rest, class U>
struct rebind_first<Tmpl<T, Rest...>, U> {
  using type = Tmpl<U, Rest...>;
};

template <class Ptr>
struct ptr_element : first_template_arg<Ptr> {};
template <class Ptr>
  requires requires { typename Ptr::element_type; }
struct ptr_element<Ptr> {
  using type = typename Ptr::element_type;
};

template <class Ptr>
struct ptr_difference {
  using type = std::ptrdiff_t;
};
template <class Ptr>
  requires requires { typename Ptr::difference_type; }
struct ptr_difference<Ptr> {
  using type = typename Ptr::difference_type;
};

template <class Ptr, class U>
struct ptr_rebind : rebind_first<Ptr, U> {};
template <class Ptr, class U>
  requires requires { typename Ptr::template rebind<U>; }
struct ptr_rebind<Ptr, U> {
  using type = typename Ptr::template rebind<U>;
};

// pointer_traits<Ptr> members exist only if element_type can be determined ([pointer.traits.types]).
template <class Ptr>
struct pointer_traits_base {};
template <class Ptr>
  requires requires { typename ptr_element<Ptr>::type; }
struct pointer_traits_base<Ptr> {
  using pointer = Ptr;
  using element_type = typename ptr_element<Ptr>::type;
  using difference_type = typename ptr_difference<Ptr>::type;
  template <class U>
  using rebind = typename ptr_rebind<Ptr, U>::type;

  static constexpr pointer pointer_to(std::conditional_t<is_void_v<element_type>, struct nat, element_type>& r)
    requires requires { Ptr::pointer_to(r); }
  {
    return Ptr::pointer_to(r);
  }
};

} // namespace ycxx::detail

namespace std {

template <class Ptr>
struct pointer_traits : ycxx::detail::pointer_traits_base<Ptr> {};

template <class T>
struct pointer_traits<T*> {
  using pointer = T*;
  using element_type = T;
  using difference_type = ptrdiff_t;
  template <class U>
  using rebind = U*;

  static constexpr pointer pointer_to(conditional_t<is_void_v<T>, struct ycxx_nat, T>& r) noexcept
    requires(!is_void_v<T>)
  {
    return __builtin_addressof(r);
  }
};

// [pointer.conversion]
template <class T>
constexpr T* to_address(T* p) noexcept {
  static_assert(!is_function_v<T>, "std::to_address: function pointer");
  return p;
}
template <class Ptr>
constexpr auto to_address(const Ptr& p) noexcept {
  if constexpr (requires { pointer_traits<Ptr>::to_address(p); })
    return pointer_traits<Ptr>::to_address(p);
  else
    return std::to_address(p.operator->());
}

// [ptr.align]
inline void* align(size_t alignment, size_t size, void*& ptr, size_t& space) {
  auto p = reinterpret_cast<__UINTPTR_TYPE__>(ptr);
  auto aligned = (p + alignment - 1) & ~static_cast<__UINTPTR_TYPE__>(alignment - 1);
  size_t pad = static_cast<size_t>(aligned - p);
  if (size > space || pad > space - size)
    return nullptr;
  space -= pad;
  return ptr = reinterpret_cast<void*>(aligned);
}

template <size_t N, class T>
[[nodiscard]] constexpr T* assume_aligned(T* ptr) {
  static_assert(N != 0 && (N & (N - 1)) == 0, "std::assume_aligned: N must be a power of two");
  if consteval {
    return ptr;
  } else {
    ycxx::detail::precondition(reinterpret_cast<__UINTPTR_TYPE__>(ptr) % N == 0,
                               "std::assume_aligned: pointer is not suitably aligned");
    return static_cast<T*>(__builtin_assume_aligned(ptr, N));
  }
}

template <size_t Alignment, class T>
[[nodiscard]] bool is_sufficiently_aligned(T* ptr) {
  static_assert(Alignment != 0 && (Alignment & (Alignment - 1)) == 0, "Alignment must be a power of two");
  return reinterpret_cast<__UINTPTR_TYPE__>(ptr) % Alignment == 0;
}

// [obj.lifetime]
template <class T>
  requires(is_implicit_lifetime_v<T> && is_aggregate_v<T>)
constexpr void start_lifetime(T& r) noexcept {
  if consteval {
    // No builtin exists on either compiler; in constant evaluation a default-initializing
    // placement new begins the lifetime without initializing subobjects of trivial types.
    ::new (static_cast<void*>(__builtin_addressof(r))) T;
  }
  // At run time storage of an implicit-lifetime type needs no action.
}

template <class T>
T* start_lifetime_as(void* p) noexcept {
  // Implicit object creation: memmove onto itself is specified to create objects ([intro.object]).
  return std::launder(static_cast<T*>(__builtin_memmove(p, p, sizeof(T))));
}
template <class T>
const T* start_lifetime_as(const void* p) noexcept {
  return start_lifetime_as<T>(const_cast<void*>(p));
}
template <class T>
volatile T* start_lifetime_as(volatile void* p) noexcept {
  return start_lifetime_as<T>(const_cast<void*>(p));
}
template <class T>
const volatile T* start_lifetime_as(const volatile void* p) noexcept {
  return start_lifetime_as<T>(const_cast<void*>(p));
}
template <class T>
T* start_lifetime_as_array(void* p, size_t n) noexcept {
  if (n == 0)
    return static_cast<T*>(p);
  return std::launder(static_cast<T*>(__builtin_memmove(p, p, n * sizeof(T))));
}
template <class T>
const T* start_lifetime_as_array(const void* p, size_t n) noexcept {
  return start_lifetime_as_array<T>(const_cast<void*>(p), n);
}
template <class T>
volatile T* start_lifetime_as_array(volatile void* p, size_t n) noexcept {
  return start_lifetime_as_array<T>(const_cast<void*>(p), n);
}
template <class T>
const volatile T* start_lifetime_as_array(const volatile void* p, size_t n) noexcept {
  return start_lifetime_as_array<T>(const_cast<void*>(p), n);
}

// [specialized.construct], [specialized.destroy]
template <class T, class... Args>
  requires requires(void* p, Args&&... args) { ::new (p) T(static_cast<Args&&>(args)...); }
constexpr T* construct_at(T* location, Args&&... args) noexcept(noexcept(::new(static_cast<void*>(location))
                                                                              T(static_cast<Args&&>(args)...))) {
  if constexpr (is_array_v<T>) {
    static_assert(sizeof...(Args) == 0, "std::construct_at: arrays take no arguments");
    return ::new (static_cast<void*>(location)) T[1]();
  } else {
    return ::new (static_cast<void*>(location)) T(static_cast<Args&&>(args)...);
  }
}

template <class T>
constexpr void destroy_at(T* location) noexcept {
  if constexpr (is_array_v<T>) {
    for (auto& e : *location)
      std::destroy_at(__builtin_addressof(e));
  } else {
    location->~T();
  }
}

template <class ForwardIt>
constexpr void destroy(ForwardIt first, ForwardIt last) {
  for (; first != last; ++first)
    std::destroy_at(__builtin_addressof(*first));
}
template <class ForwardIt, class Size>
constexpr ForwardIt destroy_n(ForwardIt first, Size n) {
  for (; n > 0; (void)++first, --n)
    std::destroy_at(__builtin_addressof(*first));
  return first;
}

// [allocator.tag]
struct allocator_arg_t {
  explicit allocator_arg_t() = default;
};
inline constexpr allocator_arg_t allocator_arg{};

// [allocator.uses.trait]
template <class T, class Alloc>
struct uses_allocator : false_type {};
template <class T, class Alloc>
  requires requires { typename T::allocator_type; } && is_convertible_v<Alloc, typename T::allocator_type>
struct uses_allocator<T, Alloc> : true_type {};
template <class T, class Alloc>
constexpr bool uses_allocator_v = uses_allocator<T, Alloc>::value;

// [allocation.result]
template <class Pointer, class SizeType = size_t>
struct allocation_result {
  Pointer ptr;
  SizeType count;
};

// [default.allocator]
template <class T>
class allocator {
  static_assert(!is_const_v<T> && !is_volatile_v<T> && !is_reference_v<T> && !is_function_v<T>,
                "std::allocator<T>: T must be a cv-unqualified object type");

  static constexpr bool overaligned = alignof(T) > __STDCPP_DEFAULT_NEW_ALIGNMENT__;

public:
  using value_type = T;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using propagate_on_container_move_assignment = true_type;

  constexpr allocator() noexcept = default;
  constexpr allocator(const allocator&) noexcept = default;
  template <class U>
  constexpr allocator(const allocator<U>&) noexcept {}
  constexpr ~allocator() = default;
  constexpr allocator& operator=(const allocator&) = default;

  [[nodiscard]] constexpr T* allocate(size_t n) {
    static_assert(sizeof(T) != 0, "std::allocator: incomplete type");
    if (n > static_cast<size_t>(-1) / sizeof(T)) {
      if consteval {
        ycxx::detail::assertion_failed("std::allocator::allocate: size overflow");
      }
      ycxx::detail::throw_bad_array_new_length();
    }
    if consteval {
      return static_cast<T*>(::operator new(n * sizeof(T)));
    } else {
      if constexpr (overaligned)
        return static_cast<T*>(__builtin_operator_new(n * sizeof(T), static_cast<align_val_t>(alignof(T))));
      else
        return static_cast<T*>(__builtin_operator_new(n * sizeof(T)));
    }
  }

  [[nodiscard]] constexpr allocation_result<T*> allocate_at_least(size_t n) { return {allocate(n), n}; }

  constexpr void deallocate(T* p, size_t n) {
    if consteval {
      ::operator delete(p);
    } else {
      if constexpr (overaligned)
        __builtin_operator_delete(p, n * sizeof(T), static_cast<align_val_t>(alignof(T)));
      else
        __builtin_operator_delete(p, n * sizeof(T));
    }
  }

  template <class U>
  friend constexpr bool operator==(const allocator&, const allocator<U>&) noexcept {
    return true;
  }
};

} // namespace std

// ---------------------------------------------------------------------------------------------
// [allocator.traits]
// ---------------------------------------------------------------------------------------------
namespace ycxx::detail {

template <class A, class Default>
struct alloc_pointer {
  using type = Default;
};
template <class A, class Default>
  requires requires { typename A::pointer; }
struct alloc_pointer<A, Default> {
  using type = typename A::pointer;
};

template <class A, class Ptr, class Default>
struct alloc_const_pointer {
  using type = typename std::pointer_traits<Ptr>::template rebind<const Default>;
};
template <class A, class Ptr, class Default>
  requires requires { typename A::const_pointer; }
struct alloc_const_pointer<A, Ptr, Default> {
  using type = typename A::const_pointer;
};

template <class A, class Ptr>
struct alloc_void_pointer {
  using type = typename std::pointer_traits<Ptr>::template rebind<void>;
};
template <class A, class Ptr>
  requires requires { typename A::void_pointer; }
struct alloc_void_pointer<A, Ptr> {
  using type = typename A::void_pointer;
};

template <class A, class Ptr>
struct alloc_const_void_pointer {
  using type = typename std::pointer_traits<Ptr>::template rebind<const void>;
};
template <class A, class Ptr>
  requires requires { typename A::const_void_pointer; }
struct alloc_const_void_pointer<A, Ptr> {
  using type = typename A::const_void_pointer;
};

template <class A, class Ptr>
struct alloc_difference {
  using type = typename std::pointer_traits<Ptr>::difference_type;
};
template <class A, class Ptr>
  requires requires { typename A::difference_type; }
struct alloc_difference<A, Ptr> {
  using type = typename A::difference_type;
};

template <class A, class Diff>
struct alloc_size {
  using type = std::make_unsigned_t<Diff>;
};
template <class A, class Diff>
  requires requires { typename A::size_type; }
struct alloc_size<A, Diff> {
  using type = typename A::size_type;
};

template <class A>
struct alloc_pocca {
  using type = std::false_type;
};
template <class A>
  requires requires { typename A::propagate_on_container_copy_assignment; }
struct alloc_pocca<A> {
  using type = typename A::propagate_on_container_copy_assignment;
};
template <class A>
struct alloc_pocma {
  using type = std::false_type;
};
template <class A>
  requires requires { typename A::propagate_on_container_move_assignment; }
struct alloc_pocma<A> {
  using type = typename A::propagate_on_container_move_assignment;
};
template <class A>
struct alloc_pocs {
  using type = std::false_type;
};
template <class A>
  requires requires { typename A::propagate_on_container_swap; }
struct alloc_pocs<A> {
  using type = typename A::propagate_on_container_swap;
};
template <class A>
struct alloc_always_equal {
  using type = std::bool_constant<__is_empty(A)>;
};
template <class A>
  requires requires { typename A::is_always_equal; }
struct alloc_always_equal<A> {
  using type = typename A::is_always_equal;
};

template <class A, class T>
struct alloc_rebind : rebind_first<A, T> {};
template <class A, class T>
  requires requires { typename A::template rebind<T>::other; }
struct alloc_rebind<A, T> {
  using type = typename A::template rebind<T>::other;
};

} // namespace ycxx::detail

namespace std {

template <class Alloc>
struct allocator_traits {
  using allocator_type = Alloc;
  using value_type = typename Alloc::value_type;
  using pointer = typename ycxx::detail::alloc_pointer<Alloc, value_type*>::type;
  using const_pointer = typename ycxx::detail::alloc_const_pointer<Alloc, pointer, value_type>::type;
  using void_pointer = typename ycxx::detail::alloc_void_pointer<Alloc, pointer>::type;
  using const_void_pointer = typename ycxx::detail::alloc_const_void_pointer<Alloc, pointer>::type;
  using difference_type = typename ycxx::detail::alloc_difference<Alloc, pointer>::type;
  using size_type = typename ycxx::detail::alloc_size<Alloc, difference_type>::type;
  using propagate_on_container_copy_assignment = typename ycxx::detail::alloc_pocca<Alloc>::type;
  using propagate_on_container_move_assignment = typename ycxx::detail::alloc_pocma<Alloc>::type;
  using propagate_on_container_swap = typename ycxx::detail::alloc_pocs<Alloc>::type;
  using is_always_equal = typename ycxx::detail::alloc_always_equal<Alloc>::type;

  template <class T>
  using rebind_alloc = typename ycxx::detail::alloc_rebind<Alloc, T>::type;
  template <class T>
  using rebind_traits = allocator_traits<rebind_alloc<T>>;

  [[nodiscard]] static constexpr pointer allocate(Alloc& a, size_type n) { return a.allocate(n); }
  [[nodiscard]] static constexpr pointer allocate(Alloc& a, size_type n, const_void_pointer hint) {
    if constexpr (requires { a.allocate(n, hint); })
      return a.allocate(n, hint);
    else
      return a.allocate(n);
  }
  [[nodiscard]] static constexpr allocation_result<pointer, size_type> allocate_at_least(Alloc& a, size_type n) {
    if constexpr (requires { a.allocate_at_least(n); })
      return a.allocate_at_least(n);
    else
      return {a.allocate(n), n};
  }
  static constexpr void deallocate(Alloc& a, pointer p, size_type n) { a.deallocate(p, n); }

  template <class T, class... Args>
  static constexpr void construct(Alloc& a, T* p, Args&&... args) {
    if constexpr (requires { a.construct(p, static_cast<Args&&>(args)...); })
      a.construct(p, static_cast<Args&&>(args)...);
    else
      std::construct_at(p, static_cast<Args&&>(args)...);
  }
  template <class T>
  static constexpr void destroy(Alloc& a, T* p) {
    if constexpr (requires { a.destroy(p); })
      a.destroy(p);
    else
      std::destroy_at(p);
  }
  static constexpr size_type max_size(const Alloc& a) noexcept {
    if constexpr (requires { a.max_size(); })
      return a.max_size();
    else
      return numeric_limits<size_type>::max() / sizeof(value_type);
  }
  static constexpr Alloc select_on_container_copy_construction(const Alloc& rhs) {
    if constexpr (requires { rhs.select_on_container_copy_construction(); })
      return rhs.select_on_container_copy_construction();
    else
      return rhs;
  }
};

} // namespace std
