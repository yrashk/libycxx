// libycxx core: deque ([deque]), its comparisons, erasure and the pmr:: alias.
//
// Representation: a map (an array of mcap_ block pointers, allocated through the allocator
// rebound to T*) and blocks of B elements each. Elements are numbered by an absolute index:
// element i lives at abs = start_ + i, in block map_[abs / B] at offset abs % B. Map slots
// that hold no block are null. Invariants, whenever map_ is non-null:
//   - the allocated blocks are exactly the slots holding elements, or, for an empty deque,
//     the single slot start_ / B;
//   - the slot after the last allocated one exists (end_slot < mcap_), so that the past-the-end
//     iterator and incrementing onto it can always read a map slot.
// Blocks never move, so insertion at either end keeps references valid; erasing at either end
// frees emptied blocks but never touches the map, so other iterators stay valid
// ([deque.modifiers]). Middle insertions of ranges append (or prepend) and rotate; single
// elements and n copies shift the shorter side.
//
// The iterator holds {cur, blk, node}: the element, its block and the map slot of the block.
// The past-the-end position of a deque whose last block is full is {null, null, end slot}.
#pragma once

#include <initializer_list>
#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/algo_mutate.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/memory_resource_fwd.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/utility_base.hpp>

namespace std {
template <class T, class Allocator>
class deque;
}

namespace ycxx::detail {
// Elements per block: about 1 KiB, a power of two, at least 16.
template <class T>
consteval std::size_t deque_block_len() {
  const std::size_t n = sizeof(T) <= 1024 / 16 ? 1024 / sizeof(T) : 16;
  std::size_t p = 16;
  while (p * 2 <= n)
    p *= 2;
  return p;
}
} // namespace ycxx::detail

namespace ycxx::adl_free {

// T is the element type, possibly const.
template <class T, class Diff>
class deque_iter {
  using V = std::remove_const_t<T>;
  static constexpr Diff B = static_cast<Diff>(::ycxx::detail::deque_block_len<V>());

  T* cur_ = nullptr;
  T* blk_ = nullptr;
  V* const* node_ = nullptr;

  template <class, class>
  friend class deque_iter;
  template <class, class>
  friend class std::deque;

  constexpr deque_iter(T* c, T* b, V* const* n) noexcept : cur_(c), blk_(b), node_(n) {}

public:
  using iterator_concept = std::random_access_iterator_tag;
  using iterator_category = std::random_access_iterator_tag;
  using value_type = V;
  using difference_type = Diff;
  using pointer = T*;
  using reference = T&;

  constexpr deque_iter() noexcept = default;
  template <class U>
    requires std::is_same_v<const U, T> && (!std::is_same_v<U, T>)
  constexpr deque_iter(const deque_iter<U, Diff>& o) noexcept : cur_(o.cur_), blk_(o.blk_), node_(o.node_) {}

  constexpr reference operator*() const noexcept { return *cur_; }
  constexpr pointer operator->() const noexcept { return cur_; }
  constexpr reference operator[](difference_type n) const noexcept { return *(*this + n); }

  constexpr deque_iter& operator++() noexcept {
    if (++cur_ == blk_ + B) {
      ++node_;
      blk_ = *node_;
      cur_ = blk_;
    }
    return *this;
  }
  constexpr deque_iter operator++(int) noexcept {
    deque_iter t = *this;
    ++*this;
    return t;
  }
  constexpr deque_iter& operator--() noexcept {
    if (cur_ == blk_) {
      --node_;
      blk_ = *node_;
      cur_ = blk_ + B;
    }
    --cur_;
    return *this;
  }
  constexpr deque_iter operator--(int) noexcept {
    deque_iter t = *this;
    --*this;
    return t;
  }
  constexpr deque_iter& operator+=(difference_type n) noexcept {
    const difference_type o = (cur_ - blk_) + n;
    if (o >= 0 && o < B) {
      cur_ += n;
    } else {
      const difference_type nodes = o >= 0 ? o / B : -((-o - 1) / B) - 1;
      node_ += nodes;
      blk_ = *node_;
      cur_ = blk_ + (o - nodes * B);
    }
    return *this;
  }
  constexpr deque_iter& operator-=(difference_type n) noexcept { return *this += -n; }

  friend constexpr deque_iter operator+(deque_iter i, difference_type n) noexcept { return i += n; }
  friend constexpr deque_iter operator+(difference_type n, deque_iter i) noexcept { return i += n; }
  friend constexpr deque_iter operator-(deque_iter i, difference_type n) noexcept { return i -= n; }
  friend constexpr difference_type operator-(const deque_iter& a, const deque_iter& b) noexcept {
    return static_cast<difference_type>((a.node_ - b.node_) * B + (a.cur_ - a.blk_) - (b.cur_ - b.blk_));
  }
  friend constexpr bool operator==(const deque_iter& a, const deque_iter& b) noexcept {
    return a.cur_ == b.cur_ && a.node_ == b.node_;
  }
  friend constexpr std::strong_ordering operator<=>(const deque_iter& a, const deque_iter& b) noexcept {
    if (a.node_ != b.node_)
      return a.node_ <=> b.node_;
    return a.cur_ <=> b.cur_;
  }
};

} // namespace ycxx::adl_free

namespace std {

template <class T, class Allocator = allocator<T>>
class deque;

template <class T, class Allocator>
class deque {
  static_assert(ycxx::detail::allocator_for<Allocator, T>,
                "std::deque: Allocator::value_type must be T ([container.alloc.reqmts])");

  using info = ycxx::detail::alloc_info<Allocator>;
  using alloc_traits = allocator_traits<Allocator>;
  using map_alloc = typename info::template rebind<T*>;
  using map_traits = allocator_traits<map_alloc>;

public:
  // ---- types ----
  using value_type = T;
  using allocator_type = Allocator;
  using pointer = typename info::pointer;
  using const_pointer = typename info::const_pointer;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = typename info::size_type;
  using difference_type = typename info::difference_type;
  using iterator = ycxx::adl_free::deque_iter<T, difference_type>;
  using const_iterator = ycxx::adl_free::deque_iter<const T, difference_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  static constexpr size_type B = ycxx::detail::deque_block_len<T>();
  static constexpr bool pocca = info::pocca;
  static constexpr bool pocma = info::pocma;
  static constexpr bool pocs = info::pocs;
  static constexpr bool always_equal = info::always_equal;

  T** map_ = nullptr;
  size_type mcap_ = 0;
  size_type start_ = 0; // absolute index of the first element
  size_type size_ = 0;
  [[no_unique_address]] Allocator alloc_;

  // ---- storage ----
  constexpr T* alloc_block() { return std::to_address(alloc_traits::allocate(alloc_, B)); }
  constexpr void free_block(T* b) noexcept {
    alloc_traits::deallocate(alloc_, ycxx::detail::to_alloc_pointer<pointer>(b), B);
  }
  constexpr T** alloc_map(size_type n) {
    map_alloc ma(alloc_);
    T** m = std::to_address(map_traits::allocate(ma, n));
    for (size_type i = 0; i < n; ++i)
      std::construct_at(m + i, nullptr);
    return m;
  }
  constexpr void free_map(T** m, size_type n) noexcept {
    map_alloc ma(alloc_);
    map_traits::deallocate(ma, ycxx::detail::to_alloc_pointer<typename map_traits::pointer>(m), n);
  }

  constexpr size_type first_slot() const noexcept { return start_ / B; }
  // One past the last slot holding a block (map_ non-null).
  constexpr size_type end_block() const noexcept { return size_ == 0 ? start_ / B + 1 : (start_ + size_ - 1) / B + 1; }
  constexpr T* ptr_at(size_type abs) const noexcept { return map_[abs / B] + abs % B; }
  constexpr T* elem(size_type i) const noexcept { return ptr_at(start_ + i); }

  constexpr iterator iter_at(size_type i) const noexcept {
    if (!map_)
      return iterator();
    const size_type abs = start_ + i;
    T* const* node = map_ + abs / B;
    return iterator(*node + abs % B, *node, node);
  }

  // Creates the map and one block; the empty deque starts at offset off of that block.
  constexpr void init_storage(size_type off) {
    const size_type cap = 8;
    T** m = alloc_map(cap);
    ycxx::detail::rollback rb{[&] { free_map(m, cap); }};
    m[cap / 2] = alloc_block();
    rb.release();
    map_ = m;
    mcap_ = cap;
    start_ = cap / 2 * B + off;
    size_ = 0;
  }

  // Makes room in the map for nf more blocks before the first one and nb after the last one
  // (plus the past-the-end slot). Only the slots move; may reallocate the map. Strong.
  constexpr void reserve_map(size_type nf, size_type nb) {
    const size_type lo = first_slot(), hi = end_block();
    if (lo >= nf && hi + nb < mcap_)
      return;
    const size_type used = hi - lo;
    const size_type need = nf + used + nb + 1;
    if (need > numeric_limits<size_type>::max() / 4)
      ycxx::detail::throw_length_error("std::deque: too many elements");
    if (mcap_ >= 2 * need) {
      // Recenter in place.
      const size_type nlo = (mcap_ - need) / 2 + nf;
      if (nlo < lo) {
        for (size_type i = 0; i < used; ++i) {
          map_[nlo + i] = map_[lo + i];
          map_[lo + i] = nullptr;
        }
      } else if (nlo > lo) {
        for (size_type i = used; i-- > 0;) {
          map_[nlo + i] = map_[lo + i];
          map_[lo + i] = nullptr;
        }
      }
      start_ = start_ - lo * B + nlo * B;
      return;
    }
    const size_type ncap = mcap_ + (mcap_ > need ? mcap_ : need);
    T** m = alloc_map(ncap);
    const size_type nlo = (ncap - need) / 2 + nf;
    for (size_type i = 0; i < used; ++i)
      m[nlo + i] = map_[lo + i];
    free_map(map_, mcap_);
    map_ = m;
    mcap_ = ncap;
    start_ = start_ - lo * B + nlo * B;
  }

  // Frees blocks allocated after the last needed one (rollback of reserve_back).
  constexpr void trim_back() noexcept {
    if (!map_)
      return;
    for (size_type s = end_block(); s < mcap_ && map_[s]; ++s) {
      free_block(map_[s]);
      map_[s] = nullptr;
    }
  }
  // Frees blocks allocated before the first needed one (rollback of reserve_front).
  constexpr void trim_front() noexcept {
    if (!map_)
      return;
    for (size_type s = first_slot(); s > 0 && map_[s - 1]; --s) {
      free_block(map_[s - 1]);
      map_[s - 1] = nullptr;
    }
  }

  // Allocates the blocks for n more elements after the last one. Strong.
  constexpr void reserve_back(size_type n) {
    if (!map_)
      init_storage(0);
    const size_type have = end_block();
    const size_type need = (start_ + size_ + n + B - 1) / B;
    if (need <= have)
      return;
    reserve_map(0, need - have);
    const size_type lo = end_block(), hi = (start_ + size_ + n + B - 1) / B;
    ycxx::detail::rollback rb{[this] { trim_back(); }};
    for (size_type s = lo; s < hi; ++s)
      map_[s] = alloc_block();
    rb.release();
  }
  // Allocates the blocks for n more elements before the first one. Strong.
  constexpr void reserve_front(size_type n) {
    if (!map_)
      init_storage(B - 1);
    const size_type off = start_ % B;
    if (n <= off)
      return;
    const size_type nblocks = (n - off + B - 1) / B;
    reserve_map(nblocks, 0);
    const size_type lo = first_slot();
    ycxx::detail::rollback rb{[this] { trim_front(); }};
    for (size_type k = 1; k <= nblocks; ++k)
      map_[lo - k] = alloc_block();
    rb.release();
  }

  constexpr void check_grow(size_type n) const {
    if (n > max_size() - size_)
      ycxx::detail::throw_length_error("std::deque: size would exceed max_size()");
  }

  // Constructs an element after the last one; a slot must have been reserved.
  template <class... Args>
  constexpr void construct_back(Args&&... args) {
    alloc_traits::construct(alloc_, ptr_at(start_ + size_), static_cast<Args&&>(args)...);
    ++size_;
  }
  // Constructs an element before the first one; a slot must have been reserved.
  template <class... Args>
  constexpr void construct_front(Args&&... args) {
    alloc_traits::construct(alloc_, ptr_at(start_ - 1), static_cast<Args&&>(args)...);
    --start_;
    ++size_;
  }

  // Destroys the first n (< size_) elements and frees the blocks they leave empty.
  constexpr void drop_front(size_type n) noexcept {
    for (size_type i = 0; i < n; ++i)
      alloc_traits::destroy(alloc_, elem(i));
    const size_type lo = first_slot();
    start_ += n;
    size_ -= n;
    for (size_type s = lo; s < first_slot(); ++s) {
      free_block(map_[s]);
      map_[s] = nullptr;
    }
  }
  // Destroys the last n (< size_) elements and frees the blocks they leave empty.
  constexpr void drop_back(size_type n) noexcept {
    for (size_type i = size_ - n; i < size_; ++i)
      alloc_traits::destroy(alloc_, elem(i));
    const size_type hi = end_block();
    size_ -= n;
    for (size_type s = end_block(); s < hi; ++s) {
      free_block(map_[s]);
      map_[s] = nullptr;
    }
  }
  constexpr void erase_front(size_type n) noexcept {
    if (n == size_)
      clear();
    else
      drop_front(n);
  }
  constexpr void erase_back(size_type n) noexcept {
    if (n == size_)
      clear();
    else
      drop_back(n);
  }

  // Frees everything; *this becomes empty without storage.
  constexpr void release_all() noexcept {
    if (!map_)
      return;
    clear();
    free_block(map_[first_slot()]);
    free_map(map_, mcap_);
    map_ = nullptr;
    mcap_ = start_ = size_ = 0;
  }
  // Takes over o's storage; *this owns none.
  constexpr void take(deque& o) noexcept {
    map_ = o.map_;
    mcap_ = o.mcap_;
    start_ = o.start_;
    size_ = o.size_;
    o.map_ = nullptr;
    o.mcap_ = o.start_ = o.size_ = 0;
  }

  // Appends n elements, the i-th constructed by make(slot); strong.
  template <class Make>
  constexpr void append_n(size_type n, Make make) {
    if (n == 0)
      return;
    check_grow(n);
    reserve_back(n);
    const size_type old = size_;
    ycxx::detail::rollback rb{[&] {
      if (size_ != old)
        erase_back(size_ - old);
      trim_back();
    }};
    for (size_type i = 0; i < n; ++i) {
      make(ptr_at(start_ + size_));
      ++size_;
    }
    rb.release();
  }
  // Appends [first, last); strong.
  template <class It, class Sent>
  constexpr void append_elems(It first, Sent last) {
    if constexpr (forward_iterator<It> || sized_sentinel_for<Sent, It>) {
      const auto d = ranges::distance(first, last);
      append_counted(static_cast<It&&>(first), static_cast<size_type>(d));
    } else {
      const size_type old = size_;
      ycxx::detail::rollback rb{[&] {
        if (size_ != old)
          erase_back(size_ - old);
        trim_back();
      }};
      for (; first != last; ++first)
        emplace_back(*first);
      rb.release();
    }
  }
  template <class It>
  constexpr void append_counted(It first, size_type n) {
    append_n(n, [&](T* p) {
      alloc_traits::construct(alloc_, p, *first);
      ++first;
    });
  }
  // Prepends [first, last) in order; strong.
  template <class It, class Sent>
  constexpr void prepend_elems(It first, Sent last) {
    if constexpr (forward_iterator<It> || sized_sentinel_for<Sent, It>) {
      const auto d = ranges::distance(first, last);
      prepend_counted(static_cast<It&&>(first), static_cast<size_type>(d));
    } else {
      // Single pass: push each to the front, then reverse them.
      size_type n = 0;
      ycxx::detail::rollback rb{[&] {
        if (n != 0)
          erase_front(n);
        trim_front();
      }};
      for (; first != last; ++first) {
        emplace_front(*first);
        ++n;
      }
      rb.release();
      std::reverse(begin(), begin() + static_cast<difference_type>(n));
    }
  }
  template <class It>
  constexpr void prepend_counted(It first, size_type n) {
    if (n == 0)
      return;
    check_grow(n);
    reserve_front(n);
    // Construct in order into [start_ - n, start_), then adopt them.
    const size_type base = start_ - n;
    size_type built = 0;
    ycxx::detail::rollback rb{[&] {
      for (size_type i = 0; i < built; ++i)
        alloc_traits::destroy(alloc_, ptr_at(base + i));
      trim_front();
    }};
    for (; built < n; ++built) {
      alloc_traits::construct(alloc_, ptr_at(base + built), *first);
      ++first;
    }
    rb.release();
    start_ = base;
    size_ += n;
  }

  // Rotates [f, l) so that *m comes first. Its temporaries are elements too: built and
  // destroyed through the allocator ([container.alloc.reqmts]/2).
  constexpr void rotate_(iterator f, iterator m, iterator l) {
    ycxx::detail::rotate_elements<ycxx::detail::alloc_temp<T, Allocator>>(f, m, l, alloc_);
  }

  // Inserts [first, last) at index k: at the closer end, then rotated into place.
  template <class It, class Sent>
  constexpr iterator insert_elems(size_type k, It first, Sent last) {
    const size_type s0 = size_;
    if (k >= s0 - k) {
      append_elems(static_cast<It&&>(first), static_cast<Sent&&>(last));
      if (k != s0)
        rotate_(begin() + static_cast<difference_type>(k), begin() + static_cast<difference_type>(s0), end());
    } else {
      prepend_elems(static_cast<It&&>(first), static_cast<Sent&&>(last));
      const auto n = static_cast<difference_type>(size_ - s0);
      if (k != 0)
        rotate_(begin(), begin() + n, begin() + n + static_cast<difference_type>(k));
    }
    return begin() + static_cast<difference_type>(k);
  }

  // A value constructed through the allocator outside the deque (the argument of a middle
  // insertion may refer to an element).
  struct temp_value {
    Allocator& a;
    T* p;
    template <class... Args>
    constexpr explicit temp_value(Allocator& al, Args&&... args) : a(al), p(std::to_address(alloc_traits::allocate(al, 1))) {
      ycxx::detail::rollback rb{[this] { alloc_traits::deallocate(a, ycxx::detail::to_alloc_pointer<pointer>(p), 1); }};
      alloc_traits::construct(a, p, static_cast<Args&&>(args)...);
      rb.release();
    }
    temp_value(const temp_value&) = delete;
    constexpr ~temp_value() {
      alloc_traits::destroy(a, p);
      alloc_traits::deallocate(a, ycxx::detail::to_alloc_pointer<pointer>(p), 1);
    }
  };

  // Inserts n copies of v (not an element of *this) at index k, 0 < k < size_, shifting the
  // shorter side.
  constexpr void insert_fill_middle(size_type k, size_type n, const T& v) {
    check_grow(n);
    const size_type s0 = size_;
    if (k >= s0 - k) {
      reserve_back(n);
      ycxx::detail::rollback rb{[this] { trim_back(); }};
      const size_type after = s0 - k;
      if (after > n) {
        for (size_type i = s0 - n; i < s0; ++i)
          construct_back(static_cast<T&&>(*elem(i)));
        const iterator b = begin();
        std::move_backward(b + static_cast<difference_type>(k), b + static_cast<difference_type>(s0 - n),
                           b + static_cast<difference_type>(s0));
        std::fill_n(b + static_cast<difference_type>(k), n, v);
      } else {
        for (size_type i = 0; i < n - after; ++i)
          construct_back(v);
        for (size_type i = k; i < s0; ++i)
          construct_back(static_cast<T&&>(*elem(i)));
        const iterator b = begin();
        std::fill(b + static_cast<difference_type>(k), b + static_cast<difference_type>(s0), v);
      }
      rb.release();
    } else {
      reserve_front(n);
      ycxx::detail::rollback rb{[this] { trim_front(); }};
      const iterator ob = begin(); // the old first element; stays valid while constructing
      const auto dn = static_cast<difference_type>(n), dk = static_cast<difference_type>(k);
      if (k >= n) {
        for (difference_type i = dn; i-- > 0;)
          construct_front(static_cast<T&&>(ob[i]));
        std::move(ob + dn, ob + dk, ob);
        std::fill(ob + (dk - dn), ob + dk, v);
      } else {
        for (size_type i = 0; i < n - k; ++i)
          construct_front(v);
        for (difference_type i = dk; i-- > 0;)
          construct_front(static_cast<T&&>(ob[i]));
        std::fill(ob, ob + dk, v);
      }
      rb.release();
    }
  }

public:
  // ---- [deque.cons] ----
  constexpr deque() noexcept(is_nothrow_default_constructible_v<Allocator>) : deque(Allocator()) {}
  constexpr explicit deque(const Allocator& a) noexcept : alloc_(a) {}
  constexpr explicit deque(size_type n, const Allocator& a = Allocator()) : deque(a) {
    append_n(n, [this](T* p) { alloc_traits::construct(alloc_, p); });
  }
  constexpr deque(size_type n, const T& value, const Allocator& a = Allocator()) : deque(a) {
    append_n(n, [&](T* p) { alloc_traits::construct(alloc_, p, value); });
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr deque(InputIterator first, InputIterator last, const Allocator& a = Allocator()) : deque(a) {
    append_elems(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
  }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr deque(from_range_t, R&& rg, const Allocator& a = Allocator()) : deque(a) {
    append_range(static_cast<R&&>(rg));
  }
  constexpr deque(const deque& x) : deque(alloc_traits::select_on_container_copy_construction(x.alloc_)) {
    append_counted(x.begin(), x.size_);
  }
  constexpr deque(deque&& x) noexcept(is_nothrow_move_constructible_v<Allocator>) : alloc_(static_cast<Allocator&&>(x.alloc_)) { take(x); }
  constexpr deque(const deque& x, const type_identity_t<Allocator>& a) : deque(a) { append_counted(x.begin(), x.size_); }
  // noexcept when the allocators always compare equal (an extension: nothing is allocated).
  constexpr deque(deque&& x, const type_identity_t<Allocator>& a) noexcept(always_equal) : deque(a) {
    if (always_equal || alloc_ == x.alloc_)
      take(x);
    else
      append_counted(std::make_move_iterator(x.begin()), x.size_);
  }
  constexpr deque(initializer_list<T> il, const Allocator& a = Allocator()) : deque(a) {
    append_counted(il.begin(), il.size());
  }
  constexpr ~deque() { release_all(); }

  constexpr deque& operator=(const deque& x) {
    if (this == __builtin_addressof(x))
      return *this;
    if constexpr (pocca) {
      if (!always_equal && alloc_ != x.alloc_)
        release_all();
      alloc_ = x.alloc_;
    }
    assign(x.begin(), x.end());
    return *this;
  }
  constexpr deque& operator=(deque&& x) noexcept(always_equal) {
    if (this == __builtin_addressof(x))
      return *this;
    if constexpr (pocma || always_equal) {
      release_all();
      if constexpr (pocma)
        alloc_ = static_cast<Allocator&&>(x.alloc_);
      take(x);
    } else {
      if (alloc_ == x.alloc_) {
        release_all();
        take(x);
      } else {
        assign(std::make_move_iterator(x.begin()), std::make_move_iterator(x.end()));
      }
    }
    return *this;
  }
  constexpr deque& operator=(initializer_list<T> il) {
    assign(il.begin(), il.end());
    return *this;
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr void assign(InputIterator first, InputIterator last) {
    assign_elems(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
  }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr void assign_range(R&& rg) {
    static_assert(assignable_from<T&, ranges::range_reference_t<R>>,
                  "std::deque::assign_range: T must be assignable from the range's reference type");
    assign_elems(ranges::begin(rg), ranges::end(rg));
  }
  constexpr void assign(size_type n, const T& t) {
    const size_type m = n < size_ ? n : size_;
    std::fill_n(begin(), m, t);
    if (n > size_)
      append_n(n - size_, [&](T* p) { alloc_traits::construct(alloc_, p, t); });
    else if (n < size_)
      erase_back(size_ - n);
  }
  constexpr void assign(initializer_list<T> il) { assign(il.begin(), il.end()); }
  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

private:
  template <class It, class Sent>
  constexpr void assign_elems(It first, Sent last) {
    iterator cur = begin();
    const iterator e = end();
    for (; first != last && cur != e; ++cur, (void)++first)
      *cur = *first;
    if (first == last)
      erase_back(static_cast<size_type>(e - cur));
    else
      append_elems(static_cast<It&&>(first), static_cast<Sent&&>(last));
  }

public:
  // ---- iterators ----
  constexpr iterator begin() noexcept { return iter_at(0); }
  constexpr const_iterator begin() const noexcept { return iter_at(0); }
  constexpr iterator end() noexcept { return iter_at(size_); }
  constexpr const_iterator end() const noexcept { return iter_at(size_); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [deque.capacity] ----
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
  constexpr size_type size() const noexcept { return size_; }
  constexpr size_type max_size() const noexcept {
    const size_type a = alloc_traits::max_size(alloc_);
    const auto d = static_cast<size_type>(numeric_limits<difference_type>::max());
    return a < d ? a : d;
  }
  constexpr void resize(size_type sz) {
    if (sz < size_)
      erase_back(size_ - sz);
    else
      append_n(sz - size_, [this](T* p) { alloc_traits::construct(alloc_, p); });
  }
  constexpr void resize(size_type sz, const T& c) {
    if (sz < size_)
      erase_back(size_ - sz);
    else
      append_n(sz - size_, [&](T* p) { alloc_traits::construct(alloc_, p, c); });
  }
  constexpr void shrink_to_fit() {
    if (!map_)
      return;
    if (size_ == 0) {
      release_all();
      return;
    }
    const size_type lo = first_slot(), used = end_block() - lo;
    if (mcap_ <= used + 1)
      return;
    T** m = alloc_map(used + 1);
    for (size_type i = 0; i < used; ++i)
      m[i] = map_[lo + i];
    free_map(map_, mcap_);
    map_ = m;
    mcap_ = used + 1;
    start_ -= lo * B;
  }

  // ---- element access ----
  constexpr reference operator[](size_type n) {
    ycxx::detail::precondition(n < size_, "std::deque::operator[]: index out of range");
    return *elem(n);
  }
  constexpr const_reference operator[](size_type n) const {
    ycxx::detail::precondition(n < size_, "std::deque::operator[]: index out of range");
    return *elem(n);
  }
  constexpr reference at(size_type n) {
    if (n >= size_)
      ycxx::detail::throw_out_of_range("std::deque::at: index out of range");
    return *elem(n);
  }
  constexpr const_reference at(size_type n) const {
    if (n >= size_)
      ycxx::detail::throw_out_of_range("std::deque::at: index out of range");
    return *elem(n);
  }
  constexpr reference front() {
    ycxx::detail::precondition(size_ != 0, "std::deque::front: empty deque");
    return *elem(0);
  }
  constexpr const_reference front() const {
    ycxx::detail::precondition(size_ != 0, "std::deque::front: empty deque");
    return *elem(0);
  }
  constexpr reference back() {
    ycxx::detail::precondition(size_ != 0, "std::deque::back: empty deque");
    return *elem(size_ - 1);
  }
  constexpr const_reference back() const {
    ycxx::detail::precondition(size_ != 0, "std::deque::back: empty deque");
    return *elem(size_ - 1);
  }

  // ---- [deque.modifiers] ----
  template <class... Args>
  constexpr reference emplace_front(Args&&... args) {
    check_grow(1);
    reserve_front(1);
    ycxx::detail::rollback rb{[this] { trim_front(); }};
    construct_front(static_cast<Args&&>(args)...);
    rb.release();
    return *elem(0);
  }
  template <class... Args>
  constexpr reference emplace_back(Args&&... args) {
    check_grow(1);
    reserve_back(1);
    ycxx::detail::rollback rb{[this] { trim_back(); }};
    construct_back(static_cast<Args&&>(args)...);
    rb.release();
    return *elem(size_ - 1);
  }
  template <class... Args>
  constexpr iterator emplace(const_iterator position, Args&&... args) {
    const auto k = static_cast<size_type>(position - cbegin());
    if (k == 0) {
      emplace_front(static_cast<Args&&>(args)...);
      return begin();
    }
    if (k == size_) {
      emplace_back(static_cast<Args&&>(args)...);
      return end() - 1;
    }
    temp_value t(alloc_, static_cast<Args&&>(args)...);
    const auto dk = static_cast<difference_type>(k);
    if (k < size_ - k) {
      emplace_front(static_cast<T&&>(*elem(0)));
      const iterator b = begin();
      std::move(b + 2, b + dk + 1, b + 1);
    } else {
      emplace_back(static_cast<T&&>(*elem(size_ - 1)));
      const iterator b = begin();
      std::move_backward(b + dk, b + static_cast<difference_type>(size_ - 2), b + static_cast<difference_type>(size_ - 1));
    }
    *elem(k) = static_cast<T&&>(*t.p);
    return begin() + dk;
  }
  constexpr void push_front(const T& x) { emplace_front(x); }
  constexpr void push_front(T&& x) { emplace_front(static_cast<T&&>(x)); }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr void prepend_range(R&& rg) {
    if constexpr (ranges::sized_range<R> && !ranges::forward_range<R>)
      prepend_counted(ranges::begin(rg), static_cast<size_type>(ranges::size(rg)));
    else
      prepend_elems(ranges::begin(rg), ranges::end(rg));
  }
  constexpr void push_back(const T& x) { emplace_back(x); }
  constexpr void push_back(T&& x) { emplace_back(static_cast<T&&>(x)); }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr void append_range(R&& rg) {
    if constexpr (ranges::sized_range<R> && !ranges::forward_range<R>)
      append_counted(ranges::begin(rg), static_cast<size_type>(ranges::size(rg)));
    else
      append_elems(ranges::begin(rg), ranges::end(rg));
  }

  constexpr iterator insert(const_iterator position, const T& x) { return emplace(position, x); }
  constexpr iterator insert(const_iterator position, T&& x) { return emplace(position, static_cast<T&&>(x)); }
  constexpr iterator insert(const_iterator position, size_type n, const T& x) {
    const auto k = static_cast<size_type>(position - cbegin());
    if (n == 0)
      return begin() + static_cast<difference_type>(k);
    if (k == size_) {
      append_n(n, [&](T* p) { alloc_traits::construct(alloc_, p, x); });
    } else if (k == 0) {
      check_grow(n);
      reserve_front(n);
      size_type built = 0;
      ycxx::detail::rollback rb{[&] {
        if (built != 0)
          erase_front(built);
        trim_front();
      }};
      for (; built < n; ++built)
        construct_front(x);
      rb.release();
    } else {
      temp_value t(alloc_, x);
      insert_fill_middle(k, n, *t.p);
    }
    return begin() + static_cast<difference_type>(k);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr iterator insert(const_iterator position, InputIterator first, InputIterator last) {
    return insert_elems(static_cast<size_type>(position - cbegin()), static_cast<InputIterator&&>(first),
                        static_cast<InputIterator&&>(last));
  }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr iterator insert_range(const_iterator position, R&& rg) {
    const auto k = static_cast<size_type>(position - cbegin());
    if constexpr (ranges::sized_range<R> && !ranges::forward_range<R>) {
      // A sized single-pass range: insert its elements at the closer end by count.
      const size_type s0 = size_;
      const auto n = static_cast<size_type>(ranges::size(rg));
      if (k >= s0 - k) {
        append_counted(ranges::begin(rg), n);
        if (k != s0)
          rotate_(begin() + static_cast<difference_type>(k), begin() + static_cast<difference_type>(s0), end());
      } else {
        prepend_counted(ranges::begin(rg), n);
        if (k != 0)
          rotate_(begin(), begin() + static_cast<difference_type>(n), begin() + static_cast<difference_type>(n + k));
      }
      return begin() + static_cast<difference_type>(k);
    } else {
      return insert_elems(k, ranges::begin(rg), ranges::end(rg));
    }
  }
  constexpr iterator insert(const_iterator position, initializer_list<T> il) {
    return insert(position, il.begin(), il.end());
  }

  constexpr void pop_front() {
    ycxx::detail::precondition(size_ != 0, "std::deque::pop_front: empty deque");
    erase_front(1);
  }
  constexpr void pop_back() {
    ycxx::detail::precondition(size_ != 0, "std::deque::pop_back: empty deque");
    erase_back(1);
  }
  constexpr iterator erase(const_iterator position) { return erase(position, position + 1); }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    const auto k = static_cast<size_type>(first - cbegin());
    const auto n = static_cast<size_type>(last - first);
    if (n == 0)
      return begin() + static_cast<difference_type>(k);
    const iterator b = begin();
    const auto dk = static_cast<difference_type>(k), dn = static_cast<difference_type>(n);
    if (k < size_ - k - n) {
      std::move_backward(b, b + dk, b + dk + dn);
      erase_front(n);
    } else {
      std::move(b + dk + dn, end(), b + dk);
      erase_back(n);
    }
    return begin() + dk;
  }
  constexpr void swap(deque& x) noexcept(always_equal) {
    if (this == __builtin_addressof(x))
      return;
    if constexpr (pocs)
      ::ycxx::detail::swap_adl::do_swap(alloc_, x.alloc_);
    else
      ycxx::detail::precondition(always_equal || alloc_ == x.alloc_,
                                 "std::deque::swap: unequal allocators that do not propagate");
    T** m = map_;
    const size_type c = mcap_, s = start_, n = size_;
    map_ = x.map_;
    mcap_ = x.mcap_;
    start_ = x.start_;
    size_ = x.size_;
    x.map_ = m;
    x.mcap_ = c;
    x.start_ = s;
    x.size_ = n;
  }
  // Destroys every element; keeps one block (the first) and the map.
  constexpr void clear() noexcept {
    if (!map_)
      return;
    for (size_type i = 0; i < size_; ++i)
      alloc_traits::destroy(alloc_, elem(i));
    const size_type lo = first_slot(), hi = end_block();
    for (size_type s = lo + 1; s < hi; ++s) {
      free_block(map_[s]);
      map_[s] = nullptr;
    }
    start_ = lo * B + B / 2;
    size_ = 0;
  }
};

// ---- deduction guides ----
template <class InputIterator, class Allocator = allocator<ycxx::detail::iter_value_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
deque(InputIterator, InputIterator, Allocator = Allocator())
    -> deque<ycxx::detail::iter_value_type<InputIterator>, Allocator>;
template <ranges::input_range R, class Allocator = allocator<ranges::range_value_t<R>>>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
deque(from_range_t, R&&, Allocator = Allocator()) -> deque<ranges::range_value_t<R>, Allocator>;

// ---- comparisons ----
template <class T, class Allocator>
constexpr bool operator==(const deque<T, Allocator>& x, const deque<T, Allocator>& y) {
  return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}
template <class T, class Allocator>
constexpr ycxx::detail::synth_three_way_result<T> operator<=>(const deque<T, Allocator>& x,
                                                              const deque<T, Allocator>& y) {
  return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(),
                                                ycxx::detail::synth_three_way);
}

template <class T, class Allocator>
constexpr void swap(deque<T, Allocator>& x, deque<T, Allocator>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

// ---- [deque.erasure] ----
template <class T, class Allocator, class Predicate>
constexpr typename deque<T, Allocator>::size_type erase_if(deque<T, Allocator>& c, Predicate pred) {
  auto it = std::remove_if(c.begin(), c.end(), pred);
  const auto r = static_cast<typename deque<T, Allocator>::size_type>(c.end() - it);
  c.erase(it, c.end());
  return r;
}
template <class T, class Allocator, class U = T>
constexpr typename deque<T, Allocator>::size_type erase(deque<T, Allocator>& c, const U& value) {
  auto it = std::remove(c.begin(), c.end(), value);
  const auto r = static_cast<typename deque<T, Allocator>::size_type>(c.end() - it);
  c.erase(it, c.end());
  return r;
}

namespace pmr {
template <class T>
using deque = std::deque<T, polymorphic_allocator<T>>;
} // namespace pmr

} // namespace std
