// libycxx core: the searchers of <functional> ([func.search]). default_searcher is built on the
// search algorithm. boyer_moore_searcher (bad-character and good-suffix rules) and
// boyer_moore_horspool_searcher (bad-character rule on the last aligned element) keep their
// tables in heap arrays owned by the searcher (deep-copied with it).
//
// Bad-character table. For a byte-sized integral value type or std::byte compared with
// equal_to it is a 256-entry array indexed by the value (hf and pred are then never called).
// Otherwise it is an open-addressing hash table with one entry per equivalence class of pattern
// elements; an entry keeps the full hash value and the index of a representative pattern
// element, and a lookup calls pred only for an entry with the same hash value
// ([func.search.bm]/2: pred(A, B) implies hf(A) == hf(B)).
//
// Complexity ([func.search.bm]/8: at most (last - first) * (pat_last_ - pat_first_) applications
// of pred). The lookups count too, so every alignment of the m-element pattern is held to at most
// m calls: the alignments are at most last - first, and each compares right to left.
// - Horspool shifts by the class of the text element under the last pattern position. When that
//   element matched pat[m-1], its class is known (precomputed shift, no lookup); otherwise the
//   alignment used one comparison and the lookup at most m - 1 (the table holds at most m - 1
//   classes).
// - Boyer-Moore: after a mismatch at pattern index j (m - j comparisons) the bad-character shift
//   can exceed the good-suffix shift only when that is below j + 1; the lookup is then given a
//   budget of j calls, and a lookup that runs out contributes no shift (the good-suffix shift
//   alone is always safe).
#pragma once

#include <ycxx/core/algo_nonmod.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/iterator_core.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// A heap array owned by its holder, copied deeply. It has no move operations: a moved-from
// searcher must still search, so moving one copies its tables.
template <class _Tp>
class __searcher_array {
  _Tp* __p_ = nullptr;
  std::size_t __n_ = 0;

public:
  __searcher_array() = default;
  explicit __searcher_array(std::size_t n) : __p_(n != 0 ? new _Tp[n]() : nullptr), __n_(n) {}
  __searcher_array(const __searcher_array& __o) : __searcher_array(__o.__n_) {
    for (std::size_t k = 0; k != __n_; ++k) __p_[k] = __o.__p_[k];
  }
  __searcher_array& operator=(const __searcher_array& __o) {
    __searcher_array copy(__o);
    _Tp* p = __p_;
    __p_ = copy.__p_;
    copy.__p_ = p;
    std::size_t n = __n_;
    __n_ = copy.__n_;
    copy.__n_ = n;
    return *this;
  }
  ~__searcher_array() { delete[] __p_; }

  std::size_t size() const noexcept { return __n_; }
  _Tp& operator[](std::size_t k) const noexcept { return __p_[k]; }
};

struct __searcher_slot {
  std::size_t hash = 0;
  std::ptrdiff_t key = -1; // index of the class's representative in the pattern; -1: empty
  std::ptrdiff_t value = 0;
};

template <class _Vp, class _Hash, class _Pred>
inline constexpr bool __searcher_byte_table =
    (std::is_integral_v<_Vp> || std::is_same_v<_Vp, std::byte>) && sizeof(_Vp) == 1 &&
    (std::is_same_v<_Pred, std::equal_to<>> || std::is_same_v<_Pred, std::equal_to<_Vp>>);

// The bad-character table: maps the class of a value to a ptrdiff_t, `none` when absent.
template <class _RAI1, class _Hash, class _Pred>
class __searcher_table {
  using _Vp = typename std::iterator_traits<_RAI1>::value_type;
  static constexpr bool bytes = __searcher_byte_table<_Vp, _Hash, _Pred>;

  __searcher_array<std::conditional_t<bytes, std::ptrdiff_t, __searcher_slot>> __slots_;

public:
  enum : std::ptrdiff_t { none = -1, unknown = -2 };

  // A table for up to `count` classes (none: an empty table, in which every lookup fails).
  explicit __searcher_table(std::size_t count) : __slots_(count == 0 ? 0 : bytes ? 256 : capacity(count)) {
    if constexpr (bytes)
      for (std::size_t k = 0; k != __slots_.size(); ++k) __slots_[k] = none;
  }

private:
  static constexpr std::size_t capacity(std::size_t count) noexcept {
    std::size_t __cap = 4; // a power of two, at least twice count: a probe always ends
    while (__cap < 2 * count) __cap *= 2;
    return __cap;
  }

public:
  // Sets the value of the class of pat[k] (inserting the class).
  void set(const _RAI1& __pat, std::ptrdiff_t k, std::ptrdiff_t value, const _Hash& __hf, const _Pred& pred) {
    if constexpr (bytes) {
      __slots_[static_cast<unsigned char>(__pat[k])] = value;
    } else {
      const std::size_t h = static_cast<std::size_t>(__hf(__pat[k]));
      const std::size_t mask = __slots_.size() - 1;
      for (std::size_t s = h & mask;; s = (s + 1) & mask) {
        __searcher_slot& __slot = __slots_[s];
        if (__slot.key < 0) {
          __slot.hash = h;
          __slot.key = k;
          __slot.value = value;
          return;
        }
        if (__slot.hash == h && static_cast<bool>(pred(__pat[k], __pat[__slot.key]))) {
          __slot.value = value;
          return;
        }
      }
    }
  }

  // The value of the class of x: none when absent, unknown when more than `__budget` calls of pred
  // would be needed to tell.
  template <class _Tp>
  std::ptrdiff_t find(const _Tp& __x, const _RAI1& __pat, const _Hash& __hf, const _Pred& pred, std::ptrdiff_t __budget) const {
    if (__slots_.size() == 0) return none;
    if constexpr (bytes) {
      (void)__pat;
      (void)__hf;
      (void)pred;
      (void)__budget;
      return __slots_[static_cast<unsigned char>(__x)];
    } else {
      const std::size_t h = static_cast<std::size_t>(__hf(__x));
      const std::size_t mask = __slots_.size() - 1;
      for (std::size_t s = h & mask;; s = (s + 1) & mask) {
        const __searcher_slot& __slot = __slots_[s];
        if (__slot.key < 0) return none;
        if (__slot.hash == h) {
          if (__budget-- == 0) return unknown;
          if (static_cast<bool>(pred(__x, __pat[__slot.key]))) return __slot.value;
        }
      }
    }
  }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _ForwardIterator1, class _BinaryPredicate = equal_to<>>
class default_searcher {
  _ForwardIterator1 __pat_first_;
  _ForwardIterator1 __pat_last_;
  _BinaryPredicate __pred_;

public:
  constexpr default_searcher(_ForwardIterator1 __pat_first, _ForwardIterator1 __pat_last,
                             _BinaryPredicate pred = _BinaryPredicate())
      : __pat_first_(__pat_first), __pat_last_(__pat_last), __pred_(pred) {}

  template <class _ForwardIterator2>
  constexpr pair<_ForwardIterator2, _ForwardIterator2> operator()(_ForwardIterator2 first, _ForwardIterator2 last) const {
    _BinaryPredicate pred = __pred_; // std::search takes its predicate by value
    return ::__ycxx::__detail::__search_impl(first, last, __pat_first_, __pat_last_, ::__ycxx::__detail::__ref_pred(pred));
  }
};

template <class _RandomAccessIterator1, class _Hash = hash<typename iterator_traits<_RandomAccessIterator1>::value_type>,
          class _BinaryPredicate = equal_to<>>
class boyer_moore_searcher {
  using __table_type = ::__ycxx::__detail::__searcher_table<_RandomAccessIterator1, _Hash, _BinaryPredicate>;

  _RandomAccessIterator1 __pat_first_;
  _RandomAccessIterator1 __pat_last_;
  _Hash __hash_;
  _BinaryPredicate __pred_;
  __table_type __last_;                                       // class -> index of its last occurrence
  ::__ycxx::__detail::__searcher_array<ptrdiff_t> __good_suffix_; // mismatch index -> shift

public:
  boyer_moore_searcher(_RandomAccessIterator1 __pat_first, _RandomAccessIterator1 __pat_last, _Hash __hf = _Hash(),
                       _BinaryPredicate pred = _BinaryPredicate())
      : __pat_first_(__pat_first), __pat_last_(__pat_last), __hash_(__hf), __pred_(pred),
        __last_(static_cast<size_t>(__pat_last - __pat_first)), __good_suffix_(static_cast<size_t>(__pat_last - __pat_first)) {
    const ptrdiff_t m = static_cast<ptrdiff_t>(__pat_last_ - __pat_first_);
    if (m == 0) return;
    const _RandomAccessIterator1& p = __pat_first_;
    for (ptrdiff_t k = 0; k != m; ++k) __last_.set(p, k, k, __hash_, __pred_);

    // suffix[i]: the length of the longest common suffix of p[0, i] and p.
    ::__ycxx::__detail::__searcher_array<ptrdiff_t> suffix(static_cast<size_t>(m));
    suffix[m - 1] = m;
    ptrdiff_t __g = m - 1, __f = m - 1;
    for (ptrdiff_t i = m - 2; i >= 0; --i) {
      if (i > __g && suffix[i + m - 1 - __f] < i - __g) {
        suffix[i] = suffix[i + m - 1 - __f];
      } else {
        if (i < __g) __g = i;
        __f = i;
        while (__g >= 0 && static_cast<bool>(__pred_(p[__g], p[__g + m - 1 - __f]))) --__g;
        suffix[i] = __f - __g;
      }
    }
    for (ptrdiff_t i = 0; i != m; ++i) __good_suffix_[i] = m;
    ptrdiff_t __j = 0;
    for (ptrdiff_t i = m - 1; i >= 0; --i) {
      if (suffix[i] == i + 1) {
        for (; __j < m - 1 - i; ++__j)
          if (__good_suffix_[__j] == m) __good_suffix_[__j] = m - 1 - i;
      }
    }
    for (ptrdiff_t i = 0; i <= m - 2; ++i) __good_suffix_[m - 1 - suffix[i]] = m - 1 - i;
  }

  template <class _RandomAccessIterator2>
  pair<_RandomAccessIterator2, _RandomAccessIterator2> operator()(_RandomAccessIterator2 first,
                                                                _RandomAccessIterator2 last) const {
    static_assert(is_same_v<typename iterator_traits<_RandomAccessIterator1>::value_type,
                            typename iterator_traits<_RandomAccessIterator2>::value_type>,
                  "boyer_moore_searcher: the pattern and the text must have the same value type");
    using _Dp = typename iterator_traits<_RandomAccessIterator2>::difference_type;
    const ptrdiff_t m = static_cast<ptrdiff_t>(__pat_last_ - __pat_first_);
    if (m == 0) return {first, first};
    const _Dp n = last - first;
    const _RandomAccessIterator1& p = __pat_first_;
    for (_Dp i = 0; n - i >= static_cast<_Dp>(m);) {
      ptrdiff_t __j = m - 1;
      while (__j >= 0 && static_cast<bool>(__pred_(first[i + static_cast<_Dp>(__j)], p[__j]))) --__j;
      if (__j < 0) {
        _RandomAccessIterator2 r = first + i;
        return {r, r + static_cast<_Dp>(m)};
      }
      ptrdiff_t shift = __good_suffix_[__j];
      if (shift < __j + 1) {
        const ptrdiff_t at = __last_.find(first[i + static_cast<_Dp>(__j)], p, __hash_, __pred_, __j);
        if (at == __table_type::none)
          shift = __j + 1;
        else if (at != __table_type::unknown && __j - at > shift)
          shift = __j - at;
      }
      i += static_cast<_Dp>(shift);
    }
    return {last, last};
  }
};

template <class _RandomAccessIterator1, class _Hash = hash<typename iterator_traits<_RandomAccessIterator1>::value_type>,
          class _BinaryPredicate = equal_to<>>
class boyer_moore_horspool_searcher {
  using __table_type = ::__ycxx::__detail::__searcher_table<_RandomAccessIterator1, _Hash, _BinaryPredicate>;

  _RandomAccessIterator1 __pat_first_;
  _RandomAccessIterator1 __pat_last_;
  _Hash __hash_;
  _BinaryPredicate __pred_;
  __table_type __shift_;         // class -> distance of its last occurrence in p[0, m-1) from p[m-1]
  ptrdiff_t __last_shift_ = 0; // the shift for the class of p[m-1]

public:
  boyer_moore_horspool_searcher(_RandomAccessIterator1 __pat_first, _RandomAccessIterator1 __pat_last, _Hash __hf = _Hash(),
                                _BinaryPredicate pred = _BinaryPredicate())
      : __pat_first_(__pat_first), __pat_last_(__pat_last), __hash_(__hf), __pred_(pred),
        __shift_(__pat_last == __pat_first ? 0 : static_cast<size_t>(__pat_last - __pat_first) - 1) {
    const ptrdiff_t m = static_cast<ptrdiff_t>(__pat_last_ - __pat_first_);
    if (m == 0) return;
    const _RandomAccessIterator1& p = __pat_first_;
    for (ptrdiff_t k = 0; k != m - 1; ++k) __shift_.set(p, k, m - 1 - k, __hash_, __pred_);
    const ptrdiff_t s = __shift_.find(p[m - 1], p, __hash_, __pred_, m);
    __last_shift_ = s == __table_type::none ? m : s;
  }

  template <class _RandomAccessIterator2>
  pair<_RandomAccessIterator2, _RandomAccessIterator2> operator()(_RandomAccessIterator2 first,
                                                                _RandomAccessIterator2 last) const {
    static_assert(is_same_v<typename iterator_traits<_RandomAccessIterator1>::value_type,
                            typename iterator_traits<_RandomAccessIterator2>::value_type>,
                  "boyer_moore_horspool_searcher: the pattern and the text must have the same value type");
    using _Dp = typename iterator_traits<_RandomAccessIterator2>::difference_type;
    const ptrdiff_t m = static_cast<ptrdiff_t>(__pat_last_ - __pat_first_);
    if (m == 0) return {first, first};
    const _Dp n = last - first;
    const _RandomAccessIterator1& p = __pat_first_;
    for (_Dp i = 0; n - i >= static_cast<_Dp>(m);) {
      const _Dp end = i + static_cast<_Dp>(m - 1);
      ptrdiff_t shift;
      if (static_cast<bool>(__pred_(first[end], p[m - 1]))) {
        ptrdiff_t __j = m - 2;
        while (__j >= 0 && static_cast<bool>(__pred_(first[i + static_cast<_Dp>(__j)], p[__j]))) --__j;
        if (__j < 0) {
          _RandomAccessIterator2 r = first + i;
          return {r, r + static_cast<_Dp>(m)};
        }
        shift = __last_shift_;
      } else {
        // The table holds at most m - 1 classes, so the lookup never runs out of its budget.
        const ptrdiff_t s = __shift_.find(first[end], p, __hash_, __pred_, m - 1);
        shift = s < 0 ? (s == __table_type::none ? m : 1) : s;
      }
      i += static_cast<_Dp>(shift);
    }
    return {last, last};
  }
};

} // namespace std
