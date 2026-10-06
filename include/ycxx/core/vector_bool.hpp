// libycxx core: vector<bool, Allocator> ([vector.bool]) and hash<vector<bool, Allocator>>.
//
// Representation: the bits packed into size_t words from the rebound allocator
// ({words_, size_ in bits, cap_ in words}); bit i is bit (i % digits) of word i / digits. Words
// are value-initialized when allocated (allocator_traits::construct is not used for them,
// [vector.bool.pspc]/2), so every word is readable in constant evaluation. Bits at or past
// size() have unspecified values: every operation that reads them masks them.
//
// reference and the iterators are shared by all vector<bool> specializations with the same
// difference type (__ycxx::__adl_free::__bit_ref / bit_iter). The formatter for reference
// ([vector.bool.fmt]) is in ycxx/core/format_vector_bool.hpp, which <vector> includes.
#pragma once

#include <ycxx/core/bit_iter_algos.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/vector.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

template <class _Word, class _Diff, bool _Const>
class __bit_iter;

// vector<bool>::reference ([vector.bool.pspc]/4-11): a word pointer and a one-bit mask.
template <class _Word>
class __bit_ref {
  template <class _Tp, class _Ap>
  friend class std::vector;
  template <class _Wp, class _Dp, bool _Cp>
  friend class __bit_iter;

  _Word* __w_;
  _Word __m_;
  constexpr __bit_ref(_Word* __w, _Word m) noexcept : __w_(__w), __m_(m) {}

public:
  constexpr __bit_ref(const __bit_ref&) noexcept = default;
  constexpr ~__bit_ref() = default;
  constexpr __bit_ref& operator=(bool __x) noexcept {
    if (__x)
      *__w_ |= __m_;
    else
      *__w_ &= static_cast<_Word>(~__m_);
    return *this;
  }
  constexpr __bit_ref& operator=(const __bit_ref& __x) noexcept { return *this = static_cast<bool>(__x); }
  constexpr const __bit_ref& operator=(bool __x) const noexcept {
    if (__x)
      *__w_ |= __m_;
    else
      *__w_ &= static_cast<_Word>(~__m_);
    return *this;
  }
  constexpr operator bool() const noexcept { return (*__w_ & __m_) != 0; }
  constexpr void flip() noexcept { *__w_ ^= __m_; }

  friend constexpr void swap(__bit_ref __x, __bit_ref y) noexcept {
    const bool b = __x;
    __x = static_cast<bool>(y);
    y = b;
  }
  friend constexpr void swap(__bit_ref __x, bool& y) noexcept {
    const bool b = __x;
    __x = y;
    y = b;
  }
  friend constexpr void swap(bool& __x, __bit_ref y) noexcept {
    const bool b = __x;
    __x = static_cast<bool>(y);
    y = b;
  }
};

// vector<bool>::iterator / const_iterator: a random access iterator over bits, {word, bit}.
template <class _Word, class _Diff, bool _Const>
class __bit_iter {
  template <class _Tp, class _Ap>
  friend class std::vector;
  template <class _Wp, class _Dp, bool _Cp>
  friend class __bit_iter;
  template <class _Ip>
  friend struct __ycxx::__detail::__bit_algos;

  using __wptr = std::conditional_t<_Const, const _Word*, _Word*>;
  static constexpr std::ptrdiff_t bits = std::numeric_limits<_Word>::digits;

  __wptr __w_ = nullptr;
  unsigned __b_ = 0;
  constexpr __bit_iter(__wptr __w, unsigned b) noexcept : __w_(__w), __b_(b) {}

public:
  using iterator_concept = std::random_access_iterator_tag;
  using iterator_category = std::random_access_iterator_tag;
  using value_type = bool;
  using difference_type = _Diff;
  using pointer = void;
  using reference = std::conditional_t<_Const, bool, __bit_ref<_Word>>;

  constexpr __bit_iter() noexcept = default;
  template <bool _C2>
    requires(_Const && !_C2)
  constexpr __bit_iter(const __bit_iter<_Word, _Diff, _C2>& __o) noexcept : __w_(__o.__w_), __b_(__o.__b_) {}

  constexpr reference operator*() const noexcept {
    if constexpr (_Const)
      return ((*__w_ >> __b_) & _Word(1)) != 0;
    else
      return reference(__w_, static_cast<_Word>(_Word(1) << __b_));
  }
  constexpr reference operator[](difference_type n) const noexcept { return *(*this + n); }

  constexpr __bit_iter& operator++() noexcept {
    if (++__b_ == static_cast<unsigned>(bits)) {
      __b_ = 0;
      ++__w_;
    }
    return *this;
  }
  constexpr __bit_iter operator++(int) noexcept {
    __bit_iter t = *this;
    ++*this;
    return t;
  }
  constexpr __bit_iter& operator--() noexcept {
    if (__b_ == 0) {
      __b_ = static_cast<unsigned>(bits - 1);
      --__w_;
    } else {
      --__b_;
    }
    return *this;
  }
  constexpr __bit_iter operator--(int) noexcept {
    __bit_iter t = *this;
    --*this;
    return t;
  }
  constexpr __bit_iter& operator+=(difference_type n) noexcept {
    const std::ptrdiff_t i = static_cast<std::ptrdiff_t>(__b_) + static_cast<std::ptrdiff_t>(n);
    if (i >= 0) {
      __w_ += i / bits;
      __b_ = static_cast<unsigned>(i % bits);
    } else {
      const std::ptrdiff_t k = (-i - 1) / bits + 1; // words to step back
      __w_ -= k;
      __b_ = static_cast<unsigned>(i + k * bits);
    }
    return *this;
  }
  constexpr __bit_iter& operator-=(difference_type n) noexcept { return *this += -n; }

  friend constexpr __bit_iter operator+(__bit_iter i, difference_type n) noexcept { return i += n; }
  friend constexpr __bit_iter operator+(difference_type n, __bit_iter i) noexcept { return i += n; }
  friend constexpr __bit_iter operator-(__bit_iter i, difference_type n) noexcept { return i -= n; }
  friend constexpr difference_type operator-(const __bit_iter& a, const __bit_iter& b) noexcept {
    return static_cast<difference_type>((a.__w_ - b.__w_) * bits + static_cast<std::ptrdiff_t>(a.__b_) -
                                        static_cast<std::ptrdiff_t>(b.__b_));
  }
  friend constexpr bool operator==(const __bit_iter& a, const __bit_iter& b) noexcept {
    return a.__w_ == b.__w_ && a.__b_ == b.__b_;
  }
  friend constexpr std::strong_ordering operator<=>(const __bit_iter& a, const __bit_iter& b) noexcept {
    if (a.__w_ != b.__w_)
      return a.__w_ <=> b.__w_;
    return a.__b_ <=> b.__b_;
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// fill, find and count over vector<bool>'s bits a word at a time (bit_iter_algos.hpp). Each
// visits the words from first's to last's, the bits [lo, hi) of each; last's word is read only
// when it has bits in the range (it may be one past the storage).
template <class _Word, class _Diff, bool _Const>
struct __bit_algos<__ycxx::__adl_free::__bit_iter<_Word, _Diff, _Const>> {
  using iter = __ycxx::__adl_free::__bit_iter<_Word, _Diff, _Const>;
  static constexpr bool __enabled = true;
  static constexpr unsigned bits = std::numeric_limits<_Word>::digits;
  static constexpr _Word all = static_cast<_Word>(~_Word(0));

  // The bits [lo, hi) of a word, lo < hi <= bits.
  static constexpr _Word mask(unsigned __lo, unsigned __hi) noexcept {
    return static_cast<_Word>((__hi == bits ? all : static_cast<_Word>(_Word(1) << __hi) - _Word(1)) & static_cast<_Word>(all << __lo));
  }

  static constexpr void fill(iter first, iter last, bool value) noexcept
    requires(!_Const)
  {
    for (_Word* __w = first.__w_;; ++__w) {
      const unsigned __lo = __w == first.__w_ ? first.__b_ : 0, __hi = __w == last.__w_ ? last.__b_ : bits;
      if (__lo < __hi)
        *__w = value ? static_cast<_Word>(*__w | mask(__lo, __hi)) : static_cast<_Word>(*__w & static_cast<_Word>(~mask(__lo, __hi)));
      if (__w == last.__w_)
        return;
    }
  }
  static constexpr iter find(iter first, iter last, bool value) noexcept {
    for (auto __w = first.__w_;; ++__w) {
      const unsigned __lo = __w == first.__w_ ? first.__b_ : 0, __hi = __w == last.__w_ ? last.__b_ : bits;
      const _Word m = __lo < __hi ? static_cast<_Word>((value ? *__w : static_cast<_Word>(~*__w)) & mask(__lo, __hi)) : _Word(0);
      if (m != 0)
        return iter(__w, static_cast<unsigned>(__builtin_ctzg(m)));
      if (__w == last.__w_)
        return last;
    }
  }
  static constexpr _Diff count(iter first, iter last, bool value) noexcept {
    _Diff n = 0;
    for (auto __w = first.__w_;; ++__w) {
      const unsigned __lo = __w == first.__w_ ? first.__b_ : 0, __hi = __w == last.__w_ ? last.__b_ : bits;
      if (__lo < __hi)
        n += static_cast<_Diff>(__builtin_popcountg(static_cast<_Word>((value ? *__w : static_cast<_Word>(~*__w)) & mask(__lo, __hi))));
      if (__w == last.__w_)
        return n;
    }
  }
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Allocator>
class vector<bool, _Allocator> {
  static_assert(is_same_v<typename _Allocator::value_type, bool>,
                "std::vector<bool>: Allocator::value_type must be bool ([container.alloc.reqmts]/5)");

  using __alloc_traits = allocator_traits<_Allocator>;
  using __word = size_t;
  using __word_alloc = typename __alloc_traits::template rebind_alloc<__word>;
  using __word_traits = allocator_traits<__word_alloc>;
  static constexpr size_t bits = numeric_limits<__word>::digits;

public:
  // ---- types ----
  using value_type = bool;
  using allocator_type = _Allocator;
  using const_reference = bool;
  using size_type = typename __alloc_traits::size_type;
  using difference_type = typename __alloc_traits::difference_type;
  using reference = __ycxx::__adl_free::__bit_ref<__word>;
  using iterator = __ycxx::__adl_free::__bit_iter<__word, difference_type, false>;
  using const_iterator = __ycxx::__adl_free::__bit_iter<__word, difference_type, true>;
  using pointer = iterator;
  using const_pointer = const_iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  friend struct hash<vector>;
  template <class _Ap>
  friend constexpr bool operator==(const vector<bool, _Ap>&, const vector<bool, _Ap>&);

  static constexpr bool __pocca = __alloc_traits::propagate_on_container_copy_assignment::value;
  static constexpr bool __pocma = __alloc_traits::propagate_on_container_move_assignment::value;
  static constexpr bool __pocs = __alloc_traits::propagate_on_container_swap::value;
  static constexpr bool __always_equal = __alloc_traits::is_always_equal::value;

  __word* __words_ = nullptr;
  size_type __size_ = 0; // bits
  size_type __cap_ = 0;  // words
  [[no_unique_address]] __word_alloc __alloc_;

  static constexpr size_type __words_for(size_type __nbits) noexcept {
    return static_cast<size_type>(__nbits / bits + (__nbits % bits != 0));
  }
  // Word i with the bits at or past size() cleared (for comparisons and hashing).
  constexpr __word __masked_word(size_type i) const noexcept {
    const __word __w = __words_[i];
    const size_type __tail = __size_ - i * bits;
    return __tail >= bits ? __w : (__w & ((__word(1) << __tail) - 1));
  }
  static constexpr bool get(const __word* __w, size_type i) noexcept {
    return ((__w[i / bits] >> (i % bits)) & __word(1)) != 0;
  }
  static constexpr void set(__word* __w, size_type i, bool __v) noexcept {
    const __word m = __word(1) << (i % bits);
    if (__v)
      __w[i / bits] |= m;
    else
      __w[i / bits] &= ~m;
  }

  // ---- storage ----
  struct block {
    __word* p;
    size_type n; // words
  };
  static constexpr typename __word_traits::pointer __to_pointer(__word* p) noexcept {
    if constexpr (is_same_v<typename __word_traits::pointer, __word*>)
      return p;
    else
      return pointer_traits<typename __word_traits::pointer>::pointer_to(*p);
  }
  constexpr block __allocate_words(size_type n) {
    auto r = __word_traits::allocate_at_least(__alloc_, n);
    __word* const p = std::to_address(r.ptr);
    for (size_type i = 0; i != static_cast<size_type>(r.count); ++i)
      std::construct_at(p + i, __word(0));
    return {p, static_cast<size_type>(r.count)};
  }
  constexpr void __free_storage() noexcept {
    if (__words_) {
      __word_traits::deallocate(__alloc_, __to_pointer(__words_), __cap_);
      __words_ = nullptr;
      __cap_ = 0;
    }
  }
  constexpr void __adopt(block b) noexcept {
    __words_ = b.p;
    __cap_ = b.n;
  }
  constexpr void take(vector& __o) noexcept {
    __words_ = __o.__words_;
    __size_ = __o.__size_;
    __cap_ = __o.__cap_;
    __o.__words_ = nullptr;
    __o.__size_ = 0;
    __o.__cap_ = 0;
  }
  struct __block_guard {
    __word_alloc& a;
    block b;
    constexpr ~__block_guard() {
      if (b.p)
        __word_traits::deallocate(a, vector::__to_pointer(b.p), b.n);
    }
    constexpr block release() noexcept {
      block r = b;
      b.p = nullptr;
      return r;
    }
  };

  constexpr void __check_size(size_type n) const {
    if (n > max_size())
      __ycxx::__detail::__throw_length_error("std::vector<bool>: size exceeds max_size()");
  }
  // Capacity in bits for size() + extra bits.
  constexpr size_type __grow_to(size_type __extra) const {
    const size_type ms = max_size();
    if (__extra > ms - __size_)
      __ycxx::__detail::__throw_length_error("std::vector<bool>: size exceeds max_size()");
    const size_type c = capacity();
    const size_type __nc = c <= ms / 2 ? 2 * c : ms;
    return __nc < __size_ + __extra ? __size_ + __extra : __nc;
  }
  // Moves the bits to storage for at least nbits bits (>= size_).
  constexpr void __reallocate(size_type __nbits) {
    const block b = __allocate_words(__words_for(__nbits));
    const size_type __y_used = __words_for(__size_);
    for (size_type i = 0; i != __y_used; ++i)
      b.p[i] = __words_[i];
    __free_storage();
    __adopt(b);
  }
  // Inserts n unspecified bits at pos; the only operation that may throw is the allocation.
  constexpr void __open_gap(size_type __pos, size_type n) {
    if (n == 0)
      return;
    const size_type __old = __size_;
    if (n > capacity() - __old) {
      const block b = __allocate_words(__words_for(__grow_to(n)));
      for (size_type i = 0, e = __words_for(__pos); i != e; ++i)
        b.p[i] = __words_[i];
      for (size_type i = __pos; i != __old; ++i)
        set(b.p, i + n, get(__words_, i));
      __free_storage();
      __adopt(b);
    } else {
      for (size_type i = __old; i != __pos;) {
        --i;
        set(__words_, i + n, get(__words_, i));
      }
    }
    __size_ = __old + n;
  }
  constexpr void fill(size_type __pos, size_type n, bool __v) noexcept {
    for (size_type i = __pos; i != __pos + n; ++i)
      set(__words_, i, __v);
  }
  constexpr size_type __index_of(const_iterator p) const noexcept { return static_cast<size_type>(p - cbegin()); }

  // Inserts the n bits of [first, last) at pos.
  template <class _It, class _Sent>
  constexpr iterator __insert_counted(size_type __pos, _It first, _Sent last, size_type n) {
    __open_gap(__pos, n);
    (void)last;
    for (size_type i = __pos; i != __pos + n; ++i) {
      set(__words_, i, static_cast<bool>(*first));
      ++first;
    }
    return begin() + static_cast<difference_type>(__pos);
  }
  // Single pass: collect the bits first.
  template <class _It, class _Sent>
  constexpr iterator __insert_input(size_type __pos, _It first, _Sent last) {
    vector __tmp(get_allocator());
    for (; first != last; ++first)
      __tmp.push_back(static_cast<bool>(*first));
    return __insert_counted(__pos, __tmp.cbegin(), __tmp.cend(), __tmp.__size_);
  }
  template <class _It, class _Sent>
  constexpr void __assign_counted(_It first, _Sent last, size_type n) {
    __check_size(n);
    if (n > capacity()) {
      __block_guard __g{__alloc_, __allocate_words(__words_for(n))};
      for (size_type i = 0; i != n; ++i) {
        set(__g.b.p, i, static_cast<bool>(*first));
        ++first;
      }
      __free_storage();
      __adopt(__g.release());
    } else {
      for (size_type i = 0; i != n; ++i) {
        set(__words_, i, static_cast<bool>(*first));
        ++first;
      }
    }
    (void)last;
    __size_ = n;
  }
  // Replaces the contents with x's bits, a word at a time.
  constexpr void __copy_words(const vector& __x) {
    const size_type __need = __words_for(__x.__size_);
    if (__need > __cap_) {
      const block b = __allocate_words(__need);
      __free_storage();
      __adopt(b);
    }
    for (size_type i = 0; i != __need; ++i)
      __words_[i] = __x.__words_[i];
    __size_ = __x.__size_;
  }
  template <class _It, class _Sent>
  constexpr void __assign_input(_It first, _Sent last) {
    vector __tmp(get_allocator());
    for (; first != last; ++first)
      __tmp.push_back(static_cast<bool>(*first));
    __assign_counted(__tmp.cbegin(), __tmp.cend(), __tmp.__size_);
  }
  template <class _Rp>
  static constexpr bool __counted_range = ranges::forward_range<_Rp> || ranges::sized_range<_Rp>;
  // The constructors that do work delegate to this one, so that the destructor cleans up if
  // they throw; it is not noexcept, so a (non-conforming) throwing allocator copy propagates.
  struct __with_alloc {
    explicit __with_alloc() = default;
  };
  constexpr vector(__with_alloc, const _Allocator& a) : __alloc_(a) {}

public:
  // ---- construct/copy/destroy ----
  constexpr vector() noexcept(is_nothrow_default_constructible_v<_Allocator>) : vector(_Allocator()) {}
  constexpr explicit vector(const _Allocator& a) noexcept : __alloc_(a) {}
  constexpr explicit vector(size_type n, const _Allocator& a = _Allocator()) : vector(n, false, a) {}
  constexpr vector(size_type n, const bool& value, const _Allocator& a = _Allocator()) : vector(__with_alloc{}, a) {
    if (n != 0) {
      __check_size(n);
      __adopt(__allocate_words(__words_for(n)));
      __size_ = n;
      if (value)
        fill(0, n, true);
    }
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr vector(_InputIterator first, _InputIterator last, const _Allocator& a = _Allocator())
      : vector(__with_alloc{}, a) {
    if constexpr (__ycxx::__detail::__multipass_iterator<_InputIterator>) {
      const auto n = __ycxx::__detail::__iter_pair_distance(first, last);
      __assign_counted(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last),
                     static_cast<size_type>(n));
    } else {
      for (; first != last; ++first)
        push_back(static_cast<bool>(*first));
    }
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<bool> _Rp>
  constexpr vector(_Tag, _Rp&& __rg, const _Allocator& a = _Allocator()) : vector(__with_alloc{}, a) {
    if constexpr (__counted_range<_Rp>) {
      const auto n = ranges::distance(__rg);
      __assign_counted(ranges::begin(__rg), ranges::end(__rg), static_cast<size_type>(n));
    } else {
      auto first = ranges::begin(__rg);
      const auto last = ranges::end(__rg);
      for (; first != last; ++first)
        push_back(static_cast<bool>(*first));
    }
  }
  constexpr vector(const vector& __x)
      : vector(__with_alloc{}, __alloc_traits::select_on_container_copy_construction(__x.get_allocator())) {
    __copy_words(__x);
  }
  constexpr vector(vector&& __x) noexcept : __alloc_(static_cast<__word_alloc&&>(__x.__alloc_)) { take(__x); }
  constexpr vector(const vector& __x, const type_identity_t<_Allocator>& a) : vector(__with_alloc{}, a) { __copy_words(__x); }
  constexpr vector(vector&& __x, const type_identity_t<_Allocator>& a) noexcept(__always_equal) : vector(__with_alloc{}, a) {
    if (__always_equal || __alloc_ == __x.__alloc_)
      take(__x);
    else
      __copy_words(__x);
  }
  constexpr vector(initializer_list<bool> il, const _Allocator& a = _Allocator()) : vector(__with_alloc{}, a) {
    __assign_counted(il.begin(), il.end(), il.size());
  }
  constexpr ~vector() { __free_storage(); }

  constexpr vector& operator=(const vector& __x) {
    if (this == __builtin_addressof(__x))
      return *this;
    if constexpr (__pocca) {
      if (!__always_equal && __alloc_ != __x.__alloc_) {
        __free_storage();
        __size_ = 0;
      }
      __alloc_ = __x.__alloc_;
    }
    __copy_words(__x);
    return *this;
  }
  constexpr vector& operator=(vector&& __x) noexcept(__pocma || __always_equal) {
    if (this == __builtin_addressof(__x))
      return *this;
    if constexpr (__pocma || __always_equal) {
      __free_storage();
      if constexpr (__pocma)
        __alloc_ = static_cast<__word_alloc&&>(__x.__alloc_);
      take(__x);
    } else {
      if (__alloc_ == __x.__alloc_) {
        __free_storage();
        take(__x);
      } else {
        __copy_words(__x);
      }
    }
    return *this;
  }
  constexpr vector& operator=(initializer_list<bool> il) {
    __assign_counted(il.begin(), il.end(), il.size());
    return *this;
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr void assign(_InputIterator first, _InputIterator last) {
    if constexpr (__ycxx::__detail::__multipass_iterator<_InputIterator>) {
      const auto n = __ycxx::__detail::__iter_pair_distance(first, last);
      __assign_counted(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last),
                     static_cast<size_type>(n));
    } else {
      __assign_input(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
    }
  }
  template <__ycxx::__detail::__container_compatible_range<bool> _Rp>
  constexpr void assign_range(_Rp&& __rg) {
    static_assert(assignable_from<bool&, ranges::range_reference_t<_Rp>>,
                  "std::vector<bool>::assign_range: bool& must be assignable from the range's reference type");
    if constexpr (__counted_range<_Rp>) {
      const auto n = ranges::distance(__rg);
      __assign_counted(ranges::begin(__rg), ranges::end(__rg), static_cast<size_type>(n));
    } else {
      __assign_input(ranges::begin(__rg), ranges::end(__rg));
    }
  }
  constexpr void assign(size_type n, const bool& t) {
    const bool __v = t;
    __check_size(n);
    if (n > capacity()) {
      const block b = __allocate_words(__words_for(n));
      __free_storage();
      __adopt(b);
    }
    __size_ = n;
    fill(0, n, __v);
  }
  constexpr void assign(initializer_list<bool> il) { __assign_counted(il.begin(), il.end(), il.size()); }
  constexpr allocator_type get_allocator() const noexcept { return _Allocator(__alloc_); }

  // ---- iterators ----
  constexpr iterator begin() noexcept { return iterator(__words_, 0); }
  constexpr const_iterator begin() const noexcept { return const_iterator(__words_, 0); }
  constexpr iterator end() noexcept { return iterator(__words_ + __size_ / bits, static_cast<unsigned>(__size_ % bits)); }
  constexpr const_iterator end() const noexcept {
    return const_iterator(__words_ + __size_ / bits, static_cast<unsigned>(__size_ % bits));
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
  [[nodiscard]] constexpr bool empty() const noexcept { return __size_ == 0; }
  constexpr size_type size() const noexcept { return __size_; }
  constexpr size_type max_size() const noexcept {
    const size_type __by_alloc = __word_traits::max_size(__alloc_);
    const auto __diff_max = static_cast<make_unsigned_t<difference_type>>(numeric_limits<difference_type>::max());
    const size_type __dm = __diff_max < numeric_limits<size_type>::max() ? static_cast<size_type>(__diff_max)
                                                                     : numeric_limits<size_type>::max();
    return __by_alloc <= __dm / bits ? static_cast<size_type>(__by_alloc * bits) : __dm;
  }
  constexpr size_type capacity() const noexcept { return static_cast<size_type>(__cap_ * bits); }
  constexpr void resize(size_type __sz, bool c = false) {
    if (__sz <= __size_)
      __size_ = __sz;
    else
      insert(cend(), __sz - __size_, c);
  }
  constexpr void reserve(size_type n) {
    if (n > max_size())
      __ycxx::__detail::__throw_length_error("std::vector<bool>::reserve: argument exceeds max_size()");
    if (n > capacity())
      __reallocate(n);
  }
  constexpr void shrink_to_fit() {
    const size_type __need = __words_for(__size_);
    if (__need == __cap_)
      return;
    if (__need == 0) {
      __free_storage();
      return;
    }
    // A non-binding request: if the allocation fails, nothing changes and nothing is thrown.
    block b{nullptr, 0};
    if constexpr (__ycxx::__detail::__cfg::exceptions) {
      try {
        b = __allocate_words(__need);
      } catch (...) {
        return;
      }
    } else {
      b = __allocate_words(__need);
    }
    __block_guard __g{__alloc_, b};
    if (__g.b.n >= __cap_)
      return; // the allocator gave nothing back
    for (size_type i = 0; i != __need; ++i)
      __g.b.p[i] = __words_[i];
    __free_storage();
    __adopt(__g.release());
  }

  // ---- element access ----
  constexpr reference operator[](size_type n) {
    __ycxx::__detail::__precondition(n < __size_, "std::vector<bool>::operator[]: index out of range");
    return reference(__words_ + n / bits, __word(1) << (n % bits));
  }
  constexpr const_reference operator[](size_type n) const {
    __ycxx::__detail::__precondition(n < __size_, "std::vector<bool>::operator[]: index out of range");
    return get(__words_, n);
  }
  constexpr reference at(size_type n) {
    if (n >= __size_)
      __ycxx::__detail::__throw_out_of_range("std::vector<bool>::at: index out of range");
    return reference(__words_ + n / bits, __word(1) << (n % bits));
  }
  constexpr const_reference at(size_type n) const {
    if (n >= __size_)
      __ycxx::__detail::__throw_out_of_range("std::vector<bool>::at: index out of range");
    return get(__words_, n);
  }
  constexpr reference front() {
    __ycxx::__detail::__precondition(__size_ != 0, "std::vector<bool>::front: empty vector");
    return reference(__words_, __word(1));
  }
  constexpr const_reference front() const {
    __ycxx::__detail::__precondition(__size_ != 0, "std::vector<bool>::front: empty vector");
    return get(__words_, 0);
  }
  constexpr reference back() {
    __ycxx::__detail::__precondition(__size_ != 0, "std::vector<bool>::back: empty vector");
    return reference(__words_ + (__size_ - 1) / bits, __word(1) << ((__size_ - 1) % bits));
  }
  constexpr const_reference back() const {
    __ycxx::__detail::__precondition(__size_ != 0, "std::vector<bool>::back: empty vector");
    return get(__words_, __size_ - 1);
  }

  // ---- modifiers ----
  template <class... _Args>
  constexpr reference emplace_back(_Args&&... __args) {
    push_back(bool(static_cast<_Args&&>(__args)...));
    return back();
  }
  constexpr void push_back(const bool& __x) {
    const bool __v = __x;
    if (__size_ == capacity())
      __reallocate(__grow_to(1));
    set(__words_, __size_, __v);
    ++__size_;
  }
  template <__ycxx::__detail::__container_compatible_range<bool> _Rp>
  constexpr void append_range(_Rp&& __rg) {
    // Unlike insert_range, append_range has no precondition that rg does not overlap *this
    // ([sequence.reqmts]): when the storage grows, the new bits are read before the old storage
    // is freed. (An input range is collected first by insert_input.)
    if constexpr (__counted_range<_Rp>) {
      const auto d = ranges::distance(__rg);
      const size_type n = static_cast<size_type>(d);
      if (n > capacity() - __size_) {
        __block_guard __g{__alloc_, __allocate_words(__words_for(__grow_to(n)))};
        for (size_type i = 0, e = __words_for(__size_); i != e; ++i)
          __g.b.p[i] = __words_[i];
        auto __it = ranges::begin(__rg);
        for (size_type i = __size_; i != __size_ + n; ++i) {
          set(__g.b.p, i, static_cast<bool>(*__it));
          ++__it;
        }
        __free_storage();
        __adopt(__g.release());
        __size_ += n;
        return;
      }
    }
    insert_range(cend(), static_cast<_Rp&&>(__rg));
  }
  constexpr void pop_back() {
    __ycxx::__detail::__precondition(__size_ != 0, "std::vector<bool>::pop_back: empty vector");
    --__size_;
  }
  template <class... _Args>
  constexpr iterator emplace(const_iterator position, _Args&&... __args) {
    return insert(position, bool(static_cast<_Args&&>(__args)...));
  }
  constexpr iterator insert(const_iterator position, const bool& __x) { return insert(position, size_type(1), __x); }
  constexpr iterator insert(const_iterator position, size_type n, const bool& __x) {
    const bool __v = __x;
    const size_type __pos = __index_of(position);
    __open_gap(__pos, n);
    fill(__pos, n, __v);
    return begin() + static_cast<difference_type>(__pos);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr iterator insert(const_iterator position, _InputIterator first, _InputIterator last) {
    const size_type __pos = __index_of(position);
    if constexpr (__ycxx::__detail::__multipass_iterator<_InputIterator>) {
      const auto n = __ycxx::__detail::__iter_pair_distance(first, last);
      return __insert_counted(__pos, static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last),
                            static_cast<size_type>(n));
    } else {
      return __insert_input(__pos, static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
    }
  }
  template <__ycxx::__detail::__container_compatible_range<bool> _Rp>
  constexpr iterator insert_range(const_iterator position, _Rp&& __rg) {
    const size_type __pos = __index_of(position);
    if constexpr (__counted_range<_Rp>) {
      const auto n = ranges::distance(__rg);
      return __insert_counted(__pos, ranges::begin(__rg), ranges::end(__rg), static_cast<size_type>(n));
    } else {
      return __insert_input(__pos, ranges::begin(__rg), ranges::end(__rg));
    }
  }
  constexpr iterator insert(const_iterator position, initializer_list<bool> il) {
    return __insert_counted(__index_of(position), il.begin(), il.end(), il.size());
  }
  constexpr iterator erase(const_iterator position) {
    __ycxx::__detail::__precondition(position != cend(), "std::vector<bool>::erase: iterator not dereferenceable");
    return erase(position, position + 1);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    const size_type p = __index_of(first);
    const size_type __q = __index_of(last);
    if (p != __q) {
      for (size_type i = __q; i != __size_; ++i)
        set(__words_, p + (i - __q), get(__words_, i));
      __size_ -= __q - p;
    }
    return begin() + static_cast<difference_type>(p);
  }
  constexpr void swap(vector& __x) noexcept(__pocs || __always_equal) {
    if constexpr (__pocs)
      __ycxx::__detail::__swap_adl::__do_swap(__alloc_, __x.__alloc_);
    else
      __ycxx::__detail::__precondition(__always_equal || __alloc_ == __x.__alloc_,
                                 "std::vector<bool>::swap: unequal allocators that do not propagate");
    __word* const __w = __words_;
    const size_type s = __size_;
    const size_type c = __cap_;
    __words_ = __x.__words_;
    __size_ = __x.__size_;
    __cap_ = __x.__cap_;
    __x.__words_ = __w;
    __x.__size_ = s;
    __x.__cap_ = c;
  }
  // [depr.vector.bool.swap] (Annex D)
  [[deprecated("vector<bool>::swap(reference, reference) is deprecated ([depr.vector.bool.swap]); use "
               "swap(x, y)")]]
  static constexpr void swap(reference __x, reference y) noexcept {
    const bool b = __x;
    __x = static_cast<bool>(y);
    y = b;
  }
  constexpr void flip() noexcept {
    for (size_type i = 0, e = __words_for(__size_); i != e; ++i)
      __words_[i] = ~__words_[i];
  }
  constexpr void clear() noexcept { __size_ = 0; }
};

// ---- comparisons for vector<bool>: more specialized than the primary template's ----
template <class _Allocator>
constexpr bool operator==(const vector<bool, _Allocator>& __x, const vector<bool, _Allocator>& y) {
  if (__x.size() != y.size())
    return false;
  for (size_t i = 0, e = __x.__words_for(__x.__size_); i != e; ++i)
    if (__x.__masked_word(i) != y.__masked_word(i))
      return false;
  return true;
}
template <class _Allocator>
constexpr strong_ordering operator<=>(const vector<bool, _Allocator>& __x, const vector<bool, _Allocator>& y) {
  const auto __nx = __x.size();
  const auto __ny = y.size();
  const auto n = __nx < __ny ? __nx : __ny;
  for (decltype(__x.size()) i = 0; i != n; ++i)
    if (__x[i] != y[i])
      return y[i] ? strong_ordering::less : strong_ordering::greater;
  return __nx <=> __ny;
}

// ---- [vector.erasure] for vector<bool> ----
template <class _Allocator, class _Predicate>
constexpr typename vector<bool, _Allocator>::size_type erase_if(vector<bool, _Allocator>& c, _Predicate pred) {
  using size_type = typename vector<bool, _Allocator>::size_type;
  const size_type n = c.size();
  size_type out = 0;
  for (size_type i = 0; i != n; ++i) {
    if (!static_cast<bool>(pred(c[i]))) { // the predicate sees a reference, as remove_if's would
      if (out != i)
        c[out] = static_cast<bool>(c[i]);
      ++out;
    }
  }
  c.erase(c.begin() + static_cast<typename vector<bool, _Allocator>::difference_type>(out), c.end());
  return n - out;
}
template <class _Allocator, class _Up = bool>
constexpr typename vector<bool, _Allocator>::size_type erase(vector<bool, _Allocator>& c, const _Up& value) {
  return std::erase_if(c, [&value](const auto& e) { return static_cast<bool>(e == value); });
}

// ---- hash support ([vector.bool.pspc]/13) ----
template <class _Allocator>
struct hash<vector<bool, _Allocator>> {
  [[nodiscard]] size_t operator()(const vector<bool, _Allocator>& __v) const noexcept {
    uint64_t h = __ycxx::__detail::__hash_seed ^
                 __ycxx::__detail::__mum(static_cast<uint64_t>(__v.size()) ^ __ycxx::__detail::__hash_k1, __ycxx::__detail::__hash_k2);
    const size_t n = __v.__words_for(__v.__size_);
    size_t i = 0;
    for (; i + 2 <= n; i += 2)
      h = __ycxx::__detail::__hash_step(h, static_cast<uint64_t>(__v.__masked_word(i)),
                                  static_cast<uint64_t>(__v.__masked_word(i + 1)));
    if (i != n)
      h = __ycxx::__detail::__hash_step(h, static_cast<uint64_t>(__v.__masked_word(i)), 0);
    return static_cast<size_t>(h);
  }
};

} // namespace std
