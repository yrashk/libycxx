// libycxx core: vector<bool, Allocator> ([vector.bool]) and hash<vector<bool, Allocator>>.
//
// Representation: the bits packed into size_t words from the rebound allocator
// ({words_, size_ in bits, cap_ in words}); bit i is bit (i % digits) of word i / digits. Words
// are value-initialized when allocated (allocator_traits::construct is not used for them,
// [vector.bool.pspc]/2), so every word is readable in constant evaluation. Bits at or past
// size() have unspecified values: every operation that reads them masks them.
//
// reference and the iterators are shared by all vector<bool> specializations with the same
// difference type (ycxx::adl_free::bit_ref / bit_iter). The formatter for reference
// ([vector.bool.fmt]) is defined with <format> (ycxx/core/format_ranges.hpp).
#pragma once

#include <ycxx/core/bit_iter_algos.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/vector.hpp>

namespace ycxx::adl_free {

template <class Word, class Diff, bool Const>
class bit_iter;

// vector<bool>::reference ([vector.bool.pspc]/4-11): a word pointer and a one-bit mask.
template <class Word>
class bit_ref {
  template <class T, class A>
  friend class std::vector;
  template <class W, class D, bool C>
  friend class bit_iter;

  Word* w_;
  Word m_;
  constexpr bit_ref(Word* w, Word m) noexcept : w_(w), m_(m) {}

public:
  constexpr bit_ref(const bit_ref&) noexcept = default;
  constexpr ~bit_ref() = default;
  constexpr bit_ref& operator=(bool x) noexcept {
    if (x)
      *w_ |= m_;
    else
      *w_ &= static_cast<Word>(~m_);
    return *this;
  }
  constexpr bit_ref& operator=(const bit_ref& x) noexcept { return *this = static_cast<bool>(x); }
  constexpr const bit_ref& operator=(bool x) const noexcept {
    if (x)
      *w_ |= m_;
    else
      *w_ &= static_cast<Word>(~m_);
    return *this;
  }
  constexpr operator bool() const noexcept { return (*w_ & m_) != 0; }
  constexpr void flip() noexcept { *w_ ^= m_; }

  friend constexpr void swap(bit_ref x, bit_ref y) noexcept {
    const bool b = x;
    x = static_cast<bool>(y);
    y = b;
  }
  friend constexpr void swap(bit_ref x, bool& y) noexcept {
    const bool b = x;
    x = y;
    y = b;
  }
  friend constexpr void swap(bool& x, bit_ref y) noexcept {
    const bool b = x;
    x = static_cast<bool>(y);
    y = b;
  }
};

// vector<bool>::iterator / const_iterator: a random access iterator over bits, {word, bit}.
template <class Word, class Diff, bool Const>
class bit_iter {
  template <class T, class A>
  friend class std::vector;
  template <class W, class D, bool C>
  friend class bit_iter;
  template <class I>
  friend struct ycxx::detail::bit_algos;

  using wptr = std::conditional_t<Const, const Word*, Word*>;
  static constexpr std::ptrdiff_t bits = std::numeric_limits<Word>::digits;

  wptr w_ = nullptr;
  unsigned b_ = 0;
  constexpr bit_iter(wptr w, unsigned b) noexcept : w_(w), b_(b) {}

public:
  using iterator_concept = std::random_access_iterator_tag;
  using iterator_category = std::random_access_iterator_tag;
  using value_type = bool;
  using difference_type = Diff;
  using pointer = void;
  using reference = std::conditional_t<Const, bool, bit_ref<Word>>;

  constexpr bit_iter() noexcept = default;
  template <bool C2>
    requires(Const && !C2)
  constexpr bit_iter(const bit_iter<Word, Diff, C2>& o) noexcept : w_(o.w_), b_(o.b_) {}

  constexpr reference operator*() const noexcept {
    if constexpr (Const)
      return ((*w_ >> b_) & Word(1)) != 0;
    else
      return reference(w_, static_cast<Word>(Word(1) << b_));
  }
  constexpr reference operator[](difference_type n) const noexcept { return *(*this + n); }

  constexpr bit_iter& operator++() noexcept {
    if (++b_ == static_cast<unsigned>(bits)) {
      b_ = 0;
      ++w_;
    }
    return *this;
  }
  constexpr bit_iter operator++(int) noexcept {
    bit_iter t = *this;
    ++*this;
    return t;
  }
  constexpr bit_iter& operator--() noexcept {
    if (b_ == 0) {
      b_ = static_cast<unsigned>(bits - 1);
      --w_;
    } else {
      --b_;
    }
    return *this;
  }
  constexpr bit_iter operator--(int) noexcept {
    bit_iter t = *this;
    --*this;
    return t;
  }
  constexpr bit_iter& operator+=(difference_type n) noexcept {
    const std::ptrdiff_t i = static_cast<std::ptrdiff_t>(b_) + static_cast<std::ptrdiff_t>(n);
    if (i >= 0) {
      w_ += i / bits;
      b_ = static_cast<unsigned>(i % bits);
    } else {
      const std::ptrdiff_t k = (-i - 1) / bits + 1; // words to step back
      w_ -= k;
      b_ = static_cast<unsigned>(i + k * bits);
    }
    return *this;
  }
  constexpr bit_iter& operator-=(difference_type n) noexcept { return *this += -n; }

  friend constexpr bit_iter operator+(bit_iter i, difference_type n) noexcept { return i += n; }
  friend constexpr bit_iter operator+(difference_type n, bit_iter i) noexcept { return i += n; }
  friend constexpr bit_iter operator-(bit_iter i, difference_type n) noexcept { return i -= n; }
  friend constexpr difference_type operator-(const bit_iter& a, const bit_iter& b) noexcept {
    return static_cast<difference_type>((a.w_ - b.w_) * bits + static_cast<std::ptrdiff_t>(a.b_) -
                                        static_cast<std::ptrdiff_t>(b.b_));
  }
  friend constexpr bool operator==(const bit_iter& a, const bit_iter& b) noexcept {
    return a.w_ == b.w_ && a.b_ == b.b_;
  }
  friend constexpr std::strong_ordering operator<=>(const bit_iter& a, const bit_iter& b) noexcept {
    if (a.w_ != b.w_)
      return a.w_ <=> b.w_;
    return a.b_ <=> b.b_;
  }
};

} // namespace ycxx::adl_free

namespace ycxx::detail {
// fill, find and count over vector<bool>'s bits a word at a time (bit_iter_algos.hpp). Each
// visits the words from first's to last's, the bits [lo, hi) of each; last's word is read only
// when it has bits in the range (it may be one past the storage).
template <class Word, class Diff, bool Const>
struct bit_algos<ycxx::adl_free::bit_iter<Word, Diff, Const>> {
  using iter = ycxx::adl_free::bit_iter<Word, Diff, Const>;
  static constexpr bool enabled = true;
  static constexpr unsigned bits = std::numeric_limits<Word>::digits;
  static constexpr Word all = static_cast<Word>(~Word(0));

  // The bits [lo, hi) of a word, lo < hi <= bits.
  static constexpr Word mask(unsigned lo, unsigned hi) noexcept {
    return static_cast<Word>((hi == bits ? all : static_cast<Word>(Word(1) << hi) - Word(1)) & static_cast<Word>(all << lo));
  }

  static constexpr void fill(iter first, iter last, bool value) noexcept
    requires(!Const)
  {
    for (Word* w = first.w_;; ++w) {
      const unsigned lo = w == first.w_ ? first.b_ : 0, hi = w == last.w_ ? last.b_ : bits;
      if (lo < hi)
        *w = value ? static_cast<Word>(*w | mask(lo, hi)) : static_cast<Word>(*w & static_cast<Word>(~mask(lo, hi)));
      if (w == last.w_)
        return;
    }
  }
  static constexpr iter find(iter first, iter last, bool value) noexcept {
    for (auto w = first.w_;; ++w) {
      const unsigned lo = w == first.w_ ? first.b_ : 0, hi = w == last.w_ ? last.b_ : bits;
      const Word m = lo < hi ? static_cast<Word>((value ? *w : static_cast<Word>(~*w)) & mask(lo, hi)) : Word(0);
      if (m != 0)
        return iter(w, static_cast<unsigned>(__builtin_ctzg(m)));
      if (w == last.w_)
        return last;
    }
  }
  static constexpr Diff count(iter first, iter last, bool value) noexcept {
    Diff n = 0;
    for (auto w = first.w_;; ++w) {
      const unsigned lo = w == first.w_ ? first.b_ : 0, hi = w == last.w_ ? last.b_ : bits;
      if (lo < hi)
        n += static_cast<Diff>(__builtin_popcountg(static_cast<Word>((value ? *w : static_cast<Word>(~*w)) & mask(lo, hi))));
      if (w == last.w_)
        return n;
    }
  }
};
} // namespace ycxx::detail

namespace std {

template <class Allocator>
class vector<bool, Allocator> {
  static_assert(is_same_v<typename Allocator::value_type, bool>,
                "std::vector<bool>: Allocator::value_type must be bool ([container.alloc.reqmts]/5)");

  using alloc_traits = allocator_traits<Allocator>;
  using word = size_t;
  using word_alloc = typename alloc_traits::template rebind_alloc<word>;
  using word_traits = allocator_traits<word_alloc>;
  static constexpr size_t bits = numeric_limits<word>::digits;

public:
  // ---- types ----
  using value_type = bool;
  using allocator_type = Allocator;
  using const_reference = bool;
  using size_type = typename alloc_traits::size_type;
  using difference_type = typename alloc_traits::difference_type;
  using reference = ycxx::adl_free::bit_ref<word>;
  using iterator = ycxx::adl_free::bit_iter<word, difference_type, false>;
  using const_iterator = ycxx::adl_free::bit_iter<word, difference_type, true>;
  using pointer = iterator;
  using const_pointer = const_iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  friend struct hash<vector>;
  template <class A>
  friend constexpr bool operator==(const vector<bool, A>&, const vector<bool, A>&);

  static constexpr bool pocca = alloc_traits::propagate_on_container_copy_assignment::value;
  static constexpr bool pocma = alloc_traits::propagate_on_container_move_assignment::value;
  static constexpr bool pocs = alloc_traits::propagate_on_container_swap::value;
  static constexpr bool always_equal = alloc_traits::is_always_equal::value;

  word* words_ = nullptr;
  size_type size_ = 0; // bits
  size_type cap_ = 0;  // words
  [[no_unique_address]] word_alloc alloc_;

  static constexpr size_type words_for(size_type nbits) noexcept {
    return static_cast<size_type>(nbits / bits + (nbits % bits != 0));
  }
  // Word i with the bits at or past size() cleared (for comparisons and hashing).
  constexpr word masked_word(size_type i) const noexcept {
    const word w = words_[i];
    const size_type tail = size_ - i * bits;
    return tail >= bits ? w : (w & ((word(1) << tail) - 1));
  }
  static constexpr bool get(const word* w, size_type i) noexcept {
    return ((w[i / bits] >> (i % bits)) & word(1)) != 0;
  }
  static constexpr void set(word* w, size_type i, bool v) noexcept {
    const word m = word(1) << (i % bits);
    if (v)
      w[i / bits] |= m;
    else
      w[i / bits] &= ~m;
  }

  // ---- storage ----
  struct block {
    word* p;
    size_type n; // words
  };
  static constexpr typename word_traits::pointer to_pointer(word* p) noexcept {
    if constexpr (is_same_v<typename word_traits::pointer, word*>)
      return p;
    else
      return pointer_traits<typename word_traits::pointer>::pointer_to(*p);
  }
  constexpr block allocate_words(size_type n) {
    auto r = word_traits::allocate_at_least(alloc_, n);
    word* const p = std::to_address(r.ptr);
    for (size_type i = 0; i != static_cast<size_type>(r.count); ++i)
      std::construct_at(p + i, word(0));
    return {p, static_cast<size_type>(r.count)};
  }
  constexpr void free_storage() noexcept {
    if (words_) {
      word_traits::deallocate(alloc_, to_pointer(words_), cap_);
      words_ = nullptr;
      cap_ = 0;
    }
  }
  constexpr void adopt(block b) noexcept {
    words_ = b.p;
    cap_ = b.n;
  }
  constexpr void take(vector& o) noexcept {
    words_ = o.words_;
    size_ = o.size_;
    cap_ = o.cap_;
    o.words_ = nullptr;
    o.size_ = 0;
    o.cap_ = 0;
  }
  struct block_guard {
    word_alloc& a;
    block b;
    constexpr ~block_guard() {
      if (b.p)
        word_traits::deallocate(a, vector::to_pointer(b.p), b.n);
    }
    constexpr block release() noexcept {
      block r = b;
      b.p = nullptr;
      return r;
    }
  };

  constexpr void check_size(size_type n) const {
    if (n > max_size())
      ycxx::detail::throw_length_error("std::vector<bool>: size exceeds max_size()");
  }
  // Capacity in bits for size() + extra bits.
  constexpr size_type grow_to(size_type extra) const {
    const size_type ms = max_size();
    if (extra > ms - size_)
      ycxx::detail::throw_length_error("std::vector<bool>: size exceeds max_size()");
    const size_type c = capacity();
    const size_type nc = c <= ms / 2 ? 2 * c : ms;
    return nc < size_ + extra ? size_ + extra : nc;
  }
  // Moves the bits to storage for at least nbits bits (>= size_).
  constexpr void reallocate(size_type nbits) {
    const block b = allocate_words(words_for(nbits));
    const size_type used = words_for(size_);
    for (size_type i = 0; i != used; ++i)
      b.p[i] = words_[i];
    free_storage();
    adopt(b);
  }
  // Inserts n unspecified bits at pos; the only operation that may throw is the allocation.
  constexpr void open_gap(size_type pos, size_type n) {
    if (n == 0)
      return;
    const size_type old = size_;
    if (n > capacity() - old) {
      const block b = allocate_words(words_for(grow_to(n)));
      for (size_type i = 0, e = words_for(pos); i != e; ++i)
        b.p[i] = words_[i];
      for (size_type i = pos; i != old; ++i)
        set(b.p, i + n, get(words_, i));
      free_storage();
      adopt(b);
    } else {
      for (size_type i = old; i != pos;) {
        --i;
        set(words_, i + n, get(words_, i));
      }
    }
    size_ = old + n;
  }
  constexpr void fill(size_type pos, size_type n, bool v) noexcept {
    for (size_type i = pos; i != pos + n; ++i)
      set(words_, i, v);
  }
  constexpr size_type index_of(const_iterator p) const noexcept { return static_cast<size_type>(p - cbegin()); }

  // Inserts the n bits of [first, last) at pos.
  template <class It, class Sent>
  constexpr iterator insert_counted(size_type pos, It first, Sent last, size_type n) {
    open_gap(pos, n);
    (void)last;
    for (size_type i = pos; i != pos + n; ++i) {
      set(words_, i, static_cast<bool>(*first));
      ++first;
    }
    return begin() + static_cast<difference_type>(pos);
  }
  // Single pass: collect the bits first.
  template <class It, class Sent>
  constexpr iterator insert_input(size_type pos, It first, Sent last) {
    vector tmp(get_allocator());
    for (; first != last; ++first)
      tmp.push_back(static_cast<bool>(*first));
    return insert_counted(pos, tmp.cbegin(), tmp.cend(), tmp.size_);
  }
  template <class It, class Sent>
  constexpr void assign_counted(It first, Sent last, size_type n) {
    check_size(n);
    if (n > capacity()) {
      block_guard g{alloc_, allocate_words(words_for(n))};
      for (size_type i = 0; i != n; ++i) {
        set(g.b.p, i, static_cast<bool>(*first));
        ++first;
      }
      free_storage();
      adopt(g.release());
    } else {
      for (size_type i = 0; i != n; ++i) {
        set(words_, i, static_cast<bool>(*first));
        ++first;
      }
    }
    (void)last;
    size_ = n;
  }
  // Replaces the contents with x's bits, a word at a time.
  constexpr void copy_words(const vector& x) {
    const size_type need = words_for(x.size_);
    if (need > cap_) {
      const block b = allocate_words(need);
      free_storage();
      adopt(b);
    }
    for (size_type i = 0; i != need; ++i)
      words_[i] = x.words_[i];
    size_ = x.size_;
  }
  template <class It, class Sent>
  constexpr void assign_input(It first, Sent last) {
    vector tmp(get_allocator());
    for (; first != last; ++first)
      tmp.push_back(static_cast<bool>(*first));
    assign_counted(tmp.cbegin(), tmp.cend(), tmp.size_);
  }
  template <class R>
  static constexpr bool counted_range = ranges::forward_range<R> || ranges::sized_range<R>;
  // The constructors that do work delegate to this one, so that the destructor cleans up if
  // they throw; it is not noexcept, so a (non-conforming) throwing allocator copy propagates.
  struct with_alloc {
    explicit with_alloc() = default;
  };
  constexpr vector(with_alloc, const Allocator& a) : alloc_(a) {}

public:
  // ---- construct/copy/destroy ----
  constexpr vector() noexcept(is_nothrow_default_constructible_v<Allocator>) : vector(Allocator()) {}
  constexpr explicit vector(const Allocator& a) noexcept : alloc_(a) {}
  constexpr explicit vector(size_type n, const Allocator& a = Allocator()) : vector(n, false, a) {}
  constexpr vector(size_type n, const bool& value, const Allocator& a = Allocator()) : vector(with_alloc{}, a) {
    if (n != 0) {
      check_size(n);
      adopt(allocate_words(words_for(n)));
      size_ = n;
      if (value)
        fill(0, n, true);
    }
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr vector(InputIterator first, InputIterator last, const Allocator& a = Allocator())
      : vector(with_alloc{}, a) {
    if constexpr (ycxx::detail::multipass_iterator<InputIterator>) {
      const auto n = ycxx::detail::iter_pair_distance(first, last);
      assign_counted(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last),
                     static_cast<size_type>(n));
    } else {
      for (; first != last; ++first)
        push_back(static_cast<bool>(*first));
    }
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<bool> R>
  constexpr vector(Tag, R&& rg, const Allocator& a = Allocator()) : vector(with_alloc{}, a) {
    if constexpr (counted_range<R>) {
      const auto n = ranges::distance(rg);
      assign_counted(ranges::begin(rg), ranges::end(rg), static_cast<size_type>(n));
    } else {
      auto first = ranges::begin(rg);
      const auto last = ranges::end(rg);
      for (; first != last; ++first)
        push_back(static_cast<bool>(*first));
    }
  }
  constexpr vector(const vector& x)
      : vector(with_alloc{}, alloc_traits::select_on_container_copy_construction(x.get_allocator())) {
    copy_words(x);
  }
  constexpr vector(vector&& x) noexcept : alloc_(static_cast<word_alloc&&>(x.alloc_)) { take(x); }
  constexpr vector(const vector& x, const type_identity_t<Allocator>& a) : vector(with_alloc{}, a) { copy_words(x); }
  constexpr vector(vector&& x, const type_identity_t<Allocator>& a) noexcept(always_equal) : vector(with_alloc{}, a) {
    if (always_equal || alloc_ == x.alloc_)
      take(x);
    else
      copy_words(x);
  }
  constexpr vector(initializer_list<bool> il, const Allocator& a = Allocator()) : vector(with_alloc{}, a) {
    assign_counted(il.begin(), il.end(), il.size());
  }
  constexpr ~vector() { free_storage(); }

  constexpr vector& operator=(const vector& x) {
    if (this == __builtin_addressof(x))
      return *this;
    if constexpr (pocca) {
      if (!always_equal && alloc_ != x.alloc_) {
        free_storage();
        size_ = 0;
      }
      alloc_ = x.alloc_;
    }
    copy_words(x);
    return *this;
  }
  constexpr vector& operator=(vector&& x) noexcept(pocma || always_equal) {
    if (this == __builtin_addressof(x))
      return *this;
    if constexpr (pocma || always_equal) {
      free_storage();
      if constexpr (pocma)
        alloc_ = static_cast<word_alloc&&>(x.alloc_);
      take(x);
    } else {
      if (alloc_ == x.alloc_) {
        free_storage();
        take(x);
      } else {
        copy_words(x);
      }
    }
    return *this;
  }
  constexpr vector& operator=(initializer_list<bool> il) {
    assign_counted(il.begin(), il.end(), il.size());
    return *this;
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr void assign(InputIterator first, InputIterator last) {
    if constexpr (ycxx::detail::multipass_iterator<InputIterator>) {
      const auto n = ycxx::detail::iter_pair_distance(first, last);
      assign_counted(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last),
                     static_cast<size_type>(n));
    } else {
      assign_input(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
    }
  }
  template <ycxx::detail::container_compatible_range<bool> R>
  constexpr void assign_range(R&& rg) {
    static_assert(assignable_from<bool&, ranges::range_reference_t<R>>,
                  "std::vector<bool>::assign_range: bool& must be assignable from the range's reference type");
    if constexpr (counted_range<R>) {
      const auto n = ranges::distance(rg);
      assign_counted(ranges::begin(rg), ranges::end(rg), static_cast<size_type>(n));
    } else {
      assign_input(ranges::begin(rg), ranges::end(rg));
    }
  }
  constexpr void assign(size_type n, const bool& t) {
    const bool v = t;
    check_size(n);
    if (n > capacity()) {
      const block b = allocate_words(words_for(n));
      free_storage();
      adopt(b);
    }
    size_ = n;
    fill(0, n, v);
  }
  constexpr void assign(initializer_list<bool> il) { assign_counted(il.begin(), il.end(), il.size()); }
  constexpr allocator_type get_allocator() const noexcept { return Allocator(alloc_); }

  // ---- iterators ----
  constexpr iterator begin() noexcept { return iterator(words_, 0); }
  constexpr const_iterator begin() const noexcept { return const_iterator(words_, 0); }
  constexpr iterator end() noexcept { return iterator(words_ + size_ / bits, static_cast<unsigned>(size_ % bits)); }
  constexpr const_iterator end() const noexcept {
    return const_iterator(words_ + size_ / bits, static_cast<unsigned>(size_ % bits));
  }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- capacity ----
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
  constexpr size_type size() const noexcept { return size_; }
  constexpr size_type max_size() const noexcept {
    const size_type by_alloc = word_traits::max_size(alloc_);
    const auto diff_max = static_cast<make_unsigned_t<difference_type>>(numeric_limits<difference_type>::max());
    const size_type dm = diff_max < numeric_limits<size_type>::max() ? static_cast<size_type>(diff_max)
                                                                     : numeric_limits<size_type>::max();
    return by_alloc <= dm / bits ? static_cast<size_type>(by_alloc * bits) : dm;
  }
  constexpr size_type capacity() const noexcept { return static_cast<size_type>(cap_ * bits); }
  constexpr void resize(size_type sz, bool c = false) {
    if (sz <= size_)
      size_ = sz;
    else
      insert(cend(), sz - size_, c);
  }
  constexpr void reserve(size_type n) {
    if (n > max_size())
      ycxx::detail::throw_length_error("std::vector<bool>::reserve: argument exceeds max_size()");
    if (n > capacity())
      reallocate(n);
  }
  constexpr void shrink_to_fit() {
    const size_type need = words_for(size_);
    if (need == cap_)
      return;
    if (need == 0) {
      free_storage();
      return;
    }
    // A non-binding request: if the allocation fails, nothing changes and nothing is thrown.
    block b{nullptr, 0};
    if constexpr (ycxx::detail::cfg::exceptions) {
      try {
        b = allocate_words(need);
      } catch (...) {
        return;
      }
    } else {
      b = allocate_words(need);
    }
    block_guard g{alloc_, b};
    if (g.b.n >= cap_)
      return; // the allocator gave nothing back
    for (size_type i = 0; i != need; ++i)
      g.b.p[i] = words_[i];
    free_storage();
    adopt(g.release());
  }

  // ---- element access ----
  constexpr reference operator[](size_type n) {
    ycxx::detail::precondition(n < size_, "std::vector<bool>::operator[]: index out of range");
    return reference(words_ + n / bits, word(1) << (n % bits));
  }
  constexpr const_reference operator[](size_type n) const {
    ycxx::detail::precondition(n < size_, "std::vector<bool>::operator[]: index out of range");
    return get(words_, n);
  }
  constexpr reference at(size_type n) {
    if (n >= size_)
      ycxx::detail::throw_out_of_range("std::vector<bool>::at: index out of range");
    return reference(words_ + n / bits, word(1) << (n % bits));
  }
  constexpr const_reference at(size_type n) const {
    if (n >= size_)
      ycxx::detail::throw_out_of_range("std::vector<bool>::at: index out of range");
    return get(words_, n);
  }
  constexpr reference front() {
    ycxx::detail::precondition(size_ != 0, "std::vector<bool>::front: empty vector");
    return reference(words_, word(1));
  }
  constexpr const_reference front() const {
    ycxx::detail::precondition(size_ != 0, "std::vector<bool>::front: empty vector");
    return get(words_, 0);
  }
  constexpr reference back() {
    ycxx::detail::precondition(size_ != 0, "std::vector<bool>::back: empty vector");
    return reference(words_ + (size_ - 1) / bits, word(1) << ((size_ - 1) % bits));
  }
  constexpr const_reference back() const {
    ycxx::detail::precondition(size_ != 0, "std::vector<bool>::back: empty vector");
    return get(words_, size_ - 1);
  }

  // ---- modifiers ----
  template <class... Args>
  constexpr reference emplace_back(Args&&... args) {
    push_back(bool(static_cast<Args&&>(args)...));
    return back();
  }
  constexpr void push_back(const bool& x) {
    const bool v = x;
    if (size_ == capacity())
      reallocate(grow_to(1));
    set(words_, size_, v);
    ++size_;
  }
  template <ycxx::detail::container_compatible_range<bool> R>
  constexpr void append_range(R&& rg) {
    insert_range(cend(), static_cast<R&&>(rg));
  }
  constexpr void pop_back() {
    ycxx::detail::precondition(size_ != 0, "std::vector<bool>::pop_back: empty vector");
    --size_;
  }
  template <class... Args>
  constexpr iterator emplace(const_iterator position, Args&&... args) {
    return insert(position, bool(static_cast<Args&&>(args)...));
  }
  constexpr iterator insert(const_iterator position, const bool& x) { return insert(position, size_type(1), x); }
  constexpr iterator insert(const_iterator position, size_type n, const bool& x) {
    const bool v = x;
    const size_type pos = index_of(position);
    open_gap(pos, n);
    fill(pos, n, v);
    return begin() + static_cast<difference_type>(pos);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr iterator insert(const_iterator position, InputIterator first, InputIterator last) {
    const size_type pos = index_of(position);
    if constexpr (ycxx::detail::multipass_iterator<InputIterator>) {
      const auto n = ycxx::detail::iter_pair_distance(first, last);
      return insert_counted(pos, static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last),
                            static_cast<size_type>(n));
    } else {
      return insert_input(pos, static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
    }
  }
  template <ycxx::detail::container_compatible_range<bool> R>
  constexpr iterator insert_range(const_iterator position, R&& rg) {
    const size_type pos = index_of(position);
    if constexpr (counted_range<R>) {
      const auto n = ranges::distance(rg);
      return insert_counted(pos, ranges::begin(rg), ranges::end(rg), static_cast<size_type>(n));
    } else {
      return insert_input(pos, ranges::begin(rg), ranges::end(rg));
    }
  }
  constexpr iterator insert(const_iterator position, initializer_list<bool> il) {
    return insert_counted(index_of(position), il.begin(), il.end(), il.size());
  }
  constexpr iterator erase(const_iterator position) {
    ycxx::detail::precondition(position != cend(), "std::vector<bool>::erase: iterator not dereferenceable");
    return erase(position, position + 1);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    const size_type p = index_of(first);
    const size_type q = index_of(last);
    if (p != q) {
      for (size_type i = q; i != size_; ++i)
        set(words_, p + (i - q), get(words_, i));
      size_ -= q - p;
    }
    return begin() + static_cast<difference_type>(p);
  }
  constexpr void swap(vector& x) noexcept(pocs || always_equal) {
    if constexpr (pocs)
      ycxx::detail::swap_adl::do_swap(alloc_, x.alloc_);
    else
      ycxx::detail::precondition(always_equal || alloc_ == x.alloc_,
                                 "std::vector<bool>::swap: unequal allocators that do not propagate");
    word* const w = words_;
    const size_type s = size_;
    const size_type c = cap_;
    words_ = x.words_;
    size_ = x.size_;
    cap_ = x.cap_;
    x.words_ = w;
    x.size_ = s;
    x.cap_ = c;
  }
  // [depr.vector.bool.swap] (Annex D)
  [[deprecated("vector<bool>::swap(reference, reference) is deprecated ([depr.vector.bool.swap]); use "
               "swap(x, y)")]]
  static constexpr void swap(reference x, reference y) noexcept {
    const bool b = x;
    x = static_cast<bool>(y);
    y = b;
  }
  constexpr void flip() noexcept {
    for (size_type i = 0, e = words_for(size_); i != e; ++i)
      words_[i] = ~words_[i];
  }
  constexpr void clear() noexcept { size_ = 0; }
};

// ---- comparisons for vector<bool>: more specialized than the primary template's ----
template <class Allocator>
constexpr bool operator==(const vector<bool, Allocator>& x, const vector<bool, Allocator>& y) {
  if (x.size() != y.size())
    return false;
  for (size_t i = 0, e = x.words_for(x.size_); i != e; ++i)
    if (x.masked_word(i) != y.masked_word(i))
      return false;
  return true;
}
template <class Allocator>
constexpr strong_ordering operator<=>(const vector<bool, Allocator>& x, const vector<bool, Allocator>& y) {
  const auto nx = x.size();
  const auto ny = y.size();
  const auto n = nx < ny ? nx : ny;
  for (decltype(x.size()) i = 0; i != n; ++i)
    if (x[i] != y[i])
      return y[i] ? strong_ordering::less : strong_ordering::greater;
  return nx <=> ny;
}

// ---- [vector.erasure] for vector<bool> ----
template <class Allocator, class Predicate>
constexpr typename vector<bool, Allocator>::size_type erase_if(vector<bool, Allocator>& c, Predicate pred) {
  using size_type = typename vector<bool, Allocator>::size_type;
  const size_type n = c.size();
  size_type out = 0;
  for (size_type i = 0; i != n; ++i) {
    if (!static_cast<bool>(pred(c[i]))) { // the predicate sees a reference, as remove_if's would
      if (out != i)
        c[out] = static_cast<bool>(c[i]);
      ++out;
    }
  }
  c.erase(c.begin() + static_cast<typename vector<bool, Allocator>::difference_type>(out), c.end());
  return n - out;
}
template <class Allocator, class U = bool>
constexpr typename vector<bool, Allocator>::size_type erase(vector<bool, Allocator>& c, const U& value) {
  return std::erase_if(c, [&value](const auto& e) { return static_cast<bool>(e == value); });
}

// ---- hash support ([vector.bool.pspc]/13) ----
template <class Allocator>
struct hash<vector<bool, Allocator>> {
  [[nodiscard]] size_t operator()(const vector<bool, Allocator>& v) const noexcept {
    uint64_t h = ycxx::detail::hash_seed ^
                 ycxx::detail::mum(static_cast<uint64_t>(v.size()) ^ ycxx::detail::hash_k1, ycxx::detail::hash_k2);
    const size_t n = v.words_for(v.size_);
    size_t i = 0;
    for (; i + 2 <= n; i += 2)
      h = ycxx::detail::hash_step(h, static_cast<uint64_t>(v.masked_word(i)),
                                  static_cast<uint64_t>(v.masked_word(i + 1)));
    if (i != n)
      h = ycxx::detail::hash_step(h, static_cast<uint64_t>(v.masked_word(i)), 0);
    return static_cast<size_t>(h);
  }
};

} // namespace std
