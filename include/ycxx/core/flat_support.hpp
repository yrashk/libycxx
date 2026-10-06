// libycxx core: the algorithms shared by flat_map, flat_multimap, flat_set and flat_multiset
// ([flat.map], [flat.set]): sorting, merging and deduplicating "rows" kept in one (flat_set) or
// two (flat_map: keys and values) parallel random-access sequence containers.
//
// Rows are ordered through a permutation of indices (stably sorted or merged by key), which is
// then applied to each container by following its cycles: every element moves at most once
// more than its final assignment, and the containers only need random-access iterators and
// move assignment. An already sorted input costs one comparison per element and no moves, so
// construction from sorted data is linear.
//
// flat_guard restores the adaptor invariants when an operation exits via an exception
// ([flat.map.overview]/6, [flat.set.overview]/6) by clearing the containers: once a container
// operation has thrown, nothing is known about the order of its elements (a sequence container
// gives only the basic guarantee for insertions in general), so the adaptor ends up empty.
#pragma once

#include <ycxx/core/algo_sort.hpp>
#include <ycxx/core/assoc_support.hpp>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/sorted_tags.hpp>
#include <ycxx/core/swap.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// The element at index i of a random-access container.
template <class _Cp>
constexpr decltype(auto) __row_at(_Cp& c, std::size_t i) {
  return *(c.begin() + static_cast<typename _Cp::difference_type>(i));
}

// A buffer of n indices, from std::allocator.
struct __index_buffer {
  std::size_t* p = nullptr;
  std::size_t n = 0;
  constexpr explicit __index_buffer(std::size_t count) : n(count) {
    if (count) {
      p = std::allocator<std::size_t>().allocate(count);
      for (std::size_t i = 0; i < count; ++i) // begins the elements' lifetimes (constant evaluation)
        std::construct_at(p + i, std::size_t(0));
    }
  }
  __index_buffer(const __index_buffer&) = delete;
  constexpr ~__index_buffer() {
    if (p)
      std::allocator<std::size_t>().deallocate(p, n);
  }
};

// Clears keys (and values, unless V is void) on scope exit, unless released.
template <class _Kp, class _Vp = void>
struct __flat_guard {
  _Kp* keys;
  _Vp* values = nullptr;
  bool __active = true;
  constexpr explicit __flat_guard(_Kp& __ks) noexcept : keys(__builtin_addressof(__ks)) {}
  template <class _Wp>
  constexpr __flat_guard(_Kp& __ks, _Wp& __vs) noexcept : keys(__builtin_addressof(__ks)), values(__builtin_addressof(__vs)) {}
  __flat_guard(const __flat_guard&) = delete;
  constexpr ~__flat_guard() {
    if (!__active)
      return;
    keys->clear();
    if constexpr (!std::is_void_v<_Vp>)
      values->clear();
  }
  constexpr void release() noexcept { __active = false; }
};
template <class _Kp>
__flat_guard(_Kp&) -> __flat_guard<_Kp>;
template <class _Kp, class _Vp>
__flat_guard(_Kp&, _Vp&) -> __flat_guard<_Kp, _Vp>;

template <class _Cp>
constexpr void __erase_from(_Cp& c, std::size_t i) {
  c.erase(c.begin() + static_cast<typename _Cp::difference_type>(i), c.end());
}
template <class _Cp>
constexpr void __move_row(_Cp& c, std::size_t to, std::size_t from) {
  using _Tp = typename _Cp::value_type;
  ::__ycxx::__detail::__row_at(c, to) = static_cast<_Tp&&>(::__ycxx::__detail::__row_at(c, from));
}

// Moves the rows of c into the order perm (position i receives the row at perm[i]), following
// the permutation's cycles with one temporary per cycle. work is scratch space for n indices.
template <class _Cp>
constexpr void permute(_Cp& c, const std::size_t* __perm, std::size_t* __work, std::size_t n) {
  using _Tp = typename _Cp::value_type;
  for (std::size_t i = 0; i < n; ++i)
    __work[i] = __perm[i];
  for (std::size_t i = 0; i < n; ++i) {
    if (__work[i] == i)
      continue;
    _Tp __tmp(static_cast<_Tp&&>(::__ycxx::__detail::__row_at(c, i)));
    std::size_t __j = i;
    for (;;) {
      const std::size_t k = __work[__j];
      __work[__j] = __j;
      if (k == i) {
        ::__ycxx::__detail::__row_at(c, __j) = static_cast<_Tp&&>(__tmp);
        break;
      }
      ::__ycxx::__detail::__row_at(c, __j) = static_cast<_Tp&&>(::__ycxx::__detail::__row_at(c, k));
      __j = k;
    }
  }
}

// The rows [0, lo) are sorted by key; sorts the rows [lo, size) stably and merges them in, so
// that all rows are sorted, stably (of equivalent keys, earlier rows first).
template <class _Less, class _Keys, class... _Others>
constexpr void __sort_rows(_Less& lt, std::size_t __lo, _Keys& keys, _Others&... __others) {
  const std::size_t n = keys.size();
  auto key = [&keys](std::size_t i) -> decltype(auto) { return ::__ycxx::__detail::__row_at(keys, i); };
  bool __tail_sorted = true;
  for (std::size_t i = __lo + 1; i < n && __tail_sorted; ++i)
    __tail_sorted = !lt(key(i), key(i - 1));
  // joined: the (sorted) new rows all belong after the old ones.
  bool __joined = __lo == 0 || __lo >= n;
  if (__tail_sorted && !__joined)
    __joined = !lt(key(__lo), key(__lo - 1));
  if (__tail_sorted && __joined)
    return;
  ::__ycxx::__detail::__index_buffer __order(n), __work(n);
  for (std::size_t i = 0; i < n; ++i)
    __order.p[i] = i;
  if (!__tail_sorted) {
    std::stable_sort(__order.p + __lo, __order.p + n,
                     [&](std::size_t a, std::size_t b) -> bool { return lt(key(a), key(b)); });
    if (!__joined)
      __joined = !lt(key(__order.p[__lo]), key(__lo - 1));
  }
  if (!__joined) {
    // Merge the identity [0, lo) with order[lo, n); old rows first among equivalent keys.
    std::size_t a = 0, b = __lo, out = 0;
    while (a < __lo && b < n) {
      if (lt(key(__order.p[b]), key(a)))
        __work.p[out++] = __order.p[b++];
      else
        __work.p[out++] = a++;
    }
    while (a < __lo)
      __work.p[out++] = a++;
    while (b < n)
      __work.p[out++] = __order.p[b++];
    for (std::size_t i = 0; i < n; ++i)
      __order.p[i] = __work.p[i];
  }
  ::__ycxx::__detail::permute(keys, __order.p, __work.p, n);
  (::__ycxx::__detail::permute(__others, __order.p, __work.p, n), ...);
}

// The rows are sorted; keeps the first row of each run of equivalent keys and erases the rest.
// One comparison per row.
template <class _Less, class _Keys, class... _Others>
constexpr void __unique_rows(_Less& lt, _Keys& keys, _Others&... __others) {
  const std::size_t n = keys.size();
  if (n < 2)
    return;
  std::size_t __w = 0;
  for (std::size_t r = 1; r < n; ++r) {
    if (lt(::__ycxx::__detail::__row_at(keys, __w), ::__ycxx::__detail::__row_at(keys, r))) {
      ++__w;
      if (__w != r) {
        ::__ycxx::__detail::__move_row(keys, __w, r);
        (::__ycxx::__detail::__move_row(__others, __w, r), ...);
      }
    }
  }
  if (__w + 1 < n) {
    ::__ycxx::__detail::__erase_from(keys, __w + 1);
    (::__ycxx::__detail::__erase_from(__others, __w + 1), ...);
  }
}

// Removes the rows for which pred(index) holds, keeping the order of the others; pred is applied
// exactly once per row ([flat.map.erasure], [flat.set.erasure]). Returns the number removed.
template <class _Pred, class _Keys, class... _Others>
constexpr std::size_t __erase_rows_if(_Pred& pred, _Keys& keys, _Others&... __others) {
  const std::size_t n = keys.size();
  std::size_t __w = 0;
  for (std::size_t r = 0; r < n; ++r) {
    if (!pred(r)) {
      if (__w != r) {
        ::__ycxx::__detail::__move_row(keys, __w, r);
        (::__ycxx::__detail::__move_row(__others, __w, r), ...);
      }
      ++__w;
    }
  }
  if (__w < n) {
    ::__ycxx::__detail::__erase_from(keys, __w);
    (::__ycxx::__detail::__erase_from(__others, __w), ...);
  }
  return n - __w;
}

// The deduction-guide constraints of [container.adaptors.general]/6.
template <class _Compare, class _KeyContainer>
concept __flat_compare_for =
    !__qualifies_as_allocator<_Compare> && requires { typename _KeyContainer::value_type; } &&
    std::is_invocable_v<const _Compare&, const typename _KeyContainer::value_type&,
                        const typename _KeyContainer::value_type&>;
template <class _Cp>
concept __flat_container_arg = !__qualifies_as_allocator<_Cp>;

// [container.adaptors.general]/8 alloc-rebind
template <class _Allocator, class _Tp>
using __rebound_alloc = typename std::allocator_traits<_Allocator>::template rebind_alloc<_Tp>;

// The allocator-extended constructors ([flat.map.cons.alloc]/1, [flat.set.cons.alloc]/1). A
// namespace-scope concept rather than a private member: the constraints are also checked for
// the constructors' implicit deduction guides.
template <class _Alloc, class... _Cs>
concept __flat_alloc_for = (std::uses_allocator_v<_Cs, _Alloc> && ...);

}} // namespace __ycxx::__detail
