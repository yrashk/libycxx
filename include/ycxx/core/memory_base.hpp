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
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// First template argument of a template specialization, and rebinding of it.
template <class _Tp>
struct __first_template_arg {};
template <template <class, class...> class _Tmpl, class _Tp, class... _Rest>
struct __first_template_arg<_Tmpl<_Tp, _Rest...>> {
  using type = _Tp;
};
template <class _Tp, class _Up>
struct __rebind_first {};
template <template <class, class...> class _Tmpl, class _Tp, class... _Rest, class _Up>
struct __rebind_first<_Tmpl<_Tp, _Rest...>, _Up> {
  using type = _Tmpl<_Up, _Rest...>;
};

template <class _Ptr>
struct __ptr_element : __first_template_arg<_Ptr> {};
template <class _Ptr>
  requires requires { typename _Ptr::element_type; }
struct __ptr_element<_Ptr> {
  using type = typename _Ptr::element_type;
};

template <class _Ptr>
struct __ptr_difference {
  using type = std::ptrdiff_t;
};
template <class _Ptr>
  requires requires { typename _Ptr::difference_type; }
struct __ptr_difference<_Ptr> {
  using type = typename _Ptr::difference_type;
};

template <class _Ptr, class _Up>
struct __ptr_rebind : __rebind_first<_Ptr, _Up> {};
template <class _Ptr, class _Up>
  requires requires { typename _Ptr::template rebind<_Up>; }
struct __ptr_rebind<_Ptr, _Up> {
  using type = typename _Ptr::template rebind<_Up>;
};

// pointer_traits<Ptr> members exist only if element_type can be determined ([pointer.traits.types]).
template <class _Ptr>
struct __pointer_traits_base {};
template <class _Ptr>
  requires requires { typename __ptr_element<_Ptr>::type; }
struct __pointer_traits_base<_Ptr> {
  using pointer = _Ptr;
  using element_type = typename __ptr_element<_Ptr>::type;
  using difference_type = typename __ptr_difference<_Ptr>::type;
  template <class _Up>
  using rebind = typename __ptr_rebind<_Ptr, _Up>::type;

  static constexpr pointer pointer_to(std::conditional_t<is_void_v<element_type>, struct __nat, element_type>& r)
    requires requires { _Ptr::pointer_to(r); }
  {
    return _Ptr::pointer_to(r);
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Ptr>
struct pointer_traits : __ycxx::__detail::__pointer_traits_base<_Ptr> {};

template <class _Tp>
struct pointer_traits<_Tp*> {
  using pointer = _Tp*;
  using element_type = _Tp;
  using difference_type = ptrdiff_t;
  template <class _Up>
  using rebind = _Up*;

  static constexpr pointer pointer_to(conditional_t<is_void_v<_Tp>, struct __ycxx_nat, _Tp>& r) noexcept
    requires(!is_void_v<_Tp>)
  {
    return __builtin_addressof(r);
  }
};

// [pointer.conversion]
template <class _Tp>
constexpr _Tp* to_address(_Tp* p) noexcept {
  static_assert(!is_function_v<_Tp>, "std::to_address: function pointer");
  return p;
}
// Constrained (the draft's deduced return type would make an invalid call a hard error).
template <class _Ptr>
  requires requires(const _Ptr& p) { pointer_traits<_Ptr>::to_address(p); } ||
           requires(const _Ptr& p) { p.operator->(); }
constexpr auto to_address(const _Ptr& p) noexcept {
  if constexpr (requires { pointer_traits<_Ptr>::to_address(p); })
    return pointer_traits<_Ptr>::to_address(p);
  else
    return std::to_address(p.operator->());
}

// [ptr.align]
inline void* align(size_t alignment, size_t size, void*& ptr, size_t& space) {
  auto p = reinterpret_cast<__UINTPTR_TYPE__>(ptr);
  auto __y_aligned = (p + alignment - 1) & ~static_cast<__UINTPTR_TYPE__>(alignment - 1);
  size_t __pad = static_cast<size_t>(__y_aligned - p);
  if (size > space || __pad > space - size)
    return nullptr;
  space -= __pad;
  return ptr = reinterpret_cast<void*>(__y_aligned);
}

template <size_t _Np, class _Tp>
[[nodiscard]] constexpr _Tp* assume_aligned(_Tp* ptr) {
  static_assert(_Np != 0 && (_Np & (_Np - 1)) == 0, "std::assume_aligned: N must be a power of two");
  if consteval {
    return ptr;
  } else {
    __ycxx::__detail::__precondition(reinterpret_cast<__UINTPTR_TYPE__>(ptr) % _Np == 0,
                               "std::assume_aligned: pointer is not suitably aligned");
    return static_cast<_Tp*>(__builtin_assume_aligned(ptr, _Np));
  }
}

template <size_t _Alignment, class _Tp>
[[nodiscard]] bool is_sufficiently_aligned(_Tp* ptr) {
  static_assert(_Alignment != 0 && (_Alignment & (_Alignment - 1)) == 0, "Alignment must be a power of two");
  return reinterpret_cast<__UINTPTR_TYPE__>(ptr) % _Alignment == 0;
}

// [obj.lifetime]
template <class _Tp>
constexpr void start_lifetime(_Tp& r) noexcept {
  static_assert(is_implicit_lifetime_v<_Tp> && is_aggregate_v<_Tp>,
                "std::start_lifetime: T must be an implicit-lifetime aggregate type");
  if consteval {
    // No builtin exists on either compiler; in constant evaluation a default-initializing
    // placement new begins the lifetime without initializing subobjects of trivial types. An
    // object already within its lifetime must be left alone ([obj.lifetime]/2).
    if constexpr (__ycxx::__detail::__y_builtin::__has_is_within_lifetime<_Tp>) {
      if (__builtin_is_within_lifetime(__builtin_addressof(r)))
        return;
    }
    ::new (static_cast<void*>(__builtin_addressof(r))) _Tp;
  }
  // At run time storage of an implicit-lifetime type needs no action.
}

template <class _Tp>
_Tp* start_lifetime_as(void* p) noexcept {
  static_assert(is_implicit_lifetime_v<_Tp>, "std::start_lifetime_as: T must be an implicit-lifetime type");
  // Implicit object creation: memmove onto itself is specified to create objects ([intro.object]).
  return std::launder(static_cast<_Tp*>(__builtin_memmove(p, p, sizeof(_Tp))));
}
template <class _Tp>
const _Tp* start_lifetime_as(const void* p) noexcept {
  return start_lifetime_as<_Tp>(const_cast<void*>(p));
}
template <class _Tp>
volatile _Tp* start_lifetime_as(volatile void* p) noexcept {
  return start_lifetime_as<_Tp>(const_cast<void*>(p));
}
template <class _Tp>
const volatile _Tp* start_lifetime_as(const volatile void* p) noexcept {
  return start_lifetime_as<_Tp>(const_cast<void*>(p));
}
template <class _Tp>
_Tp* start_lifetime_as_array(void* p, size_t n) noexcept {
  if (n == 0)
    return static_cast<_Tp*>(p);
  return std::launder(static_cast<_Tp*>(__builtin_memmove(p, p, n * sizeof(_Tp))));
}
template <class _Tp>
const _Tp* start_lifetime_as_array(const void* p, size_t n) noexcept {
  return start_lifetime_as_array<_Tp>(const_cast<void*>(p), n);
}
template <class _Tp>
volatile _Tp* start_lifetime_as_array(volatile void* p, size_t n) noexcept {
  return start_lifetime_as_array<_Tp>(const_cast<void*>(p), n);
}
template <class _Tp>
const volatile _Tp* start_lifetime_as_array(const volatile void* p, size_t n) noexcept {
  return start_lifetime_as_array<_Tp>(const_cast<void*>(p), n);
}

// [specialized.construct], [specialized.destroy]
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
template <class _Tp, class... _Args>
concept __construct_at_ok =
    !std::is_unbounded_array_v<_Tp> && requires(void* p, _Args&&... __args) { ::new (p) _Tp(static_cast<_Args&&>(__args)...); };
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp, class... _Args>
  requires __ycxx::__detail::__construct_at_ok<_Tp, _Args...>
constexpr _Tp* construct_at(_Tp* location, _Args&&... __args) noexcept(noexcept(::new(static_cast<void*>(location))
                                                                              _Tp(static_cast<_Args&&>(__args)...))) {
  if constexpr (is_array_v<_Tp>) {
    static_assert(sizeof...(_Args) == 0, "std::construct_at: arrays take no arguments");
    // [specialized.construct]/3 specifies `::new (__voidify(*location)) _Tp[1]()`. Value-initializing
    // the array object T itself has the same effect, and Clang's constant evaluator rejects the
    // T[1] form ("would change type of storage").
    ::new (static_cast<void*>(location)) _Tp();
    return std::launder(location);
  } else {
    return ::new (static_cast<void*>(location)) _Tp(static_cast<_Args&&>(__args)...);
  }
}

template <class _Tp>
constexpr void destroy_at(_Tp* location) noexcept {
  if constexpr (is_array_v<_Tp>) {
    for (auto& e : *location)
      std::destroy_at(__builtin_addressof(e));
  } else {
    location->~_Tp();
  }
}

template <class _ForwardIt>
constexpr void destroy(_ForwardIt first, _ForwardIt last) {
  for (; first != last; ++first)
    std::destroy_at(__builtin_addressof(*first));
}
template <class _ForwardIt, class _Size>
constexpr _ForwardIt destroy_n(_ForwardIt first, _Size n) {
  for (; n > 0; (void)++first, --n)
    std::destroy_at(__builtin_addressof(*first));
  return first;
}

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
namespace __construct_at_ns {
struct __fn {
  template <class _Tp, class... _Args>
    requires __construct_at_ok<_Tp, _Args...>
  static constexpr _Tp* operator()(_Tp* location, _Args&&... __args) noexcept(
      noexcept(std::construct_at(location, static_cast<_Args&&>(__args)...))) {
    return std::construct_at(location, static_cast<_Args&&>(__args)...);
  }
};
} // namespace construct_at_ns
namespace __destroy_at_ns {
struct __fn {
  template <class _Tp>
    requires std::is_nothrow_destructible_v<_Tp> // destructible<T>
  static constexpr void operator()(_Tp* location) noexcept {
    std::destroy_at(location);
  }
};
} // namespace destroy_at_ns
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline constexpr __ycxx::__detail::__construct_at_ns::__fn construct_at{};
inline constexpr __ycxx::__detail::__destroy_at_ns::__fn destroy_at{};
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] std {

// [allocator.tag]
struct allocator_arg_t {
  explicit allocator_arg_t() = default;
};
inline constexpr allocator_arg_t allocator_arg{};

// [allocator.uses.trait]
template <class _Tp, class _Alloc>
struct uses_allocator : false_type {};
template <class _Tp, class _Alloc>
  requires requires { typename _Tp::allocator_type; } && is_convertible_v<_Alloc, typename _Tp::allocator_type>
struct uses_allocator<_Tp, _Alloc> : true_type {};
template <class _Tp, class _Alloc>
constexpr bool uses_allocator_v = uses_allocator<_Tp, _Alloc>::value;

// [allocation.result]
template <class _Pointer, class _SizeType = size_t>
struct allocation_result {
  _Pointer ptr;
  _SizeType count;
};

// [default.allocator]
template <class _Tp>
class allocator {
  static_assert(!is_const_v<_Tp> && !is_volatile_v<_Tp> && !is_reference_v<_Tp> && !is_function_v<_Tp>,
                "std::allocator<T>: T must be a cv-unqualified object type");

  static constexpr bool __overaligned = alignof(_Tp) > __STDCPP_DEFAULT_NEW_ALIGNMENT__;

public:
  using value_type = _Tp;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using propagate_on_container_move_assignment = true_type;

  constexpr allocator() noexcept = default;
  constexpr allocator(const allocator&) noexcept = default;
  template <class _Up>
  constexpr allocator(const allocator<_Up>&) noexcept {}
  constexpr ~allocator() = default;
  constexpr allocator& operator=(const allocator&) = default;

  [[nodiscard]] constexpr _Tp* allocate(size_t n) {
    static_assert(sizeof(_Tp) != 0, "std::allocator: incomplete type");
    if (n > static_cast<size_t>(-1) / sizeof(_Tp))
      __ycxx::__detail::__throw_bad_array_new_length(); // constexpr: throws in constant evaluation too
    if consteval {
      return static_cast<_Tp*>(::operator new(n * sizeof(_Tp)));
    } else {
      if constexpr (__overaligned)
        return static_cast<_Tp*>(__builtin_operator_new(n * sizeof(_Tp), static_cast<align_val_t>(alignof(_Tp))));
      else
        return static_cast<_Tp*>(__builtin_operator_new(n * sizeof(_Tp)));
    }
  }

  [[nodiscard]] constexpr allocation_result<_Tp*> allocate_at_least(size_t n) { return {allocate(n), n}; }

  constexpr void deallocate(_Tp* p, size_t n) {
    if consteval {
      ::operator delete(p);
    } else {
      if constexpr (__overaligned)
        __builtin_operator_delete(p, n * sizeof(_Tp), static_cast<align_val_t>(alignof(_Tp)));
      else
        __builtin_operator_delete(p, n * sizeof(_Tp));
    }
  }

  template <class _Up>
  friend constexpr bool operator==(const allocator&, const allocator<_Up>&) noexcept {
    return true;
  }
};

} // namespace std

// ---------------------------------------------------------------------------------------------
// [allocator.traits]
// ---------------------------------------------------------------------------------------------
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Ap, class _Default>
struct __alloc_pointer {
  using type = _Default;
};
template <class _Ap, class _Default>
  requires requires { typename _Ap::pointer; }
struct __alloc_pointer<_Ap, _Default> {
  using type = typename _Ap::pointer;
};

template <class _Ap, class _Ptr, class _Default>
struct __alloc_const_pointer {
  using type = typename std::pointer_traits<_Ptr>::template rebind<const _Default>;
};
template <class _Ap, class _Ptr, class _Default>
  requires requires { typename _Ap::const_pointer; }
struct __alloc_const_pointer<_Ap, _Ptr, _Default> {
  using type = typename _Ap::const_pointer;
};

template <class _Ap, class _Ptr>
struct __alloc_void_pointer {
  using type = typename std::pointer_traits<_Ptr>::template rebind<void>;
};
template <class _Ap, class _Ptr>
  requires requires { typename _Ap::void_pointer; }
struct __alloc_void_pointer<_Ap, _Ptr> {
  using type = typename _Ap::void_pointer;
};

template <class _Ap, class _Ptr>
struct __alloc_const_void_pointer {
  using type = typename std::pointer_traits<_Ptr>::template rebind<const void>;
};
template <class _Ap, class _Ptr>
  requires requires { typename _Ap::const_void_pointer; }
struct __alloc_const_void_pointer<_Ap, _Ptr> {
  using type = typename _Ap::const_void_pointer;
};

template <class _Ap, class _Ptr>
struct __alloc_difference {
  using type = typename std::pointer_traits<_Ptr>::difference_type;
};
template <class _Ap, class _Ptr>
  requires requires { typename _Ap::difference_type; }
struct __alloc_difference<_Ap, _Ptr> {
  using type = typename _Ap::difference_type;
};

template <class _Ap, class _Diff>
struct __y_alloc_size {
  using type = std::make_unsigned_t<_Diff>;
};
template <class _Ap, class _Diff>
  requires requires { typename _Ap::size_type; }
struct __y_alloc_size<_Ap, _Diff> {
  using type = typename _Ap::size_type;
};

template <class _Ap>
struct __alloc_pocca {
  using type = std::false_type;
};
template <class _Ap>
  requires requires { typename _Ap::propagate_on_container_copy_assignment; }
struct __alloc_pocca<_Ap> {
  using type = typename _Ap::propagate_on_container_copy_assignment;
};
template <class _Ap>
struct __alloc_pocma {
  using type = std::false_type;
};
template <class _Ap>
  requires requires { typename _Ap::propagate_on_container_move_assignment; }
struct __alloc_pocma<_Ap> {
  using type = typename _Ap::propagate_on_container_move_assignment;
};
template <class _Ap>
struct __alloc_pocs {
  using type = std::false_type;
};
template <class _Ap>
  requires requires { typename _Ap::propagate_on_container_swap; }
struct __alloc_pocs<_Ap> {
  using type = typename _Ap::propagate_on_container_swap;
};
template <class _Ap>
struct __alloc_always_equal {
  using type = std::bool_constant<__is_empty(_Ap)>;
};
template <class _Ap>
  requires requires { typename _Ap::is_always_equal; }
struct __alloc_always_equal<_Ap> {
  using type = typename _Ap::is_always_equal;
};

template <class _Ap, class _Tp>
struct __alloc_rebind : __rebind_first<_Ap, _Tp> {};
template <class _Ap, class _Tp>
  requires requires { typename _Ap::template rebind<_Tp>::other; }
struct __alloc_rebind<_Ap, _Tp> {
  using type = typename _Ap::template rebind<_Tp>::other;
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Alloc>
struct allocator_traits {
  using allocator_type = _Alloc;
  using value_type = typename _Alloc::value_type;
  using pointer = typename __ycxx::__detail::__alloc_pointer<_Alloc, value_type*>::type;
  using const_pointer = typename __ycxx::__detail::__alloc_const_pointer<_Alloc, pointer, value_type>::type;
  using void_pointer = typename __ycxx::__detail::__alloc_void_pointer<_Alloc, pointer>::type;
  using const_void_pointer = typename __ycxx::__detail::__alloc_const_void_pointer<_Alloc, pointer>::type;
  using difference_type = typename __ycxx::__detail::__alloc_difference<_Alloc, pointer>::type;
  using size_type = typename __ycxx::__detail::__y_alloc_size<_Alloc, difference_type>::type;
  using propagate_on_container_copy_assignment = typename __ycxx::__detail::__alloc_pocca<_Alloc>::type;
  using propagate_on_container_move_assignment = typename __ycxx::__detail::__alloc_pocma<_Alloc>::type;
  using propagate_on_container_swap = typename __ycxx::__detail::__alloc_pocs<_Alloc>::type;
  using is_always_equal = typename __ycxx::__detail::__alloc_always_equal<_Alloc>::type;

  template <class _Tp>
  using rebind_alloc = typename __ycxx::__detail::__alloc_rebind<_Alloc, _Tp>::type;
  template <class _Tp>
  using rebind_traits = allocator_traits<rebind_alloc<_Tp>>;

  [[nodiscard]] static constexpr pointer allocate(_Alloc& a, size_type n) { return a.allocate(n); }
  [[nodiscard]] static constexpr pointer allocate(_Alloc& a, size_type n, const_void_pointer __hint) {
    if constexpr (requires { a.allocate(n, __hint); })
      return a.allocate(n, __hint);
    else
      return a.allocate(n);
  }
  [[nodiscard]] static constexpr allocation_result<pointer, size_type> allocate_at_least(_Alloc& a, size_type n) {
    if constexpr (requires { a.allocate_at_least(n); })
      return a.allocate_at_least(n);
    else
      return {a.allocate(n), n};
  }
  static constexpr void deallocate(_Alloc& a, pointer p, size_type n) { a.deallocate(p, n); }

  template <class _Tp, class... _Args>
  static constexpr void construct(_Alloc& a, _Tp* p, _Args&&... __args) {
    if constexpr (requires { a.construct(p, static_cast<_Args&&>(__args)...); })
      a.construct(p, static_cast<_Args&&>(__args)...);
    else
      std::construct_at(p, static_cast<_Args&&>(__args)...);
  }
  template <class _Tp>
  static constexpr void destroy(_Alloc& a, _Tp* p) {
    if constexpr (requires { a.destroy(p); })
      a.destroy(p);
    else
      std::destroy_at(p);
  }
  static constexpr size_type max_size(const _Alloc& a) noexcept {
    if constexpr (requires { a.max_size(); })
      return a.max_size();
    else
      return numeric_limits<size_type>::max() / sizeof(value_type);
  }
  static constexpr _Alloc select_on_container_copy_construction(const _Alloc& __rhs) {
    if constexpr (requires { __rhs.select_on_container_copy_construction(); })
      return __rhs.select_on_container_copy_construction();
    else
      return __rhs;
  }
};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// Whether allocator_traits<A>::construct(a, p, args...) cannot throw: a container may then make
// room first and construct afterwards, with no rollback of the room it made.
template <class _Alloc, class _Tp, class... _Args>
inline constexpr bool __alloc_nothrow_construct = std::is_nothrow_constructible_v<_Tp, _Args...>;
template <class _Alloc, class _Tp, class... _Args>
  requires requires(_Alloc& __a, _Tp* __p, _Args&&... __args) { __a.construct(__p, static_cast<_Args&&>(__args)...); }
inline constexpr bool __alloc_nothrow_construct<_Alloc, _Tp, _Args...> =
    noexcept(std::declval<_Alloc&>().construct(static_cast<_Tp*>(nullptr), std::declval<_Args>()...));
}} // namespace __ycxx::__detail
