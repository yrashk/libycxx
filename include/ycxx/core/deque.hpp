// libycxx core: deque ([deque]), its comparisons, erasure and the pmr:: alias.
//
// Representation: a map (an array of mcap_ block pointers, allocated through the allocator
// rebound to T*) and blocks of B elements each. Elements are numbered by an absolute index:
// element i lives at abs = start_ + i (i < end_ - start_), in block map_[abs / B] at offset
// abs % B; head_ and tail_ cache the addresses of the ends (below). Map slots
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

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Tp, class _Allocator>
class deque;
}

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// Elements per block: about 1 KiB, a power of two, at least 16.
template <class _Tp>
consteval std::size_t __deque_block_len() {
  const std::size_t n = sizeof(_Tp) <= 1024 / 16 ? 1024 / sizeof(_Tp) : 16;
  std::size_t p = 16;
  while (p * 2 <= n)
    p *= 2;
  return p;
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// T is the element type, possibly const.
template <class _Tp, class _Diff>
class __deque_iter {
  using _Vp = std::remove_const_t<_Tp>;
  static constexpr _Diff _Bp = static_cast<_Diff>(::__ycxx::__detail::__deque_block_len<_Vp>());

  _Tp* __cur_ = nullptr;
  _Tp* __blk_ = nullptr;
  _Vp* const* __node_ = nullptr;

  template <class, class>
  friend class __deque_iter;
  template <class, class>
  friend class std::deque;

  constexpr __deque_iter(_Tp* c, _Tp* b, _Vp* const* n) noexcept : __cur_(c), __blk_(b), __node_(n) {}

public:
  using iterator_concept = std::random_access_iterator_tag;
  using iterator_category = std::random_access_iterator_tag;
  using value_type = _Vp;
  using difference_type = _Diff;
  using pointer = _Tp*;
  using reference = _Tp&;

  constexpr __deque_iter() noexcept = default;
  template <class _Up>
    requires std::is_same_v<const _Up, _Tp> && (!std::is_same_v<_Up, _Tp>)
  constexpr __deque_iter(const __deque_iter<_Up, _Diff>& __o) noexcept : __cur_(__o.__cur_), __blk_(__o.__blk_), __node_(__o.__node_) {}

  constexpr reference operator*() const noexcept { return *__cur_; }
  constexpr pointer operator->() const noexcept { return __cur_; }
  constexpr reference operator[](difference_type n) const noexcept { return *(*this + n); }

  constexpr __deque_iter& operator++() noexcept {
    if (++__cur_ == __blk_ + _Bp) {
      ++__node_;
      __blk_ = *__node_;
      __cur_ = __blk_;
    }
    return *this;
  }
  constexpr __deque_iter operator++(int) noexcept {
    __deque_iter t = *this;
    ++*this;
    return t;
  }
  constexpr __deque_iter& operator--() noexcept {
    if (__cur_ == __blk_) {
      --__node_;
      __blk_ = *__node_;
      __cur_ = __blk_ + _Bp;
    }
    --__cur_;
    return *this;
  }
  constexpr __deque_iter operator--(int) noexcept {
    __deque_iter t = *this;
    --*this;
    return t;
  }
  constexpr __deque_iter& operator+=(difference_type n) noexcept {
    const difference_type __o = (__cur_ - __blk_) + n;
    if (__o >= 0 && __o < _Bp) {
      __cur_ += n;
    } else {
      const difference_type __nodes = __o >= 0 ? __o / _Bp : -((-__o - 1) / _Bp) - 1;
      __node_ += __nodes;
      __blk_ = *__node_;
      __cur_ = __blk_ + (__o - __nodes * _Bp);
    }
    return *this;
  }
  constexpr __deque_iter& operator-=(difference_type n) noexcept { return *this += -n; }

  friend constexpr __deque_iter operator+(__deque_iter i, difference_type n) noexcept { return i += n; }
  friend constexpr __deque_iter operator+(difference_type n, __deque_iter i) noexcept { return i += n; }
  friend constexpr __deque_iter operator-(__deque_iter i, difference_type n) noexcept { return i -= n; }
  friend constexpr difference_type operator-(const __deque_iter& a, const __deque_iter& b) noexcept {
    return static_cast<difference_type>((a.__node_ - b.__node_) * _Bp + (a.__cur_ - a.__blk_) - (b.__cur_ - b.__blk_));
  }
  // Element addresses identify positions: the only null cur_ in a deque is its past-the-end
  // position after a full last block (and a value-initialized iterator).
  friend constexpr bool operator==(const __deque_iter& a, const __deque_iter& b) noexcept { return a.__cur_ == b.__cur_; }
  friend constexpr std::strong_ordering operator<=>(const __deque_iter& a, const __deque_iter& b) noexcept {
    if (a.__node_ != b.__node_)
      return a.__node_ <=> b.__node_;
    return a.__cur_ <=> b.__cur_;
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp, class _Allocator = allocator<_Tp>>
class deque;

template <class _Tp, class _Allocator>
class deque {
  static_assert(__ycxx::__detail::__allocator_for<_Allocator, _Tp>,
                "std::deque: Allocator::value_type must be T ([container.alloc.reqmts])");

  using info = __ycxx::__detail::__alloc_info<_Allocator>;
  using __alloc_traits = allocator_traits<_Allocator>;
  using __map_alloc = typename info::template rebind<_Tp*>;
  using __map_traits = allocator_traits<__map_alloc>;

public:
  // ---- types ----
  using value_type = _Tp;
  using allocator_type = _Allocator;
  using pointer = typename info::pointer;
  using const_pointer = typename info::const_pointer;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = typename info::size_type;
  using difference_type = typename info::difference_type;
  using iterator = __ycxx::__adl_free::__deque_iter<_Tp, difference_type>;
  using const_iterator = __ycxx::__adl_free::__deque_iter<const _Tp, difference_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  static constexpr size_type _Bp = __ycxx::__detail::__deque_block_len<_Tp>();
  static constexpr bool __pocca = info::__pocca;
  static constexpr bool __pocma = info::__pocma;
  static constexpr bool __pocs = info::__pocs;
  static constexpr bool __always_equal = info::__always_equal;

  _Tp** __map_ = nullptr;
  size_type __mcap_ = 0;
  size_type __start_ = 0; // absolute index of the first element
  size_type __end_ = 0;   // absolute index one past the last element
  // Caches of the ends, kept whenever map_ is non-null: head_ is the address of element 0 (of
  // slot start_ for an empty deque), tail_ one past the address of the last element (head_ for an
  // empty deque; it may be the end of the last block). Blocks never move, so recentering the map
  // leaves them valid; every other change of start_ or end_ updates them (sync_ends).
  _Tp* __head_ = nullptr;
  _Tp* __tail_ = nullptr;
  [[no_unique_address]] _Allocator __alloc_;

  // ---- storage ----
  constexpr _Tp* __alloc_block() { return std::to_address(__alloc_traits::allocate(__alloc_, _Bp)); }
  constexpr void __free_block(_Tp* b) noexcept {
    __alloc_traits::deallocate(__alloc_, __ycxx::__detail::__to_alloc_pointer<pointer>(b), _Bp);
  }
  constexpr _Tp** __alloc_map(size_type n) {
    __map_alloc __ma(__alloc_);
    _Tp** m = std::to_address(__map_traits::allocate(__ma, n));
    for (size_type i = 0; i < n; ++i)
      std::construct_at(m + i, nullptr);
    return m;
  }
  constexpr void __free_map(_Tp** m, size_type n) noexcept {
    __map_alloc __ma(__alloc_);
    __map_traits::deallocate(__ma, __ycxx::__detail::__to_alloc_pointer<typename __map_traits::pointer>(m), n);
  }

  constexpr size_type __count() const noexcept { return __end_ - __start_; }
  constexpr size_type __first_slot() const noexcept { return __start_ / _Bp; }
  // One past the last slot holding a block (map_ non-null).
  constexpr size_type __end_block() const noexcept { return __count() == 0 ? __start_ / _Bp + 1 : (__end_ - 1) / _Bp + 1; }
  constexpr _Tp* __ptr_at(size_type abs) const noexcept { return __map_[abs / _Bp] + abs % _Bp; }
  constexpr _Tp* __elem(size_type i) const noexcept { return __ptr_at(__start_ + i); }
  // Recomputes head_ and tail_ from start_ and end_ (map_ non-null).
  constexpr void __sync_ends() noexcept {
    __head_ = __ptr_at(__start_);
    __tail_ = __count() == 0 ? __head_ : __ptr_at(__end_ - 1) + 1;
  }

  constexpr iterator __iter_at(size_type i) const noexcept {
    if (!__map_)
      return iterator();
    const size_type abs = __start_ + i;
    _Tp* const* node = __map_ + abs / _Bp;
    return iterator(*node + abs % _Bp, *node, node);
  }

  // Creates the map and one block; the empty deque starts at offset off of that block.
  constexpr void __init_storage(size_type __off) {
    const size_type __cap = 8;
    _Tp** m = __alloc_map(__cap);
    __ycxx::__detail::__rollback __rb{[&] { __free_map(m, __cap); }};
    m[__cap / 2] = __alloc_block();
    __rb.release();
    __map_ = m;
    __mcap_ = __cap;
    __start_ = __cap / 2 * _Bp + __off;
    __end_ = __start_;
    __sync_ends();
  }

  // Makes room in the map for nf more blocks before the first one and nb after the last one
  // (plus the past-the-end slot). Only the slots move; may reallocate the map. Strong.
  constexpr void __reserve_map(size_type __nf, size_type __nb) {
    const size_type __lo = __first_slot(), __hi = __end_block();
    if (__lo >= __nf && __hi + __nb < __mcap_)
      return;
    const size_type __y_used = __hi - __lo;
    const size_type __need = __nf + __y_used + __nb + 1;
    if (__need > numeric_limits<size_type>::max() / 4)
      __ycxx::__detail::__throw_length_error("std::deque: too many elements");
    if (__mcap_ >= 2 * __need) {
      // Recenter in place.
      const size_type __nlo = (__mcap_ - __need) / 2 + __nf;
      if (__nlo < __lo) {
        for (size_type i = 0; i < __y_used; ++i) {
          __map_[__nlo + i] = __map_[__lo + i];
          __map_[__lo + i] = nullptr;
        }
      } else if (__nlo > __lo) {
        for (size_type i = __y_used; i-- > 0;) {
          __map_[__nlo + i] = __map_[__lo + i];
          __map_[__lo + i] = nullptr;
        }
      }
      __start_ = __start_ - __lo * _Bp + __nlo * _Bp;
      __end_ = __end_ - __lo * _Bp + __nlo * _Bp;
      return;
    }
    const size_type __ncap = __mcap_ + (__mcap_ > __need ? __mcap_ : __need);
    _Tp** m = __alloc_map(__ncap);
    const size_type __nlo = (__ncap - __need) / 2 + __nf;
    for (size_type i = 0; i < __y_used; ++i)
      m[__nlo + i] = __map_[__lo + i];
    __free_map(__map_, __mcap_);
    __map_ = m;
    __mcap_ = __ncap;
    __start_ = __start_ - __lo * _Bp + __nlo * _Bp;
    __end_ = __end_ - __lo * _Bp + __nlo * _Bp;
  }

  // Frees blocks allocated after the last needed one (rollback of reserve_back).
  constexpr void __trim_back() noexcept {
    if (!__map_)
      return;
    for (size_type s = __end_block(); s < __mcap_ && __map_[s]; ++s) {
      __free_block(__map_[s]);
      __map_[s] = nullptr;
    }
  }
  // Frees blocks allocated before the first needed one (rollback of reserve_front).
  constexpr void __trim_front() noexcept {
    if (!__map_)
      return;
    for (size_type s = __first_slot(); s > 0 && __map_[s - 1]; --s) {
      __free_block(__map_[s - 1]);
      __map_[s - 1] = nullptr;
    }
  }

  // Allocates the blocks for n more elements after the last one. Strong.
  constexpr void __reserve_back(size_type n) {
    if (!__map_)
      __init_storage(0);
    const size_type __have = __end_block();
    const size_type __need = (__end_ + n + _Bp - 1) / _Bp;
    if (__need <= __have)
      return;
    __reserve_map(0, __need - __have);
    const size_type __lo = __end_block(), __hi = (__end_ + n + _Bp - 1) / _Bp;
    __ycxx::__detail::__rollback __rb{[this] { __trim_back(); }};
    for (size_type s = __lo; s < __hi; ++s)
      __map_[s] = __alloc_block();
    __rb.release();
  }
  // Allocates the blocks for n more elements before the first one. Strong.
  constexpr void __reserve_front(size_type n) {
    if (!__map_)
      __init_storage(_Bp - 1);
    const size_type __off = __start_ % _Bp;
    if (n <= __off)
      return;
    const size_type __nblocks = (n - __off + _Bp - 1) / _Bp;
    __reserve_map(__nblocks, 0);
    const size_type __lo = __first_slot();
    __ycxx::__detail::__rollback __rb{[this] { __trim_front(); }};
    for (size_type k = 1; k <= __nblocks; ++k)
      __map_[__lo - k] = __alloc_block();
    __rb.release();
  }

  constexpr void __check_grow(size_type n) const {
    if (n > max_size() - __count())
      __ycxx::__detail::__throw_length_error("std::deque: size would exceed max_size()");
  }

  // Constructs an element after the last one; a slot must have been reserved.
  template <class... _Args>
  constexpr void __construct_back(_Args&&... __args) {
    __alloc_traits::construct(__alloc_, __ptr_at(__end_), static_cast<_Args&&>(__args)...);
    ++__end_;
    __sync_ends();
  }
  // Constructs an element before the first one; a slot must have been reserved.
  template <class... _Args>
  constexpr void __construct_front(_Args&&... __args) {
    __alloc_traits::construct(__alloc_, __ptr_at(__start_ - 1), static_cast<_Args&&>(__args)...);
    --__start_;
    __sync_ends();
  }

  // Destroys the first n (< size()) elements and frees the blocks they leave empty.
  constexpr void __drop_front(size_type n) noexcept {
    for (size_type i = 0; i < n; ++i)
      __alloc_traits::destroy(__alloc_, __elem(i));
    const size_type __lo = __first_slot();
    __start_ += n;
    __sync_ends();
    for (size_type s = __lo; s < __first_slot(); ++s) {
      __free_block(__map_[s]);
      __map_[s] = nullptr;
    }
  }
  // Destroys the last n (< size()) elements and frees the blocks they leave empty.
  constexpr void __drop_back(size_type n) noexcept {
    for (size_type i = __count() - n; i < __count(); ++i)
      __alloc_traits::destroy(__alloc_, __elem(i));
    const size_type __hi = __end_block();
    __end_ -= n;
    __sync_ends();
    for (size_type s = __end_block(); s < __hi; ++s) {
      __free_block(__map_[s]);
      __map_[s] = nullptr;
    }
  }
  constexpr void __erase_front(size_type n) noexcept {
    if (n == __count())
      clear();
    else
      __drop_front(n);
  }
  constexpr void __erase_back(size_type n) noexcept {
    if (n == __count())
      clear();
    else
      __drop_back(n);
  }

  // Frees everything; *this becomes empty without storage.
  constexpr void __release_all() noexcept {
    if (!__map_)
      return;
    clear();
    __free_block(__map_[__first_slot()]);
    __free_map(__map_, __mcap_);
    __map_ = nullptr;
    __mcap_ = __start_ = __end_ = 0;
    __head_ = __tail_ = nullptr;
  }
  // Takes over o's storage; *this owns none.
  constexpr void take(deque& __o) noexcept {
    __map_ = __o.__map_;
    __mcap_ = __o.__mcap_;
    __start_ = __o.__start_;
    __end_ = __o.__end_;
    __head_ = __o.__head_;
    __tail_ = __o.__tail_;
    __o.__map_ = nullptr;
    __o.__mcap_ = __o.__start_ = __o.__end_ = 0;
    __o.__head_ = __o.__tail_ = nullptr;
  }

  // Appends n elements, the i-th constructed by make(slot); strong.
  template <class _Make>
  constexpr void __append_n(size_type n, _Make __make) {
    if (n == 0)
      return;
    __check_grow(n);
    __reserve_back(n);
    const size_type __old = __count();
    __ycxx::__detail::__rollback __rb{[&] {
      if (__count() != __old)
        __erase_back(__count() - __old);
      __trim_back();
    }};
    for (size_type i = 0; i < n; ++i) {
      __make(__ptr_at(__end_));
      ++__end_;
    }
    __rb.release();
    __sync_ends();
  }
  // Appends [first, last); strong.
  template <class _It, class _Sent>
  constexpr void __append_elems(_It first, _Sent last) {
    if constexpr (forward_iterator<_It> || sized_sentinel_for<_Sent, _It>) {
      const auto d = ranges::distance(first, last);
      __append_counted(static_cast<_It&&>(first), static_cast<size_type>(d));
    } else {
      const size_type __old = __count();
      __ycxx::__detail::__rollback __rb{[&] {
        if (__count() != __old)
          __erase_back(__count() - __old);
        __trim_back();
      }};
      for (; first != last; ++first)
        emplace_back(*first);
      __rb.release();
    }
  }
  // The iterator is advanced before constructing the next element, not after constructing the
  // previous one: an exception from ++first then leaves no constructed-but-uncounted element
  // ([deque.modifiers]/3: no effects; [res.on.exception.handling]).
  template <class _It>
  constexpr void __append_counted(_It first, size_type n) {
    bool __started = false;
    __append_n(n, [&](_Tp* p) {
      if (__started)
        ++first;
      __started = true;
      __alloc_traits::construct(__alloc_, p, *first);
    });
  }
  // Prepends [first, last) in order; strong.
  template <class _It, class _Sent>
  constexpr void __prepend_elems(_It first, _Sent last) {
    if constexpr (forward_iterator<_It> || sized_sentinel_for<_Sent, _It>) {
      const auto d = ranges::distance(first, last);
      __prepend_counted(static_cast<_It&&>(first), static_cast<size_type>(d));
    } else {
      // Single pass: push each to the front, then reverse them.
      size_type n = 0;
      __ycxx::__detail::__rollback __rb{[&] {
        if (n != 0)
          __erase_front(n);
        __trim_front();
      }};
      for (; first != last; ++first) {
        emplace_front(*first);
        ++n;
      }
      __rb.release();
      // Reversed with move assignments and a temporary built through the allocator (not
      // std::reverse, whose swaps make the temporaries outside it).
      for (iterator __lo = begin(), __hi = begin() + static_cast<difference_type>(n); __lo != __hi && __lo != --__hi; ++__lo) {
        __ycxx::__detail::__alloc_temp<_Tp, _Allocator> __tmp(__alloc_, static_cast<_Tp&&>(*__lo));
        *__lo = static_cast<_Tp&&>(*__hi);
        *__hi = static_cast<_Tp&&>(__tmp.__v);
      }
    }
  }
  template <class _It>
  constexpr void __prepend_counted(_It first, size_type n) {
    if (n == 0)
      return;
    __check_grow(n);
    __reserve_front(n);
    // Construct in order into [start_ - n, start_), then adopt them.
    const size_type base = __start_ - n;
    size_type __built = 0;
    __ycxx::__detail::__rollback __rb{[&] {
      for (size_type i = 0; i < __built; ++i)
        __alloc_traits::destroy(__alloc_, __ptr_at(base + i));
      __trim_front();
    }};
    for (; __built < n; ++__built) {
      if (__built != 0)
        ++first; // before constructing, as in append_counted
      __alloc_traits::construct(__alloc_, __ptr_at(base + __built), *first);
    }
    __rb.release();
    __start_ = base;
    __sync_ends();
  }

  // Rotates [f, l) so that *m comes first. Its temporaries are elements too: built and
  // destroyed through the allocator ([container.alloc.reqmts]/2).
  constexpr void __rotate_(iterator __f, iterator m, iterator __l) {
    __ycxx::__detail::__rotate_elements<__ycxx::__detail::__alloc_temp<_Tp, _Allocator>>(__f, m, __l, __alloc_);
  }

  // Inserts [first, last) at index k: at the closer end, then rotated into place.
  template <class _It, class _Sent>
  constexpr iterator __insert_elems(size_type k, _It first, _Sent last) {
    const size_type __s0 = __count();
    if (k >= __s0 - k) {
      __append_elems(static_cast<_It&&>(first), static_cast<_Sent&&>(last));
      if (k != __s0)
        __rotate_(begin() + static_cast<difference_type>(k), begin() + static_cast<difference_type>(__s0), end());
    } else {
      __prepend_elems(static_cast<_It&&>(first), static_cast<_Sent&&>(last));
      const auto n = static_cast<difference_type>(__count() - __s0);
      if (k != 0)
        __rotate_(begin(), begin() + n, begin() + n + static_cast<difference_type>(k));
    }
    return begin() + static_cast<difference_type>(k);
  }

  // Inserts n copies of v (not an element of *this) at index k, 0 < k < size(), shifting the
  // shorter side.
  constexpr void __insert_fill_middle(size_type k, size_type n, const _Tp& __v) {
    __check_grow(n);
    const size_type __s0 = __count();
    if (k >= __s0 - k) {
      __reserve_back(n);
      __ycxx::__detail::__rollback __rb{[this] { __trim_back(); }};
      const size_type __after = __s0 - k;
      if (__after > n) {
        for (size_type i = __s0 - n; i < __s0; ++i)
          __construct_back(static_cast<_Tp&&>(*__elem(i)));
        const iterator b = begin();
        std::move_backward(b + static_cast<difference_type>(k), b + static_cast<difference_type>(__s0 - n),
                           b + static_cast<difference_type>(__s0));
        std::fill_n(b + static_cast<difference_type>(k), n, __v);
      } else {
        for (size_type i = 0; i < n - __after; ++i)
          __construct_back(__v);
        for (size_type i = k; i < __s0; ++i)
          __construct_back(static_cast<_Tp&&>(*__elem(i)));
        const iterator b = begin();
        std::fill(b + static_cast<difference_type>(k), b + static_cast<difference_type>(__s0), __v);
      }
      __rb.release();
    } else {
      __reserve_front(n);
      __ycxx::__detail::__rollback __rb{[this] { __trim_front(); }};
      const iterator __ob = begin(); // the old first element; stays valid while constructing
      const auto __dn = static_cast<difference_type>(n), __dk = static_cast<difference_type>(k);
      if (k >= n) {
        for (difference_type i = __dn; i-- > 0;)
          __construct_front(static_cast<_Tp&&>(__ob[i]));
        std::move(__ob + __dn, __ob + __dk, __ob);
        std::fill(__ob + (__dk - __dn), __ob + __dk, __v);
      } else {
        for (size_type i = 0; i < n - k; ++i)
          __construct_front(__v);
        for (difference_type i = __dk; i-- > 0;)
          __construct_front(static_cast<_Tp&&>(__ob[i]));
        std::fill(__ob, __ob + __dk, __v);
      }
      __rb.release();
    }
  }

public:
  // ---- [deque.cons] ----
  constexpr deque() noexcept(is_nothrow_default_constructible_v<_Allocator>) : deque(_Allocator()) {}
  constexpr explicit deque(const _Allocator& a) noexcept : __alloc_(__ycxx::__detail::__alloc_copy(a)) {}
  constexpr explicit deque(size_type n, const _Allocator& a = _Allocator()) : deque(a) {
    __append_n(n, [this](_Tp* p) { __alloc_traits::construct(__alloc_, p); });
  }
  constexpr deque(size_type n, const _Tp& value, const _Allocator& a = _Allocator()) : deque(a) {
    __append_n(n, [&](_Tp* p) { __alloc_traits::construct(__alloc_, p, value); });
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr deque(_InputIterator first, _InputIterator last, const _Allocator& a = _Allocator()) : deque(a) {
    __append_elems(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr deque(from_range_t, _Rp&& __rg, const _Allocator& a = _Allocator()) : deque(a) {
    append_range(static_cast<_Rp&&>(__rg));
  }
  constexpr deque(const deque& __x) : deque(__alloc_traits::select_on_container_copy_construction(__x.__alloc_)) {
    __append_counted(__x.begin(), __x.__count());
  }
  constexpr deque(deque&& __x) noexcept(is_nothrow_move_constructible_v<_Allocator>) : __alloc_(static_cast<_Allocator&&>(__x.__alloc_)) { take(__x); }
  constexpr deque(const deque& __x, const type_identity_t<_Allocator>& a) : deque(a) { __append_counted(__x.begin(), __x.__count()); }
  // noexcept when the allocators always compare equal (an extension: nothing is allocated).
  constexpr deque(deque&& __x, const type_identity_t<_Allocator>& a) noexcept(__always_equal) : deque(a) {
    if (__always_equal || __alloc_ == __x.__alloc_)
      take(__x);
    else
      __append_counted(std::make_move_iterator(__x.begin()), __x.__count());
  }
  constexpr deque(initializer_list<_Tp> il, const _Allocator& a = _Allocator()) : deque(a) {
    __append_counted(il.begin(), il.size());
  }
  constexpr ~deque() { __release_all(); }

  constexpr deque& operator=(const deque& __x) {
    if (this == __builtin_addressof(__x))
      return *this;
    if constexpr (__pocca) {
      if (!__always_equal && __alloc_ != __x.__alloc_)
        __release_all();
      __alloc_ = __x.__alloc_;
    }
    assign(__x.begin(), __x.end());
    return *this;
  }
  constexpr deque& operator=(deque&& __x) noexcept(__always_equal) {
    if (this == __builtin_addressof(__x))
      return *this;
    if constexpr (__pocma || __always_equal) {
      __release_all();
      if constexpr (__pocma)
        __alloc_ = static_cast<_Allocator&&>(__x.__alloc_);
      take(__x);
    } else {
      if (__alloc_ == __x.__alloc_) {
        __release_all();
        take(__x);
      } else {
        assign(std::make_move_iterator(__x.begin()), std::make_move_iterator(__x.end()));
      }
    }
    return *this;
  }
  constexpr deque& operator=(initializer_list<_Tp> il) {
    assign(il.begin(), il.end());
    return *this;
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr void assign(_InputIterator first, _InputIterator last) {
    __assign_elems(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr void assign_range(_Rp&& __rg) {
    static_assert(assignable_from<_Tp&, ranges::range_reference_t<_Rp>>,
                  "std::deque::assign_range: T must be assignable from the range's reference type");
    __assign_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr void assign(size_type n, const _Tp& t) {
    const size_type m = n < __count() ? n : __count();
    std::fill_n(begin(), m, t);
    if (n > __count())
      __append_n(n - __count(), [&](_Tp* p) { __alloc_traits::construct(__alloc_, p, t); });
    else if (n < __count())
      __erase_back(__count() - n);
  }
  constexpr void assign(initializer_list<_Tp> il) { assign(il.begin(), il.end()); }
  constexpr allocator_type get_allocator() const noexcept { return __alloc_; }

private:
  template <class _It, class _Sent>
  constexpr void __assign_elems(_It first, _Sent last) {
    iterator cur = begin();
    const iterator e = end();
    for (; first != last && cur != e; ++cur, (void)++first)
      *cur = *first;
    if (first == last)
      __erase_back(static_cast<size_type>(e - cur));
    else
      __append_elems(static_cast<_It&&>(first), static_cast<_Sent&&>(last));
  }

public:
  // ---- iterators ----
  constexpr iterator begin() noexcept { return __iter_at(0); }
  constexpr const_iterator begin() const noexcept { return __iter_at(0); }
  constexpr iterator end() noexcept { return __iter_at(__count()); }
  constexpr const_iterator end() const noexcept { return __iter_at(__count()); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [deque.capacity] ----
  [[nodiscard]] constexpr bool empty() const noexcept { return __count() == 0; }
  constexpr size_type size() const noexcept { return __count(); }
  constexpr size_type max_size() const noexcept {
    const size_type a = __alloc_traits::max_size(__alloc_);
    const auto d = static_cast<size_type>(numeric_limits<difference_type>::max());
    return a < d ? a : d;
  }
  constexpr void resize(size_type __sz) {
    if (__sz < __count())
      __erase_back(__count() - __sz);
    else
      __append_n(__sz - __count(), [this](_Tp* p) { __alloc_traits::construct(__alloc_, p); });
  }
  constexpr void resize(size_type __sz, const _Tp& c) {
    if (__sz < __count())
      __erase_back(__count() - __sz);
    else
      __append_n(__sz - __count(), [&](_Tp* p) { __alloc_traits::construct(__alloc_, p, c); });
  }
  constexpr void shrink_to_fit() {
    if (!__map_)
      return;
    if (__count() == 0) {
      __release_all();
      return;
    }
    const size_type __lo = __first_slot(), __y_used = __end_block() - __lo;
    if (__mcap_ <= __y_used + 1)
      return;
    _Tp** m = __alloc_map(__y_used + 1);
    for (size_type i = 0; i < __y_used; ++i)
      m[i] = __map_[__lo + i];
    __free_map(__map_, __mcap_);
    __map_ = m;
    __mcap_ = __y_used + 1;
    __start_ -= __lo * _Bp;
    __end_ -= __lo * _Bp;
  }

  // ---- element access ----
  constexpr reference operator[](size_type n) {
    __ycxx::__detail::__precondition(n < __count(), "std::deque::operator[]: index out of range");
    return *__elem(n);
  }
  constexpr const_reference operator[](size_type n) const {
    __ycxx::__detail::__precondition(n < __count(), "std::deque::operator[]: index out of range");
    return *__elem(n);
  }
  constexpr reference at(size_type n) {
    if (n >= __count())
      __ycxx::__detail::__throw_out_of_range("std::deque::at: index out of range");
    return *__elem(n);
  }
  constexpr const_reference at(size_type n) const {
    if (n >= __count())
      __ycxx::__detail::__throw_out_of_range("std::deque::at: index out of range");
    return *__elem(n);
  }
  constexpr reference front() {
    __ycxx::__detail::__precondition(__count() != 0, "std::deque::front: empty deque");
    return *__head_;
  }
  constexpr const_reference front() const {
    __ycxx::__detail::__precondition(__count() != 0, "std::deque::front: empty deque");
    return *__head_;
  }
  constexpr reference back() {
    __ycxx::__detail::__precondition(__count() != 0, "std::deque::back: empty deque");
    return __tail_[-1];
  }
  constexpr const_reference back() const {
    __ycxx::__detail::__precondition(__count() != 0, "std::deque::back: empty deque");
    return __tail_[-1];
  }

  // ---- [deque.modifiers] ----
  // The ends' fast paths are small enough to inline everywhere: allocating a block is out of
  // line. A deque without storage has start_ == end_ == 0, so the offset tests below send it to
  // the slow path without a test of map_. No max_size() check on the fast path: a
  // free slot in an allocated block means size() + 1 elements fit in memory, so size() + 1 <=
  // allocator max_size(); and size() + 1 <= the number of allocated slots, at most
  // PTRDIFF_MAX for any object representation ([deque.overview] max_size).
  // When the construction cannot throw, the slow path only makes room (without the arguments, so
  // that they need not live in memory) and the construction stays inline.
  template <class... _Args>
  constexpr reference emplace_front(_Args&&... __args) {
    if (__start_ % _Bp == 0) [[unlikely]] { // no room in the first block
      if constexpr (!__nothrow_construct<_Args...>)
        return __emplace_front_slow(static_cast<_Args&&>(__args)...);
      else
        __grow_front();
    }
    _Tp* const p = __head_ - 1;
    __alloc_traits::construct(__alloc_, p, static_cast<_Args&&>(__args)...);
    __head_ = p;
    --__start_;
    return *p;
  }
  template <class... _Args>
  constexpr reference emplace_back(_Args&&... __args) {
    if (__end_ % _Bp == 0) [[unlikely]] { // no room in the last block
      if constexpr (!__nothrow_construct<_Args...>)
        return __emplace_back_slow(static_cast<_Args&&>(__args)...);
      else
        __grow_back();
    }
    _Tp* const p = __tail_;
    __alloc_traits::construct(__alloc_, p, static_cast<_Args&&>(__args)...);
    __tail_ = p + 1;
    ++__end_;
    return *p;
  }

private:
  template <class... _Args>
  static constexpr bool __nothrow_construct = __ycxx::__detail::__alloc_nothrow_construct<_Allocator, _Tp, _Args...>;

  // Makes room for one element before the first one / after the last one (strong), and points
  // head_ / tail_ just past / at that slot, for the construction that follows and cannot throw
  // (the caches are exact again once it is counted). An empty deque's tail_ moves with head_.
  [[__gnu__::__noinline__]] constexpr void __grow_front() {
    __check_grow(1);
    __reserve_front(1);
    __head_ = __ptr_at(__start_ - 1) + 1;
    if (__count() == 0)
      __tail_ = __head_;
  }
  [[__gnu__::__noinline__]] constexpr void __grow_back() {
    __check_grow(1);
    __reserve_back(1);
    __tail_ = __ptr_at(__end_);
  }
  template <class... _Args>
  [[__gnu__::__noinline__]] constexpr reference __emplace_front_slow(_Args&&... __args) {
    __check_grow(1);
    __reserve_front(1);
    __ycxx::__detail::__rollback __rb{[this] { __trim_front(); }};
    __construct_front(static_cast<_Args&&>(__args)...);
    __rb.release();
    return *__elem(0);
  }
  template <class... _Args>
  [[__gnu__::__noinline__]] constexpr reference __emplace_back_slow(_Args&&... __args) {
    __check_grow(1);
    __reserve_back(1);
    __ycxx::__detail::__rollback __rb{[this] { __trim_back(); }};
    __construct_back(static_cast<_Args&&>(__args)...);
    __rb.release();
    return *__elem(__count() - 1);
  }

public:
  template <class... _Args>
  constexpr iterator emplace(const_iterator position, _Args&&... __args) {
    const auto k = static_cast<size_type>(position - cbegin());
    if (k == 0) {
      emplace_front(static_cast<_Args&&>(__args)...);
      return begin();
    }
    if (k == __count()) {
      emplace_back(static_cast<_Args&&>(__args)...);
      return end() - 1;
    }
    // The arguments may refer to elements that are about to move.
    __ycxx::__detail::__alloc_temp<_Tp, _Allocator> t(__alloc_, static_cast<_Args&&>(__args)...);
    const auto __dk = static_cast<difference_type>(k);
    if (k < __count() - k) {
      emplace_front(static_cast<_Tp&&>(*__elem(0)));
      const iterator b = begin();
      std::move(b + 2, b + __dk + 1, b + 1);
    } else {
      emplace_back(static_cast<_Tp&&>(*__elem(__count() - 1)));
      const iterator b = begin();
      std::move_backward(b + __dk, b + static_cast<difference_type>(__count() - 2), b + static_cast<difference_type>(__count() - 1));
    }
    *__elem(k) = static_cast<_Tp&&>(t.__v);
    return begin() + __dk;
  }
  constexpr void push_front(const _Tp& __x) { emplace_front(__x); }
  constexpr void push_front(_Tp&& __x) { emplace_front(static_cast<_Tp&&>(__x)); }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr void prepend_range(_Rp&& __rg) {
    if constexpr (ranges::sized_range<_Rp> && !ranges::forward_range<_Rp>)
      __prepend_counted(ranges::begin(__rg), static_cast<size_type>(ranges::size(__rg)));
    else
      __prepend_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr void push_back(const _Tp& __x) { emplace_back(__x); }
  constexpr void push_back(_Tp&& __x) { emplace_back(static_cast<_Tp&&>(__x)); }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr void append_range(_Rp&& __rg) {
    if constexpr (ranges::sized_range<_Rp> && !ranges::forward_range<_Rp>)
      __append_counted(ranges::begin(__rg), static_cast<size_type>(ranges::size(__rg)));
    else
      __append_elems(ranges::begin(__rg), ranges::end(__rg));
  }

  constexpr iterator insert(const_iterator position, const _Tp& __x) { return emplace(position, __x); }
  constexpr iterator insert(const_iterator position, _Tp&& __x) { return emplace(position, static_cast<_Tp&&>(__x)); }
  constexpr iterator insert(const_iterator position, size_type n, const _Tp& __x) {
    const auto k = static_cast<size_type>(position - cbegin());
    if (n == 0)
      return begin() + static_cast<difference_type>(k);
    if (k == __count()) {
      __append_n(n, [&](_Tp* p) { __alloc_traits::construct(__alloc_, p, __x); });
    } else if (k == 0) {
      __check_grow(n);
      __reserve_front(n);
      size_type __built = 0;
      __ycxx::__detail::__rollback __rb{[&] {
        if (__built != 0)
          __erase_front(__built);
        __trim_front();
      }};
      for (; __built < n; ++__built)
        __construct_front(__x);
      __rb.release();
    } else {
      __ycxx::__detail::__alloc_temp<_Tp, _Allocator> t(__alloc_, __x); // x may be an element
      __insert_fill_middle(k, n, t.__v);
    }
    return begin() + static_cast<difference_type>(k);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr iterator insert(const_iterator position, _InputIterator first, _InputIterator last) {
    return __insert_elems(static_cast<size_type>(position - cbegin()), static_cast<_InputIterator&&>(first),
                        static_cast<_InputIterator&&>(last));
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr iterator insert_range(const_iterator position, _Rp&& __rg) {
    const auto k = static_cast<size_type>(position - cbegin());
    if constexpr (ranges::sized_range<_Rp> && !ranges::forward_range<_Rp>) {
      // A sized single-pass range: insert its elements at the closer end by count.
      const size_type __s0 = __count();
      const auto n = static_cast<size_type>(ranges::size(__rg));
      if (k >= __s0 - k) {
        __append_counted(ranges::begin(__rg), n);
        if (k != __s0)
          __rotate_(begin() + static_cast<difference_type>(k), begin() + static_cast<difference_type>(__s0), end());
      } else {
        __prepend_counted(ranges::begin(__rg), n);
        if (k != 0)
          __rotate_(begin(), begin() + static_cast<difference_type>(n), begin() + static_cast<difference_type>(n + k));
      }
      return begin() + static_cast<difference_type>(k);
    } else {
      return __insert_elems(k, ranges::begin(__rg), ranges::end(__rg));
    }
  }
  constexpr iterator insert(const_iterator position, initializer_list<_Tp> il) {
    return insert(position, il.begin(), il.end());
  }

  // An element that leaves its block non-empty is popped inline (also the last element: the
  // empty deque then keeps its block with start_ inside it); emptying a block is out of line.
  constexpr void pop_front() {
    __ycxx::__detail::__precondition(__count() != 0, "std::deque::pop_front: empty deque");
    if ((__start_ + 1) % _Bp != 0) [[__likely__]] { // the first block keeps its slot start_ + 1
      __alloc_traits::destroy(__alloc_, __head_);
      ++__head_;
      ++__start_;
      return;
    }
    __pop_front_slow();
  }
  constexpr void pop_back() {
    __ycxx::__detail::__precondition(__count() != 0, "std::deque::pop_back: empty deque");
    if ((__end_ - 1) % _Bp != 0) [[__likely__]] { // the last block keeps elements (or start_)
      __alloc_traits::destroy(__alloc_, --__tail_);
      --__end_;
      return;
    }
    __pop_back_slow();
  }

private:
  [[__gnu__::__noinline__]] constexpr void __pop_front_slow() noexcept { __erase_front(1); }
  [[__gnu__::__noinline__]] constexpr void __pop_back_slow() noexcept { __erase_back(1); }

public:
  constexpr iterator erase(const_iterator position) { return erase(position, position + 1); }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    const auto k = static_cast<size_type>(first - cbegin());
    const auto n = static_cast<size_type>(last - first);
    if (n == 0)
      return begin() + static_cast<difference_type>(k);
    const iterator b = begin();
    const auto __dk = static_cast<difference_type>(k), __dn = static_cast<difference_type>(n);
    if (k < __count() - k - n) {
      std::move_backward(b, b + __dk, b + __dk + __dn);
      __erase_front(n);
    } else {
      std::move(b + __dk + __dn, end(), b + __dk);
      __erase_back(n);
    }
    return begin() + __dk;
  }
  constexpr void swap(deque& __x) noexcept(__always_equal) {
    if (this == __builtin_addressof(__x))
      return;
    if constexpr (__pocs)
      ::__ycxx::__detail::__swap_adl::__do_swap(__alloc_, __x.__alloc_);
    else
      __ycxx::__detail::__precondition(__always_equal || __alloc_ == __x.__alloc_,
                                 "std::deque::swap: unequal allocators that do not propagate");
    _Tp** m = __map_;
    const size_type c = __mcap_, s = __start_, n = __end_;
    _Tp* const h = __head_;
    _Tp* const t = __tail_;
    __map_ = __x.__map_;
    __mcap_ = __x.__mcap_;
    __start_ = __x.__start_;
    __end_ = __x.__end_;
    __head_ = __x.__head_;
    __tail_ = __x.__tail_;
    __x.__map_ = m;
    __x.__mcap_ = c;
    __x.__start_ = s;
    __x.__end_ = n;
    __x.__head_ = h;
    __x.__tail_ = t;
  }
  // Destroys every element; keeps one block (the first) and the map.
  constexpr void clear() noexcept {
    if (!__map_)
      return;
    for (size_type i = 0; i < __count(); ++i)
      __alloc_traits::destroy(__alloc_, __elem(i));
    const size_type __lo = __first_slot(), __hi = __end_block();
    for (size_type s = __lo + 1; s < __hi; ++s) {
      __free_block(__map_[s]);
      __map_[s] = nullptr;
    }
    __start_ = __lo * _Bp + _Bp / 2;
    __end_ = __start_;
    __sync_ends();
  }
};

// ---- deduction guides ----
template <class _InputIterator, class _Allocator = allocator<__ycxx::__detail::__iter_value_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
deque(_InputIterator, _InputIterator, _Allocator = _Allocator())
    -> deque<__ycxx::__detail::__iter_value_type<_InputIterator>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator = allocator<ranges::range_value_t<_Rp>>>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
deque(from_range_t, _Rp&&, _Allocator = _Allocator()) -> deque<ranges::range_value_t<_Rp>, _Allocator>;

// ---- comparisons ----
template <class _Tp, class _Allocator>
constexpr bool operator==(const deque<_Tp, _Allocator>& __x, const deque<_Tp, _Allocator>& y) {
  return __x.size() == y.size() && std::equal(__x.begin(), __x.end(), y.begin());
}
template <class _Tp, class _Allocator>
constexpr __ycxx::__detail::__synth_three_way_result<_Tp> operator<=>(const deque<_Tp, _Allocator>& __x,
                                                              const deque<_Tp, _Allocator>& y) {
  return std::lexicographical_compare_three_way(__x.begin(), __x.end(), y.begin(), y.end(),
                                                __ycxx::__detail::__synth_three_way);
}

template <class _Tp, class _Allocator>
constexpr void swap(deque<_Tp, _Allocator>& __x, deque<_Tp, _Allocator>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}

// ---- [deque.erasure] ----
template <class _Tp, class _Allocator, class _Predicate>
constexpr typename deque<_Tp, _Allocator>::size_type erase_if(deque<_Tp, _Allocator>& c, _Predicate pred) {
  auto __it = std::remove_if(c.begin(), c.end(), pred);
  const auto r = static_cast<typename deque<_Tp, _Allocator>::size_type>(c.end() - __it);
  c.erase(__it, c.end());
  return r;
}
template <class _Tp, class _Allocator, class _Up = _Tp>
constexpr typename deque<_Tp, _Allocator>::size_type erase(deque<_Tp, _Allocator>& c, const _Up& value) {
  auto __it = std::remove(c.begin(), c.end(), value);
  const auto r = static_cast<typename deque<_Tp, _Allocator>::size_type>(c.end() - __it);
  c.erase(__it, c.end());
  return r;
}

namespace pmr {
template <class _Tp>
using deque = std::deque<_Tp, polymorphic_allocator<_Tp>>;
} // namespace pmr

} // namespace std
