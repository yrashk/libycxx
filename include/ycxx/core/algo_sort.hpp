// libycxx core: sorting and related operations ([alg.sorting]) other than min/max: sort,
// stable_sort, partial_sort, nth_element, binary search, partitions, merge, set operations,
// heap operations and permutations, in both the std:: and the std::ranges:: forms.
//
// sort and nth_element are introsort / introselect (median-of-three or ninther pivots, Hoare
// partitioning, heapsort when the recursion gets too deep). stable_sort, stable_partition and
// inplace_merge use a temporary buffer (operator new(nothrow) at run time, std::allocator in
// constant evaluation) and fall back to in-place rotation-based algorithms when none is had.
#pragma once

#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/algo_mutate.hpp>
#include <ycxx/core/new.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// ---- temporary buffer --------------------------------------------------------------------
// Uninitialized storage for up to `capacity` objects of type T. At run time it comes from
// operator new(nothrow); requests that cannot be met are halved until they can, and capacity 0
// means no buffer.
template <class T>
class temp_buffer {
  T* data_ = nullptr;
  std::ptrdiff_t capacity_ = 0;

  static constexpr bool overaligned = alignof(T) > cfg::default_new_alignment;

public:
  constexpr explicit temp_buffer(std::ptrdiff_t want) noexcept {
    if consteval {
      // Constant evaluation: std::allocator is the only allocation available, and it cannot
      // fail short of the evaluation itself failing.
      if (want > 0) {
        data_ = std::allocator<T>().allocate(static_cast<std::size_t>(want));
        capacity_ = want;
      }
    } else {
      constexpr std::ptrdiff_t max_count = static_cast<std::ptrdiff_t>(~std::size_t(0) / 2 / sizeof(T));
      if (want > max_count)
        want = max_count;
      for (; want > 0; want /= 2) {
        void* p;
        if constexpr (overaligned)
          p = ::operator new(static_cast<std::size_t>(want) * sizeof(T), std::align_val_t(alignof(T)), std::nothrow);
        else
          p = ::operator new(static_cast<std::size_t>(want) * sizeof(T), std::nothrow);
        if (p) {
          data_ = static_cast<T*>(p);
          capacity_ = want;
          return;
        }
      }
    }
  }
  temp_buffer(const temp_buffer&) = delete;
  temp_buffer& operator=(const temp_buffer&) = delete;
  constexpr ~temp_buffer() {
    if consteval {
      if (data_)
        std::allocator<T>().deallocate(data_, static_cast<std::size_t>(capacity_));
    } else {
      if (data_) {
        if constexpr (overaligned)
          ::operator delete(data_, std::align_val_t(alignof(T)));
        else
          ::operator delete(data_);
      }
    }
  }
  constexpr T* data() const noexcept { return data_; }
  constexpr std::ptrdiff_t capacity() const noexcept { return capacity_; }
};

// Destroys the objects [p, p + n) of a temporary buffer on scope exit.
template <class T>
struct buffer_objects {
  T* p;
  std::ptrdiff_t n = 0;
  constexpr ~buffer_objects() {
    for (std::ptrdiff_t i = 0; i != n; ++i)
      p[i].~T();
  }
};

// Moves [first, last) into the uninitialized buffer `objs` (counting as it goes).
template <class Ops, class I, class T>
constexpr void move_into_buffer(I first, I last, buffer_objects<T>& objs) {
  for (; first != last; ++first) {
    std::construct_at(objs.p + objs.n, Ops::iter_move(first));
    ++objs.n;
  }
}

// The iterator arithmetic below uses difference_type operands only: an iterator need not
// accept an int ([iterator.requirements.general]; libc++'s robust_re_difference_type).
template <class I>
inline constexpr std::iter_difference_t<I> diff_one = 1;

constexpr int floor_log2(unsigned long long n) noexcept {
  int r = 0;
  while (n > 1) {
    n >>= 1;
    ++r;
  }
  return r;
}

// ---- insertion sort ------------------------------------------------------------------------
template <class Ops, class I, class C>
constexpr void insertion_sort(I first, I last, C less) {
  if (first == last)
    return;
  for (I i = first + ::ycxx::detail::diff_one<I>; i != last; ++i) {
    I j = i;
    if (!less(*i, *(j - ::ycxx::detail::diff_one<I>)))
      continue;
    std::iter_value_t<I> tmp(Ops::iter_move(i));
    do {
      I k = j;
      --k;
      *j = Ops::iter_move(k);
      j = k;
    } while (j != first && less(tmp, *(j - ::ycxx::detail::diff_one<I>)));
    *j = std::move(tmp);
  }
}

// Binary insertion sort: fewer comparisons, used for the leaves of stable_sort. Stable: an
// element goes after every element it is not less than.
template <class Ops, class I, class C>
constexpr void binary_insertion_sort(I first, I last, C less) {
  if (first == last)
    return;
  for (I i = first + ::ycxx::detail::diff_one<I>; i != last; ++i) {
    if (!less(*i, *(i - ::ycxx::detail::diff_one<I>)))
      continue;
    // upper bound of *i in [first, i - 1)
    I lo = first;
    auto n = (i - ::ycxx::detail::diff_one<I>) - first;
    while (n > 0) {
      auto half = n / 2;
      I mid = lo + half;
      if (less(*i, *mid)) {
        n = half;
      } else {
        lo = mid + ::ycxx::detail::diff_one<I>;
        n -= half + 1;
      }
    }
    std::iter_value_t<I> tmp(Ops::iter_move(i));
    for (I j = i; j != lo;) {
      I k = j;
      *j = Ops::iter_move(--k);
      j = k;
    }
    *lo = std::move(tmp);
  }
}

// ---- heaps ---------------------------------------------------------------------------------
// Moves `value` up from the hole at `hole` (a max-heap on [first, ...)).
template <class Ops, class I, class C>
constexpr void heap_push_hole(I first, std::iter_difference_t<I> hole, std::iter_value_t<I>& value, C less) {
  while (hole > 0) {
    auto parent = (hole - 1) / 2;
    I p = first + parent;
    if (!less(*p, value))
      break;
    *(first + hole) = Ops::iter_move(p);
    hole = parent;
  }
  *(first + hole) = std::move(value);
}
// Moves `value` down from the hole at `hole` in a heap of `len` elements: two comparisons per
// level.
template <class Ops, class I, class C>
constexpr void heap_sift_hole(I first, std::iter_difference_t<I> len, std::iter_difference_t<I> hole,
                              std::iter_value_t<I>& value, C less) {
  for (;;) {
    auto child = 2 * hole + 1;
    if (child >= len)
      break;
    I c = first + child;
    if (child + 1 < len && less(*c, *(c + ::ycxx::detail::diff_one<I>))) {
      ++child;
      ++c;
    }
    if (!less(value, *c))
      break;
    *(first + hole) = Ops::iter_move(c);
    hole = child;
  }
  *(first + hole) = std::move(value);
}
// pop_heap's sift (Floyd): the hole first goes down to a leaf along the larger children (one
// comparison per level), then `value`, which comes from the back of the heap and so usually
// belongs near the bottom, moves up from there.
template <class Ops, class I, class C>
constexpr void heap_sift_leaf(I first, std::iter_difference_t<I> len, std::iter_difference_t<I> hole,
                              std::iter_value_t<I>& value, C less) {
  const auto top = hole;
  // While both children exist; the choice is arithmetic, not a branch (it is unpredictable).
  for (auto child = 2 * hole + 2; child < len; child = 2 * hole + 2) {
    child -= static_cast<bool>(less(*(first + child), *(first + (child - 1))));
    I c = first + child;
    *(first + hole) = Ops::iter_move(c);
    hole = child;
  }
  if (2 * hole + 2 == len) { // a last, lone left child
    I c = first + (len - 1);
    *(first + hole) = Ops::iter_move(c);
    hole = len - 1;
  }
  while (hole > top) {
    const auto parent = (hole - 1) / 2;
    I p = first + parent;
    if (!less(*p, value))
      break;
    *(first + hole) = Ops::iter_move(p);
    hole = parent;
  }
  *(first + hole) = std::move(value);
}
template <class Ops, class I, class C>
constexpr void push_heap_impl(I first, I last, C less) {
  auto n = last - first;
  if (n < 2)
    return;
  I back = last - ::ycxx::detail::diff_one<I>;
  std::iter_value_t<I> v(Ops::iter_move(back));
  ::ycxx::detail::heap_push_hole<Ops>(first, n - 1, v, less);
}
template <class Ops, class I, class C>
constexpr void pop_heap_impl(I first, I last, C less) {
  auto n = last - first;
  if (n < 2)
    return;
  I back = last - ::ycxx::detail::diff_one<I>;
  std::iter_value_t<I> v(Ops::iter_move(back));
  *back = Ops::iter_move(first);
  ::ycxx::detail::heap_sift_leaf<Ops>(first, n - 1, 0, v, less);
}
template <class Ops, class I, class C>
constexpr void make_heap_impl(I first, I last, C less) {
  auto n = last - first;
  if (n < 2)
    return;
  for (auto start = (n - 2) / 2;; --start) {
    I s = first + start;
    std::iter_value_t<I> v(Ops::iter_move(s));
    ::ycxx::detail::heap_sift_hole<Ops>(first, n, start, v, less);
    if (start == 0)
      break;
  }
}
template <class Ops, class I, class C>
constexpr void sort_heap_impl(I first, I last, C less) {
  for (; last - first > 1; --last)
    ::ycxx::detail::pop_heap_impl<Ops>(first, last, less);
}
template <class I, class C>
constexpr I is_heap_until_impl(I first, I last, C less) {
  auto n = last - first;
  for (std::iter_difference_t<I> i = 1; i < n; ++i)
    if (less(*(first + (i - 1) / 2), *(first + i)))
      return first + i;
  return last;
}

// ---- sort / nth_element --------------------------------------------------------------------
template <class Ops, class I, class C>
constexpr void sort3(I a, I b, I c, C less) {
  if (less(*b, *a))
    Ops::iter_swap(a, b);
  if (less(*c, *b)) {
    Ops::iter_swap(b, c);
    if (less(*b, *a))
      Ops::iter_swap(a, b);
  }
}

// Chooses a pivot, moves it to *first and partitions [first + 1, last); returns cut such that
// every element of [first, cut) is not greater and every element of [cut, last) not less than
// the pivot, with first < cut < last. Needs last - first >= 4.
template <class Ops, class I, class C>
constexpr I partition_pivot(I first, I last, C less) {
  using D = std::iter_difference_t<I>;
  D n = last - first;
  I mid = first + n / 2;
  I f1 = first + D(1); // the first and last elements of [first + 1, last)
  I l1 = last - D(1);
  if (n > 128) { // ninther
    D s = n / 8;
    ::ycxx::detail::sort3<Ops>(f1, f1 + s, f1 + 2 * s, less);
    ::ycxx::detail::sort3<Ops>(mid - s, mid, mid + s, less);
    ::ycxx::detail::sort3<Ops>(l1 - 2 * s, l1 - s, l1, less);
    ::ycxx::detail::sort3<Ops>(f1 + s, mid, l1 - s, less);
  } else {
    ::ycxx::detail::sort3<Ops>(f1, mid, l1, less);
  }
  // Now an element not greater than the pivot lies left of mid and one not less right of it;
  // they bound both scans below.
  Ops::iter_swap(first, mid);
  I lo = f1;
  I hi = l1;
  for (;;) {
    while (less(*lo, *first))
      ++lo;
    while (less(*first, *hi))
      --hi;
    if (!(lo < hi))
      return lo;
    Ops::iter_swap(lo, hi);
    ++lo;
    --hi;
  }
}

template <class Ops, class I, class C>
constexpr void heap_sort(I first, I last, C less) {
  ::ycxx::detail::make_heap_impl<Ops>(first, last, less);
  ::ycxx::detail::sort_heap_impl<Ops>(first, last, less);
}

inline constexpr int insertion_sort_threshold = 16;

// depth: partitioning rounds left before switching to heapsort, shared by the whole recursion
// (2 floor(log2 N) at the top), so the worst case stays O(N log N).
template <class Ops, class I, class C>
constexpr void introsort_loop(I first, I last, int depth, C less) {
  while (last - first > insertion_sort_threshold) {
    if (depth-- == 0) {
      ::ycxx::detail::heap_sort<Ops>(first, last, less);
      return;
    }
    I cut = ::ycxx::detail::partition_pivot<Ops>(first, last, less);
    // Recurse into the smaller part, iterate on the larger: O(log N) stack.
    if (cut - first < last - cut) {
      ::ycxx::detail::introsort_loop<Ops>(first, cut, depth, less);
      first = cut;
    } else {
      ::ycxx::detail::introsort_loop<Ops>(cut, last, depth, less);
      last = cut;
    }
  }
  ::ycxx::detail::insertion_sort<Ops>(first, last, less);
}
template <class Ops, class I, class C>
constexpr void sort_impl(I first, I last, C less) {
  int depth = 2 * ::ycxx::detail::floor_log2(static_cast<unsigned long long>(last - first));
  ::ycxx::detail::introsort_loop<Ops>(first, last, depth, less);
}

// Sorts [first, last) so that [first, middle) holds the smallest elements in order.
template <class Ops, class I, class C>
constexpr void partial_sort_impl(I first, I middle, I last, C less) {
  if (first == middle)
    return;
  ::ycxx::detail::make_heap_impl<Ops>(first, middle, less);
  auto len = middle - first;
  for (I i = middle; i != last; ++i)
    if (less(*i, *first)) {
      std::iter_value_t<I> v(Ops::iter_move(i));
      *i = Ops::iter_move(first);
      ::ycxx::detail::heap_sift_hole<Ops>(first, len, 0, v, less);
    }
  ::ycxx::detail::sort_heap_impl<Ops>(first, middle, less);
}

template <class Ops, class I, class C>
constexpr void nth_element_impl(I first, I nth, I last, C less) {
  if (nth == last)
    return;
  int depth = 2 * ::ycxx::detail::floor_log2(static_cast<unsigned long long>(last - first));
  while (last - first > 3) {
    if (depth-- == 0) {
      ::ycxx::detail::partial_sort_impl<Ops>(first, nth + ::ycxx::detail::diff_one<I>, last, less);
      return;
    }
    I cut = ::ycxx::detail::partition_pivot<Ops>(first, last, less);
    if (cut <= nth)
      first = cut;
    else
      last = cut;
  }
  ::ycxx::detail::insertion_sort<Ops>(first, last, less);
}

template <class I, class S, class C>
constexpr I is_sorted_until_impl(I first, S last, C less) {
  if (first == last)
    return first;
  I next = first;
  while (++next != last) {
    if (less(*next, *first))
      return next;
    first = next;
  }
  return next;
}

// ---- binary search -------------------------------------------------------------------------
// The first element of [first, first + n) for which pred is false ([alg.partitions]).
template <class I, class P>
constexpr I partition_point_n(I first, std::iter_difference_t<I> n, P pred) {
  while (n > 0) {
    auto half = n / 2;
    I mid = ::ycxx::detail::iter_next(first, half);
    if (pred(*mid)) {
      first = ++mid;
      n -= half + 1;
    } else {
      n = half;
    }
  }
  return first;
}

// ---- partitions ----------------------------------------------------------------------------
template <class Ops, class I, class S, class P>
constexpr std::pair<I, I> partition_impl(I first, S last, P pred) {
  if constexpr (std::bidirectional_iterator<I> && (std::same_as<I, S> || std::sized_sentinel_for<S, I>)) {
    I end = ::ycxx::detail::iter_at(first, last);
    I hi = end;
    for (;;) {
      for (;; ++first) {
        if (first == hi)
          return {first, end};
        if (!pred(*first))
          break;
      }
      do {
        if (first == --hi)
          return {first, end};
      } while (!pred(*hi));
      Ops::iter_swap(first, hi);
      ++first;
    }
  } else {
    first = ::ycxx::detail::find_if_impl(static_cast<I&&>(first), last, negated<P>{pred});
    if (first == last)
      return {first, first};
    I i = first;
    for (++i; i != last; ++i)
      if (pred(*i)) {
        Ops::iter_swap(first, i);
        ++first;
      }
    return {first, i};
  }
}

// Stable partition of [first, first + len), which starts with an element for which pred is
// false; buf holds room for `cap` values.
template <class Ops, class I, class P, class T>
constexpr I stable_partition_adaptive(I first, I last, std::iter_difference_t<I> len, P pred, T* buf,
                                      std::ptrdiff_t cap) {
  if (len <= cap) {
    // The false elements go to the buffer, the true ones are packed at the front.
    buffer_objects<T> objs{buf};
    I out = first;
    std::construct_at(buf, Ops::iter_move(first));
    objs.n = 1;
    for (I i = ::ycxx::detail::iter_next(first, 1); i != last; ++i) {
      if (pred(*i)) {
        *out = Ops::iter_move(i);
        ++out;
      } else {
        std::construct_at(buf + objs.n, Ops::iter_move(i));
        ++objs.n;
      }
    }
    I r = out;
    for (std::ptrdiff_t k = 0; k != objs.n; (void)++k, (void)++out)
      *out = std::move(buf[k]);
    return r;
  }
  if (len == 1)
    return first; // the first element is false
  // Divide and conquer: partition both halves, then rotate the middle.
  auto half = len / 2;
  I mid = ::ycxx::detail::iter_next(first, half);
  I left = ::ycxx::detail::stable_partition_adaptive<Ops>(first, mid, half, pred, buf, cap);
  // The right half may start with true elements: skip them (each predicate call is used once).
  I m2 = mid;
  auto rlen = len - half;
  while (rlen > 0 && pred(*m2)) {
    ++m2;
    --rlen;
  }
  I right = rlen > 0 ? ::ycxx::detail::stable_partition_adaptive<Ops>(m2, last, rlen, pred, buf, cap) : m2;
  return ::ycxx::detail::rotate_impl<Ops>(left, mid, right);
}

template <class Ops, class I, class S, class P>
constexpr std::pair<I, I> stable_partition_impl(I first, S last, P pred) {
  I end = ::ycxx::detail::iter_at(first, last);
  // Leading true elements stay in place.
  while (first != end && pred(*first))
    ++first;
  if (first == end)
    return {first, end};
  // [first, end) now starts with a false element; every element is tested exactly once.
  auto len = ::ycxx::detail::range_length(first, end);
  using T = std::iter_value_t<I>;
  temp_buffer<T> buf(len);
  return {::ycxx::detail::stable_partition_adaptive<Ops>(first, end, len, pred, buf.data(), buf.capacity()), end};
}

// ---- merging -------------------------------------------------------------------------------
template <class I1, class S1, class I2, class S2, class O, class C>
constexpr void merge_loop(I1& first1, S1 last1, I2& first2, S2 last2, O& result, C less) {
  while (first1 != last1 && first2 != last2) {
    if (less.rev(*first2, *first1)) {
      *result = *first2;
      ++first2;
    } else {
      *result = *first1;
      ++first1;
    }
    ++result;
  }
}

// Merges the sorted runs [first, middle) and [middle, last) with a buffer of at least
// min(len1, len2) elements: N - 1 comparisons at most.
template <class Ops, class I, class C, class T>
constexpr void merge_with_buffer(I first, I middle, I last, std::iter_difference_t<I> len1,
                                 std::iter_difference_t<I> len2, C less, T* buf) {
  buffer_objects<T> objs{buf};
  if (len1 <= len2) {
    ::ycxx::detail::move_into_buffer<Ops>(first, middle, objs);
    T* b = buf;
    T* be = buf + objs.n;
    I out = first;
    while (b != be) {
      if (middle == last) {
        for (; b != be; (void)++b, (void)++out)
          *out = std::move(*b);
        return;
      }
      if (less(*middle, *b)) {
        *out = Ops::iter_move(middle);
        ++middle;
      } else {
        *out = std::move(*b);
        ++b;
      }
      ++out;
    }
  } else {
    ::ycxx::detail::move_into_buffer<Ops>(middle, last, objs);
    T* bb = buf;
    T* b = buf + objs.n; // one past the next buffer element to place
    I out = last;
    I l = middle; // one past the next left element to place
    while (b != bb) {
      if (l == first) {
        while (b != bb)
          *--out = std::move(*--b);
        return;
      }
      I lp = l;
      --lp;
      if (less(*(b - 1), *lp)) {
        *--out = Ops::iter_move(lp);
        l = lp;
      } else {
        *--out = std::move(*--b);
      }
    }
  }
}

template <class Ops, class I, class C, class T>
constexpr void merge_adaptive(I first, I middle, I last, std::iter_difference_t<I> len1,
                              std::iter_difference_t<I> len2, C less, T* buf, std::ptrdiff_t cap) {
  for (;;) {
    if (len1 == 0 || len2 == 0)
      return;
    if (len1 <= cap || len2 <= cap) {
      ::ycxx::detail::merge_with_buffer<Ops>(first, middle, last, len1, len2, less, buf);
      return;
    }
    if (len1 + len2 == 2) {
      if (less(*middle, *first))
        Ops::iter_swap(first, middle);
      return;
    }
    // Split the longer run in half, find the matching cut in the other one, rotate the two
    // inner pieces into place and merge both sides.
    I cut1 = first, cut2 = middle;
    std::iter_difference_t<I> len11, len22;
    if (len1 > len2) {
      len11 = len1 / 2;
      ::ycxx::detail::iter_advance(cut1, len11);
      // lower bound of *cut1 in [middle, last)
      auto&& v = *cut1;
      cut2 = ::ycxx::detail::partition_point_n(middle, len2, [&](auto&& e) { return less(e, v); });
      len22 = ::ycxx::detail::range_length(middle, cut2);
    } else {
      len22 = len2 / 2;
      ::ycxx::detail::iter_advance(cut2, len22);
      // upper bound of *cut2 in [first, middle)
      auto&& v = *cut2;
      cut1 = ::ycxx::detail::partition_point_n(first, len1, [&](auto&& e) { return !less(v, e); });
      len11 = ::ycxx::detail::range_length(first, cut1);
    }
    I new_mid = ::ycxx::detail::rotate_impl<Ops>(cut1, middle, cut2);
    // Recurse into the smaller side, iterate on the other.
    if (len11 + len22 < (len1 - len11) + (len2 - len22)) {
      ::ycxx::detail::merge_adaptive<Ops>(first, cut1, new_mid, len11, len22, less, buf, cap);
      first = new_mid;
      middle = cut2;
      len1 -= len11;
      len2 -= len22;
    } else {
      ::ycxx::detail::merge_adaptive<Ops>(new_mid, cut2, last, len1 - len11, len2 - len22, less, buf, cap);
      last = new_mid;
      middle = cut1;
      len1 = len11;
      len2 = len22;
    }
  }
}

template <class Ops, class I, class C>
constexpr void inplace_merge_impl(I first, I middle, I last, C less) {
  if (first == middle || middle == last)
    return;
  auto len1 = ::ycxx::detail::range_length(first, middle);
  auto len2 = ::ycxx::detail::range_length(middle, last);
  using T = std::iter_value_t<I>;
  temp_buffer<T> buf(len1 < len2 ? len1 : len2);
  ::ycxx::detail::merge_adaptive<Ops>(first, middle, last, len1, len2, less, buf.data(), buf.capacity());
}

template <class Ops, class I, class C, class T>
constexpr void stable_sort_adaptive(I first, I last, std::iter_difference_t<I> len, C less, T* buf,
                                    std::ptrdiff_t cap) {
  if (len <= insertion_sort_threshold) {
    ::ycxx::detail::binary_insertion_sort<Ops>(first, last, less);
    return;
  }
  auto half = len / 2;
  I mid = first + half;
  ::ycxx::detail::stable_sort_adaptive<Ops>(first, mid, half, less, buf, cap);
  ::ycxx::detail::stable_sort_adaptive<Ops>(mid, last, len - half, less, buf, cap);
  if (!less(*mid, *(mid - ::ycxx::detail::diff_one<I>)))
    return; // already in order
  ::ycxx::detail::merge_adaptive<Ops>(first, mid, last, half, len - half, less, buf, cap);
}

template <class Ops, class I, class C>
constexpr void stable_sort_impl(I first, I last, C less) {
  auto len = last - first;
  if (len <= insertion_sort_threshold) {
    ::ycxx::detail::binary_insertion_sort<Ops>(first, last, less);
    return;
  }
  using T = std::iter_value_t<I>;
  temp_buffer<T> buf((len + 1) / 2);
  ::ycxx::detail::stable_sort_adaptive<Ops>(first, last, len, less, buf.data(), buf.capacity());
}

// ---- set operations ------------------------------------------------------------------------
template <class I1, class S1, class I2, class S2, class C>
constexpr bool includes_impl(I1 first1, S1 last1, I2 first2, S2 last2, C less) {
  for (; first2 != last2; ++first1) {
    if (first1 == last1 || less.rev(*first2, *first1))
      return false;
    if (!less(*first1, *first2))
      ++first2;
  }
  return true;
}

template <class I1, class S1, class I2, class S2, class O, class C>
constexpr void set_union_loop(I1& first1, S1 last1, I2& first2, S2 last2, O& result, C less) {
  while (first1 != last1 && first2 != last2) {
    if (less(*first1, *first2)) {
      *result = *first1;
      ++first1;
    } else if (less.rev(*first2, *first1)) {
      *result = *first2;
      ++first2;
    } else {
      *result = *first1;
      ++first1;
      ++first2;
    }
    ++result;
  }
}
template <class I1, class S1, class I2, class S2, class O, class C>
constexpr void set_intersection_loop(I1& first1, S1 last1, I2& first2, S2 last2, O& result, C less) {
  while (first1 != last1 && first2 != last2) {
    if (less(*first1, *first2)) {
      ++first1;
    } else if (less.rev(*first2, *first1)) {
      ++first2;
    } else {
      *result = *first1;
      ++result;
      ++first1;
      ++first2;
    }
  }
}
template <class I1, class S1, class I2, class S2, class O, class C>
constexpr void set_difference_loop(I1& first1, S1 last1, I2& first2, S2 last2, O& result, C less) {
  while (first1 != last1 && first2 != last2) {
    if (less(*first1, *first2)) {
      *result = *first1;
      ++result;
      ++first1;
    } else {
      if (!less.rev(*first2, *first1))
        ++first1;
      ++first2;
    }
  }
}
template <class I1, class S1, class I2, class S2, class O, class C>
constexpr void set_symmetric_difference_loop(I1& first1, S1 last1, I2& first2, S2 last2, O& result, C less) {
  while (first1 != last1 && first2 != last2) {
    if (less(*first1, *first2)) {
      *result = *first1;
      ++result;
      ++first1;
    } else if (less.rev(*first2, *first1)) {
      *result = *first2;
      ++result;
      ++first2;
    } else {
      ++first1;
      ++first2;
    }
  }
}

// ---- permutations --------------------------------------------------------------------------
template <class Ops, class I, class C>
constexpr bool next_permutation_impl(I first, I last, C less) {
  if (first == last)
    return false;
  I i = last;
  if (first == --i)
    return false;
  for (;;) {
    I i1 = i;
    if (less(*--i, *i1)) {
      I i2 = last;
      while (!less(*i, *--i2)) {
      }
      Ops::iter_swap(i, i2);
      ::ycxx::detail::reverse_impl<Ops>(i1, last);
      return true;
    }
    if (i == first) {
      ::ycxx::detail::reverse_impl<Ops>(first, last);
      return false;
    }
  }
}
template <class Ops, class I, class C>
constexpr bool prev_permutation_impl(I first, I last, C less) {
  if (first == last)
    return false;
  I i = last;
  if (first == --i)
    return false;
  for (;;) {
    I i1 = i;
    if (less(*i1, *--i)) {
      I i2 = last;
      while (!less(*--i2, *i)) {
      }
      Ops::iter_swap(i, i2);
      ::ycxx::detail::reverse_impl<Ops>(i1, last);
      return true;
    }
    if (i == first) {
      ::ycxx::detail::reverse_impl<Ops>(first, last);
      return false;
    }
  }
}

}} // namespace ycxx::detail

// =============================================================================================
// std:: forms
// =============================================================================================
namespace [[gnu::visibility("hidden")]] std {

// [sort], [stable.sort], [partial.sort], [partial.sort.copy], [is.sorted]
template <class RandomAccessIterator, class Compare>
constexpr void sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp) {
  ::ycxx::detail::sort_impl<ycxx::detail::classic_ops>(first, last, ::ycxx::detail::ref_pred(comp));
}
template <class RandomAccessIterator>
constexpr void sort(RandomAccessIterator first, RandomAccessIterator last) {
  std::sort(first, last, less<>{});
}
template <class RandomAccessIterator, class Compare>
constexpr void stable_sort(RandomAccessIterator first, RandomAccessIterator last, Compare comp) {
  ::ycxx::detail::stable_sort_impl<ycxx::detail::classic_ops>(first, last, ::ycxx::detail::ref_pred(comp));
}
template <class RandomAccessIterator>
constexpr void stable_sort(RandomAccessIterator first, RandomAccessIterator last) {
  std::stable_sort(first, last, less<>{});
}
template <class RandomAccessIterator, class Compare>
constexpr void partial_sort(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last,
                            Compare comp) {
  ::ycxx::detail::partial_sort_impl<ycxx::detail::classic_ops>(first, middle, last, ::ycxx::detail::ref_pred(comp));
}
template <class RandomAccessIterator>
constexpr void partial_sort(RandomAccessIterator first, RandomAccessIterator middle, RandomAccessIterator last) {
  std::partial_sort(first, middle, last, less<>{});
}
template <class InputIterator, class RandomAccessIterator, class Compare>
constexpr RandomAccessIterator partial_sort_copy(InputIterator first, InputIterator last,
                                                 RandomAccessIterator result_first, RandomAccessIterator result_last,
                                                 Compare comp) {
  auto less = ::ycxx::detail::ref_pred(comp);
  RandomAccessIterator r = result_first;
  for (; first != last && r != result_last; (void)++first, (void)++r)
    *r = *first;
  if (r == result_first)
    return r;
  ::ycxx::detail::make_heap_impl<ycxx::detail::classic_ops>(result_first, r, less);
  auto len = r - result_first;
  for (; first != last; ++first)
    if (less(*first, *result_first)) {
      *result_first = *first;
      iter_value_t<RandomAccessIterator> v(::ycxx::detail::classic_ops::iter_move(result_first));
      ::ycxx::detail::heap_sift_hole<ycxx::detail::classic_ops>(result_first, len, 0, v, less);
    }
  ::ycxx::detail::sort_heap_impl<ycxx::detail::classic_ops>(result_first, r, less);
  return r;
}
template <class InputIterator, class RandomAccessIterator>
constexpr RandomAccessIterator partial_sort_copy(InputIterator first, InputIterator last,
                                                 RandomAccessIterator result_first, RandomAccessIterator result_last) {
  return std::partial_sort_copy(first, last, result_first, result_last, less<>{});
}
template <class ForwardIterator, class Compare>
[[nodiscard]] constexpr ForwardIterator is_sorted_until(ForwardIterator first, ForwardIterator last, Compare comp) {
  return ::ycxx::detail::is_sorted_until_impl(first, last, ::ycxx::detail::ref_pred(comp));
}
template <class ForwardIterator>
[[nodiscard]] constexpr ForwardIterator is_sorted_until(ForwardIterator first, ForwardIterator last) {
  return std::is_sorted_until(first, last, less<>{});
}
template <class ForwardIterator, class Compare>
[[nodiscard]] constexpr bool is_sorted(ForwardIterator first, ForwardIterator last, Compare comp) {
  return std::is_sorted_until(first, last, comp) == last;
}
template <class ForwardIterator>
[[nodiscard]] constexpr bool is_sorted(ForwardIterator first, ForwardIterator last) {
  return std::is_sorted_until(first, last, less<>{}) == last;
}

// [alg.nth.element]
template <class RandomAccessIterator, class Compare>
constexpr void nth_element(RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last,
                           Compare comp) {
  ::ycxx::detail::nth_element_impl<ycxx::detail::classic_ops>(first, nth, last, ::ycxx::detail::ref_pred(comp));
}
template <class RandomAccessIterator>
constexpr void nth_element(RandomAccessIterator first, RandomAccessIterator nth, RandomAccessIterator last) {
  std::nth_element(first, nth, last, less<>{});
}

// [alg.binary.search]
template <class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type, class Compare>
[[nodiscard]] constexpr ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last, const T& value,
                                                    Compare comp) {
  return ::ycxx::detail::partition_point_n(first, ::ycxx::detail::range_length(first, last),
                                                   [&](auto&& e) -> bool { return static_cast<bool>(comp(e, value)); });
}
template <class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type>
[[nodiscard]] constexpr ForwardIterator lower_bound(ForwardIterator first, ForwardIterator last, const T& value) {
  return std::lower_bound(first, last, value, less<>{});
}
template <class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type, class Compare>
[[nodiscard]] constexpr ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last, const T& value,
                                                    Compare comp) {
  return ::ycxx::detail::partition_point_n(first, ::ycxx::detail::range_length(first, last),
                                                   [&](auto&& e) -> bool { return !static_cast<bool>(comp(value, e)); });
}
template <class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type>
[[nodiscard]] constexpr ForwardIterator upper_bound(ForwardIterator first, ForwardIterator last, const T& value) {
  return std::upper_bound(first, last, value, less<>{});
}
template <class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type, class Compare>
[[nodiscard]] constexpr pair<ForwardIterator, ForwardIterator> equal_range(ForwardIterator first, ForwardIterator last,
                                                                           const T& value, Compare comp) {
  ForwardIterator lo = std::lower_bound(first, last, value, comp);
  return {lo, std::upper_bound(lo, last, value, comp)};
}
template <class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type>
[[nodiscard]] constexpr pair<ForwardIterator, ForwardIterator> equal_range(ForwardIterator first, ForwardIterator last,
                                                                           const T& value) {
  return std::equal_range(first, last, value, less<>{});
}
template <class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type, class Compare>
[[nodiscard]] constexpr bool binary_search(ForwardIterator first, ForwardIterator last, const T& value, Compare comp) {
  first = std::lower_bound(first, last, value, comp);
  return first != last && !static_cast<bool>(comp(value, *first));
}
template <class ForwardIterator, class T = typename iterator_traits<ForwardIterator>::value_type>
[[nodiscard]] constexpr bool binary_search(ForwardIterator first, ForwardIterator last, const T& value) {
  return std::binary_search(first, last, value, less<>{});
}

// [alg.partitions]
template <class InputIterator, class Predicate>
[[nodiscard]] constexpr bool is_partitioned(InputIterator first, InputIterator last, Predicate pred) {
  auto p = ::ycxx::detail::ref_pred(pred);
  first = ::ycxx::detail::find_if_impl(first, last, ::ycxx::detail::negated{p});
  if (first == last)
    return true;
  ++first;
  return ::ycxx::detail::find_if_impl(first, last, p) == last;
}
template <class ForwardIterator, class Predicate>
constexpr ForwardIterator partition(ForwardIterator first, ForwardIterator last, Predicate pred) {
  return ::ycxx::detail::partition_impl<ycxx::detail::classic_ops>(first, last, ::ycxx::detail::ref_pred(pred)).first;
}
template <class BidirectionalIterator, class Predicate>
constexpr BidirectionalIterator stable_partition(BidirectionalIterator first, BidirectionalIterator last,
                                                 Predicate pred) {
  return ::ycxx::detail::stable_partition_impl<ycxx::detail::classic_ops>(first, last, ::ycxx::detail::ref_pred(pred))
      .first;
}
template <class InputIterator, class OutputIterator1, class OutputIterator2, class Predicate>
constexpr pair<OutputIterator1, OutputIterator2> partition_copy(InputIterator first, InputIterator last,
                                                                OutputIterator1 out_true, OutputIterator2 out_false,
                                                                Predicate pred) {
  for (; first != last; ++first) {
    if (pred(*first)) {
      *out_true = *first;
      ++out_true;
    } else {
      *out_false = *first;
      ++out_false;
    }
  }
  return {out_true, out_false};
}
template <class ForwardIterator, class Predicate>
[[nodiscard]] constexpr ForwardIterator partition_point(ForwardIterator first, ForwardIterator last, Predicate pred) {
  return ::ycxx::detail::partition_point_n(first, ::ycxx::detail::range_length(first, last),
                                                   ::ycxx::detail::ref_pred(pred));
}

// [alg.merge]
template <class InputIterator1, class InputIterator2, class OutputIterator, class Compare>
constexpr OutputIterator merge(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                               InputIterator2 last2, OutputIterator result, Compare comp) {
  ::ycxx::detail::merge_loop(first1, last1, first2, last2, result, ::ycxx::detail::ref_pred(comp));
  result = ::ycxx::detail::copy_dispatch(first1, last1, result).second;
  return ::ycxx::detail::copy_dispatch(first2, last2, result).second;
}
template <class InputIterator1, class InputIterator2, class OutputIterator>
constexpr OutputIterator merge(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                               InputIterator2 last2, OutputIterator result) {
  return std::merge(first1, last1, first2, last2, result, less<>{});
}
template <class BidirectionalIterator, class Compare>
constexpr void inplace_merge(BidirectionalIterator first, BidirectionalIterator middle, BidirectionalIterator last,
                             Compare comp) {
  ::ycxx::detail::inplace_merge_impl<ycxx::detail::classic_ops>(first, middle, last, ::ycxx::detail::ref_pred(comp));
}
template <class BidirectionalIterator>
constexpr void inplace_merge(BidirectionalIterator first, BidirectionalIterator middle, BidirectionalIterator last) {
  std::inplace_merge(first, middle, last, less<>{});
}

// [alg.set.operations]
template <class InputIterator1, class InputIterator2, class Compare>
[[nodiscard]] constexpr bool includes(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                                      InputIterator2 last2, Compare comp) {
  return ::ycxx::detail::includes_impl(first1, last1, first2, last2, ::ycxx::detail::ref_pred(comp));
}
template <class InputIterator1, class InputIterator2>
[[nodiscard]] constexpr bool includes(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                                      InputIterator2 last2) {
  return std::includes(first1, last1, first2, last2, less<>{});
}
template <class InputIterator1, class InputIterator2, class OutputIterator, class Compare>
constexpr OutputIterator set_union(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                                   InputIterator2 last2, OutputIterator result, Compare comp) {
  ::ycxx::detail::set_union_loop(first1, last1, first2, last2, result, ::ycxx::detail::ref_pred(comp));
  result = ::ycxx::detail::copy_dispatch(first1, last1, result).second;
  return ::ycxx::detail::copy_dispatch(first2, last2, result).second;
}
template <class InputIterator1, class InputIterator2, class OutputIterator>
constexpr OutputIterator set_union(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                                   InputIterator2 last2, OutputIterator result) {
  return std::set_union(first1, last1, first2, last2, result, less<>{});
}
template <class InputIterator1, class InputIterator2, class OutputIterator, class Compare>
constexpr OutputIterator set_intersection(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                                          InputIterator2 last2, OutputIterator result, Compare comp) {
  ::ycxx::detail::set_intersection_loop(first1, last1, first2, last2, result, ::ycxx::detail::ref_pred(comp));
  return result;
}
template <class InputIterator1, class InputIterator2, class OutputIterator>
constexpr OutputIterator set_intersection(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                                          InputIterator2 last2, OutputIterator result) {
  return std::set_intersection(first1, last1, first2, last2, result, less<>{});
}
template <class InputIterator1, class InputIterator2, class OutputIterator, class Compare>
constexpr OutputIterator set_difference(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                                        InputIterator2 last2, OutputIterator result, Compare comp) {
  ::ycxx::detail::set_difference_loop(first1, last1, first2, last2, result, ::ycxx::detail::ref_pred(comp));
  return ::ycxx::detail::copy_dispatch(first1, last1, result).second;
}
template <class InputIterator1, class InputIterator2, class OutputIterator>
constexpr OutputIterator set_difference(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                                        InputIterator2 last2, OutputIterator result) {
  return std::set_difference(first1, last1, first2, last2, result, less<>{});
}
template <class InputIterator1, class InputIterator2, class OutputIterator, class Compare>
constexpr OutputIterator set_symmetric_difference(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                                                  InputIterator2 last2, OutputIterator result, Compare comp) {
  ::ycxx::detail::set_symmetric_difference_loop(first1, last1, first2, last2, result, ::ycxx::detail::ref_pred(comp));
  result = ::ycxx::detail::copy_dispatch(first1, last1, result).second;
  return ::ycxx::detail::copy_dispatch(first2, last2, result).second;
}
template <class InputIterator1, class InputIterator2, class OutputIterator>
constexpr OutputIterator set_symmetric_difference(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2,
                                                  InputIterator2 last2, OutputIterator result) {
  return std::set_symmetric_difference(first1, last1, first2, last2, result, less<>{});
}

// [alg.heap.operations]
template <class RandomAccessIterator, class Compare>
constexpr void push_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp) {
  ::ycxx::detail::push_heap_impl<ycxx::detail::classic_ops>(first, last, ::ycxx::detail::ref_pred(comp));
}
template <class RandomAccessIterator>
constexpr void push_heap(RandomAccessIterator first, RandomAccessIterator last) {
  std::push_heap(first, last, less<>{});
}
template <class RandomAccessIterator, class Compare>
constexpr void pop_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp) {
  ::ycxx::detail::pop_heap_impl<ycxx::detail::classic_ops>(first, last, ::ycxx::detail::ref_pred(comp));
}
template <class RandomAccessIterator>
constexpr void pop_heap(RandomAccessIterator first, RandomAccessIterator last) {
  std::pop_heap(first, last, less<>{});
}
template <class RandomAccessIterator, class Compare>
constexpr void make_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp) {
  ::ycxx::detail::make_heap_impl<ycxx::detail::classic_ops>(first, last, ::ycxx::detail::ref_pred(comp));
}
template <class RandomAccessIterator>
constexpr void make_heap(RandomAccessIterator first, RandomAccessIterator last) {
  std::make_heap(first, last, less<>{});
}
template <class RandomAccessIterator, class Compare>
constexpr void sort_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp) {
  ::ycxx::detail::sort_heap_impl<ycxx::detail::classic_ops>(first, last, ::ycxx::detail::ref_pred(comp));
}
template <class RandomAccessIterator>
constexpr void sort_heap(RandomAccessIterator first, RandomAccessIterator last) {
  std::sort_heap(first, last, less<>{});
}
template <class RandomAccessIterator, class Compare>
[[nodiscard]] constexpr RandomAccessIterator is_heap_until(RandomAccessIterator first, RandomAccessIterator last,
                                                           Compare comp) {
  return ::ycxx::detail::is_heap_until_impl(first, last, ::ycxx::detail::ref_pred(comp));
}
template <class RandomAccessIterator>
[[nodiscard]] constexpr RandomAccessIterator is_heap_until(RandomAccessIterator first, RandomAccessIterator last) {
  return std::is_heap_until(first, last, less<>{});
}
template <class RandomAccessIterator, class Compare>
[[nodiscard]] constexpr bool is_heap(RandomAccessIterator first, RandomAccessIterator last, Compare comp) {
  return std::is_heap_until(first, last, comp) == last;
}
template <class RandomAccessIterator>
[[nodiscard]] constexpr bool is_heap(RandomAccessIterator first, RandomAccessIterator last) {
  return std::is_heap_until(first, last, less<>{}) == last;
}

// [alg.permutation.generators]
template <class BidirectionalIterator, class Compare>
constexpr bool next_permutation(BidirectionalIterator first, BidirectionalIterator last, Compare comp) {
  return ::ycxx::detail::next_permutation_impl<ycxx::detail::classic_ops>(first, last, ::ycxx::detail::ref_pred(comp));
}
template <class BidirectionalIterator>
constexpr bool next_permutation(BidirectionalIterator first, BidirectionalIterator last) {
  return std::next_permutation(first, last, less<>{});
}
template <class BidirectionalIterator, class Compare>
constexpr bool prev_permutation(BidirectionalIterator first, BidirectionalIterator last, Compare comp) {
  return ::ycxx::detail::prev_permutation_impl<ycxx::detail::classic_ops>(first, last, ::ycxx::detail::ref_pred(comp));
}
template <class BidirectionalIterator>
constexpr bool prev_permutation(BidirectionalIterator first, BidirectionalIterator last) {
  return std::prev_permutation(first, last, less<>{});
}

} // namespace std

// =============================================================================================
// std::ranges:: forms
// =============================================================================================
namespace [[gnu::visibility("hidden")]] std { namespace ranges {
template <class I1, class I2>
using partial_sort_copy_result = in_out_result<I1, I2>;
template <class I, class O1, class O2>
using partition_copy_result = in_out_out_result<I, O1, O2>;
template <class I1, class I2, class O>
using merge_result = in_in_out_result<I1, I2, O>;
template <class I1, class I2, class O>
using set_union_result = in_in_out_result<I1, I2, O>;
template <class I1, class I2, class O>
using set_intersection_result = in_in_out_result<I1, I2, O>;
template <class I, class O>
using set_difference_result = in_out_result<I, O>;
template <class I1, class I2, class O>
using set_symmetric_difference_result = in_in_out_result<I1, I2, O>;
template <class I>
using next_permutation_result = in_found_result<I>;
template <class I>
using prev_permutation_result = in_found_result<I>;
}} // namespace std::ranges

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::ranges_algo {

using std::ranges::borrowed_iterator_t;
using std::ranges::borrowed_subrange_t;
using std::ranges::iterator_t;

struct sort_fn {
  template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires std::sortable<I, Comp, Proj>
  constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
    I end = ::ycxx::detail::iter_at(first, std::move(last));
    ::ycxx::detail::sort_impl<ranges_ops>(std::move(first), end, ::ycxx::detail::make_comp(comp, proj));
    return end;
  }
  template <std::ranges::random_access_range R, class Comp = std::ranges::less, class Proj = std::identity>
    requires std::sortable<iterator_t<R>, Comp, Proj>
  constexpr borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct stable_sort_fn {
  template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires std::sortable<I, Comp, Proj>
  constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
    I end = ::ycxx::detail::iter_at(first, std::move(last));
    ::ycxx::detail::stable_sort_impl<ranges_ops>(std::move(first), end, ::ycxx::detail::make_comp(comp, proj));
    return end;
  }
  template <std::ranges::random_access_range R, class Comp = std::ranges::less, class Proj = std::identity>
    requires std::sortable<iterator_t<R>, Comp, Proj>
  constexpr borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct partial_sort_fn {
  template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires std::sortable<I, Comp, Proj>
  constexpr I operator()(I first, I middle, S last, Comp comp = {}, Proj proj = {}) const {
    I end = ::ycxx::detail::iter_at(middle, std::move(last));
    ::ycxx::detail::partial_sort_impl<ranges_ops>(std::move(first), std::move(middle), end,
                                                  ::ycxx::detail::make_comp(comp, proj));
    return end;
  }
  template <std::ranges::random_access_range R, class Comp = std::ranges::less, class Proj = std::identity>
    requires std::sortable<iterator_t<R>, Comp, Proj>
  constexpr borrowed_iterator_t<R> operator()(R&& r, iterator_t<R> middle, Comp comp = {}, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::move(middle), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct partial_sort_copy_fn {
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::random_access_iterator I2, std::sentinel_for<I2> S2,
            class Comp = std::ranges::less, class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_copyable<I1, I2> && std::sortable<I2, Comp, Proj2> &&
             std::indirect_strict_weak_order<Comp, std::projected<I1, Proj1>, std::projected<I2, Proj2>>
  constexpr std::ranges::partial_sort_copy_result<I1, I2> operator()(I1 first, S1 last, I2 result_first,
                                                                    S2 result_last, Comp comp = {}, Proj1 proj1 = {},
                                                                    Proj2 proj2 = {}) const {
    auto less = ::ycxx::detail::make_comp(comp, proj2);
    auto cross = ::ycxx::detail::make_comp2(comp, proj1, proj2);
    I2 r = result_first;
    for (; first != last && r != result_last; (void)++first, (void)++r)
      *r = *first;
    if (r != result_first) {
      ::ycxx::detail::make_heap_impl<ranges_ops>(result_first, r, less);
      auto len = r - result_first;
      for (; first != last; ++first)
        if (cross(*first, *result_first)) {
          *result_first = *first;
          std::iter_value_t<I2> v(std::ranges::iter_move(result_first));
          ::ycxx::detail::heap_sift_hole<ranges_ops>(result_first, len, 0, v, less);
        }
      ::ycxx::detail::sort_heap_impl<ranges_ops>(result_first, r, less);
    } else {
      first = ::ycxx::detail::iter_at(std::move(first), last);
    }
    return {std::move(first), std::move(r)};
  }
  template <std::ranges::input_range R1, std::ranges::random_access_range R2, class Comp = std::ranges::less,
            class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::indirectly_copyable<iterator_t<R1>, iterator_t<R2>> && std::sortable<iterator_t<R2>, Comp, Proj2> &&
             std::indirect_strict_weak_order<Comp, std::projected<iterator_t<R1>, Proj1>,
                                             std::projected<iterator_t<R2>, Proj2>>
  constexpr std::ranges::partial_sort_copy_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>>
  operator()(R1&& r, R2&& result_r, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(result_r),
                   std::ranges::end(result_r), std::move(comp), std::move(proj1), std::move(proj2));
  }
};
struct is_sorted_until_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
    return ::ycxx::detail::is_sorted_until_impl(std::move(first), last, ::ycxx::detail::make_comp(comp, proj));
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<R>, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct is_sorted_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
    return ::ycxx::detail::is_sorted_until_impl(std::move(first), last, ::ycxx::detail::make_comp(comp, proj)) == last;
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<R>, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct nth_element_fn {
  template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires std::sortable<I, Comp, Proj>
  constexpr I operator()(I first, I nth, S last, Comp comp = {}, Proj proj = {}) const {
    I end = ::ycxx::detail::iter_at(nth, std::move(last));
    ::ycxx::detail::nth_element_impl<ranges_ops>(std::move(first), std::move(nth), end,
                                                 ::ycxx::detail::make_comp(comp, proj));
    return end;
  }
  template <std::ranges::random_access_range R, class Comp = std::ranges::less, class Proj = std::identity>
    requires std::sortable<iterator_t<R>, Comp, Proj>
  constexpr borrowed_iterator_t<R> operator()(R&& r, iterator_t<R> nth, Comp comp = {}, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::move(nth), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};

// [alg.binary.search]
template <class Comp, class Proj, class T>
struct proj_less_value { // invoke(comp, invoke(proj, e), value)
  Comp& comp;
  Proj& proj;
  const T& value;
  template <class A>
  constexpr bool operator()(A&& a) const {
    return static_cast<bool>(::ycxx::detail::invoke(comp, ::ycxx::detail::invoke(proj, static_cast<A&&>(a)), value));
  }
};
template <class Comp, class Proj, class T>
struct proj_not_value_less { // !invoke(comp, value, invoke(proj, e))
  Comp& comp;
  Proj& proj;
  const T& value;
  template <class A>
  constexpr bool operator()(A&& a) const {
    return !static_cast<bool>(::ycxx::detail::invoke(comp, value, ::ycxx::detail::invoke(proj, static_cast<A&&>(a))));
  }
};

struct lower_bound_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>,
            std::indirect_strict_weak_order<const T*, std::projected<I, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr I operator()(I first, S last, const T& value, Comp comp = {}, Proj proj = {}) const {
    auto n = std::ranges::distance(first, last);
    return ::ycxx::detail::partition_point_n(std::move(first), n, proj_less_value<Comp, Proj, T>{comp, proj, value});
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            class T = std::projected_value_t<iterator_t<R>, Proj>,
            std::indirect_strict_weak_order<const T*, std::projected<iterator_t<R>, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr borrowed_iterator_t<R> operator()(R&& r, const T& value, Comp comp = {},
                                                           Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(comp), std::move(proj));
  }
};
struct upper_bound_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>,
            std::indirect_strict_weak_order<const T*, std::projected<I, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr I operator()(I first, S last, const T& value, Comp comp = {}, Proj proj = {}) const {
    auto n = std::ranges::distance(first, last);
    return ::ycxx::detail::partition_point_n(std::move(first), n,
                                             proj_not_value_less<Comp, Proj, T>{comp, proj, value});
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            class T = std::projected_value_t<iterator_t<R>, Proj>,
            std::indirect_strict_weak_order<const T*, std::projected<iterator_t<R>, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr borrowed_iterator_t<R> operator()(R&& r, const T& value, Comp comp = {},
                                                           Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(comp), std::move(proj));
  }
};
struct equal_range_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>,
            std::indirect_strict_weak_order<const T*, std::projected<I, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr std::ranges::subrange<I> operator()(I first, S last, const T& value, Comp comp = {},
                                                              Proj proj = {}) const {
    auto n = std::ranges::distance(first, last);
    I lo = ::ycxx::detail::partition_point_n(first, n, proj_less_value<Comp, Proj, T>{comp, proj, value});
    n -= std::ranges::distance(first, lo);
    I hi = ::ycxx::detail::partition_point_n(lo, n, proj_not_value_less<Comp, Proj, T>{comp, proj, value});
    return {std::move(lo), std::move(hi)};
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            class T = std::projected_value_t<iterator_t<R>, Proj>,
            std::indirect_strict_weak_order<const T*, std::projected<iterator_t<R>, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr borrowed_subrange_t<R> operator()(R&& r, const T& value, Comp comp = {},
                                                           Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(comp), std::move(proj));
  }
};
struct binary_search_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            class T = std::projected_value_t<I, Proj>,
            std::indirect_strict_weak_order<const T*, std::projected<I, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(I first, S last, const T& value, Comp comp = {}, Proj proj = {}) const {
    auto n = std::ranges::distance(first, last);
    I i = ::ycxx::detail::partition_point_n(std::move(first), n, proj_less_value<Comp, Proj, T>{comp, proj, value});
    return i != last && !static_cast<bool>(::ycxx::detail::invoke(comp, value, ::ycxx::detail::invoke(proj, *i)));
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            class T = std::projected_value_t<iterator_t<R>, Proj>,
            std::indirect_strict_weak_order<const T*, std::projected<iterator_t<R>, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(R&& r, const T& value, Comp comp = {}, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(comp), std::move(proj));
  }
};

// [alg.partitions]
struct is_partitioned_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
  [[nodiscard]] constexpr bool operator()(I first, S last, Pred pred, Proj proj = {}) const {
    auto p = ::ycxx::detail::make_pred(pred, proj);
    first = ::ycxx::detail::find_if_impl(std::move(first), last, ::ycxx::detail::negated{p});
    if (first == last)
      return true;
    ++first;
    return ::ycxx::detail::find_if_impl(std::move(first), last, p) == last;
  }
  template <std::ranges::input_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
  [[nodiscard]] constexpr bool operator()(R&& r, Pred pred, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};
struct partition_fn {
  template <std::permutable I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
  constexpr std::ranges::subrange<I> operator()(I first, S last, Pred pred, Proj proj = {}) const {
    auto r = ::ycxx::detail::partition_impl<ranges_ops>(std::move(first), std::move(last),
                                                        ::ycxx::detail::make_pred(pred, proj));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
    requires std::permutable<iterator_t<R>>
  constexpr borrowed_subrange_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};
struct stable_partition_fn {
  template <std::bidirectional_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires std::permutable<I>
  constexpr std::ranges::subrange<I> operator()(I first, S last, Pred pred, Proj proj = {}) const {
    auto r = ::ycxx::detail::stable_partition_impl<ranges_ops>(std::move(first), std::move(last),
                                                               ::ycxx::detail::make_pred(pred, proj));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::bidirectional_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
    requires std::permutable<iterator_t<R>>
  constexpr borrowed_subrange_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};
struct partition_copy_fn {
  template <std::input_iterator I, std::sentinel_for<I> S, std::weakly_incrementable O1, std::weakly_incrementable O2,
            class Proj = std::identity, std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
    requires std::indirectly_copyable<I, O1> && std::indirectly_copyable<I, O2>
  constexpr std::ranges::partition_copy_result<I, O1, O2> operator()(I first, S last, O1 out_true, O2 out_false,
                                                                    Pred pred, Proj proj = {}) const {
    for (; first != last; ++first) {
      if (::ycxx::detail::invoke(pred, ::ycxx::detail::invoke(proj, *first))) {
        *out_true = *first;
        ++out_true;
      } else {
        *out_false = *first;
        ++out_false;
      }
    }
    return {std::move(first), std::move(out_true), std::move(out_false)};
  }
  template <std::ranges::input_range R, std::weakly_incrementable O1, std::weakly_incrementable O2,
            class Proj = std::identity, std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
    requires std::indirectly_copyable<iterator_t<R>, O1> && std::indirectly_copyable<iterator_t<R>, O2>
  constexpr std::ranges::partition_copy_result<borrowed_iterator_t<R>, O1, O2>
  operator()(R&& r, O1 out_true, O2 out_false, Pred pred, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(out_true), std::move(out_false),
                   std::move(pred), std::move(proj));
  }
};
struct partition_point_fn {
  template <std::forward_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
  [[nodiscard]] constexpr I operator()(I first, S last, Pred pred, Proj proj = {}) const {
    auto n = std::ranges::distance(first, last);
    return ::ycxx::detail::partition_point_n(std::move(first), n, ::ycxx::detail::make_pred(pred, proj));
  }
  template <std::ranges::forward_range R, class Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<R>, Proj>> Pred>
  [[nodiscard]] constexpr borrowed_iterator_t<R> operator()(R&& r, Pred pred, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};

// [alg.merge]
struct merge_fn {
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2, std::sentinel_for<I2> S2,
            std::weakly_incrementable O, class Comp = std::ranges::less, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires std::mergeable<I1, I2, O, Comp, Proj1, Proj2>
  constexpr std::ranges::merge_result<I1, I2, O> operator()(I1 first1, S1 last1, I2 first2, S2 last2, O result,
                                                           Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    ::ycxx::detail::merge_loop(first1, last1, first2, last2, result, ::ycxx::detail::make_comp2(comp, proj1, proj2));
    auto r1 = ::ycxx::detail::copy_dispatch(std::move(first1), std::move(last1), std::move(result));
    auto r2 = ::ycxx::detail::copy_dispatch(std::move(first2), std::move(last2), std::move(r1.second));
    return {std::move(r1.first), std::move(r2.first), std::move(r2.second)};
  }
  template <std::ranges::input_range R1, std::ranges::input_range R2, std::weakly_incrementable O,
            class Comp = std::ranges::less, class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::mergeable<iterator_t<R1>, iterator_t<R2>, O, Comp, Proj1, Proj2>
  constexpr std::ranges::merge_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>, O>
  operator()(R1&& r1, R2&& r2, O result, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                   std::move(result), std::move(comp), std::move(proj1), std::move(proj2));
  }
};
struct inplace_merge_fn {
  template <std::bidirectional_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires std::sortable<I, Comp, Proj>
  constexpr I operator()(I first, I middle, S last, Comp comp = {}, Proj proj = {}) const {
    I end = ::ycxx::detail::iter_at(middle, std::move(last));
    ::ycxx::detail::inplace_merge_impl<ranges_ops>(std::move(first), std::move(middle), end,
                                                   ::ycxx::detail::make_comp(comp, proj));
    return end;
  }
  template <std::ranges::bidirectional_range R, class Comp = std::ranges::less, class Proj = std::identity>
    requires std::sortable<iterator_t<R>, Comp, Proj>
  constexpr borrowed_iterator_t<R> operator()(R&& r, iterator_t<R> middle, Comp comp = {}, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::move(middle), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};

// [alg.set.operations]
struct includes_fn {
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2, std::sentinel_for<I2> S2,
            class Proj1 = std::identity, class Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<I1, Proj1>, std::projected<I2, Proj2>> Comp =
                std::ranges::less>
  [[nodiscard]] constexpr bool operator()(I1 first1, S1 last1, I2 first2, S2 last2, Comp comp = {}, Proj1 proj1 = {},
                                          Proj2 proj2 = {}) const {
    return ::ycxx::detail::includes_impl(std::move(first1), last1, std::move(first2), last2,
                                         ::ycxx::detail::make_comp2(comp, proj1, proj2));
  }
  template <std::ranges::input_range R1, std::ranges::input_range R2, class Proj1 = std::identity,
            class Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<R1>, Proj1>, std::projected<iterator_t<R2>, Proj2>>
                Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(R1&& r1, R2&& r2, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                   std::move(comp), std::move(proj1), std::move(proj2));
  }
};
struct set_union_fn {
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2, std::sentinel_for<I2> S2,
            std::weakly_incrementable O, class Comp = std::ranges::less, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires std::mergeable<I1, I2, O, Comp, Proj1, Proj2>
  constexpr std::ranges::set_union_result<I1, I2, O> operator()(I1 first1, S1 last1, I2 first2, S2 last2, O result,
                                                               Comp comp = {}, Proj1 proj1 = {},
                                                               Proj2 proj2 = {}) const {
    ::ycxx::detail::set_union_loop(first1, last1, first2, last2, result, ::ycxx::detail::make_comp2(comp, proj1, proj2));
    auto r1 = ::ycxx::detail::copy_dispatch(std::move(first1), std::move(last1), std::move(result));
    auto r2 = ::ycxx::detail::copy_dispatch(std::move(first2), std::move(last2), std::move(r1.second));
    return {std::move(r1.first), std::move(r2.first), std::move(r2.second)};
  }
  template <std::ranges::input_range R1, std::ranges::input_range R2, std::weakly_incrementable O,
            class Comp = std::ranges::less, class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::mergeable<iterator_t<R1>, iterator_t<R2>, O, Comp, Proj1, Proj2>
  constexpr std::ranges::set_union_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>, O>
  operator()(R1&& r1, R2&& r2, O result, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                   std::move(result), std::move(comp), std::move(proj1), std::move(proj2));
  }
};
struct set_intersection_fn {
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2, std::sentinel_for<I2> S2,
            std::weakly_incrementable O, class Comp = std::ranges::less, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires std::mergeable<I1, I2, O, Comp, Proj1, Proj2>
  constexpr std::ranges::set_intersection_result<I1, I2, O> operator()(I1 first1, S1 last1, I2 first2, S2 last2,
                                                                      O result, Comp comp = {}, Proj1 proj1 = {},
                                                                      Proj2 proj2 = {}) const {
    ::ycxx::detail::set_intersection_loop(first1, last1, first2, last2, result,
                                          ::ycxx::detail::make_comp2(comp, proj1, proj2));
    I1 e1 = ::ycxx::detail::iter_at(std::move(first1), std::move(last1));
    I2 e2 = ::ycxx::detail::iter_at(std::move(first2), std::move(last2));
    return {std::move(e1), std::move(e2), std::move(result)};
  }
  template <std::ranges::input_range R1, std::ranges::input_range R2, std::weakly_incrementable O,
            class Comp = std::ranges::less, class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::mergeable<iterator_t<R1>, iterator_t<R2>, O, Comp, Proj1, Proj2>
  constexpr std::ranges::set_intersection_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>, O>
  operator()(R1&& r1, R2&& r2, O result, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                   std::move(result), std::move(comp), std::move(proj1), std::move(proj2));
  }
};
struct set_difference_fn {
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2, std::sentinel_for<I2> S2,
            std::weakly_incrementable O, class Comp = std::ranges::less, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires std::mergeable<I1, I2, O, Comp, Proj1, Proj2>
  constexpr std::ranges::set_difference_result<I1, O> operator()(I1 first1, S1 last1, I2 first2, S2 last2, O result,
                                                                Comp comp = {}, Proj1 proj1 = {},
                                                                Proj2 proj2 = {}) const {
    ::ycxx::detail::set_difference_loop(first1, last1, first2, last2, result,
                                        ::ycxx::detail::make_comp2(comp, proj1, proj2));
    auto r = ::ycxx::detail::copy_dispatch(std::move(first1), std::move(last1), std::move(result));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::input_range R1, std::ranges::input_range R2, std::weakly_incrementable O,
            class Comp = std::ranges::less, class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::mergeable<iterator_t<R1>, iterator_t<R2>, O, Comp, Proj1, Proj2>
  constexpr std::ranges::set_difference_result<borrowed_iterator_t<R1>, O>
  operator()(R1&& r1, R2&& r2, O result, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                   std::move(result), std::move(comp), std::move(proj1), std::move(proj2));
  }
};
struct set_symmetric_difference_fn {
  template <std::input_iterator I1, std::sentinel_for<I1> S1, std::input_iterator I2, std::sentinel_for<I2> S2,
            std::weakly_incrementable O, class Comp = std::ranges::less, class Proj1 = std::identity,
            class Proj2 = std::identity>
    requires std::mergeable<I1, I2, O, Comp, Proj1, Proj2>
  constexpr std::ranges::set_symmetric_difference_result<I1, I2, O> operator()(I1 first1, S1 last1, I2 first2,
                                                                              S2 last2, O result, Comp comp = {},
                                                                              Proj1 proj1 = {},
                                                                              Proj2 proj2 = {}) const {
    ::ycxx::detail::set_symmetric_difference_loop(first1, last1, first2, last2, result,
                                                  ::ycxx::detail::make_comp2(comp, proj1, proj2));
    auto r1 = ::ycxx::detail::copy_dispatch(std::move(first1), std::move(last1), std::move(result));
    auto r2 = ::ycxx::detail::copy_dispatch(std::move(first2), std::move(last2), std::move(r1.second));
    return {std::move(r1.first), std::move(r2.first), std::move(r2.second)};
  }
  template <std::ranges::input_range R1, std::ranges::input_range R2, std::weakly_incrementable O,
            class Comp = std::ranges::less, class Proj1 = std::identity, class Proj2 = std::identity>
    requires std::mergeable<iterator_t<R1>, iterator_t<R2>, O, Comp, Proj1, Proj2>
  constexpr std::ranges::set_symmetric_difference_result<borrowed_iterator_t<R1>, borrowed_iterator_t<R2>, O>
  operator()(R1&& r1, R2&& r2, O result, Comp comp = {}, Proj1 proj1 = {}, Proj2 proj2 = {}) const {
    return (*this)(std::ranges::begin(r1), std::ranges::end(r1), std::ranges::begin(r2), std::ranges::end(r2),
                   std::move(result), std::move(comp), std::move(proj1), std::move(proj2));
  }
};

// [alg.heap.operations]
template <int Op>
struct heap_op_fn {
  template <std::random_access_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires std::sortable<I, Comp, Proj>
  constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
    I end = ::ycxx::detail::iter_at(first, std::move(last));
    auto less = ::ycxx::detail::make_comp(comp, proj);
    if constexpr (Op == 0)
      ::ycxx::detail::push_heap_impl<ranges_ops>(std::move(first), end, less);
    else if constexpr (Op == 1)
      ::ycxx::detail::pop_heap_impl<ranges_ops>(std::move(first), end, less);
    else if constexpr (Op == 2)
      ::ycxx::detail::make_heap_impl<ranges_ops>(std::move(first), end, less);
    else
      ::ycxx::detail::sort_heap_impl<ranges_ops>(std::move(first), end, less);
    return end;
  }
  template <std::ranges::random_access_range R, class Comp = std::ranges::less, class Proj = std::identity>
    requires std::sortable<iterator_t<R>, Comp, Proj>
  constexpr borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct is_heap_until_fn {
  template <std::random_access_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr I operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
    I end = ::ycxx::detail::iter_at(first, std::move(last));
    return ::ycxx::detail::is_heap_until_impl(std::move(first), end, ::ycxx::detail::make_comp(comp, proj));
  }
  template <std::ranges::random_access_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<R>, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr borrowed_iterator_t<R> operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct is_heap_fn {
  template <std::random_access_iterator I, std::sentinel_for<I> S, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<I, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
    I end = ::ycxx::detail::iter_at(first, std::move(last));
    return ::ycxx::detail::is_heap_until_impl(std::move(first), end, ::ycxx::detail::make_comp(comp, proj)) == end;
  }
  template <std::ranges::random_access_range R, class Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<R>, Proj>> Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(R&& r, Comp comp = {}, Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};

// [alg.permutation.generators]
template <bool Next>
struct permutation_fn {
  template <std::bidirectional_iterator I, std::sentinel_for<I> S, class Comp = std::ranges::less,
            class Proj = std::identity>
    requires std::sortable<I, Comp, Proj>
  constexpr std::ranges::in_found_result<I> operator()(I first, S last, Comp comp = {}, Proj proj = {}) const {
    I end = ::ycxx::detail::iter_at(first, std::move(last));
    bool found;
    if constexpr (Next)
      found = ::ycxx::detail::next_permutation_impl<ranges_ops>(std::move(first), end,
                                                                ::ycxx::detail::make_comp(comp, proj));
    else
      found = ::ycxx::detail::prev_permutation_impl<ranges_ops>(std::move(first), end,
                                                                ::ycxx::detail::make_comp(comp, proj));
    return {std::move(end), found};
  }
  template <std::ranges::bidirectional_range R, class Comp = std::ranges::less, class Proj = std::identity>
    requires std::sortable<iterator_t<R>, Comp, Proj>
  constexpr std::ranges::in_found_result<borrowed_iterator_t<R>> operator()(R&& r, Comp comp = {},
                                                                           Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};

}} // namespace ycxx::detail::ranges_algo

namespace [[gnu::visibility("hidden")]] std { namespace ranges {
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::sort_fn, ycxx::detail::par::kind::sort> sort{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::stable_sort_fn, ycxx::detail::par::kind::stable_sort> stable_sort{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::partial_sort_fn, ycxx::detail::par::kind::partial_sort> partial_sort{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::partial_sort_copy_fn, ycxx::detail::par::kind::partial_sort_copy> partial_sort_copy{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::is_sorted_fn, ycxx::detail::par::kind::is_sorted> is_sorted{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::is_sorted_until_fn, ycxx::detail::par::kind::is_sorted_until> is_sorted_until{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::nth_element_fn, ycxx::detail::par::kind::nth_element> nth_element{};
inline constexpr ycxx::detail::ranges_algo::lower_bound_fn lower_bound{};
inline constexpr ycxx::detail::ranges_algo::upper_bound_fn upper_bound{};
inline constexpr ycxx::detail::ranges_algo::equal_range_fn equal_range{};
inline constexpr ycxx::detail::ranges_algo::binary_search_fn binary_search{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::is_partitioned_fn, ycxx::detail::par::kind::is_partitioned> is_partitioned{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::partition_fn, ycxx::detail::par::kind::partition> partition{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::stable_partition_fn, ycxx::detail::par::kind::stable_partition> stable_partition{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::partition_copy_fn, ycxx::detail::par::kind::partition_copy> partition_copy{};
inline constexpr ycxx::detail::ranges_algo::partition_point_fn partition_point{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::merge_fn, ycxx::detail::par::kind::merge> merge{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::inplace_merge_fn, ycxx::detail::par::kind::inplace_merge> inplace_merge{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::includes_fn, ycxx::detail::par::kind::includes> includes{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::set_union_fn, ycxx::detail::par::kind::set_union> set_union{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::set_intersection_fn, ycxx::detail::par::kind::set_intersection> set_intersection{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::set_difference_fn, ycxx::detail::par::kind::set_difference> set_difference{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::set_symmetric_difference_fn, ycxx::detail::par::kind::set_symmetric_difference> set_symmetric_difference{};
inline constexpr ycxx::detail::ranges_algo::heap_op_fn<0> push_heap{};
inline constexpr ycxx::detail::ranges_algo::heap_op_fn<1> pop_heap{};
inline constexpr ycxx::detail::ranges_algo::heap_op_fn<2> make_heap{};
inline constexpr ycxx::detail::ranges_algo::heap_op_fn<3> sort_heap{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::is_heap_fn, ycxx::detail::par::kind::is_heap> is_heap{};
inline constexpr ycxx::adl_free::ranges_par_algo<ycxx::detail::ranges_algo::is_heap_until_fn, ycxx::detail::par::kind::is_heap_until> is_heap_until{};
inline constexpr ycxx::detail::ranges_algo::permutation_fn<true> next_permutation{};
inline constexpr ycxx::detail::ranges_algo::permutation_fn<false> prev_permutation{};
}} // namespace std::ranges
