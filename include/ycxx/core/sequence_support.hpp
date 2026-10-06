// libycxx core: helpers shared by the containers: iterator-pair classification and distance,
// the from_range_t tag check, in-place element rotation and temporaries built through the
// allocator (vector, inplace_vector, deque); iter-value-type, conversion of a raw pointer back
// to an allocator's pointer, a rollback guard, the stable merge and merge sort of singly linked
// node chains (list::sort/merge, forward_list::sort/merge), and the "ranges::to<Container>"
// construction the adaptors use.
#pragma once

#include <ycxx/core/concepts.hpp>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/memory_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// The first parameter of the from_range_t constructors is a template parameter checked by
// this concept first: otherwise overload resolution for an unrelated call (vector(it, it))
// would check container-compatible-range on the other argument, which may perform ADL on a
// type that must not be completed.
template <class _Tag>
concept __from_range_tag = std::is_same_v<_Tag, std::from_range_t>;

// A Cpp17ForwardIterator by category, or a C++20 forward iterator: may be traversed twice.
// The category is tested first: checking the C++20 concepts performs ADL for operator
// expressions on the iterator, which must not be needed for a plain Cpp17 iterator.
template <class _It>
concept __multipass_iterator =
    std::is_convertible_v<typename std::iterator_traits<_It>::iterator_category, std::forward_iterator_tag> ||
    std::forward_iterator<_It>;

// An iterator pair whose distance is last - first: random access by category, or (checked
// only otherwise) a sized sentinel.
template <class _It>
concept __subtractable_iterator =
    std::is_convertible_v<typename std::iterator_traits<_It>::iterator_category, std::random_access_iterator_tag> ||
    std::sized_sentinel_for<_It, _It>;

// distance(first, last) for an iterator pair (which need not model sentinel_for).
template <class _It>
constexpr auto __iter_pair_distance(_It first, _It last) {
  if constexpr (__subtractable_iterator<_It>) {
    return last - first;
  } else {
    typename std::iterator_traits<_It>::difference_type n = 0;
    for (; first != last; ++first)
      ++n;
    return n;
  }
}

// The address of the element an iterator refers to, for pointers and the library's own
// contiguous iterators (other iterator types are never inspected with the C++20 concepts, so
// iterators over incomplete-class pointers stay usable).
template <class _It>
inline constexpr bool __is_plain_contiguous = std::is_pointer_v<_It>;
template <class _Tp, class _Owner, class _Diff>
inline constexpr bool __is_plain_contiguous<__ycxx::__adl_free::__contiguous_iter<_Tp, _Owner, _Diff>> = true;
template <class _It>
constexpr auto __plain_address(const _It& __it) noexcept {
  if constexpr (std::is_pointer_v<_It>)
    return __it;
  else
    return __it.base();
}

// One element constructed through an allocator outside the container's storage: the copy of
// an argument that may refer to an element which is about to be moved.
template <class _Tp, class _Ap>
struct __alloc_temp {
  _Ap& a;
  union {
    _Tp __v;
  };
  template <class... _Args>
  constexpr explicit __alloc_temp(_Ap& __al, _Args&&... __args) : a(__al) {
    std::allocator_traits<_Ap>::construct(a, __builtin_addressof(__v), static_cast<_Args&&>(__args)...);
  }
  __alloc_temp(const __alloc_temp&) = delete;
  __alloc_temp& operator=(const __alloc_temp&) = delete;
  constexpr ~__alloc_temp() { std::allocator_traits<_Ap>::destroy(a, __builtin_addressof(__v)); }
};

// A temporary element for rotate_elements in containers without an allocator.
template <class _Tp>
struct __plain_temp {
  _Tp __v;
  constexpr explicit __plain_temp(_Tp&& __x) : __v(static_cast<_Tp&&>(__x)) {}
};

// Rotates [f, l) left so that *m becomes the first element, with move assignments and one
// temporary per cycle (Temp(ctx..., T&&), holding the element in .v). No unqualified calls.
// It is a pointer or a random-access iterator into the container's own elements.
template <class _Temp, class _It, class... _Ctx>
constexpr void __rotate_elements(_It __f, _It m, _It __l, _Ctx&... __ctx) {
  using _Tp = std::remove_reference_t<decltype(*__f)>;
  const std::ptrdiff_t n = __l - __f;
  const std::ptrdiff_t k = m - __f;
  if (k == 0 || k == n)
    return;
  std::ptrdiff_t __cycles = n;
  for (std::ptrdiff_t b = k; b != 0;) { // gcd(n, k) cycles
    const std::ptrdiff_t r = __cycles % b;
    __cycles = b;
    b = r;
  }
  for (std::ptrdiff_t i = 0; i != __cycles; ++i) {
    _Temp __tmp(__ctx..., static_cast<_Tp&&>(__f[i]));
    std::ptrdiff_t __j = i;
    for (;;) {
      std::ptrdiff_t next = __j + k;
      if (next >= n)
        next -= n;
      if (next == i)
        break;
      __f[__j] = static_cast<_Tp&&>(__f[next]);
      __j = next;
    }
    __f[__j] = static_cast<_Tp&&>(__tmp.__v);
  }
}

// ---- node-based containers and adaptors ----

// [sequences.general] iter-value-type
template <class _Ip>
using __iter_value_type = typename std::iterator_traits<_Ip>::value_type;

// The allocator pointer for p, a raw pointer into storage the allocator returned.
template <class _Pointer, class _Tp>
constexpr _Pointer __to_alloc_pointer(_Tp* p) noexcept {
  if constexpr (std::is_same_v<_Pointer, _Tp*>)
    return p;
  else
    return std::pointer_traits<_Pointer>::pointer_to(*p);
}

// The allocator-derived types and traits a container names at class scope. For a type that does
// not qualify as an allocator they are placeholders: class template argument deduction may
// form a container specialization with such a type while it considers the implicit deduction
// guides of the constructors (whose parameters name size_type), and that instantiation must
// not be a hard error ([sequence.reqmts]/69.3).
template <class _Ap>
struct __alloc_info {
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = void*;
  using const_pointer = const void*;
  template <class _Up>
  using rebind = _Ap;
  static constexpr bool __pocca = false, __pocma = false, __pocs = false, __always_equal = false;
};
template <class _Ap>
  requires __qualifies_as_allocator<_Ap>
struct __alloc_info<_Ap> {
  using __traits = std::allocator_traits<_Ap>;
  using size_type = typename __traits::size_type;
  using difference_type = typename __traits::difference_type;
  using pointer = typename __traits::pointer;
  using const_pointer = typename __traits::const_pointer;
  template <class _Up>
  using rebind = typename __traits::template rebind_alloc<_Up>;
  static constexpr bool __pocca = __traits::propagate_on_container_copy_assignment::value;
  static constexpr bool __pocma = __traits::propagate_on_container_move_assignment::value;
  static constexpr bool __pocs = __traits::propagate_on_container_swap::value;
  static constexpr bool __always_equal = __traits::is_always_equal::value;
};

// [container.alloc.reqmts]/2 Mandates: allocator_type::value_type is the container's value_type
// (checked only for types that qualify as allocators, see alloc_info).
template <class _Ap, class _Tp>
concept __allocator_for = !__qualifies_as_allocator<_Ap> || std::is_same_v<typename _Ap::value_type, _Tp>;

// Runs f() on scope exit unless release() was called: rollback on an exception.
template <class _Fp>
struct __rollback {
  _Fp __f;
  bool __active = true;
  constexpr ~__rollback() {
    if (__active)
      __f();
  }
  constexpr void release() noexcept { __active = false; }
};
template <class _Fp>
__rollback(_Fp) -> __rollback<_Fp>;

// ---- node chains ----
// A chain is a null-terminated sequence of nodes linked through `next` (a pointer to the node
// base type NB). The comparison is applied to the elements through `value(__nb)`.

// Merges the sorted chain b into the sorted chain a; a receives the result and b becomes empty.
// Stable: of equivalent elements, those of a come first. At most len(a) + len(b) - 1
// comparisons. If a comparison throws, a still receives every node of both chains (in an
// unspecified order), so nothing is lost.
template <class _NB, class _Value, class _Comp>
constexpr void __merge_chains(_NB*& a, _NB*& b, _Value& value, _Comp& comp) {
  struct state {
    _NB* __head;
    _NB** __tail;
    _NB* __x;
    _NB* y;
    _NB** out;
    constexpr state(_NB*& a, _NB*& b) : __head(nullptr), __tail(&__head), __x(a), y(b), out(&a) { b = nullptr; }
    state(const state&) = delete;
    constexpr ~state() {
      // Append what is left of x, then of y.
      *__tail = __x;
      if (y) {
        while (*__tail)
          __tail = &(*__tail)->next;
        *__tail = y;
      }
      *out = __head;
    }
  } s(a, b);
  while (s.__x && s.y) {
    _NB*& from = static_cast<bool>(comp(value(s.y), value(s.__x))) ? s.y : s.__x;
    _NB* n = from;
    from = n->next;
    *s.__tail = n;
    s.__tail = &n->next;
  }
}

// Stable bottom-up merge sort of a chain, without allocating: bins[i] holds a sorted chain of
// 2^i nodes (or none). About N log N comparisons. On exit, normal or by an exception from a
// comparison, finish(chain) receives a chain holding every node (sorted on normal exit).
template <class _NB, class _Value, class _Comp, class _Finish>
constexpr void __sort_chain(_NB* in, _Value& value, _Comp& comp, _Finish& finish) {
  struct state {
    _NB* in;
    _NB* __carry = nullptr;
    _NB* result = nullptr;
    _NB* __bins[64] = {};
    int fill = 0;
    _Finish* __fin;
    constexpr ~state() {
      _NB* __head = result;
      _NB** __tail = &__head;
      auto append = [&__tail](_NB* c) {
        while (*__tail)
          __tail = &(*__tail)->next;
        *__tail = c;
      };
      for (int i = 0; i < fill; ++i)
        if (__bins[i])
          append(__bins[i]);
      if (__carry)
        append(__carry);
      if (in)
        append(in);
      (*__fin)(__head);
    }
  } s{in};
  s.__fin = __builtin_addressof(finish);
  while (s.in) {
    s.__carry = s.in;
    s.in = s.in->next;
    s.__carry->next = nullptr;
    int i = 0;
    for (; i < s.fill && s.__bins[i]; ++i) {
      // bins[i] holds earlier elements than carry.
      ::__ycxx::__detail::__merge_chains(s.__bins[i], s.__carry, value, comp);
      s.__carry = s.__bins[i];
      s.__bins[i] = nullptr;
    }
    s.__bins[i] = s.__carry;
    s.__carry = nullptr;
    if (i == s.fill)
      ++s.fill;
  }
  // Lower bins hold later elements.
  for (int i = 0; i < s.fill; ++i) {
    if (s.__bins[i]) {
      ::__ycxx::__detail::__merge_chains(s.__bins[i], s.result, value, comp);
      s.result = s.__bins[i];
      s.__bins[i] = nullptr;
    }
  }
}

// ---- ranges::to<C>(r, args...) for the container adaptors ([range.utility.conv.to]) ----
template <class _Cp, class _Rp, class... _Args>
constexpr _Cp __range_to(_Rp&& r, _Args&&... __args) {
  if constexpr (std::constructible_from<_Cp, _Rp, _Args...>) {
    return _Cp(static_cast<_Rp&&>(r), static_cast<_Args&&>(__args)...);
  } else if constexpr (std::constructible_from<_Cp, std::from_range_t, _Rp, _Args...>) {
    return _Cp(std::from_range, static_cast<_Rp&&>(r), static_cast<_Args&&>(__args)...);
  } else if constexpr (std::ranges::common_range<_Rp> &&
                       ::__ycxx::__detail::__qualifies_as_input_iterator<std::ranges::iterator_t<_Rp>> &&
                       std::constructible_from<_Cp, std::ranges::iterator_t<_Rp>, std::ranges::iterator_t<_Rp>, _Args...>) {
    return _Cp(std::ranges::begin(r), std::ranges::end(r), static_cast<_Args&&>(__args)...);
  } else {
    _Cp c(static_cast<_Args&&>(__args)...);
    if constexpr (std::ranges::sized_range<_Rp> && requires(typename _Cp::size_type n) {
                    c.reserve(n);
                    { c.capacity() } -> std::same_as<typename _Cp::size_type>;
                    { c.max_size() } -> std::same_as<typename _Cp::size_type>;
                  })
      c.reserve(static_cast<typename _Cp::size_type>(std::ranges::size(r)));
    for (auto&& e : r) {
      if constexpr (requires { c.emplace_back(static_cast<decltype(e)&&>(e)); })
        c.emplace_back(static_cast<decltype(e)&&>(e));
      else if constexpr (requires { c.push_back(static_cast<decltype(e)&&>(e)); })
        c.push_back(static_cast<decltype(e)&&>(e));
      else if constexpr (requires { c.emplace(c.end(), static_cast<decltype(e)&&>(e)); })
        c.emplace(c.end(), static_cast<decltype(e)&&>(e));
      else
        c.insert(c.end(), static_cast<decltype(e)&&>(e));
    }
    return c;
  }
}

}} // namespace __ycxx::__detail
