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

namespace ycxx::detail {

// The element at index i of a random-access container.
template <class C>
constexpr decltype(auto) row_at(C& c, std::size_t i) {
  return *(c.begin() + static_cast<typename C::difference_type>(i));
}

// A buffer of n indices, from std::allocator.
struct index_buffer {
  std::size_t* p = nullptr;
  std::size_t n = 0;
  constexpr explicit index_buffer(std::size_t count) : n(count) {
    if (count) {
      p = std::allocator<std::size_t>().allocate(count);
      for (std::size_t i = 0; i < count; ++i) // begins the elements' lifetimes (constant evaluation)
        std::construct_at(p + i, std::size_t(0));
    }
  }
  index_buffer(const index_buffer&) = delete;
  constexpr ~index_buffer() {
    if (p)
      std::allocator<std::size_t>().deallocate(p, n);
  }
};

// Clears keys (and values, unless V is void) on scope exit, unless released.
template <class K, class V = void>
struct flat_guard {
  K* keys;
  V* values = nullptr;
  bool active = true;
  constexpr explicit flat_guard(K& ks) noexcept : keys(__builtin_addressof(ks)) {}
  template <class W>
  constexpr flat_guard(K& ks, W& vs) noexcept : keys(__builtin_addressof(ks)), values(__builtin_addressof(vs)) {}
  flat_guard(const flat_guard&) = delete;
  constexpr ~flat_guard() {
    if (!active)
      return;
    keys->clear();
    if constexpr (!std::is_void_v<V>)
      values->clear();
  }
  constexpr void release() noexcept { active = false; }
};
template <class K>
flat_guard(K&) -> flat_guard<K>;
template <class K, class V>
flat_guard(K&, V&) -> flat_guard<K, V>;

template <class C>
constexpr void erase_from(C& c, std::size_t i) {
  c.erase(c.begin() + static_cast<typename C::difference_type>(i), c.end());
}
template <class C>
constexpr void move_row(C& c, std::size_t to, std::size_t from) {
  using T = typename C::value_type;
  ::ycxx::detail::row_at(c, to) = static_cast<T&&>(::ycxx::detail::row_at(c, from));
}

// Moves the rows of c into the order perm (position i receives the row at perm[i]), following
// the permutation's cycles with one temporary per cycle. work is scratch space for n indices.
template <class C>
constexpr void permute(C& c, const std::size_t* perm, std::size_t* work, std::size_t n) {
  using T = typename C::value_type;
  for (std::size_t i = 0; i < n; ++i)
    work[i] = perm[i];
  for (std::size_t i = 0; i < n; ++i) {
    if (work[i] == i)
      continue;
    T tmp(static_cast<T&&>(::ycxx::detail::row_at(c, i)));
    std::size_t j = i;
    for (;;) {
      const std::size_t k = work[j];
      work[j] = j;
      if (k == i) {
        ::ycxx::detail::row_at(c, j) = static_cast<T&&>(tmp);
        break;
      }
      ::ycxx::detail::row_at(c, j) = static_cast<T&&>(::ycxx::detail::row_at(c, k));
      j = k;
    }
  }
}

// The rows [0, lo) are sorted by key; sorts the rows [lo, size) stably and merges them in, so
// that all rows are sorted, stably (of equivalent keys, earlier rows first).
template <class Less, class Keys, class... Others>
constexpr void sort_rows(Less& lt, std::size_t lo, Keys& keys, Others&... others) {
  const std::size_t n = keys.size();
  auto key = [&keys](std::size_t i) -> decltype(auto) { return ::ycxx::detail::row_at(keys, i); };
  bool tail_sorted = true;
  for (std::size_t i = lo + 1; i < n && tail_sorted; ++i)
    tail_sorted = !lt(key(i), key(i - 1));
  // joined: the (sorted) new rows all belong after the old ones.
  bool joined = lo == 0 || lo >= n;
  if (tail_sorted && !joined)
    joined = !lt(key(lo), key(lo - 1));
  if (tail_sorted && joined)
    return;
  ::ycxx::detail::index_buffer order(n), work(n);
  for (std::size_t i = 0; i < n; ++i)
    order.p[i] = i;
  if (!tail_sorted) {
    std::stable_sort(order.p + lo, order.p + n,
                     [&](std::size_t a, std::size_t b) -> bool { return lt(key(a), key(b)); });
    if (!joined)
      joined = !lt(key(order.p[lo]), key(lo - 1));
  }
  if (!joined) {
    // Merge the identity [0, lo) with order[lo, n); old rows first among equivalent keys.
    std::size_t a = 0, b = lo, out = 0;
    while (a < lo && b < n) {
      if (lt(key(order.p[b]), key(a)))
        work.p[out++] = order.p[b++];
      else
        work.p[out++] = a++;
    }
    while (a < lo)
      work.p[out++] = a++;
    while (b < n)
      work.p[out++] = order.p[b++];
    for (std::size_t i = 0; i < n; ++i)
      order.p[i] = work.p[i];
  }
  ::ycxx::detail::permute(keys, order.p, work.p, n);
  (::ycxx::detail::permute(others, order.p, work.p, n), ...);
}

// The rows are sorted; keeps the first row of each run of equivalent keys and erases the rest.
// One comparison per row.
template <class Less, class Keys, class... Others>
constexpr void unique_rows(Less& lt, Keys& keys, Others&... others) {
  const std::size_t n = keys.size();
  if (n < 2)
    return;
  std::size_t w = 0;
  for (std::size_t r = 1; r < n; ++r) {
    if (lt(::ycxx::detail::row_at(keys, w), ::ycxx::detail::row_at(keys, r))) {
      ++w;
      if (w != r) {
        ::ycxx::detail::move_row(keys, w, r);
        (::ycxx::detail::move_row(others, w, r), ...);
      }
    }
  }
  if (w + 1 < n) {
    ::ycxx::detail::erase_from(keys, w + 1);
    (::ycxx::detail::erase_from(others, w + 1), ...);
  }
}

// Removes the rows for which pred(index) holds, keeping the order of the others; pred is applied
// exactly once per row ([flat.map.erasure], [flat.set.erasure]). Returns the number removed.
template <class Pred, class Keys, class... Others>
constexpr std::size_t erase_rows_if(Pred& pred, Keys& keys, Others&... others) {
  const std::size_t n = keys.size();
  std::size_t w = 0;
  for (std::size_t r = 0; r < n; ++r) {
    if (!pred(r)) {
      if (w != r) {
        ::ycxx::detail::move_row(keys, w, r);
        (::ycxx::detail::move_row(others, w, r), ...);
      }
      ++w;
    }
  }
  if (w < n) {
    ::ycxx::detail::erase_from(keys, w);
    (::ycxx::detail::erase_from(others, w), ...);
  }
  return n - w;
}

// The deduction-guide constraints of [container.adaptors.general]/6.
template <class Compare, class KeyContainer>
concept flat_compare_for =
    !qualifies_as_allocator<Compare> && requires { typename KeyContainer::value_type; } &&
    std::is_invocable_v<const Compare&, const typename KeyContainer::value_type&,
                        const typename KeyContainer::value_type&>;
template <class C>
concept flat_container_arg = !qualifies_as_allocator<C>;

// [container.adaptors.general]/8 alloc-rebind
template <class Allocator, class T>
using rebound_alloc = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;

// The allocator-extended constructors ([flat.map.cons.alloc]/1, [flat.set.cons.alloc]/1). A
// namespace-scope concept rather than a private member: the constraints are also checked for
// the constructors' implicit deduction guides.
template <class Alloc, class... Cs>
concept flat_alloc_for = (std::uses_allocator_v<Cs, Alloc> && ...);

} // namespace ycxx::detail
