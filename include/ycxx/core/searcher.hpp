// libycxx core: the searchers of <functional> ([func.search]). default_searcher is built on the
// search algorithm. boyer_moore_searcher (bad-character and good-suffix rules) and
// boyer_moore_horspool_searcher (bad-character rule on the last aligned element) keep their
// tables in heap arrays owned by the searcher (deep-copied with it).
//
// Bad-character table. For a byte-sized integral value type compared with equal_to it is a
// 256-entry array indexed by the value (hf and pred are then never called). Otherwise it is an
// open-addressing hash table with one entry per equivalence class of pattern elements; an entry
// keeps the full hash value and the index of a representative pattern element, and a lookup
// calls pred only for an entry with the same hash value ([func.search.bm]/2: pred(A, B) implies
// hf(A) == hf(B)).
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
#include <ycxx/core/hash.hpp>
#include <ycxx/core/iterator_core.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// A heap array owned by its holder, copied deeply.
template <class T>
class searcher_array {
  T* p_ = nullptr;
  std::size_t n_ = 0;

public:
  searcher_array() = default;
  explicit searcher_array(std::size_t n) : p_(n != 0 ? new T[n]() : nullptr), n_(n) {}
  searcher_array(const searcher_array& o) : searcher_array(o.n_) {
    for (std::size_t k = 0; k != n_; ++k) p_[k] = o.p_[k];
  }
  searcher_array(searcher_array&& o) noexcept : p_(o.p_), n_(o.n_) {
    o.p_ = nullptr;
    o.n_ = 0;
  }
  searcher_array& operator=(searcher_array o) noexcept {
    T* p = p_;
    p_ = o.p_;
    o.p_ = p;
    std::size_t n = n_;
    n_ = o.n_;
    o.n_ = n;
    return *this;
  }
  ~searcher_array() { delete[] p_; }

  T* data() const noexcept { return p_; }
  std::size_t size() const noexcept { return n_; }
  T& operator[](std::size_t k) const noexcept { return p_[k]; }
};

struct searcher_slot {
  std::size_t hash = 0;
  std::ptrdiff_t key = -1; // index of the class's representative in the pattern; -1: empty
  std::ptrdiff_t value = 0;
};

template <class V, class Hash, class Pred>
inline constexpr bool searcher_byte_table =
    std::is_integral_v<V> && sizeof(V) == 1 &&
    (std::is_same_v<Pred, std::equal_to<>> || std::is_same_v<Pred, std::equal_to<V>>);

// The bad-character table: maps the class of a value to a ptrdiff_t, `none` when absent.
template <class RAI1, class Hash, class Pred>
class searcher_table {
  using V = typename std::iterator_traits<RAI1>::value_type;
  static constexpr bool bytes = searcher_byte_table<V, Hash, Pred>;

  searcher_array<std::conditional_t<bytes, std::ptrdiff_t, searcher_slot>> slots_;

public:
  enum : std::ptrdiff_t { none = -1, unknown = -2 };

  searcher_table() = default;

  explicit searcher_table(std::size_t count) {
    if constexpr (bytes) {
      slots_ = searcher_array<std::ptrdiff_t>(256);
      for (std::size_t k = 0; k != 256; ++k) slots_[k] = none;
    } else {
      std::size_t cap = 4;
      while (cap < 2 * count) cap *= 2;
      slots_ = searcher_array<searcher_slot>(cap);
    }
  }

  // Sets the value of the class of pat[k] (inserting the class).
  void set(const RAI1& pat, std::ptrdiff_t k, std::ptrdiff_t value, const Hash& hf, const Pred& pred) {
    if constexpr (bytes) {
      slots_[static_cast<unsigned char>(pat[k])] = value;
    } else {
      const std::size_t h = static_cast<std::size_t>(hf(pat[k]));
      const std::size_t mask = slots_.size() - 1;
      for (std::size_t s = h & mask;; s = (s + 1) & mask) {
        searcher_slot& slot = slots_[s];
        if (slot.key < 0) {
          slot.hash = h;
          slot.key = k;
          slot.value = value;
          return;
        }
        if (slot.hash == h && pred(pat[k], pat[slot.key])) {
          slot.value = value;
          return;
        }
      }
    }
  }

  // The value of the class of x: none when absent, unknown when more than `budget` calls of pred
  // would be needed to tell.
  template <class T>
  std::ptrdiff_t find(const T& x, const RAI1& pat, const Hash& hf, const Pred& pred, std::ptrdiff_t budget) const {
    if constexpr (bytes) {
      (void)pat;
      (void)hf;
      (void)pred;
      (void)budget;
      return slots_[static_cast<unsigned char>(x)];
    } else {
      const std::size_t h = static_cast<std::size_t>(hf(x));
      const std::size_t mask = slots_.size() - 1;
      for (std::size_t s = h & mask;; s = (s + 1) & mask) {
        const searcher_slot& slot = slots_[s];
        if (slot.key < 0) return none;
        if (slot.hash == h) {
          if (budget-- == 0) return unknown;
          if (pred(x, pat[slot.key])) return slot.value;
        }
      }
    }
  }
};

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

template <class ForwardIterator1, class BinaryPredicate = equal_to<>>
class default_searcher {
  ForwardIterator1 pat_first_;
  ForwardIterator1 pat_last_;
  BinaryPredicate pred_;

public:
  constexpr default_searcher(ForwardIterator1 pat_first, ForwardIterator1 pat_last,
                             BinaryPredicate pred = BinaryPredicate())
      : pat_first_(pat_first), pat_last_(pat_last), pred_(pred) {}

  template <class ForwardIterator2>
  constexpr pair<ForwardIterator2, ForwardIterator2> operator()(ForwardIterator2 first, ForwardIterator2 last) const {
    BinaryPredicate pred = pred_; // std::search takes its predicate by value
    return ::ycxx::detail::search_impl(first, last, pat_first_, pat_last_, ::ycxx::detail::ref_pred(pred));
  }
};

template <class RandomAccessIterator1, class Hash = hash<typename iterator_traits<RandomAccessIterator1>::value_type>,
          class BinaryPredicate = equal_to<>>
class boyer_moore_searcher {
  using table_type = ::ycxx::detail::searcher_table<RandomAccessIterator1, Hash, BinaryPredicate>;

  RandomAccessIterator1 pat_first_;
  RandomAccessIterator1 pat_last_;
  Hash hash_;
  BinaryPredicate pred_;
  table_type last_;                                    // class -> index of its last occurrence
  ::ycxx::detail::searcher_array<ptrdiff_t> good_suffix_; // mismatch index -> shift

public:
  boyer_moore_searcher(RandomAccessIterator1 pat_first, RandomAccessIterator1 pat_last, Hash hf = Hash(),
                       BinaryPredicate pred = BinaryPredicate())
      : pat_first_(pat_first), pat_last_(pat_last), hash_(hf), pred_(pred) {
    const ptrdiff_t m = static_cast<ptrdiff_t>(pat_last_ - pat_first_);
    if (m == 0) return;
    const RandomAccessIterator1& p = pat_first_;
    last_ = table_type(static_cast<size_t>(m));
    for (ptrdiff_t k = 0; k != m; ++k) last_.set(p, k, k, hash_, pred_);

    // suffix[i]: the length of the longest common suffix of p[0, i] and p.
    ::ycxx::detail::searcher_array<ptrdiff_t> suffix(static_cast<size_t>(m));
    suffix[m - 1] = m;
    ptrdiff_t g = m - 1, f = m - 1;
    for (ptrdiff_t i = m - 2; i >= 0; --i) {
      if (i > g && suffix[i + m - 1 - f] < i - g) {
        suffix[i] = suffix[i + m - 1 - f];
      } else {
        if (i < g) g = i;
        f = i;
        while (g >= 0 && pred_(p[g], p[g + m - 1 - f])) --g;
        suffix[i] = f - g;
      }
    }
    good_suffix_ = ::ycxx::detail::searcher_array<ptrdiff_t>(static_cast<size_t>(m));
    for (ptrdiff_t i = 0; i != m; ++i) good_suffix_[i] = m;
    ptrdiff_t j = 0;
    for (ptrdiff_t i = m - 1; i >= 0; --i) {
      if (suffix[i] == i + 1) {
        for (; j < m - 1 - i; ++j)
          if (good_suffix_[j] == m) good_suffix_[j] = m - 1 - i;
      }
    }
    for (ptrdiff_t i = 0; i <= m - 2; ++i) good_suffix_[m - 1 - suffix[i]] = m - 1 - i;
  }

  template <class RandomAccessIterator2>
  pair<RandomAccessIterator2, RandomAccessIterator2> operator()(RandomAccessIterator2 first,
                                                                RandomAccessIterator2 last) const {
    static_assert(is_same_v<typename iterator_traits<RandomAccessIterator1>::value_type,
                            typename iterator_traits<RandomAccessIterator2>::value_type>,
                  "boyer_moore_searcher: the pattern and the text must have the same value type");
    using D = typename iterator_traits<RandomAccessIterator2>::difference_type;
    const ptrdiff_t m = static_cast<ptrdiff_t>(pat_last_ - pat_first_);
    if (m == 0) return {first, first};
    const D n = last - first;
    const RandomAccessIterator1& p = pat_first_;
    for (D i = 0; n - i >= static_cast<D>(m);) {
      ptrdiff_t j = m - 1;
      while (j >= 0 && pred_(first[i + static_cast<D>(j)], p[j])) --j;
      if (j < 0) {
        RandomAccessIterator2 r = first + i;
        return {r, r + static_cast<D>(m)};
      }
      ptrdiff_t shift = good_suffix_[j];
      if (shift < j + 1) {
        const ptrdiff_t at = last_.find(first[i + static_cast<D>(j)], p, hash_, pred_, j);
        if (at == table_type::none)
          shift = j + 1;
        else if (at != table_type::unknown && j - at > shift)
          shift = j - at;
      }
      i += static_cast<D>(shift);
    }
    return {last, last};
  }
};

template <class RandomAccessIterator1, class Hash = hash<typename iterator_traits<RandomAccessIterator1>::value_type>,
          class BinaryPredicate = equal_to<>>
class boyer_moore_horspool_searcher {
  using table_type = ::ycxx::detail::searcher_table<RandomAccessIterator1, Hash, BinaryPredicate>;

  RandomAccessIterator1 pat_first_;
  RandomAccessIterator1 pat_last_;
  Hash hash_;
  BinaryPredicate pred_;
  table_type shift_;      // class -> distance of its last occurrence in p[0, m-1) from p[m-1]
  ptrdiff_t last_shift_ = 0; // the shift for the class of p[m-1]

public:
  boyer_moore_horspool_searcher(RandomAccessIterator1 pat_first, RandomAccessIterator1 pat_last, Hash hf = Hash(),
                                BinaryPredicate pred = BinaryPredicate())
      : pat_first_(pat_first), pat_last_(pat_last), hash_(hf), pred_(pred) {
    const ptrdiff_t m = static_cast<ptrdiff_t>(pat_last_ - pat_first_);
    if (m == 0) return;
    const RandomAccessIterator1& p = pat_first_;
    shift_ = table_type(static_cast<size_t>(m - 1));
    for (ptrdiff_t k = 0; k != m - 1; ++k) shift_.set(p, k, m - 1 - k, hash_, pred_);
    const ptrdiff_t s = shift_.find(p[m - 1], p, hash_, pred_, m);
    last_shift_ = s == table_type::none ? m : s;
  }

  template <class RandomAccessIterator2>
  pair<RandomAccessIterator2, RandomAccessIterator2> operator()(RandomAccessIterator2 first,
                                                                RandomAccessIterator2 last) const {
    static_assert(is_same_v<typename iterator_traits<RandomAccessIterator1>::value_type,
                            typename iterator_traits<RandomAccessIterator2>::value_type>,
                  "boyer_moore_horspool_searcher: the pattern and the text must have the same value type");
    using D = typename iterator_traits<RandomAccessIterator2>::difference_type;
    const ptrdiff_t m = static_cast<ptrdiff_t>(pat_last_ - pat_first_);
    if (m == 0) return {first, first};
    const D n = last - first;
    const RandomAccessIterator1& p = pat_first_;
    for (D i = 0; n - i >= static_cast<D>(m);) {
      const D end = i + static_cast<D>(m - 1);
      ptrdiff_t shift;
      if (pred_(first[end], p[m - 1])) {
        ptrdiff_t j = m - 2;
        while (j >= 0 && pred_(first[i + static_cast<D>(j)], p[j])) --j;
        if (j < 0) {
          RandomAccessIterator2 r = first + i;
          return {r, r + static_cast<D>(m)};
        }
        shift = last_shift_;
      } else {
        // The table holds at most m - 1 classes, so the lookup never runs out of its budget.
        const ptrdiff_t s = shift_.find(first[end], p, hash_, pred_, m - 1);
        shift = s < 0 ? (s == table_type::none ? m : 1) : s;
      }
      i += static_cast<D>(shift);
    }
    return {last, last};
  }
};

} // namespace std
