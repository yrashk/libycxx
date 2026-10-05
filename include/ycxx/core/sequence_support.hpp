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

namespace ycxx::detail {

// The first parameter of the from_range_t constructors is a template parameter checked by
// this concept first: otherwise overload resolution for an unrelated call (vector(it, it))
// would check container-compatible-range on the other argument, which may perform ADL on a
// type that must not be completed.
template <class Tag>
concept from_range_tag = std::is_same_v<Tag, std::from_range_t>;

// A Cpp17ForwardIterator by category, or a C++20 forward iterator: may be traversed twice.
// The category is tested first: checking the C++20 concepts performs ADL for operator
// expressions on the iterator, which must not be needed for a plain Cpp17 iterator.
template <class It>
concept multipass_iterator =
    std::is_convertible_v<typename std::iterator_traits<It>::iterator_category, std::forward_iterator_tag> ||
    std::forward_iterator<It>;

// An iterator pair whose distance is last - first: random access by category, or (checked
// only otherwise) a sized sentinel.
template <class It>
concept subtractable_iterator =
    std::is_convertible_v<typename std::iterator_traits<It>::iterator_category, std::random_access_iterator_tag> ||
    std::sized_sentinel_for<It, It>;

// distance(first, last) for an iterator pair (which need not model sentinel_for).
template <class It>
constexpr auto iter_pair_distance(It first, It last) {
  if constexpr (subtractable_iterator<It>) {
    return last - first;
  } else {
    typename std::iterator_traits<It>::difference_type n = 0;
    for (; first != last; ++first)
      ++n;
    return n;
  }
}

// The address of the element an iterator refers to, for pointers and the library's own
// contiguous iterators (other iterator types are never inspected with the C++20 concepts, so
// iterators over incomplete-class pointers stay usable).
template <class It>
inline constexpr bool is_plain_contiguous = std::is_pointer_v<It>;
template <class T, class Owner, class Diff>
inline constexpr bool is_plain_contiguous<ycxx::adl_free::contiguous_iter<T, Owner, Diff>> = true;
template <class It>
constexpr auto plain_address(const It& it) noexcept {
  if constexpr (std::is_pointer_v<It>)
    return it;
  else
    return it.base();
}

// One element constructed through an allocator outside the container's storage: the copy of
// an argument that may refer to an element which is about to be moved.
template <class T, class A>
struct alloc_temp {
  A& a;
  union {
    T v;
  };
  template <class... Args>
  constexpr explicit alloc_temp(A& al, Args&&... args) : a(al) {
    std::allocator_traits<A>::construct(a, __builtin_addressof(v), static_cast<Args&&>(args)...);
  }
  alloc_temp(const alloc_temp&) = delete;
  alloc_temp& operator=(const alloc_temp&) = delete;
  constexpr ~alloc_temp() { std::allocator_traits<A>::destroy(a, __builtin_addressof(v)); }
};

// A temporary element for rotate_elements in containers without an allocator.
template <class T>
struct plain_temp {
  T v;
  constexpr explicit plain_temp(T&& x) : v(static_cast<T&&>(x)) {}
};

// Rotates [f, l) left so that *m becomes the first element, with move assignments and one
// temporary per cycle (Temp(ctx..., T&&), holding the element in .v). No unqualified calls.
// It is a pointer or a random-access iterator into the container's own elements.
template <class Temp, class It, class... Ctx>
constexpr void rotate_elements(It f, It m, It l, Ctx&... ctx) {
  using T = std::remove_reference_t<decltype(*f)>;
  const std::ptrdiff_t n = l - f;
  const std::ptrdiff_t k = m - f;
  if (k == 0 || k == n)
    return;
  std::ptrdiff_t cycles = n;
  for (std::ptrdiff_t b = k; b != 0;) { // gcd(n, k) cycles
    const std::ptrdiff_t r = cycles % b;
    cycles = b;
    b = r;
  }
  for (std::ptrdiff_t i = 0; i != cycles; ++i) {
    Temp tmp(ctx..., static_cast<T&&>(f[i]));
    std::ptrdiff_t j = i;
    for (;;) {
      std::ptrdiff_t next = j + k;
      if (next >= n)
        next -= n;
      if (next == i)
        break;
      f[j] = static_cast<T&&>(f[next]);
      j = next;
    }
    f[j] = static_cast<T&&>(tmp.v);
  }
}

// ---- node-based containers and adaptors ----

// [sequences.general] iter-value-type
template <class I>
using iter_value_type = typename std::iterator_traits<I>::value_type;

// The allocator pointer for p, a raw pointer into storage the allocator returned.
template <class Pointer, class T>
constexpr Pointer to_alloc_pointer(T* p) noexcept {
  if constexpr (std::is_same_v<Pointer, T*>)
    return p;
  else
    return std::pointer_traits<Pointer>::pointer_to(*p);
}

// The allocator-derived types and traits a container names at class scope. For a type that does
// not qualify as an allocator they are placeholders: class template argument deduction may
// form a container specialization with such a type while it considers the implicit deduction
// guides of the constructors (whose parameters name size_type), and that instantiation must
// not be a hard error ([sequence.reqmts]/69.3).
template <class A>
struct alloc_info {
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = void*;
  using const_pointer = const void*;
  template <class U>
  using rebind = A;
  static constexpr bool pocca = false, pocma = false, pocs = false, always_equal = false;
};
template <class A>
  requires qualifies_as_allocator<A>
struct alloc_info<A> {
  using traits = std::allocator_traits<A>;
  using size_type = typename traits::size_type;
  using difference_type = typename traits::difference_type;
  using pointer = typename traits::pointer;
  using const_pointer = typename traits::const_pointer;
  template <class U>
  using rebind = typename traits::template rebind_alloc<U>;
  static constexpr bool pocca = traits::propagate_on_container_copy_assignment::value;
  static constexpr bool pocma = traits::propagate_on_container_move_assignment::value;
  static constexpr bool pocs = traits::propagate_on_container_swap::value;
  static constexpr bool always_equal = traits::is_always_equal::value;
};

// [container.alloc.reqmts]/2 Mandates: allocator_type::value_type is the container's value_type
// (checked only for types that qualify as allocators, see alloc_info).
template <class A, class T>
concept allocator_for = !qualifies_as_allocator<A> || std::is_same_v<typename A::value_type, T>;

// Runs f() on scope exit unless release() was called: rollback on an exception.
template <class F>
struct rollback {
  F f;
  bool active = true;
  constexpr ~rollback() {
    if (active)
      f();
  }
  constexpr void release() noexcept { active = false; }
};
template <class F>
rollback(F) -> rollback<F>;

// ---- node chains ----
// A chain is a null-terminated sequence of nodes linked through `next` (a pointer to the node
// base type NB). The comparison is applied to the elements through `value(nb)`.

// Merges the sorted chain b into the sorted chain a; a receives the result and b becomes empty.
// Stable: of equivalent elements, those of a come first. At most len(a) + len(b) - 1
// comparisons. If a comparison throws, a still receives every node of both chains (in an
// unspecified order), so nothing is lost.
template <class NB, class Value, class Comp>
constexpr void merge_chains(NB*& a, NB*& b, Value& value, Comp& comp) {
  struct state {
    NB* head;
    NB** tail;
    NB* x;
    NB* y;
    NB** out;
    constexpr state(NB*& a, NB*& b) : head(nullptr), tail(&head), x(a), y(b), out(&a) { b = nullptr; }
    state(const state&) = delete;
    constexpr ~state() {
      // Append what is left of x, then of y.
      *tail = x;
      if (y) {
        while (*tail)
          tail = &(*tail)->next;
        *tail = y;
      }
      *out = head;
    }
  } s(a, b);
  while (s.x && s.y) {
    NB*& from = static_cast<bool>(comp(value(s.y), value(s.x))) ? s.y : s.x;
    NB* n = from;
    from = n->next;
    *s.tail = n;
    s.tail = &n->next;
  }
}

// Stable bottom-up merge sort of a chain, without allocating: bins[i] holds a sorted chain of
// 2^i nodes (or none). About N log N comparisons. On exit, normal or by an exception from a
// comparison, finish(chain) receives a chain holding every node (sorted on normal exit).
template <class NB, class Value, class Comp, class Finish>
constexpr void sort_chain(NB* in, Value& value, Comp& comp, Finish& finish) {
  struct state {
    NB* in;
    NB* carry = nullptr;
    NB* result = nullptr;
    NB* bins[64] = {};
    int fill = 0;
    Finish* fin;
    constexpr ~state() {
      NB* head = result;
      NB** tail = &head;
      auto append = [&tail](NB* c) {
        while (*tail)
          tail = &(*tail)->next;
        *tail = c;
      };
      for (int i = 0; i < fill; ++i)
        if (bins[i])
          append(bins[i]);
      if (carry)
        append(carry);
      if (in)
        append(in);
      (*fin)(head);
    }
  } s{in};
  s.fin = __builtin_addressof(finish);
  while (s.in) {
    s.carry = s.in;
    s.in = s.in->next;
    s.carry->next = nullptr;
    int i = 0;
    for (; i < s.fill && s.bins[i]; ++i) {
      // bins[i] holds earlier elements than carry.
      ::ycxx::detail::merge_chains(s.bins[i], s.carry, value, comp);
      s.carry = s.bins[i];
      s.bins[i] = nullptr;
    }
    s.bins[i] = s.carry;
    s.carry = nullptr;
    if (i == s.fill)
      ++s.fill;
  }
  // Lower bins hold later elements.
  for (int i = 0; i < s.fill; ++i) {
    if (s.bins[i]) {
      ::ycxx::detail::merge_chains(s.bins[i], s.result, value, comp);
      s.result = s.bins[i];
      s.bins[i] = nullptr;
    }
  }
}

// ---- ranges::to<C>(r, args...) for the container adaptors ([range.utility.conv.to]) ----
template <class C, class R, class... Args>
constexpr C range_to(R&& r, Args&&... args) {
  if constexpr (std::constructible_from<C, R, Args...>) {
    return C(static_cast<R&&>(r), static_cast<Args&&>(args)...);
  } else if constexpr (std::constructible_from<C, std::from_range_t, R, Args...>) {
    return C(std::from_range, static_cast<R&&>(r), static_cast<Args&&>(args)...);
  } else if constexpr (std::ranges::common_range<R> &&
                       ::ycxx::detail::qualifies_as_input_iterator<std::ranges::iterator_t<R>> &&
                       std::constructible_from<C, std::ranges::iterator_t<R>, std::ranges::iterator_t<R>, Args...>) {
    return C(std::ranges::begin(r), std::ranges::end(r), static_cast<Args&&>(args)...);
  } else {
    C c(static_cast<Args&&>(args)...);
    if constexpr (std::ranges::sized_range<R> && requires(typename C::size_type n) {
                    c.reserve(n);
                    { c.capacity() } -> std::same_as<typename C::size_type>;
                    { c.max_size() } -> std::same_as<typename C::size_type>;
                  })
      c.reserve(static_cast<typename C::size_type>(std::ranges::size(r)));
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

} // namespace ycxx::detail
