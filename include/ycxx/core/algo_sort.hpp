// libycxx core: sorting and related operations ([alg.sorting]) other than min/max: sort,
// stable_sort, partial_sort, nth_element, binary search, partitions, merge, set operations,
// heap operations and permutations, in both the std:: and the std::ranges:: forms.
//
// sort is pattern-defeating quicksort (below, with branchless block partitioning for contiguous
// ranges of scalars); nth_element is introselect (median-of-three or ninther pivots, Hoare
// partitioning, heapsort when the recursion gets too deep). stable_sort, stable_partition and
// inplace_merge use a temporary buffer (operator new(nothrow) at run time, std::allocator in
// constant evaluation) and fall back to in-place rotation-based algorithms when none is had.
#pragma once

#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/algo_mutate.hpp>
#include <ycxx/core/new.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// ---- temporary buffer --------------------------------------------------------------------
// Uninitialized storage for up to `capacity` objects of type T. At run time it comes from
// operator new(nothrow); requests that cannot be met are halved until they can, and capacity 0
// means no buffer.
template <class _Tp>
class __temp_buffer {
  _Tp* __data_ = nullptr;
  std::ptrdiff_t __capacity_ = 0;

  static constexpr bool __overaligned = alignof(_Tp) > __cfg::__default_new_alignment;

public:
  constexpr explicit __temp_buffer(std::ptrdiff_t __want) noexcept {
    if consteval {
      // Constant evaluation: std::allocator is the only allocation available, and it cannot
      // fail short of the evaluation itself failing.
      if (__want > 0) {
        __data_ = std::allocator<_Tp>().allocate(static_cast<std::size_t>(__want));
        __capacity_ = __want;
      }
    } else {
      constexpr std::ptrdiff_t __max_count = static_cast<std::ptrdiff_t>(~std::size_t(0) / 2 / sizeof(_Tp));
      if (__want > __max_count)
        __want = __max_count;
      for (; __want > 0; __want /= 2) {
        void* p;
        if constexpr (__overaligned)
          p = ::operator new(static_cast<std::size_t>(__want) * sizeof(_Tp), std::align_val_t(alignof(_Tp)), std::nothrow);
        else
          p = ::operator new(static_cast<std::size_t>(__want) * sizeof(_Tp), std::nothrow);
        if (p) {
          __data_ = static_cast<_Tp*>(p);
          __capacity_ = __want;
          return;
        }
      }
    }
  }
  __temp_buffer(const __temp_buffer&) = delete;
  __temp_buffer& operator=(const __temp_buffer&) = delete;
  constexpr ~__temp_buffer() {
    if consteval {
      if (__data_)
        std::allocator<_Tp>().deallocate(__data_, static_cast<std::size_t>(__capacity_));
    } else {
      if (__data_) {
        if constexpr (__overaligned)
          ::operator delete(__data_, std::align_val_t(alignof(_Tp)));
        else
          ::operator delete(__data_);
      }
    }
  }
  constexpr _Tp* data() const noexcept { return __data_; }
  constexpr std::ptrdiff_t capacity() const noexcept { return __capacity_; }
};

// Destroys the objects [p, p + n) of a temporary buffer on scope exit.
template <class _Tp>
struct __buffer_objects {
  _Tp* p;
  std::ptrdiff_t n = 0;
  constexpr ~__buffer_objects() {
    for (std::ptrdiff_t i = 0; i != n; ++i)
      p[i].~_Tp();
  }
};

// Moves [first, last) into the uninitialized buffer `__objs` (counting as it goes).
template <class _Ops, class _Ip, class _Tp>
constexpr void __move_into_buffer(_Ip first, _Ip last, __buffer_objects<_Tp>& __objs) {
  for (; first != last; ++first) {
    std::construct_at(__objs.p + __objs.n, _Ops::iter_move(first));
    ++__objs.n;
  }
}

// The iterator arithmetic below uses difference_type operands only: an iterator need not
// accept an int ([iterator.requirements.general]; libc++'s robust_re_difference_type).
template <class _Ip>
inline constexpr std::iter_difference_t<_Ip> __diff_one = 1;

constexpr int __floor_log2(unsigned long long n) noexcept {
  int r = 0;
  while (n > 1) {
    n >>= 1;
    ++r;
  }
  return r;
}

// ---- insertion sort ------------------------------------------------------------------------
template <class _Ops, class _Ip, class _Cp>
constexpr void __insertion_sort(_Ip first, _Ip last, _Cp less) {
  if (first == last)
    return;
  for (_Ip i = first + ::__ycxx::__detail::__diff_one<_Ip>; i != last; ++i) {
    _Ip __j = i;
    if (!less(*i, *(__j - ::__ycxx::__detail::__diff_one<_Ip>)))
      continue;
    std::iter_value_t<_Ip> __tmp(_Ops::iter_move(i));
    do {
      _Ip k = __j;
      --k;
      *__j = _Ops::iter_move(k);
      __j = k;
    } while (__j != first && less(__tmp, *(__j - ::__ycxx::__detail::__diff_one<_Ip>)));
    *__j = std::move(__tmp);
  }
}

// Binary insertion sort: fewer comparisons, used for the leaves of stable_sort. Stable: an
// element goes after every element it is not less than.
template <class _Ops, class _Ip, class _Cp>
constexpr void __binary_insertion_sort(_Ip first, _Ip last, _Cp less) {
  if (first == last)
    return;
  for (_Ip i = first + ::__ycxx::__detail::__diff_one<_Ip>; i != last; ++i) {
    if (!less(*i, *(i - ::__ycxx::__detail::__diff_one<_Ip>)))
      continue;
    // upper bound of *i in [first, i - 1)
    _Ip __lo = first;
    auto n = (i - ::__ycxx::__detail::__diff_one<_Ip>) - first;
    while (n > 0) {
      auto __half = n / 2;
      _Ip __mid = __lo + __half;
      if (less(*i, *__mid)) {
        n = __half;
      } else {
        __lo = __mid + ::__ycxx::__detail::__diff_one<_Ip>;
        n -= __half + 1;
      }
    }
    std::iter_value_t<_Ip> __tmp(_Ops::iter_move(i));
    for (_Ip __j = i; __j != __lo;) {
      _Ip k = __j;
      *__j = _Ops::iter_move(--k);
      __j = k;
    }
    *__lo = std::move(__tmp);
  }
}

// ---- heaps ---------------------------------------------------------------------------------
// Moves `value` up from the hole at `__hole` (a max-heap on [first, ...)).
template <class _Ops, class _Ip, class _Cp>
constexpr void __heap_push_hole(_Ip first, std::iter_difference_t<_Ip> __hole, std::iter_value_t<_Ip>& value, _Cp less) {
  while (__hole > 0) {
    auto __parent = (__hole - 1) / 2;
    _Ip p = first + __parent;
    if (!less(*p, value))
      break;
    *(first + __hole) = _Ops::iter_move(p);
    __hole = __parent;
  }
  *(first + __hole) = std::move(value);
}
// Moves `value` down from the hole at `__hole` in a heap of `__len` elements: two comparisons per
// level.
template <class _Ops, class _Ip, class _Cp>
constexpr void __heap_sift_hole(_Ip first, std::iter_difference_t<_Ip> __len, std::iter_difference_t<_Ip> __hole,
                              std::iter_value_t<_Ip>& value, _Cp less) {
  for (;;) {
    auto __child = 2 * __hole + 1;
    if (__child >= __len)
      break;
    _Ip c = first + __child;
    if (__child + 1 < __len && less(*c, *(c + ::__ycxx::__detail::__diff_one<_Ip>))) {
      ++__child;
      ++c;
    }
    if (!less(value, *c))
      break;
    *(first + __hole) = _Ops::iter_move(c);
    __hole = __child;
  }
  *(first + __hole) = std::move(value);
}
// pop_heap's sift (Floyd): the hole first goes down to a leaf along the larger children (one
// comparison per level), then `value`, which comes from the back of the heap and so usually
// belongs near the bottom, moves up from there.
template <class _Ops, class _Ip, class _Cp>
constexpr void __heap_sift_leaf(_Ip first, std::iter_difference_t<_Ip> __len, std::iter_difference_t<_Ip> __hole,
                              std::iter_value_t<_Ip>& value, _Cp less) {
  const auto top = __hole;
  // While both children exist; the choice is arithmetic, not a branch (it is unpredictable).
  for (auto __child = 2 * __hole + 2; __child < __len; __child = 2 * __hole + 2) {
    __child -= static_cast<bool>(less(*(first + __child), *(first + (__child - 1))));
    _Ip c = first + __child;
    *(first + __hole) = _Ops::iter_move(c);
    __hole = __child;
  }
  if (2 * __hole + 2 == __len) { // a last, lone left child
    _Ip c = first + (__len - 1);
    *(first + __hole) = _Ops::iter_move(c);
    __hole = __len - 1;
  }
  while (__hole > top) {
    const auto __parent = (__hole - 1) / 2;
    _Ip p = first + __parent;
    if (!less(*p, value))
      break;
    *(first + __hole) = _Ops::iter_move(p);
    __hole = __parent;
  }
  *(first + __hole) = std::move(value);
}
template <class _Ops, class _Ip, class _Cp>
constexpr void __push_heap_impl(_Ip first, _Ip last, _Cp less) {
  auto n = last - first;
  if (n < 2)
    return;
  _Ip back = last - ::__ycxx::__detail::__diff_one<_Ip>;
  std::iter_value_t<_Ip> __v(_Ops::iter_move(back));
  ::__ycxx::__detail::__heap_push_hole<_Ops>(first, n - 1, __v, less);
}
template <class _Ops, class _Ip, class _Cp>
constexpr void __pop_heap_impl(_Ip first, _Ip last, _Cp less) {
  auto n = last - first;
  if (n < 2)
    return;
  _Ip back = last - ::__ycxx::__detail::__diff_one<_Ip>;
  std::iter_value_t<_Ip> __v(_Ops::iter_move(back));
  *back = _Ops::iter_move(first);
  ::__ycxx::__detail::__heap_sift_leaf<_Ops>(first, n - 1, 0, __v, less);
}
template <class _Ops, class _Ip, class _Cp>
constexpr void __make_heap_impl(_Ip first, _Ip last, _Cp less) {
  auto n = last - first;
  if (n < 2)
    return;
  for (auto start = (n - 2) / 2;; --start) {
    _Ip s = first + start;
    std::iter_value_t<_Ip> __v(_Ops::iter_move(s));
    ::__ycxx::__detail::__heap_sift_hole<_Ops>(first, n, start, __v, less);
    if (start == 0)
      break;
  }
}
template <class _Ops, class _Ip, class _Cp>
constexpr void __sort_heap_impl(_Ip first, _Ip last, _Cp less) {
  for (; last - first > 1; --last)
    ::__ycxx::__detail::__pop_heap_impl<_Ops>(first, last, less);
}
template <class _Ip, class _Cp>
constexpr _Ip __is_heap_until_impl(_Ip first, _Ip last, _Cp less) {
  auto n = last - first;
  for (std::iter_difference_t<_Ip> i = 1; i < n; ++i)
    if (less(*(first + (i - 1) / 2), *(first + i)))
      return first + i;
  return last;
}

// ---- sort / nth_element --------------------------------------------------------------------
template <class _Ops, class _Ip, class _Cp>
constexpr void __sort3(_Ip a, _Ip b, _Ip c, _Cp less) {
  if (less(*b, *a))
    _Ops::iter_swap(a, b);
  if (less(*c, *b)) {
    _Ops::iter_swap(b, c);
    if (less(*b, *a))
      _Ops::iter_swap(a, b);
  }
}

// Chooses a pivot, moves it to *first and partitions [first + 1, last); returns cut such that
// every element of [first, cut) is not greater and every element of [cut, last) not less than
// the pivot, with first < cut < last. Needs last - first >= 4.
template <class _Ops, class _Ip, class _Cp>
constexpr _Ip __partition_pivot(_Ip first, _Ip last, _Cp less) {
  using _Dp = std::iter_difference_t<_Ip>;
  _Dp n = last - first;
  _Ip __mid = first + n / 2;
  _Ip __f1 = first + _Dp(1); // the first and last elements of [first + 1, last)
  _Ip __l1 = last - _Dp(1);
  if (n > 128) { // ninther
    _Dp s = n / 8;
    ::__ycxx::__detail::__sort3<_Ops>(__f1, __f1 + s, __f1 + 2 * s, less);
    ::__ycxx::__detail::__sort3<_Ops>(__mid - s, __mid, __mid + s, less);
    ::__ycxx::__detail::__sort3<_Ops>(__l1 - 2 * s, __l1 - s, __l1, less);
    ::__ycxx::__detail::__sort3<_Ops>(__f1 + s, __mid, __l1 - s, less);
  } else {
    ::__ycxx::__detail::__sort3<_Ops>(__f1, __mid, __l1, less);
  }
  // Now an element not greater than the pivot lies left of mid and one not less right of it;
  // they bound both scans below.
  _Ops::iter_swap(first, __mid);
  _Ip __lo = __f1;
  _Ip __hi = __l1;
  for (;;) {
    while (less(*__lo, *first))
      ++__lo;
    while (less(*first, *__hi))
      --__hi;
    if (!(__lo < __hi))
      return __lo;
    _Ops::iter_swap(__lo, __hi);
    ++__lo;
    --__hi;
  }
}

template <class _Ops, class _Ip, class _Cp>
constexpr void __heap_sort(_Ip first, _Ip last, _Cp less) {
  ::__ycxx::__detail::__make_heap_impl<_Ops>(first, last, less);
  ::__ycxx::__detail::__sort_heap_impl<_Ops>(first, last, less);
}

inline constexpr int __insertion_sort_threshold = 16;

// ---- sort: pattern-defeating quicksort --------------------------------------------------------
// O. R. L. Peters, "Pattern-defeating Quicksort" (arXiv:2106.05123), with the branchless block
// partitioning of S. Edelkamp and A. Weiss, "BlockQuicksort: Avoiding Branch Mispredictions in
// Quicksort" (ESA 2016, JEA 2019) for contiguous ranges of scalars:
//   - median of three, or Tukey's ninther above 128 elements, as the pivot;
//   - a partition that swapped nothing (sorted or nearly sorted input) is followed by an
//     insertion sort that gives up after 8 moves, so sorted runs cost O(n);
//   - when the element left of the range equals the pivot (it is a previous pivot, not greater
//     than anything in the range), the elements equal to it are split off at once, so many
//     duplicates cost O(n log k) for k distinct values;
//   - a partition leaving fewer than 1/8 on one side swaps a few elements to break patterns,
//     and after log2(n) such partitions the range is heapsorted: O(n log n) comparisons in the
//     worst case ([sort]/5).
inline constexpr std::ptrdiff_t __pdq_insertion_threshold = 24;
inline constexpr std::ptrdiff_t __pdq_ninther_threshold = 128;
inline constexpr std::ptrdiff_t __pdq_partial_insertion_limit = 8;
inline constexpr int __pdq_block = 64;

// Insertion sort of [first, last) whose element before first is not greater than any of them
// (a previous pivot), so the inner loop needs no bound check.
template <class _Ops, class _Ip, class _Cp>
constexpr void __unguarded_insertion_sort(_Ip first, _Ip last, _Cp less) {
  if (first == last)
    return;
  for (_Ip i = first + ::__ycxx::__detail::__diff_one<_Ip>; i != last; ++i) {
    _Ip __j = i;
    _Ip k = i - ::__ycxx::__detail::__diff_one<_Ip>;
    if (!less(*__j, *k))
      continue;
    std::iter_value_t<_Ip> __tmp(_Ops::iter_move(__j));
    do {
      *__j = _Ops::iter_move(k);
      __j = k;
      --k;
    } while (less(__tmp, *k));
    *__j = std::move(__tmp);
  }
}

// Insertion sort that gives up after moving elements __pdq_partial_insertion_limit places in
// all; returns whether [first, last) is sorted.
template <class _Ops, class _Ip, class _Cp>
constexpr bool __partial_insertion_sort(_Ip first, _Ip last, _Cp less) {
  if (first == last)
    return true;
  std::iter_difference_t<_Ip> __moved = 0;
  for (_Ip i = first + ::__ycxx::__detail::__diff_one<_Ip>; i != last; ++i) {
    if (__moved > __pdq_partial_insertion_limit)
      return false;
    _Ip __j = i;
    _Ip k = i - ::__ycxx::__detail::__diff_one<_Ip>;
    if (!less(*__j, *k))
      continue;
    std::iter_value_t<_Ip> __tmp(_Ops::iter_move(__j));
    do {
      *__j = _Ops::iter_move(k);
      __j = k;
    } while (__j != first && less(__tmp, *--k));
    *__j = std::move(__tmp);
    __moved += i - __j;
  }
  return true;
}

// Partitions [first, last) around the pivot *first: returns the pivot's final position p
// (every element left of it less than the pivot, every element right of it not less) and
// whether nothing had to be swapped. The pivot choice left an element not less than the pivot
// in (first, last), which bounds the first scan.
template <class _Ops, class _Ip, class _Cp>
constexpr std::pair<_Ip, bool> __partition_right(_Ip __begin, _Ip __end, _Cp less) {
  std::iter_value_t<_Ip> __pivot(_Ops::iter_move(__begin));
  _Ip first = __begin;
  _Ip last = __end;
  while (less(*++first, __pivot)) {
  }
  if (first - ::__ycxx::__detail::__diff_one<_Ip> == __begin) {
    while (first < last && !less(*--last, __pivot)) {
    }
  } else {
    while (!less(*--last, __pivot)) {
    }
  }
  const bool __already = !(first < last);
  while (first < last) {
    _Ops::iter_swap(first, last);
    while (less(*++first, __pivot)) {
    }
    while (!less(*--last, __pivot)) {
    }
  }
  _Ip __pos = first - ::__ycxx::__detail::__diff_one<_Ip>;
  *__begin = _Ops::iter_move(__pos);
  *__pos = std::move(__pivot);
  return {__pos, __already};
}

// Swaps the misplaced elements at first + ol[i] and last - or[i], i < n: as a cycle of moves
// (one temporary) or, when both sides have the same count, as swaps (which keeps descending
// input linear).
template <class _Ops, class _Ip>
constexpr void __swap_offsets(_Ip first, _Ip last, const unsigned char* __ol, const unsigned char* __or, int n, bool __swaps) {
  using _Dp = std::iter_difference_t<_Ip>;
  if (__swaps) {
    for (int i = 0; i < n; ++i) {
      _Ip __l = first + _Dp(__ol[i]);
      _Ip r = last - _Dp(__or[i]);
      _Ops::iter_swap(__l, r);
    }
  } else if (n > 0) {
    _Ip __l = first + _Dp(__ol[0]);
    _Ip r = last - _Dp(__or[0]);
    std::iter_value_t<_Ip> __tmp(_Ops::iter_move(__l));
    *__l = _Ops::iter_move(r);
    for (int i = 1; i < n; ++i) {
      __l = first + _Dp(__ol[i]);
      *r = _Ops::iter_move(__l);
      r = last - _Dp(__or[i]);
      *__l = _Ops::iter_move(r);
    }
    *r = std::move(__tmp);
  }
}

// __partition_right with branchless block partitioning: the scans record the offsets of the
// misplaced elements of a block of each side in byte arrays without branching on the
// comparisons, then the recorded elements are exchanged.
template <class _Ops, class _Ip, class _Cp>
constexpr std::pair<_Ip, bool> __partition_right_branchless(_Ip __begin, _Ip __end, _Cp less) {
  using _Dp = std::iter_difference_t<_Ip>;
  constexpr int _Bk = __pdq_block;
  std::iter_value_t<_Ip> __pivot(_Ops::iter_move(__begin));
  _Ip first = __begin;
  _Ip last = __end;
  while (less(*++first, __pivot)) {
  }
  if (first - ::__ycxx::__detail::__diff_one<_Ip> == __begin) {
    while (first < last && !less(*--last, __pivot)) {
    }
  } else {
    while (!less(*--last, __pivot)) {
    }
  }
  const bool __already = !(first < last);
  if (!__already) {
    _Ops::iter_swap(first, last);
    ++first;
    // [first, last) is unpartitioned; [__begin + 1, first) is less than the pivot and
    // [last, __end) is not.
    [[indeterminate]] unsigned char __offl[_Bk];
    [[indeterminate]] unsigned char __offr[_Bk];
    int __numl = 0, __numr = 0, __startl = 0, __startr = 0;
    while (last - first > 2 * _Bk) {
      if (__numl == 0) {
        __startl = 0;
        _Ip __it = first;
        for (int i = 0; i < _Bk; ++i, ++__it) {
          __offl[__numl] = static_cast<unsigned char>(i);
          __numl += !less(*__it, __pivot);
        }
      }
      if (__numr == 0) {
        __startr = 0;
        _Ip __it = last;
        for (int i = 0; i < _Bk;) {
          __offr[__numr] = static_cast<unsigned char>(++i);
          __numr += less(*--__it, __pivot);
        }
      }
      const int n = __numl < __numr ? __numl : __numr;
      ::__ycxx::__detail::__swap_offsets<_Ops>(first, last, __offl + __startl, __offr + __startr, n, __numl == __numr);
      __numl -= n;
      __numr -= n;
      __startl += n;
      __startr += n;
      if (__numl == 0)
        first += _Dp(_Bk);
      if (__numr == 0)
        last -= _Dp(_Bk);
    }
    // At most two blocks are left, one of which may still hold recorded offsets.
    int __sizel = 0, __sizer = 0;
    const int __unknown = static_cast<int>(last - first) - ((__numr != 0 || __numl != 0) ? _Bk : 0);
    if (__numr != 0) {
      __sizel = __unknown;
      __sizer = _Bk;
    } else if (__numl != 0) {
      __sizel = _Bk;
      __sizer = __unknown;
    } else {
      __sizel = __unknown / 2;
      __sizer = __unknown - __sizel;
    }
    if (__unknown != 0 && __numl == 0) {
      __startl = 0;
      _Ip __it = first;
      for (int i = 0; i < __sizel; ++i, ++__it) {
        __offl[__numl] = static_cast<unsigned char>(i);
        __numl += !less(*__it, __pivot);
      }
    }
    if (__unknown != 0 && __numr == 0) {
      __startr = 0;
      _Ip __it = last;
      for (int i = 0; i < __sizer;) {
        __offr[__numr] = static_cast<unsigned char>(++i);
        __numr += less(*--__it, __pivot);
      }
    }
    const int n = __numl < __numr ? __numl : __numr;
    ::__ycxx::__detail::__swap_offsets<_Ops>(first, last, __offl + __startl, __offr + __startr, n, __numl == __numr);
    __numl -= n;
    __numr -= n;
    __startl += n;
    __startr += n;
    if (__numl == 0)
      first += _Dp(__sizel);
    if (__numr == 0)
      last -= _Dp(__sizer);
    // One side's leftovers go to the far end of what remains unknown.
    if (__numl != 0) {
      while (__numl-- != 0) {
        _Ip __l = first + _Dp(__offl[__startl + __numl]);
        --last;
        _Ops::iter_swap(__l, last);
      }
      first = last;
    }
    if (__numr != 0) {
      while (__numr-- != 0) {
        _Ip r = last - _Dp(__offr[__startr + __numr]);
        _Ops::iter_swap(r, first);
        ++first;
      }
      last = first;
    }
  }
  _Ip __pos = first - ::__ycxx::__detail::__diff_one<_Ip>;
  *__begin = _Ops::iter_move(__pos);
  *__pos = std::move(__pivot);
  return {__pos, __already};
}

// Partitions [first, last) around the pivot *first with the elements equal to it on the left;
// returns the pivot's final position. Used when the pivot equals the element before first, a
// previous pivot not greater than anything here: everything left of the result equals it.
template <class _Ops, class _Ip, class _Cp>
constexpr _Ip __partition_left(_Ip __begin, _Ip __end, _Cp less) {
  std::iter_value_t<_Ip> __pivot(_Ops::iter_move(__begin));
  _Ip first = __begin;
  _Ip last = __end;
  while (less(__pivot, *--last)) {
  }
  if (last + ::__ycxx::__detail::__diff_one<_Ip> == __end) {
    while (first < last && !less(__pivot, *++first)) {
    }
  } else {
    while (!less(__pivot, *++first)) {
    }
  }
  while (first < last) {
    _Ops::iter_swap(first, last);
    while (less(__pivot, *--last)) {
    }
    while (!less(__pivot, *++first)) {
    }
  }
  _Ip __pos = last;
  *__begin = _Ops::iter_move(__pos);
  *__pos = std::move(__pivot);
  return __pos;
}

// Contiguous ranges of scalars (arithmetic, enumeration, pointer) take the branchless
// partition: their comparisons are cheap and their moves are copies.
template <class _Ip>
concept __pdq_branchless = std::contiguous_iterator<_Ip> && std::is_scalar_v<std::iter_value_t<_Ip>>;

template <class _Ops, bool _Branchless, class _Ip, class _Cp>
constexpr void __pdqsort_loop(_Ip __begin, _Ip __end, _Cp less, int __bad_allowed, bool __leftmost) {
  using _Dp = std::iter_difference_t<_Ip>;
  for (;;) {
    const _Dp __size = __end - __begin;
    if (__size < __pdq_insertion_threshold) {
      if (__leftmost)
        ::__ycxx::__detail::__insertion_sort<_Ops>(__begin, __end, less);
      else
        ::__ycxx::__detail::__unguarded_insertion_sort<_Ops>(__begin, __end, less);
      return;
    }
    // The pivot goes to *begin; the median of three leaves an element not less than it at
    // end - 1 and one not greater at begin + s2 (the ninther: at begin + s2 + 1 and
    // begin + s2 - 1), which bound the partitions' first scans.
    const _Dp __s2 = __size / 2;
    if (__size > __pdq_ninther_threshold) {
      ::__ycxx::__detail::__sort3<_Ops>(__begin, __begin + __s2, __end - _Dp(1), less);
      ::__ycxx::__detail::__sort3<_Ops>(__begin + _Dp(1), __begin + (__s2 - 1), __end - _Dp(2), less);
      ::__ycxx::__detail::__sort3<_Ops>(__begin + _Dp(2), __begin + (__s2 + 1), __end - _Dp(3), less);
      ::__ycxx::__detail::__sort3<_Ops>(__begin + (__s2 - 1), __begin + __s2, __begin + (__s2 + 1), less);
      _Ip __m = __begin + __s2;
      _Ops::iter_swap(__begin, __m);
    } else {
      ::__ycxx::__detail::__sort3<_Ops>(__begin + __s2, __begin, __end - _Dp(1), less);
    }
    // The element before the range is a previous pivot, not greater than the pivot; if it is
    // not less either, they are equal and so is everything that partition_left puts left.
    if (!__leftmost && !less(*(__begin - _Dp(1)), *__begin)) {
      __begin = ::__ycxx::__detail::__partition_left<_Ops>(__begin, __end, less) + _Dp(1);
      continue;
    }
    std::pair<_Ip, bool> __part = [&] {
      if constexpr (_Branchless)
        return ::__ycxx::__detail::__partition_right_branchless<_Ops>(__begin, __end, less);
      else
        return ::__ycxx::__detail::__partition_right<_Ops>(__begin, __end, less);
    }();
    _Ip __pos = __part.first;
    const _Dp __lsize = __pos - __begin;
    const _Dp __rsize = __end - (__pos + _Dp(1));
    if (__lsize < __size / 8 || __rsize < __size / 8) {
      if (--__bad_allowed == 0) {
        ::__ycxx::__detail::__heap_sort<_Ops>(__begin, __end, less);
        return;
      }
      // Break patterns: swap a few elements of each side with ones a quarter further in.
      auto __exchange = [](_Ip a, _Ip b) { _Ops::iter_swap(a, b); };
      if (__lsize >= __pdq_insertion_threshold) {
        const _Dp __q = __lsize / 4;
        __exchange(__begin, __begin + __q);
        __exchange(__pos - _Dp(1), __pos - __q);
        if (__lsize > __pdq_ninther_threshold) {
          __exchange(__begin + _Dp(1), __begin + (__q + 1));
          __exchange(__begin + _Dp(2), __begin + (__q + 2));
          __exchange(__pos - _Dp(2), __pos - (__q + 1));
          __exchange(__pos - _Dp(3), __pos - (__q + 2));
        }
      }
      if (__rsize >= __pdq_insertion_threshold) {
        const _Dp __q = __rsize / 4;
        __exchange(__pos + _Dp(1), __pos + (__q + 1));
        __exchange(__end - _Dp(1), __end - __q);
        if (__rsize > __pdq_ninther_threshold) {
          __exchange(__pos + _Dp(2), __pos + (__q + 2));
          __exchange(__pos + _Dp(3), __pos + (__q + 3));
          __exchange(__end - _Dp(2), __end - (__q + 1));
          __exchange(__end - _Dp(3), __end - (__q + 2));
        }
      }
    } else if (__part.second && ::__ycxx::__detail::__partial_insertion_sort<_Ops>(__begin, __pos, less) &&
               ::__ycxx::__detail::__partial_insertion_sort<_Ops>(__pos + _Dp(1), __end, less)) {
      return; // nothing was swapped and both sides were (nearly) sorted
    }
    // Recurse into the left side, iterate on the right one.
    ::__ycxx::__detail::__pdqsort_loop<_Ops, _Branchless>(__begin, __pos, less, __bad_allowed, __leftmost);
    __begin = __pos + _Dp(1);
    __leftmost = false;
  }
}

template <class _Ops, class _Ip, class _Cp>
constexpr void __sort_impl(_Ip first, _Ip last, _Cp less) {
  const auto n = last - first;
  if (n < 2)
    return;
  ::__ycxx::__detail::__pdqsort_loop<_Ops, __pdq_branchless<_Ip>>(
      first, last, less, ::__ycxx::__detail::__floor_log2(static_cast<unsigned long long>(n)), true);
}

// Sorts [first, last) so that [first, middle) holds the smallest elements in order.
template <class _Ops, class _Ip, class _Cp>
constexpr void __partial_sort_impl(_Ip first, _Ip __middle, _Ip last, _Cp less) {
  if (first == __middle)
    return;
  ::__ycxx::__detail::__make_heap_impl<_Ops>(first, __middle, less);
  auto __len = __middle - first;
  for (_Ip i = __middle; i != last; ++i)
    if (less(*i, *first)) {
      std::iter_value_t<_Ip> __v(_Ops::iter_move(i));
      *i = _Ops::iter_move(first);
      ::__ycxx::__detail::__heap_sift_hole<_Ops>(first, __len, 0, __v, less);
    }
  ::__ycxx::__detail::__sort_heap_impl<_Ops>(first, __middle, less);
}

template <class _Ops, class _Ip, class _Cp>
constexpr void __nth_element_impl(_Ip first, _Ip __nth, _Ip last, _Cp less) {
  if (__nth == last)
    return;
  int depth = 2 * ::__ycxx::__detail::__floor_log2(static_cast<unsigned long long>(last - first));
  while (last - first > 3) {
    if (depth-- == 0) {
      ::__ycxx::__detail::__partial_sort_impl<_Ops>(first, __nth + ::__ycxx::__detail::__diff_one<_Ip>, last, less);
      return;
    }
    _Ip __cut = ::__ycxx::__detail::__partition_pivot<_Ops>(first, last, less);
    if (__cut <= __nth)
      first = __cut;
    else
      last = __cut;
  }
  ::__ycxx::__detail::__insertion_sort<_Ops>(first, last, less);
}

template <class _Ip, class _Sp, class _Cp>
constexpr _Ip __is_sorted_until_impl(_Ip first, _Sp last, _Cp less) {
  if (first == last)
    return first;
  _Ip next = first;
  while (++next != last) {
    if (less(*next, *first))
      return next;
    first = next;
  }
  return next;
}

// ---- binary search -------------------------------------------------------------------------
// The first element of [first, first + n) for which pred is false ([alg.partitions]).
template <class _Ip, class _Pp>
constexpr _Ip __partition_point_n(_Ip first, std::iter_difference_t<_Ip> n, _Pp pred) {
  while (n > 0) {
    auto __half = n / 2;
    _Ip __mid = ::__ycxx::__detail::__iter_next(first, __half);
    if (pred(*__mid)) {
      first = ++__mid;
      n -= __half + 1;
    } else {
      n = __half;
    }
  }
  return first;
}

// ---- partitions ----------------------------------------------------------------------------
template <class _Ops, class _Ip, class _Sp, class _Pp>
constexpr std::pair<_Ip, _Ip> __partition_impl(_Ip first, _Sp last, _Pp pred) {
  if constexpr (std::bidirectional_iterator<_Ip> && (std::same_as<_Ip, _Sp> || std::sized_sentinel_for<_Sp, _Ip>)) {
    _Ip end = ::__ycxx::__detail::__iter_at(first, last);
    _Ip __hi = end;
    for (;;) {
      for (;; ++first) {
        if (first == __hi)
          return {first, end};
        if (!pred(*first))
          break;
      }
      do {
        if (first == --__hi)
          return {first, end};
      } while (!pred(*__hi));
      _Ops::iter_swap(first, __hi);
      ++first;
    }
  } else {
    first = ::__ycxx::__detail::__find_if_impl(static_cast<_Ip&&>(first), last, __negated<_Pp>{pred});
    if (first == last)
      return {first, first};
    _Ip i = first;
    for (++i; i != last; ++i)
      if (pred(*i)) {
        _Ops::iter_swap(first, i);
        ++first;
      }
    return {first, i};
  }
}

// Stable partition of [first, first + len), which starts with an element for which pred is
// false; buf holds room for `__cap` values.
template <class _Ops, class _Ip, class _Pp, class _Tp>
constexpr _Ip __stable_partition_adaptive(_Ip first, _Ip last, std::iter_difference_t<_Ip> __len, _Pp pred, _Tp* __buf,
                                      std::ptrdiff_t __cap) {
  if (__len <= __cap) {
    // The false elements go to the buffer, the true ones are packed at the front.
    __buffer_objects<_Tp> __objs{__buf};
    _Ip out = first;
    std::construct_at(__buf, _Ops::iter_move(first));
    __objs.n = 1;
    for (_Ip i = ::__ycxx::__detail::__iter_next(first, 1); i != last; ++i) {
      if (pred(*i)) {
        *out = _Ops::iter_move(i);
        ++out;
      } else {
        std::construct_at(__buf + __objs.n, _Ops::iter_move(i));
        ++__objs.n;
      }
    }
    _Ip r = out;
    for (std::ptrdiff_t k = 0; k != __objs.n; (void)++k, (void)++out)
      *out = std::move(__buf[k]);
    return r;
  }
  if (__len == 1)
    return first; // the first element is false
  // Divide and conquer: partition both halves, then rotate the middle.
  auto __half = __len / 2;
  _Ip __mid = ::__ycxx::__detail::__iter_next(first, __half);
  _Ip left = ::__ycxx::__detail::__stable_partition_adaptive<_Ops>(first, __mid, __half, pred, __buf, __cap);
  // The right half may start with true elements: skip them (each predicate call is used once).
  _Ip __m2 = __mid;
  auto __rlen = __len - __half;
  while (__rlen > 0 && pred(*__m2)) {
    ++__m2;
    --__rlen;
  }
  _Ip right = __rlen > 0 ? ::__ycxx::__detail::__stable_partition_adaptive<_Ops>(__m2, last, __rlen, pred, __buf, __cap) : __m2;
  return ::__ycxx::__detail::__rotate_impl<_Ops>(left, __mid, right);
}

template <class _Ops, class _Ip, class _Sp, class _Pp>
constexpr std::pair<_Ip, _Ip> __stable_partition_impl(_Ip first, _Sp last, _Pp pred) {
  _Ip end = ::__ycxx::__detail::__iter_at(first, last);
  // Leading true elements stay in place.
  while (first != end && pred(*first))
    ++first;
  if (first == end)
    return {first, end};
  // [first, end) now starts with a false element; every element is tested exactly once.
  auto __len = ::__ycxx::__detail::__range_length(first, end);
  using _Tp = std::iter_value_t<_Ip>;
  __temp_buffer<_Tp> __buf(__len);
  return {::__ycxx::__detail::__stable_partition_adaptive<_Ops>(first, end, __len, pred, __buf.data(), __buf.capacity()), end};
}

// ---- merging -------------------------------------------------------------------------------
template <class _I1, class _S1, class _I2, class _S2, class _Op, class _Cp>
constexpr void __merge_loop(_I1& __first1, _S1 __last1, _I2& __first2, _S2 __last2, _Op& result, _Cp less) {
  while (__first1 != __last1 && __first2 != __last2) {
    if (less.__rev(*__first2, *__first1)) {
      *result = *__first2;
      ++__first2;
    } else {
      *result = *__first1;
      ++__first1;
    }
    ++result;
  }
}

// Merges the sorted runs [first, middle) and [middle, last) with a buffer of at least
// min(len1, len2) elements: N - 1 comparisons at most.
template <class _Ops, class _Ip, class _Cp, class _Tp>
constexpr void __merge_with_buffer(_Ip first, _Ip __middle, _Ip last, std::iter_difference_t<_Ip> __len1,
                                 std::iter_difference_t<_Ip> __len2, _Cp less, _Tp* __buf) {
  __buffer_objects<_Tp> __objs{__buf};
  if (__len1 <= __len2) {
    ::__ycxx::__detail::__move_into_buffer<_Ops>(first, __middle, __objs);
    _Tp* b = __buf;
    _Tp* __be = __buf + __objs.n;
    _Ip out = first;
    while (b != __be) {
      if (__middle == last) {
        for (; b != __be; (void)++b, (void)++out)
          *out = std::move(*b);
        return;
      }
      if (less(*__middle, *b)) {
        *out = _Ops::iter_move(__middle);
        ++__middle;
      } else {
        *out = std::move(*b);
        ++b;
      }
      ++out;
    }
  } else {
    ::__ycxx::__detail::__move_into_buffer<_Ops>(__middle, last, __objs);
    _Tp* __bb = __buf;
    _Tp* b = __buf + __objs.n; // one past the next buffer element to place
    _Ip out = last;
    _Ip __l = __middle; // one past the next left element to place
    while (b != __bb) {
      if (__l == first) {
        while (b != __bb)
          *--out = std::move(*--b);
        return;
      }
      _Ip __lp = __l;
      --__lp;
      if (less(*(b - 1), *__lp)) {
        *--out = _Ops::iter_move(__lp);
        __l = __lp;
      } else {
        *--out = std::move(*--b);
      }
    }
  }
}

template <class _Ops, class _Ip, class _Cp, class _Tp>
constexpr void __merge_adaptive(_Ip first, _Ip __middle, _Ip last, std::iter_difference_t<_Ip> __len1,
                              std::iter_difference_t<_Ip> __len2, _Cp less, _Tp* __buf, std::ptrdiff_t __cap) {
  for (;;) {
    if (__len1 == 0 || __len2 == 0)
      return;
    if (__len1 <= __cap || __len2 <= __cap) {
      ::__ycxx::__detail::__merge_with_buffer<_Ops>(first, __middle, last, __len1, __len2, less, __buf);
      return;
    }
    if (__len1 + __len2 == 2) {
      if (less(*__middle, *first))
        _Ops::iter_swap(first, __middle);
      return;
    }
    // Split the longer run in half, find the matching cut in the other one, rotate the two
    // inner pieces into place and merge both sides.
    _Ip __cut1 = first, __cut2 = __middle;
    std::iter_difference_t<_Ip> __len11, __len22;
    if (__len1 > __len2) {
      __len11 = __len1 / 2;
      ::__ycxx::__detail::__iter_advance(__cut1, __len11);
      // lower bound of *cut1 in [middle, last)
      auto&& __v = *__cut1;
      __cut2 = ::__ycxx::__detail::__partition_point_n(__middle, __len2, [&](auto&& e) { return less(e, __v); });
      __len22 = ::__ycxx::__detail::__range_length(__middle, __cut2);
    } else {
      __len22 = __len2 / 2;
      ::__ycxx::__detail::__iter_advance(__cut2, __len22);
      // upper bound of *cut2 in [first, middle)
      auto&& __v = *__cut2;
      __cut1 = ::__ycxx::__detail::__partition_point_n(first, __len1, [&](auto&& e) { return !less(__v, e); });
      __len11 = ::__ycxx::__detail::__range_length(first, __cut1);
    }
    _Ip __new_mid = ::__ycxx::__detail::__rotate_impl<_Ops>(__cut1, __middle, __cut2);
    // Recurse into the smaller side, iterate on the other.
    if (__len11 + __len22 < (__len1 - __len11) + (__len2 - __len22)) {
      ::__ycxx::__detail::__merge_adaptive<_Ops>(first, __cut1, __new_mid, __len11, __len22, less, __buf, __cap);
      first = __new_mid;
      __middle = __cut2;
      __len1 -= __len11;
      __len2 -= __len22;
    } else {
      ::__ycxx::__detail::__merge_adaptive<_Ops>(__new_mid, __cut2, last, __len1 - __len11, __len2 - __len22, less, __buf, __cap);
      last = __new_mid;
      __middle = __cut1;
      __len1 = __len11;
      __len2 = __len22;
    }
  }
}

template <class _Ops, class _Ip, class _Cp>
constexpr void __inplace_merge_impl(_Ip first, _Ip __middle, _Ip last, _Cp less) {
  if (first == __middle || __middle == last)
    return;
  auto __len1 = ::__ycxx::__detail::__range_length(first, __middle);
  auto __len2 = ::__ycxx::__detail::__range_length(__middle, last);
  using _Tp = std::iter_value_t<_Ip>;
  __temp_buffer<_Tp> __buf(__len1 < __len2 ? __len1 : __len2);
  ::__ycxx::__detail::__merge_adaptive<_Ops>(first, __middle, last, __len1, __len2, less, __buf.data(), __buf.capacity());
}

template <class _Ops, class _Ip, class _Cp, class _Tp>
constexpr void __stable_sort_adaptive(_Ip first, _Ip last, std::iter_difference_t<_Ip> __len, _Cp less, _Tp* __buf,
                                    std::ptrdiff_t __cap) {
  if (__len <= __insertion_sort_threshold) {
    ::__ycxx::__detail::__binary_insertion_sort<_Ops>(first, last, less);
    return;
  }
  auto __half = __len / 2;
  _Ip __mid = first + __half;
  ::__ycxx::__detail::__stable_sort_adaptive<_Ops>(first, __mid, __half, less, __buf, __cap);
  ::__ycxx::__detail::__stable_sort_adaptive<_Ops>(__mid, last, __len - __half, less, __buf, __cap);
  if (!less(*__mid, *(__mid - ::__ycxx::__detail::__diff_one<_Ip>)))
    return; // already in order
  ::__ycxx::__detail::__merge_adaptive<_Ops>(first, __mid, last, __half, __len - __half, less, __buf, __cap);
}

// ---- stable_sort of integers ---------------------------------------------------------------
// Integers ordered by one of the standard comparison objects: equivalent elements are equal
// values, so the order among them cannot be observed and stable_sort may use any sort. Large
// ranges get an LSD radix sort (a counting pass per byte, skipping bytes that all elements share)
// through a buffer of n elements, smaller ones (or without the buffer) pdqsort; both stay within
// [stable.sort]/5's N log N comparisons (radix sort makes none).

// 1: the comparator orders T ascending, -1: descending, 0: neither or unknown.
template <class _Cp, class _Tp>
inline constexpr int __std_order = 0;
template <class _Tp>
inline constexpr int __std_order<__pred_ref<std::less<void>>, _Tp> = 1;
template <class _Tp>
inline constexpr int __std_order<__pred_ref<std::less<_Tp>>, _Tp> = 1;
template <class _Tp>
inline constexpr int __std_order<__pred_ref<std::ranges::less>, _Tp> = 1;
template <class _Tp>
inline constexpr int __std_order<__pred_ref<std::greater<void>>, _Tp> = -1;
template <class _Tp>
inline constexpr int __std_order<__pred_ref<std::greater<_Tp>>, _Tp> = -1;
template <class _Tp>
inline constexpr int __std_order<__pred_ref<std::ranges::greater>, _Tp> = -1;
template <class _Tp>
inline constexpr int __std_order<__proj_comp<std::ranges::less, std::identity>, _Tp> = 1;
template <class _Tp>
inline constexpr int __std_order<__proj_comp<std::ranges::greater, std::identity>, _Tp> = -1;

template <class _Ip, class _Cp>
concept __radix_sortable = std::contiguous_iterator<_Ip> && std::is_integral_v<std::iter_value_t<_Ip>> &&
                           !std::is_same_v<std::iter_value_t<_Ip>, bool> &&
                           std::is_same_v<std::iter_reference_t<_Ip>, std::iter_value_t<_Ip>&> &&
                           __std_order<_Cp, std::iter_value_t<_Ip>> != 0;

inline constexpr std::ptrdiff_t __radix_threshold = 512;

// Sorts a[0, n) ascending (descending with _Desc), using b[0, n) as scratch.
template <bool _Desc, class _Tp>
void __radix_sort(_Tp* a, std::size_t n, _Tp* b) noexcept {
  using _Up = std::make_unsigned_t<_Tp>;
  constexpr int _Kp = sizeof(_Tp);
  constexpr _Up __flip = std::is_signed_v<_Tp> ? static_cast<_Up>(_Up(1) << (8 * _Kp - 1)) : _Up(0);
  // The key's unsigned order is the wanted order of the values.
  auto key = [](_Tp __x) noexcept {
    _Up __u = static_cast<_Up>(static_cast<_Up>(__x) ^ __flip);
    if constexpr (_Desc)
      __u = static_cast<_Up>(~__u);
    return __u;
  };
  std::size_t __cnt[_Kp][256] = {};
  for (std::size_t i = 0; i != n; ++i) {
    const _Up k = key(a[i]);
    for (int d = 0; d < _Kp; ++d)
      ++__cnt[d][(k >> (8 * d)) & 0xff];
  }
  _Tp* __src = a;
  _Tp* __dst = b;
  for (int d = 0; d < _Kp; ++d) {
    std::size_t* c = __cnt[d];
    if (c[(key(a[0]) >> (8 * d)) & 0xff] == n)
      continue; // every element has the same digit here
    std::size_t __sum = 0;
    for (int __v = 0; __v < 256; ++__v) {
      const std::size_t __here = c[__v];
      c[__v] = __sum;
      __sum += __here;
    }
    for (std::size_t i = 0; i != n; ++i) {
      const _Tp __x = __src[i];
      __dst[c[(key(__x) >> (8 * d)) & 0xff]++] = __x;
    }
    _Tp* t = __src;
    __src = __dst;
    __dst = t;
  }
  if (__src != a)
    for (std::size_t i = 0; i != n; ++i)
      a[i] = __src[i];
}

template <class _Ops, class _Ip, class _Cp>
constexpr void __stable_sort_impl(_Ip first, _Ip last, _Cp less) {
  auto __len = last - first;
  using _Tp = std::iter_value_t<_Ip>;
  if constexpr (::__ycxx::__detail::__radix_sortable<_Ip, _Cp>) {
    if !consteval {
      if (__len >= __radix_threshold) {
        __temp_buffer<_Tp> __buf(__len);
        if (__buf.capacity() == __len) {
          ::__ycxx::__detail::__radix_sort<(__std_order<_Cp, _Tp> < 0)>(std::to_address(first), static_cast<std::size_t>(__len),
                                                                     __buf.data());
          return;
        }
      }
    }
    ::__ycxx::__detail::__sort_impl<_Ops>(first, last, less);
    return;
  }
  if (__len <= __insertion_sort_threshold) {
    ::__ycxx::__detail::__binary_insertion_sort<_Ops>(first, last, less);
    return;
  }
  __temp_buffer<_Tp> __buf((__len + 1) / 2);
  ::__ycxx::__detail::__stable_sort_adaptive<_Ops>(first, last, __len, less, __buf.data(), __buf.capacity());
}

// ---- set operations ------------------------------------------------------------------------
template <class _I1, class _S1, class _I2, class _S2, class _Cp>
constexpr bool __includes_impl(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Cp less) {
  for (; __first2 != __last2; ++__first1) {
    if (__first1 == __last1 || less.__rev(*__first2, *__first1))
      return false;
    if (!less(*__first1, *__first2))
      ++__first2;
  }
  return true;
}

template <class _I1, class _S1, class _I2, class _S2, class _Op, class _Cp>
constexpr void __set_union_loop(_I1& __first1, _S1 __last1, _I2& __first2, _S2 __last2, _Op& result, _Cp less) {
  while (__first1 != __last1 && __first2 != __last2) {
    if (less(*__first1, *__first2)) {
      *result = *__first1;
      ++__first1;
    } else if (less.__rev(*__first2, *__first1)) {
      *result = *__first2;
      ++__first2;
    } else {
      *result = *__first1;
      ++__first1;
      ++__first2;
    }
    ++result;
  }
}
template <class _I1, class _S1, class _I2, class _S2, class _Op, class _Cp>
constexpr void __set_intersection_loop(_I1& __first1, _S1 __last1, _I2& __first2, _S2 __last2, _Op& result, _Cp less) {
  while (__first1 != __last1 && __first2 != __last2) {
    if (less(*__first1, *__first2)) {
      ++__first1;
    } else if (less.__rev(*__first2, *__first1)) {
      ++__first2;
    } else {
      *result = *__first1;
      ++result;
      ++__first1;
      ++__first2;
    }
  }
}
template <class _I1, class _S1, class _I2, class _S2, class _Op, class _Cp>
constexpr void __set_difference_loop(_I1& __first1, _S1 __last1, _I2& __first2, _S2 __last2, _Op& result, _Cp less) {
  while (__first1 != __last1 && __first2 != __last2) {
    if (less(*__first1, *__first2)) {
      *result = *__first1;
      ++result;
      ++__first1;
    } else {
      if (!less.__rev(*__first2, *__first1))
        ++__first1;
      ++__first2;
    }
  }
}
template <class _I1, class _S1, class _I2, class _S2, class _Op, class _Cp>
constexpr void __set_symmetric_difference_loop(_I1& __first1, _S1 __last1, _I2& __first2, _S2 __last2, _Op& result, _Cp less) {
  while (__first1 != __last1 && __first2 != __last2) {
    if (less(*__first1, *__first2)) {
      *result = *__first1;
      ++result;
      ++__first1;
    } else if (less.__rev(*__first2, *__first1)) {
      *result = *__first2;
      ++result;
      ++__first2;
    } else {
      ++__first1;
      ++__first2;
    }
  }
}

// ---- permutations --------------------------------------------------------------------------
template <class _Ops, class _Ip, class _Cp>
constexpr bool __next_permutation_impl(_Ip first, _Ip last, _Cp less) {
  if (first == last)
    return false;
  _Ip i = last;
  if (first == --i)
    return false;
  for (;;) {
    _Ip __i1 = i;
    if (less(*--i, *__i1)) {
      _Ip __i2 = last;
      while (!less(*i, *--__i2)) {
      }
      _Ops::iter_swap(i, __i2);
      ::__ycxx::__detail::__reverse_impl<_Ops>(__i1, last);
      return true;
    }
    if (i == first) {
      ::__ycxx::__detail::__reverse_impl<_Ops>(first, last);
      return false;
    }
  }
}
template <class _Ops, class _Ip, class _Cp>
constexpr bool __prev_permutation_impl(_Ip first, _Ip last, _Cp less) {
  if (first == last)
    return false;
  _Ip i = last;
  if (first == --i)
    return false;
  for (;;) {
    _Ip __i1 = i;
    if (less(*__i1, *--i)) {
      _Ip __i2 = last;
      while (!less(*--__i2, *i)) {
      }
      _Ops::iter_swap(i, __i2);
      ::__ycxx::__detail::__reverse_impl<_Ops>(__i1, last);
      return true;
    }
    if (i == first) {
      ::__ycxx::__detail::__reverse_impl<_Ops>(first, last);
      return false;
    }
  }
}

}} // namespace __ycxx::__detail

// =============================================================================================
// std:: forms
// =============================================================================================
namespace [[__gnu__::__visibility__("hidden")]] std {

// [sort], [stable.sort], [partial.sort], [partial.sort.copy], [is.sorted]
template <class _RandomAccessIterator, class _Compare>
constexpr void sort(_RandomAccessIterator first, _RandomAccessIterator last, _Compare comp) {
  ::__ycxx::__detail::__sort_impl<__ycxx::__detail::__classic_ops>(first, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _RandomAccessIterator>
constexpr void sort(_RandomAccessIterator first, _RandomAccessIterator last) {
  std::sort(first, last, less<>{});
}
template <class _RandomAccessIterator, class _Compare>
constexpr void stable_sort(_RandomAccessIterator first, _RandomAccessIterator last, _Compare comp) {
  ::__ycxx::__detail::__stable_sort_impl<__ycxx::__detail::__classic_ops>(first, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _RandomAccessIterator>
constexpr void stable_sort(_RandomAccessIterator first, _RandomAccessIterator last) {
  std::stable_sort(first, last, less<>{});
}
template <class _RandomAccessIterator, class _Compare>
constexpr void partial_sort(_RandomAccessIterator first, _RandomAccessIterator __middle, _RandomAccessIterator last,
                            _Compare comp) {
  ::__ycxx::__detail::__partial_sort_impl<__ycxx::__detail::__classic_ops>(first, __middle, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _RandomAccessIterator>
constexpr void partial_sort(_RandomAccessIterator first, _RandomAccessIterator __middle, _RandomAccessIterator last) {
  std::partial_sort(first, __middle, last, less<>{});
}
template <class _InputIterator, class _RandomAccessIterator, class _Compare>
constexpr _RandomAccessIterator partial_sort_copy(_InputIterator first, _InputIterator last,
                                                 _RandomAccessIterator __result_first, _RandomAccessIterator __result_last,
                                                 _Compare comp) {
  auto less = ::__ycxx::__detail::__ref_pred(comp);
  _RandomAccessIterator r = __result_first;
  for (; first != last && r != __result_last; (void)++first, (void)++r)
    *r = *first;
  if (r == __result_first)
    return r;
  ::__ycxx::__detail::__make_heap_impl<__ycxx::__detail::__classic_ops>(__result_first, r, less);
  auto __len = r - __result_first;
  for (; first != last; ++first)
    if (less(*first, *__result_first)) {
      *__result_first = *first;
      iter_value_t<_RandomAccessIterator> __v(::__ycxx::__detail::__classic_ops::iter_move(__result_first));
      ::__ycxx::__detail::__heap_sift_hole<__ycxx::__detail::__classic_ops>(__result_first, __len, 0, __v, less);
    }
  ::__ycxx::__detail::__sort_heap_impl<__ycxx::__detail::__classic_ops>(__result_first, r, less);
  return r;
}
template <class _InputIterator, class _RandomAccessIterator>
constexpr _RandomAccessIterator partial_sort_copy(_InputIterator first, _InputIterator last,
                                                 _RandomAccessIterator __result_first, _RandomAccessIterator __result_last) {
  return std::partial_sort_copy(first, last, __result_first, __result_last, less<>{});
}
template <class _ForwardIterator, class _Compare>
[[nodiscard]] constexpr _ForwardIterator is_sorted_until(_ForwardIterator first, _ForwardIterator last, _Compare comp) {
  return ::__ycxx::__detail::__is_sorted_until_impl(first, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _ForwardIterator>
[[nodiscard]] constexpr _ForwardIterator is_sorted_until(_ForwardIterator first, _ForwardIterator last) {
  return std::is_sorted_until(first, last, less<>{});
}
template <class _ForwardIterator, class _Compare>
[[nodiscard]] constexpr bool is_sorted(_ForwardIterator first, _ForwardIterator last, _Compare comp) {
  return std::is_sorted_until(first, last, comp) == last;
}
template <class _ForwardIterator>
[[nodiscard]] constexpr bool is_sorted(_ForwardIterator first, _ForwardIterator last) {
  return std::is_sorted_until(first, last, less<>{}) == last;
}

// [alg.nth.element]
template <class _RandomAccessIterator, class _Compare>
constexpr void nth_element(_RandomAccessIterator first, _RandomAccessIterator __nth, _RandomAccessIterator last,
                           _Compare comp) {
  ::__ycxx::__detail::__nth_element_impl<__ycxx::__detail::__classic_ops>(first, __nth, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _RandomAccessIterator>
constexpr void nth_element(_RandomAccessIterator first, _RandomAccessIterator __nth, _RandomAccessIterator last) {
  std::nth_element(first, __nth, last, less<>{});
}

// [alg.binary.search]
template <class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type, class _Compare>
[[nodiscard]] constexpr _ForwardIterator lower_bound(_ForwardIterator first, _ForwardIterator last, const _Tp& value,
                                                    _Compare comp) {
  return ::__ycxx::__detail::__partition_point_n(first, ::__ycxx::__detail::__range_length(first, last),
                                                   [&](auto&& e) -> bool { return static_cast<bool>(comp(e, value)); });
}
template <class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
[[nodiscard]] constexpr _ForwardIterator lower_bound(_ForwardIterator first, _ForwardIterator last, const _Tp& value) {
  return std::lower_bound(first, last, value, less<>{});
}
template <class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type, class _Compare>
[[nodiscard]] constexpr _ForwardIterator upper_bound(_ForwardIterator first, _ForwardIterator last, const _Tp& value,
                                                    _Compare comp) {
  return ::__ycxx::__detail::__partition_point_n(first, ::__ycxx::__detail::__range_length(first, last),
                                                   [&](auto&& e) -> bool { return !static_cast<bool>(comp(value, e)); });
}
template <class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
[[nodiscard]] constexpr _ForwardIterator upper_bound(_ForwardIterator first, _ForwardIterator last, const _Tp& value) {
  return std::upper_bound(first, last, value, less<>{});
}
template <class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type, class _Compare>
[[nodiscard]] constexpr pair<_ForwardIterator, _ForwardIterator> equal_range(_ForwardIterator first, _ForwardIterator last,
                                                                           const _Tp& value, _Compare comp) {
  _ForwardIterator __lo = std::lower_bound(first, last, value, comp);
  return {__lo, std::upper_bound(__lo, last, value, comp)};
}
template <class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
[[nodiscard]] constexpr pair<_ForwardIterator, _ForwardIterator> equal_range(_ForwardIterator first, _ForwardIterator last,
                                                                           const _Tp& value) {
  return std::equal_range(first, last, value, less<>{});
}
template <class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type, class _Compare>
[[nodiscard]] constexpr bool binary_search(_ForwardIterator first, _ForwardIterator last, const _Tp& value, _Compare comp) {
  first = std::lower_bound(first, last, value, comp);
  return first != last && !static_cast<bool>(comp(value, *first));
}
template <class _ForwardIterator, class _Tp = typename iterator_traits<_ForwardIterator>::value_type>
[[nodiscard]] constexpr bool binary_search(_ForwardIterator first, _ForwardIterator last, const _Tp& value) {
  return std::binary_search(first, last, value, less<>{});
}

// [alg.partitions]
template <class _InputIterator, class _Predicate>
[[nodiscard]] constexpr bool is_partitioned(_InputIterator first, _InputIterator last, _Predicate pred) {
  auto p = ::__ycxx::__detail::__ref_pred(pred);
  first = ::__ycxx::__detail::__find_if_impl(first, last, ::__ycxx::__detail::__negated{p});
  if (first == last)
    return true;
  ++first;
  return ::__ycxx::__detail::__find_if_impl(first, last, p) == last;
}
template <class _ForwardIterator, class _Predicate>
constexpr _ForwardIterator partition(_ForwardIterator first, _ForwardIterator last, _Predicate pred) {
  return ::__ycxx::__detail::__partition_impl<__ycxx::__detail::__classic_ops>(first, last, ::__ycxx::__detail::__ref_pred(pred)).first;
}
template <class _BidirectionalIterator, class _Predicate>
constexpr _BidirectionalIterator stable_partition(_BidirectionalIterator first, _BidirectionalIterator last,
                                                 _Predicate pred) {
  return ::__ycxx::__detail::__stable_partition_impl<__ycxx::__detail::__classic_ops>(first, last, ::__ycxx::__detail::__ref_pred(pred))
      .first;
}
template <class _InputIterator, class _OutputIterator1, class _OutputIterator2, class _Predicate>
constexpr pair<_OutputIterator1, _OutputIterator2> partition_copy(_InputIterator first, _InputIterator last,
                                                                _OutputIterator1 __out_true, _OutputIterator2 __out_false,
                                                                _Predicate pred) {
  for (; first != last; ++first) {
    if (pred(*first)) {
      *__out_true = *first;
      ++__out_true;
    } else {
      *__out_false = *first;
      ++__out_false;
    }
  }
  return {__out_true, __out_false};
}
template <class _ForwardIterator, class _Predicate>
[[nodiscard]] constexpr _ForwardIterator partition_point(_ForwardIterator first, _ForwardIterator last, _Predicate pred) {
  return ::__ycxx::__detail::__partition_point_n(first, ::__ycxx::__detail::__range_length(first, last),
                                                   ::__ycxx::__detail::__ref_pred(pred));
}

// [alg.merge]
template <class _InputIterator1, class _InputIterator2, class _OutputIterator, class _Compare>
constexpr _OutputIterator merge(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                               _InputIterator2 __last2, _OutputIterator result, _Compare comp) {
  ::__ycxx::__detail::__merge_loop(__first1, __last1, __first2, __last2, result, ::__ycxx::__detail::__ref_pred(comp));
  result = ::__ycxx::__detail::__copy_dispatch(__first1, __last1, result).second;
  return ::__ycxx::__detail::__copy_dispatch(__first2, __last2, result).second;
}
template <class _InputIterator1, class _InputIterator2, class _OutputIterator>
constexpr _OutputIterator merge(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                               _InputIterator2 __last2, _OutputIterator result) {
  return std::merge(__first1, __last1, __first2, __last2, result, less<>{});
}
template <class _BidirectionalIterator, class _Compare>
constexpr void inplace_merge(_BidirectionalIterator first, _BidirectionalIterator __middle, _BidirectionalIterator last,
                             _Compare comp) {
  ::__ycxx::__detail::__inplace_merge_impl<__ycxx::__detail::__classic_ops>(first, __middle, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _BidirectionalIterator>
constexpr void inplace_merge(_BidirectionalIterator first, _BidirectionalIterator __middle, _BidirectionalIterator last) {
  std::inplace_merge(first, __middle, last, less<>{});
}

// [alg.set.operations]
template <class _InputIterator1, class _InputIterator2, class _Compare>
[[nodiscard]] constexpr bool includes(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                                      _InputIterator2 __last2, _Compare comp) {
  return ::__ycxx::__detail::__includes_impl(__first1, __last1, __first2, __last2, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _InputIterator1, class _InputIterator2>
[[nodiscard]] constexpr bool includes(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                                      _InputIterator2 __last2) {
  return std::includes(__first1, __last1, __first2, __last2, less<>{});
}
template <class _InputIterator1, class _InputIterator2, class _OutputIterator, class _Compare>
constexpr _OutputIterator set_union(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                                   _InputIterator2 __last2, _OutputIterator result, _Compare comp) {
  ::__ycxx::__detail::__set_union_loop(__first1, __last1, __first2, __last2, result, ::__ycxx::__detail::__ref_pred(comp));
  result = ::__ycxx::__detail::__copy_dispatch(__first1, __last1, result).second;
  return ::__ycxx::__detail::__copy_dispatch(__first2, __last2, result).second;
}
template <class _InputIterator1, class _InputIterator2, class _OutputIterator>
constexpr _OutputIterator set_union(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                                   _InputIterator2 __last2, _OutputIterator result) {
  return std::set_union(__first1, __last1, __first2, __last2, result, less<>{});
}
template <class _InputIterator1, class _InputIterator2, class _OutputIterator, class _Compare>
constexpr _OutputIterator set_intersection(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                                          _InputIterator2 __last2, _OutputIterator result, _Compare comp) {
  ::__ycxx::__detail::__set_intersection_loop(__first1, __last1, __first2, __last2, result, ::__ycxx::__detail::__ref_pred(comp));
  return result;
}
template <class _InputIterator1, class _InputIterator2, class _OutputIterator>
constexpr _OutputIterator set_intersection(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                                          _InputIterator2 __last2, _OutputIterator result) {
  return std::set_intersection(__first1, __last1, __first2, __last2, result, less<>{});
}
template <class _InputIterator1, class _InputIterator2, class _OutputIterator, class _Compare>
constexpr _OutputIterator set_difference(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                                        _InputIterator2 __last2, _OutputIterator result, _Compare comp) {
  ::__ycxx::__detail::__set_difference_loop(__first1, __last1, __first2, __last2, result, ::__ycxx::__detail::__ref_pred(comp));
  return ::__ycxx::__detail::__copy_dispatch(__first1, __last1, result).second;
}
template <class _InputIterator1, class _InputIterator2, class _OutputIterator>
constexpr _OutputIterator set_difference(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                                        _InputIterator2 __last2, _OutputIterator result) {
  return std::set_difference(__first1, __last1, __first2, __last2, result, less<>{});
}
template <class _InputIterator1, class _InputIterator2, class _OutputIterator, class _Compare>
constexpr _OutputIterator set_symmetric_difference(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                                                  _InputIterator2 __last2, _OutputIterator result, _Compare comp) {
  ::__ycxx::__detail::__set_symmetric_difference_loop(__first1, __last1, __first2, __last2, result, ::__ycxx::__detail::__ref_pred(comp));
  result = ::__ycxx::__detail::__copy_dispatch(__first1, __last1, result).second;
  return ::__ycxx::__detail::__copy_dispatch(__first2, __last2, result).second;
}
template <class _InputIterator1, class _InputIterator2, class _OutputIterator>
constexpr _OutputIterator set_symmetric_difference(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2,
                                                  _InputIterator2 __last2, _OutputIterator result) {
  return std::set_symmetric_difference(__first1, __last1, __first2, __last2, result, less<>{});
}

// [alg.heap.operations]
template <class _RandomAccessIterator, class _Compare>
constexpr void push_heap(_RandomAccessIterator first, _RandomAccessIterator last, _Compare comp) {
  ::__ycxx::__detail::__push_heap_impl<__ycxx::__detail::__classic_ops>(first, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _RandomAccessIterator>
constexpr void push_heap(_RandomAccessIterator first, _RandomAccessIterator last) {
  std::push_heap(first, last, less<>{});
}
template <class _RandomAccessIterator, class _Compare>
constexpr void pop_heap(_RandomAccessIterator first, _RandomAccessIterator last, _Compare comp) {
  ::__ycxx::__detail::__pop_heap_impl<__ycxx::__detail::__classic_ops>(first, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _RandomAccessIterator>
constexpr void pop_heap(_RandomAccessIterator first, _RandomAccessIterator last) {
  std::pop_heap(first, last, less<>{});
}
template <class _RandomAccessIterator, class _Compare>
constexpr void make_heap(_RandomAccessIterator first, _RandomAccessIterator last, _Compare comp) {
  ::__ycxx::__detail::__make_heap_impl<__ycxx::__detail::__classic_ops>(first, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _RandomAccessIterator>
constexpr void make_heap(_RandomAccessIterator first, _RandomAccessIterator last) {
  std::make_heap(first, last, less<>{});
}
template <class _RandomAccessIterator, class _Compare>
constexpr void sort_heap(_RandomAccessIterator first, _RandomAccessIterator last, _Compare comp) {
  ::__ycxx::__detail::__sort_heap_impl<__ycxx::__detail::__classic_ops>(first, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _RandomAccessIterator>
constexpr void sort_heap(_RandomAccessIterator first, _RandomAccessIterator last) {
  std::sort_heap(first, last, less<>{});
}
template <class _RandomAccessIterator, class _Compare>
[[nodiscard]] constexpr _RandomAccessIterator is_heap_until(_RandomAccessIterator first, _RandomAccessIterator last,
                                                           _Compare comp) {
  return ::__ycxx::__detail::__is_heap_until_impl(first, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _RandomAccessIterator>
[[nodiscard]] constexpr _RandomAccessIterator is_heap_until(_RandomAccessIterator first, _RandomAccessIterator last) {
  return std::is_heap_until(first, last, less<>{});
}
template <class _RandomAccessIterator, class _Compare>
[[nodiscard]] constexpr bool is_heap(_RandomAccessIterator first, _RandomAccessIterator last, _Compare comp) {
  return std::is_heap_until(first, last, comp) == last;
}
template <class _RandomAccessIterator>
[[nodiscard]] constexpr bool is_heap(_RandomAccessIterator first, _RandomAccessIterator last) {
  return std::is_heap_until(first, last, less<>{}) == last;
}

// [alg.permutation.generators]
template <class _BidirectionalIterator, class _Compare>
constexpr bool next_permutation(_BidirectionalIterator first, _BidirectionalIterator last, _Compare comp) {
  return ::__ycxx::__detail::__next_permutation_impl<__ycxx::__detail::__classic_ops>(first, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _BidirectionalIterator>
constexpr bool next_permutation(_BidirectionalIterator first, _BidirectionalIterator last) {
  return std::next_permutation(first, last, less<>{});
}
template <class _BidirectionalIterator, class _Compare>
constexpr bool prev_permutation(_BidirectionalIterator first, _BidirectionalIterator last, _Compare comp) {
  return ::__ycxx::__detail::__prev_permutation_impl<__ycxx::__detail::__classic_ops>(first, last, ::__ycxx::__detail::__ref_pred(comp));
}
template <class _BidirectionalIterator>
constexpr bool prev_permutation(_BidirectionalIterator first, _BidirectionalIterator last) {
  return std::prev_permutation(first, last, less<>{});
}

} // namespace std

// =============================================================================================
// std::ranges:: forms
// =============================================================================================
namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
template <class _I1, class _I2>
using partial_sort_copy_result = in_out_result<_I1, _I2>;
template <class _Ip, class _O1, class _O2>
using partition_copy_result = in_out_out_result<_Ip, _O1, _O2>;
template <class _I1, class _I2, class _Op>
using merge_result = in_in_out_result<_I1, _I2, _Op>;
template <class _I1, class _I2, class _Op>
using set_union_result = in_in_out_result<_I1, _I2, _Op>;
template <class _I1, class _I2, class _Op>
using set_intersection_result = in_in_out_result<_I1, _I2, _Op>;
template <class _Ip, class _Op>
using set_difference_result = in_out_result<_Ip, _Op>;
template <class _I1, class _I2, class _Op>
using set_symmetric_difference_result = in_in_out_result<_I1, _I2, _Op>;
template <class _Ip>
using next_permutation_result = in_found_result<_Ip>;
template <class _Ip>
using prev_permutation_result = in_found_result<_Ip>;
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__ranges_algo {

using std::ranges::borrowed_iterator_t;
using std::ranges::borrowed_subrange_t;
using std::ranges::iterator_t;

struct __sort_fn {
  template <std::random_access_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires std::sortable<_Ip, _Comp, _Proj>
  constexpr _Ip operator()(_Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const {
    _Ip end = ::__ycxx::__detail::__iter_at(first, std::move(last));
    ::__ycxx::__detail::__sort_impl<__ranges_ops>(std::move(first), end, ::__ycxx::__detail::__make_comp(comp, proj));
    return end;
  }
  template <std::ranges::random_access_range _Rp, class _Comp = std::ranges::less, class _Proj = std::identity>
    requires std::sortable<iterator_t<_Rp>, _Comp, _Proj>
  constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Comp comp = {}, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct __stable_sort_fn {
  template <std::random_access_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires std::sortable<_Ip, _Comp, _Proj>
  constexpr _Ip operator()(_Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const {
    _Ip end = ::__ycxx::__detail::__iter_at(first, std::move(last));
    ::__ycxx::__detail::__stable_sort_impl<__ranges_ops>(std::move(first), end, ::__ycxx::__detail::__make_comp(comp, proj));
    return end;
  }
  template <std::ranges::random_access_range _Rp, class _Comp = std::ranges::less, class _Proj = std::identity>
    requires std::sortable<iterator_t<_Rp>, _Comp, _Proj>
  constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Comp comp = {}, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct __partial_sort_fn {
  template <std::random_access_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires std::sortable<_Ip, _Comp, _Proj>
  constexpr _Ip operator()(_Ip first, _Ip __middle, _Sp last, _Comp comp = {}, _Proj proj = {}) const {
    _Ip end = ::__ycxx::__detail::__iter_at(__middle, std::move(last));
    ::__ycxx::__detail::__partial_sort_impl<__ranges_ops>(std::move(first), std::move(__middle), end,
                                                  ::__ycxx::__detail::__make_comp(comp, proj));
    return end;
  }
  template <std::ranges::random_access_range _Rp, class _Comp = std::ranges::less, class _Proj = std::identity>
    requires std::sortable<iterator_t<_Rp>, _Comp, _Proj>
  constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, iterator_t<_Rp> __middle, _Comp comp = {}, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::move(__middle), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct __partial_sort_copy_fn {
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::random_access_iterator _I2, std::sentinel_for<_I2> _S2,
            class _Comp = std::ranges::less, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_copyable<_I1, _I2> && std::sortable<_I2, _Comp, _Proj2> &&
             std::indirect_strict_weak_order<_Comp, std::projected<_I1, _Proj1>, std::projected<_I2, _Proj2>>
  constexpr std::ranges::partial_sort_copy_result<_I1, _I2> operator()(_I1 first, _S1 last, _I2 __result_first,
                                                                    _S2 __result_last, _Comp comp = {}, _Proj1 __proj1 = {},
                                                                    _Proj2 __proj2 = {}) const {
    auto less = ::__ycxx::__detail::__make_comp(comp, __proj2);
    auto __cross = ::__ycxx::__detail::__make_comp2(comp, __proj1, __proj2);
    _I2 r = __result_first;
    for (; first != last && r != __result_last; (void)++first, (void)++r)
      *r = *first;
    if (r != __result_first) {
      ::__ycxx::__detail::__make_heap_impl<__ranges_ops>(__result_first, r, less);
      auto __len = r - __result_first;
      for (; first != last; ++first)
        if (__cross(*first, *__result_first)) {
          *__result_first = *first;
          std::iter_value_t<_I2> __v(std::ranges::iter_move(__result_first));
          ::__ycxx::__detail::__heap_sift_hole<__ranges_ops>(__result_first, __len, 0, __v, less);
        }
      ::__ycxx::__detail::__sort_heap_impl<__ranges_ops>(__result_first, r, less);
    } else {
      first = ::__ycxx::__detail::__iter_at(std::move(first), last);
    }
    return {std::move(first), std::move(r)};
  }
  template <std::ranges::input_range _R1, std::ranges::random_access_range _R2, class _Comp = std::ranges::less,
            class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::indirectly_copyable<iterator_t<_R1>, iterator_t<_R2>> && std::sortable<iterator_t<_R2>, _Comp, _Proj2> &&
             std::indirect_strict_weak_order<_Comp, std::projected<iterator_t<_R1>, _Proj1>,
                                             std::projected<iterator_t<_R2>, _Proj2>>
  constexpr std::ranges::partial_sort_copy_result<borrowed_iterator_t<_R1>, borrowed_iterator_t<_R2>>
  operator()(_R1&& r, _R2&& __result_r, _Comp comp = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::ranges::begin(__result_r),
                   std::ranges::end(__result_r), std::move(comp), std::move(__proj1), std::move(__proj2));
  }
};
struct __is_sorted_until_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr _Ip operator()(_Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const {
    return ::__ycxx::__detail::__is_sorted_until_impl(std::move(first), last, ::__ycxx::__detail::__make_comp(comp, proj));
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Comp comp = {}, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct __is_sorted_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(_Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const {
    return ::__ycxx::__detail::__is_sorted_until_impl(std::move(first), last, ::__ycxx::__detail::__make_comp(comp, proj)) == last;
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(_Rp&& r, _Comp comp = {}, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct __nth_element_fn {
  template <std::random_access_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires std::sortable<_Ip, _Comp, _Proj>
  constexpr _Ip operator()(_Ip first, _Ip __nth, _Sp last, _Comp comp = {}, _Proj proj = {}) const {
    _Ip end = ::__ycxx::__detail::__iter_at(__nth, std::move(last));
    ::__ycxx::__detail::__nth_element_impl<__ranges_ops>(std::move(first), std::move(__nth), end,
                                                 ::__ycxx::__detail::__make_comp(comp, proj));
    return end;
  }
  template <std::ranges::random_access_range _Rp, class _Comp = std::ranges::less, class _Proj = std::identity>
    requires std::sortable<iterator_t<_Rp>, _Comp, _Proj>
  constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, iterator_t<_Rp> __nth, _Comp comp = {}, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::move(__nth), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};

// [alg.binary.search]
template <class _Comp, class _Proj, class _Tp>
struct __proj_less_value { // invoke(comp, invoke(proj, e), value)
  _Comp& comp;
  _Proj& proj;
  const _Tp& value;
  template <class _Ap>
  constexpr bool operator()(_Ap&& a) const {
    return static_cast<bool>(::__ycxx::__detail::invoke(comp, ::__ycxx::__detail::invoke(proj, static_cast<_Ap&&>(a)), value));
  }
};
template <class _Comp, class _Proj, class _Tp>
struct __proj_not_value_less { // !invoke(comp, value, invoke(proj, e))
  _Comp& comp;
  _Proj& proj;
  const _Tp& value;
  template <class _Ap>
  constexpr bool operator()(_Ap&& a) const {
    return !static_cast<bool>(::__ycxx::__detail::invoke(comp, value, ::__ycxx::__detail::invoke(proj, static_cast<_Ap&&>(a))));
  }
};

struct __lower_bound_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>,
            std::indirect_strict_weak_order<const _Tp*, std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr _Ip operator()(_Ip first, _Sp last, const _Tp& value, _Comp comp = {}, _Proj proj = {}) const {
    auto n = std::ranges::distance(first, last);
    return ::__ycxx::__detail::__partition_point_n(std::move(first), n, __proj_less_value<_Comp, _Proj, _Tp>{comp, proj, value});
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<iterator_t<_Rp>, _Proj>,
            std::indirect_strict_weak_order<const _Tp*, std::projected<iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, const _Tp& value, _Comp comp = {},
                                                           _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(comp), std::move(proj));
  }
};
struct __upper_bound_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>,
            std::indirect_strict_weak_order<const _Tp*, std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr _Ip operator()(_Ip first, _Sp last, const _Tp& value, _Comp comp = {}, _Proj proj = {}) const {
    auto n = std::ranges::distance(first, last);
    return ::__ycxx::__detail::__partition_point_n(std::move(first), n,
                                             __proj_not_value_less<_Comp, _Proj, _Tp>{comp, proj, value});
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<iterator_t<_Rp>, _Proj>,
            std::indirect_strict_weak_order<const _Tp*, std::projected<iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, const _Tp& value, _Comp comp = {},
                                                           _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(comp), std::move(proj));
  }
};
struct __equal_range_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>,
            std::indirect_strict_weak_order<const _Tp*, std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr std::ranges::subrange<_Ip> operator()(_Ip first, _Sp last, const _Tp& value, _Comp comp = {},
                                                              _Proj proj = {}) const {
    auto n = std::ranges::distance(first, last);
    _Ip __lo = ::__ycxx::__detail::__partition_point_n(first, n, __proj_less_value<_Comp, _Proj, _Tp>{comp, proj, value});
    n -= std::ranges::distance(first, __lo);
    _Ip __hi = ::__ycxx::__detail::__partition_point_n(__lo, n, __proj_not_value_less<_Comp, _Proj, _Tp>{comp, proj, value});
    return {std::move(__lo), std::move(__hi)};
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<iterator_t<_Rp>, _Proj>,
            std::indirect_strict_weak_order<const _Tp*, std::projected<iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr borrowed_subrange_t<_Rp> operator()(_Rp&& r, const _Tp& value, _Comp comp = {},
                                                           _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(comp), std::move(proj));
  }
};
struct __binary_search_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<_Ip, _Proj>,
            std::indirect_strict_weak_order<const _Tp*, std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(_Ip first, _Sp last, const _Tp& value, _Comp comp = {}, _Proj proj = {}) const {
    auto n = std::ranges::distance(first, last);
    _Ip i = ::__ycxx::__detail::__partition_point_n(std::move(first), n, __proj_less_value<_Comp, _Proj, _Tp>{comp, proj, value});
    return i != last && !static_cast<bool>(::__ycxx::__detail::invoke(comp, value, ::__ycxx::__detail::invoke(proj, *i)));
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            class _Tp = std::projected_value_t<iterator_t<_Rp>, _Proj>,
            std::indirect_strict_weak_order<const _Tp*, std::projected<iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(_Rp&& r, const _Tp& value, _Comp comp = {}, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), value, std::move(comp), std::move(proj));
  }
};

// [alg.partitions]
struct __is_partitioned_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
  [[nodiscard]] constexpr bool operator()(_Ip first, _Sp last, _Pred pred, _Proj proj = {}) const {
    auto p = ::__ycxx::__detail::__make_pred(pred, proj);
    first = ::__ycxx::__detail::__find_if_impl(std::move(first), last, ::__ycxx::__detail::__negated{p});
    if (first == last)
      return true;
    ++first;
    return ::__ycxx::__detail::__find_if_impl(std::move(first), last, p) == last;
  }
  template <std::ranges::input_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
  [[nodiscard]] constexpr bool operator()(_Rp&& r, _Pred pred, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};
struct __partition_fn {
  template <std::permutable _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
  constexpr std::ranges::subrange<_Ip> operator()(_Ip first, _Sp last, _Pred pred, _Proj proj = {}) const {
    auto r = ::__ycxx::__detail::__partition_impl<__ranges_ops>(std::move(first), std::move(last),
                                                        ::__ycxx::__detail::__make_pred(pred, proj));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
    requires std::permutable<iterator_t<_Rp>>
  constexpr borrowed_subrange_t<_Rp> operator()(_Rp&& r, _Pred pred, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};
struct __stable_partition_fn {
  template <std::bidirectional_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires std::permutable<_Ip>
  constexpr std::ranges::subrange<_Ip> operator()(_Ip first, _Sp last, _Pred pred, _Proj proj = {}) const {
    auto r = ::__ycxx::__detail::__stable_partition_impl<__ranges_ops>(std::move(first), std::move(last),
                                                               ::__ycxx::__detail::__make_pred(pred, proj));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::bidirectional_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
    requires std::permutable<iterator_t<_Rp>>
  constexpr borrowed_subrange_t<_Rp> operator()(_Rp&& r, _Pred pred, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};
struct __partition_copy_fn {
  template <std::input_iterator _Ip, std::sentinel_for<_Ip> _Sp, std::weakly_incrementable _O1, std::weakly_incrementable _O2,
            class _Proj = std::identity, std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
    requires std::indirectly_copyable<_Ip, _O1> && std::indirectly_copyable<_Ip, _O2>
  constexpr std::ranges::partition_copy_result<_Ip, _O1, _O2> operator()(_Ip first, _Sp last, _O1 __out_true, _O2 __out_false,
                                                                    _Pred pred, _Proj proj = {}) const {
    for (; first != last; ++first) {
      if (::__ycxx::__detail::invoke(pred, ::__ycxx::__detail::invoke(proj, *first))) {
        *__out_true = *first;
        ++__out_true;
      } else {
        *__out_false = *first;
        ++__out_false;
      }
    }
    return {std::move(first), std::move(__out_true), std::move(__out_false)};
  }
  template <std::ranges::input_range _Rp, std::weakly_incrementable _O1, std::weakly_incrementable _O2,
            class _Proj = std::identity, std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
    requires std::indirectly_copyable<iterator_t<_Rp>, _O1> && std::indirectly_copyable<iterator_t<_Rp>, _O2>
  constexpr std::ranges::partition_copy_result<borrowed_iterator_t<_Rp>, _O1, _O2>
  operator()(_Rp&& r, _O1 __out_true, _O2 __out_false, _Pred pred, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(__out_true), std::move(__out_false),
                   std::move(pred), std::move(proj));
  }
};
struct __partition_point_fn {
  template <std::forward_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<_Ip, _Proj>> _Pred>
  [[nodiscard]] constexpr _Ip operator()(_Ip first, _Sp last, _Pred pred, _Proj proj = {}) const {
    auto n = std::ranges::distance(first, last);
    return ::__ycxx::__detail::__partition_point_n(std::move(first), n, ::__ycxx::__detail::__make_pred(pred, proj));
  }
  template <std::ranges::forward_range _Rp, class _Proj = std::identity,
            std::indirect_unary_predicate<std::projected<iterator_t<_Rp>, _Proj>> _Pred>
  [[nodiscard]] constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Pred pred, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(pred), std::move(proj));
  }
};

// [alg.merge]
struct __merge_fn {
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::input_iterator _I2, std::sentinel_for<_I2> _S2,
            std::weakly_incrementable _Op, class _Comp = std::ranges::less, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires std::mergeable<_I1, _I2, _Op, _Comp, _Proj1, _Proj2>
  constexpr std::ranges::merge_result<_I1, _I2, _Op> operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Op result,
                                                           _Comp comp = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    ::__ycxx::__detail::__merge_loop(__first1, __last1, __first2, __last2, result, ::__ycxx::__detail::__make_comp2(comp, __proj1, __proj2));
    auto __r1 = ::__ycxx::__detail::__copy_dispatch(std::move(__first1), std::move(__last1), std::move(result));
    auto __r2 = ::__ycxx::__detail::__copy_dispatch(std::move(__first2), std::move(__last2), std::move(__r1.second));
    return {std::move(__r1.first), std::move(__r2.first), std::move(__r2.second)};
  }
  template <std::ranges::input_range _R1, std::ranges::input_range _R2, std::weakly_incrementable _Op,
            class _Comp = std::ranges::less, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::mergeable<iterator_t<_R1>, iterator_t<_R2>, _Op, _Comp, _Proj1, _Proj2>
  constexpr std::ranges::merge_result<borrowed_iterator_t<_R1>, borrowed_iterator_t<_R2>, _Op>
  operator()(_R1&& __r1, _R2&& __r2, _Op result, _Comp comp = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    return (*this)(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                   std::move(result), std::move(comp), std::move(__proj1), std::move(__proj2));
  }
};
struct __inplace_merge_fn {
  template <std::bidirectional_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires std::sortable<_Ip, _Comp, _Proj>
  constexpr _Ip operator()(_Ip first, _Ip __middle, _Sp last, _Comp comp = {}, _Proj proj = {}) const {
    _Ip end = ::__ycxx::__detail::__iter_at(__middle, std::move(last));
    ::__ycxx::__detail::__inplace_merge_impl<__ranges_ops>(std::move(first), std::move(__middle), end,
                                                   ::__ycxx::__detail::__make_comp(comp, proj));
    return end;
  }
  template <std::ranges::bidirectional_range _Rp, class _Comp = std::ranges::less, class _Proj = std::identity>
    requires std::sortable<iterator_t<_Rp>, _Comp, _Proj>
  constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, iterator_t<_Rp> __middle, _Comp comp = {}, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::move(__middle), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};

// [alg.set.operations]
struct __includes_fn {
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::input_iterator _I2, std::sentinel_for<_I2> _S2,
            class _Proj1 = std::identity, class _Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<_I1, _Proj1>, std::projected<_I2, _Proj2>> _Comp =
                std::ranges::less>
  [[nodiscard]] constexpr bool operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Comp comp = {}, _Proj1 __proj1 = {},
                                          _Proj2 __proj2 = {}) const {
    return ::__ycxx::__detail::__includes_impl(std::move(__first1), __last1, std::move(__first2), __last2,
                                         ::__ycxx::__detail::__make_comp2(comp, __proj1, __proj2));
  }
  template <std::ranges::input_range _R1, std::ranges::input_range _R2, class _Proj1 = std::identity,
            class _Proj2 = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<_R1>, _Proj1>, std::projected<iterator_t<_R2>, _Proj2>>
                _Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(_R1&& __r1, _R2&& __r2, _Comp comp = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    return (*this)(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                   std::move(comp), std::move(__proj1), std::move(__proj2));
  }
};
struct __set_union_fn {
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::input_iterator _I2, std::sentinel_for<_I2> _S2,
            std::weakly_incrementable _Op, class _Comp = std::ranges::less, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires std::mergeable<_I1, _I2, _Op, _Comp, _Proj1, _Proj2>
  constexpr std::ranges::set_union_result<_I1, _I2, _Op> operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Op result,
                                                               _Comp comp = {}, _Proj1 __proj1 = {},
                                                               _Proj2 __proj2 = {}) const {
    ::__ycxx::__detail::__set_union_loop(__first1, __last1, __first2, __last2, result, ::__ycxx::__detail::__make_comp2(comp, __proj1, __proj2));
    auto __r1 = ::__ycxx::__detail::__copy_dispatch(std::move(__first1), std::move(__last1), std::move(result));
    auto __r2 = ::__ycxx::__detail::__copy_dispatch(std::move(__first2), std::move(__last2), std::move(__r1.second));
    return {std::move(__r1.first), std::move(__r2.first), std::move(__r2.second)};
  }
  template <std::ranges::input_range _R1, std::ranges::input_range _R2, std::weakly_incrementable _Op,
            class _Comp = std::ranges::less, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::mergeable<iterator_t<_R1>, iterator_t<_R2>, _Op, _Comp, _Proj1, _Proj2>
  constexpr std::ranges::set_union_result<borrowed_iterator_t<_R1>, borrowed_iterator_t<_R2>, _Op>
  operator()(_R1&& __r1, _R2&& __r2, _Op result, _Comp comp = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    return (*this)(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                   std::move(result), std::move(comp), std::move(__proj1), std::move(__proj2));
  }
};
struct __set_intersection_fn {
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::input_iterator _I2, std::sentinel_for<_I2> _S2,
            std::weakly_incrementable _Op, class _Comp = std::ranges::less, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires std::mergeable<_I1, _I2, _Op, _Comp, _Proj1, _Proj2>
  constexpr std::ranges::set_intersection_result<_I1, _I2, _Op> operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2,
                                                                      _Op result, _Comp comp = {}, _Proj1 __proj1 = {},
                                                                      _Proj2 __proj2 = {}) const {
    ::__ycxx::__detail::__set_intersection_loop(__first1, __last1, __first2, __last2, result,
                                          ::__ycxx::__detail::__make_comp2(comp, __proj1, __proj2));
    _I1 __e1 = ::__ycxx::__detail::__iter_at(std::move(__first1), std::move(__last1));
    _I2 __e2 = ::__ycxx::__detail::__iter_at(std::move(__first2), std::move(__last2));
    return {std::move(__e1), std::move(__e2), std::move(result)};
  }
  template <std::ranges::input_range _R1, std::ranges::input_range _R2, std::weakly_incrementable _Op,
            class _Comp = std::ranges::less, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::mergeable<iterator_t<_R1>, iterator_t<_R2>, _Op, _Comp, _Proj1, _Proj2>
  constexpr std::ranges::set_intersection_result<borrowed_iterator_t<_R1>, borrowed_iterator_t<_R2>, _Op>
  operator()(_R1&& __r1, _R2&& __r2, _Op result, _Comp comp = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    return (*this)(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                   std::move(result), std::move(comp), std::move(__proj1), std::move(__proj2));
  }
};
struct __set_difference_fn {
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::input_iterator _I2, std::sentinel_for<_I2> _S2,
            std::weakly_incrementable _Op, class _Comp = std::ranges::less, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires std::mergeable<_I1, _I2, _Op, _Comp, _Proj1, _Proj2>
  constexpr std::ranges::set_difference_result<_I1, _Op> operator()(_I1 __first1, _S1 __last1, _I2 __first2, _S2 __last2, _Op result,
                                                                _Comp comp = {}, _Proj1 __proj1 = {},
                                                                _Proj2 __proj2 = {}) const {
    ::__ycxx::__detail::__set_difference_loop(__first1, __last1, __first2, __last2, result,
                                        ::__ycxx::__detail::__make_comp2(comp, __proj1, __proj2));
    auto r = ::__ycxx::__detail::__copy_dispatch(std::move(__first1), std::move(__last1), std::move(result));
    return {std::move(r.first), std::move(r.second)};
  }
  template <std::ranges::input_range _R1, std::ranges::input_range _R2, std::weakly_incrementable _Op,
            class _Comp = std::ranges::less, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::mergeable<iterator_t<_R1>, iterator_t<_R2>, _Op, _Comp, _Proj1, _Proj2>
  constexpr std::ranges::set_difference_result<borrowed_iterator_t<_R1>, _Op>
  operator()(_R1&& __r1, _R2&& __r2, _Op result, _Comp comp = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    return (*this)(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                   std::move(result), std::move(comp), std::move(__proj1), std::move(__proj2));
  }
};
struct __set_symmetric_difference_fn {
  template <std::input_iterator _I1, std::sentinel_for<_I1> _S1, std::input_iterator _I2, std::sentinel_for<_I2> _S2,
            std::weakly_incrementable _Op, class _Comp = std::ranges::less, class _Proj1 = std::identity,
            class _Proj2 = std::identity>
    requires std::mergeable<_I1, _I2, _Op, _Comp, _Proj1, _Proj2>
  constexpr std::ranges::set_symmetric_difference_result<_I1, _I2, _Op> operator()(_I1 __first1, _S1 __last1, _I2 __first2,
                                                                              _S2 __last2, _Op result, _Comp comp = {},
                                                                              _Proj1 __proj1 = {},
                                                                              _Proj2 __proj2 = {}) const {
    ::__ycxx::__detail::__set_symmetric_difference_loop(__first1, __last1, __first2, __last2, result,
                                                  ::__ycxx::__detail::__make_comp2(comp, __proj1, __proj2));
    auto __r1 = ::__ycxx::__detail::__copy_dispatch(std::move(__first1), std::move(__last1), std::move(result));
    auto __r2 = ::__ycxx::__detail::__copy_dispatch(std::move(__first2), std::move(__last2), std::move(__r1.second));
    return {std::move(__r1.first), std::move(__r2.first), std::move(__r2.second)};
  }
  template <std::ranges::input_range _R1, std::ranges::input_range _R2, std::weakly_incrementable _Op,
            class _Comp = std::ranges::less, class _Proj1 = std::identity, class _Proj2 = std::identity>
    requires std::mergeable<iterator_t<_R1>, iterator_t<_R2>, _Op, _Comp, _Proj1, _Proj2>
  constexpr std::ranges::set_symmetric_difference_result<borrowed_iterator_t<_R1>, borrowed_iterator_t<_R2>, _Op>
  operator()(_R1&& __r1, _R2&& __r2, _Op result, _Comp comp = {}, _Proj1 __proj1 = {}, _Proj2 __proj2 = {}) const {
    return (*this)(std::ranges::begin(__r1), std::ranges::end(__r1), std::ranges::begin(__r2), std::ranges::end(__r2),
                   std::move(result), std::move(comp), std::move(__proj1), std::move(__proj2));
  }
};

// [alg.heap.operations]
template <int _Op_>
struct __heap_op_fn {
  template <std::random_access_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires std::sortable<_Ip, _Comp, _Proj>
  constexpr _Ip operator()(_Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const {
    _Ip end = ::__ycxx::__detail::__iter_at(first, std::move(last));
    auto less = ::__ycxx::__detail::__make_comp(comp, proj);
    if constexpr (_Op_ == 0)
      ::__ycxx::__detail::__push_heap_impl<__ranges_ops>(std::move(first), end, less);
    else if constexpr (_Op_ == 1)
      ::__ycxx::__detail::__pop_heap_impl<__ranges_ops>(std::move(first), end, less);
    else if constexpr (_Op_ == 2)
      ::__ycxx::__detail::__make_heap_impl<__ranges_ops>(std::move(first), end, less);
    else
      ::__ycxx::__detail::__sort_heap_impl<__ranges_ops>(std::move(first), end, less);
    return end;
  }
  template <std::ranges::random_access_range _Rp, class _Comp = std::ranges::less, class _Proj = std::identity>
    requires std::sortable<iterator_t<_Rp>, _Comp, _Proj>
  constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Comp comp = {}, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct __is_heap_until_fn {
  template <std::random_access_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr _Ip operator()(_Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const {
    _Ip end = ::__ycxx::__detail::__iter_at(first, std::move(last));
    return ::__ycxx::__detail::__is_heap_until_impl(std::move(first), end, ::__ycxx::__detail::__make_comp(comp, proj));
  }
  template <std::ranges::random_access_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr borrowed_iterator_t<_Rp> operator()(_Rp&& r, _Comp comp = {}, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};
struct __is_heap_fn {
  template <std::random_access_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<_Ip, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(_Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const {
    _Ip end = ::__ycxx::__detail::__iter_at(first, std::move(last));
    return ::__ycxx::__detail::__is_heap_until_impl(std::move(first), end, ::__ycxx::__detail::__make_comp(comp, proj)) == end;
  }
  template <std::ranges::random_access_range _Rp, class _Proj = std::identity,
            std::indirect_strict_weak_order<std::projected<iterator_t<_Rp>, _Proj>> _Comp = std::ranges::less>
  [[nodiscard]] constexpr bool operator()(_Rp&& r, _Comp comp = {}, _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};

// [alg.permutation.generators]
template <bool _Next>
struct __permutation_fn {
  template <std::bidirectional_iterator _Ip, std::sentinel_for<_Ip> _Sp, class _Comp = std::ranges::less,
            class _Proj = std::identity>
    requires std::sortable<_Ip, _Comp, _Proj>
  constexpr std::ranges::in_found_result<_Ip> operator()(_Ip first, _Sp last, _Comp comp = {}, _Proj proj = {}) const {
    _Ip end = ::__ycxx::__detail::__iter_at(first, std::move(last));
    bool found;
    if constexpr (_Next)
      found = ::__ycxx::__detail::__next_permutation_impl<__ranges_ops>(std::move(first), end,
                                                                ::__ycxx::__detail::__make_comp(comp, proj));
    else
      found = ::__ycxx::__detail::__prev_permutation_impl<__ranges_ops>(std::move(first), end,
                                                                ::__ycxx::__detail::__make_comp(comp, proj));
    return {std::move(end), found};
  }
  template <std::ranges::bidirectional_range _Rp, class _Comp = std::ranges::less, class _Proj = std::identity>
    requires std::sortable<iterator_t<_Rp>, _Comp, _Proj>
  constexpr std::ranges::in_found_result<borrowed_iterator_t<_Rp>> operator()(_Rp&& r, _Comp comp = {},
                                                                           _Proj proj = {}) const {
    return (*this)(std::ranges::begin(r), std::ranges::end(r), std::move(comp), std::move(proj));
  }
};

}} // namespace __ycxx::__detail::__ranges_algo

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__sort_fn, __ycxx::__detail::par::kind::sort> sort{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__stable_sort_fn, __ycxx::__detail::par::kind::stable_sort> stable_sort{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__partial_sort_fn, __ycxx::__detail::par::kind::partial_sort> partial_sort{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__partial_sort_copy_fn, __ycxx::__detail::par::kind::partial_sort_copy> partial_sort_copy{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__is_sorted_fn, __ycxx::__detail::par::kind::is_sorted> is_sorted{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__is_sorted_until_fn, __ycxx::__detail::par::kind::is_sorted_until> is_sorted_until{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__nth_element_fn, __ycxx::__detail::par::kind::nth_element> nth_element{};
inline constexpr __ycxx::__detail::__ranges_algo::__lower_bound_fn lower_bound{};
inline constexpr __ycxx::__detail::__ranges_algo::__upper_bound_fn upper_bound{};
inline constexpr __ycxx::__detail::__ranges_algo::__equal_range_fn equal_range{};
inline constexpr __ycxx::__detail::__ranges_algo::__binary_search_fn binary_search{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__is_partitioned_fn, __ycxx::__detail::par::kind::is_partitioned> is_partitioned{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__partition_fn, __ycxx::__detail::par::kind::partition> partition{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__stable_partition_fn, __ycxx::__detail::par::kind::stable_partition> stable_partition{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__partition_copy_fn, __ycxx::__detail::par::kind::partition_copy> partition_copy{};
inline constexpr __ycxx::__detail::__ranges_algo::__partition_point_fn partition_point{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__merge_fn, __ycxx::__detail::par::kind::merge> merge{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__inplace_merge_fn, __ycxx::__detail::par::kind::inplace_merge> inplace_merge{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__includes_fn, __ycxx::__detail::par::kind::includes> includes{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__set_union_fn, __ycxx::__detail::par::kind::set_union> set_union{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__set_intersection_fn, __ycxx::__detail::par::kind::set_intersection> set_intersection{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__set_difference_fn, __ycxx::__detail::par::kind::set_difference> set_difference{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__set_symmetric_difference_fn, __ycxx::__detail::par::kind::set_symmetric_difference> set_symmetric_difference{};
inline constexpr __ycxx::__detail::__ranges_algo::__heap_op_fn<0> push_heap{};
inline constexpr __ycxx::__detail::__ranges_algo::__heap_op_fn<1> pop_heap{};
inline constexpr __ycxx::__detail::__ranges_algo::__heap_op_fn<2> make_heap{};
inline constexpr __ycxx::__detail::__ranges_algo::__heap_op_fn<3> sort_heap{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__is_heap_fn, __ycxx::__detail::par::kind::is_heap> is_heap{};
inline constexpr __ycxx::__adl_free::__ranges_par_algo<__ycxx::__detail::__ranges_algo::__is_heap_until_fn, __ycxx::__detail::par::kind::is_heap_until> is_heap_until{};
inline constexpr __ycxx::__detail::__ranges_algo::__permutation_fn<true> next_permutation{};
inline constexpr __ycxx::__detail::__ranges_algo::__permutation_fn<false> prev_permutation{};
}} // namespace std::ranges
