// libycxx core: hive ([hive]), hive_limits, erasure and the pmr:: alias.
//
// Representation: element blocks ("groups"). A group owns an array of slots (raw storage for
// a T, or for the links of a free run), a skipfield of capacity + 1 16-bit entries, and its place in
// two lists: the active groups in iteration order, and the active groups that have erased
// slots. Reserved (empty) groups sit in a third, singly linked list. Slots [0, hw) of a group
// have been used (hw, the high-water mark, only grows while the group is active); the
// elements live there, and erased slots form runs.
// Skipfield (a jump-counting skipfield): skip[i] is 0 for an element and for an unused slot;
// for a run of erased slots the first and last entries hold the run's length (interior
// entries are nonzero but otherwise unspecified), so iteration jumps over a run in one step
// in either direction, and erasing or reusing a slot updates O(1) entries. skip[capacity]
// stays 0, a sentinel. Each run is in its group's doubly linked free list, whose links live
// in the run's first slot (16-bit slot indices). Insertion reuses the first slot of a free
// run when there is one, else appends at the last group's high-water mark, else activates a
// reserved group or allocates one of capacity clamp(size(), limits) (so total capacity about
// doubles). A group that becomes empty becomes a reserved group (capacity() is unchanged
// until trim_capacity()). Iterators are (group, slot index); groups carry increasing
// numbers in iteration order, so iterator comparison is O(1). end() is the last group's
// high-water mark. sort sorts an index array and permutes the elements along its cycles
// with one temporary element (through the allocator). Block capacities are limited to
// [2, 65535] (the hard limits) by the 16-bit skipfield.
#pragma once

#include <initializer_list>
#include <ycxx/core/algo_sort.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/memory_resource_fwd.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/utility_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

struct hive_limits {
  size_t min;
  size_t max;
  constexpr hive_limits(size_t __minimum, size_t __maximum) noexcept : min(__minimum), max(__maximum) {}
};

template <class _Tp, class _Allocator>
class hive;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

using __hive_index = std::uint16_t;
inline constexpr __hive_index __hive_none = 0xffff; // no slot (a free list's end)

// The links of a free run, kept in the run's first slot.
struct __hive_run {
  __hive_index prev, next;
};

// Storage for one element, or for a hive_run (an implicit-lifetime type: the allocated array
// needs no construction).
template <class _Tp>
struct __hive_slot {
  alignas(_Tp) alignas(__hive_run) unsigned char __bytes[sizeof(_Tp) > sizeof(__hive_run) ? sizeof(_Tp) : sizeof(__hive_run)];
};
template <class _Tp>
inline _Tp* __hive_elem(__hive_slot<_Tp>* s) noexcept {
  return std::launder(reinterpret_cast<_Tp*>(s->__bytes));
}

template <class _Tp>
struct __hive_group {
  __hive_slot<_Tp>* __slots;
  __hive_index* __skip; // capacity + 1 entries
  __hive_group* next; // active list, or the reserved list
  __hive_group* prev;
  __hive_group* __free_next; // active groups with erased slots
  __hive_group* __free_prev;
  std::size_t __number; // increasing along the active list
  std::size_t capacity;
  std::size_t size; // elements
  std::size_t __hw;   // high-water mark
  __hive_index __free_head; // first free run, or hive_none
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// T is the element type, possibly const.
template <class _Tp, class _Diff>
class __hive_iter {
  using _Vp = std::remove_const_t<_Tp>;
  using __group = ::__ycxx::__detail::__hive_group<_Vp>;
  __group* __g_ = nullptr;
  std::size_t __i_ = 0;

  template <class, class>
  friend class __hive_iter;
  template <class, class>
  friend class std::hive;

  constexpr __hive_iter(__group* __g, std::size_t i) noexcept : __g_(__g), __i_(i) {}

public:
  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = _Vp;
  using difference_type = _Diff;
  using pointer = _Tp*;
  using reference = _Tp&;

  constexpr __hive_iter() noexcept = default;
  template <class _Up>
    requires std::is_same_v<const _Up, _Tp> && (!std::is_same_v<_Up, _Tp>)
  constexpr __hive_iter(const __hive_iter<_Up, _Diff>& __o) noexcept : __g_(__o.__g_), __i_(__o.__i_) {}

  reference operator*() const noexcept { return *::__ycxx::__detail::__hive_elem(__g_->__slots + __i_); }
  pointer operator->() const noexcept { return ::__ycxx::__detail::__hive_elem(__g_->__slots + __i_); }

  __hive_iter& operator++() noexcept {
    ++__i_;
    __i_ += __g_->__skip[__i_];
    if (__i_ == __g_->__hw && __g_->next) {
      __g_ = __g_->next;
      __i_ = __g_->__skip[0];
    }
    return *this;
  }
  __hive_iter operator++(int) noexcept {
    __hive_iter t = *this;
    ++*this;
    return t;
  }
  __hive_iter& operator--() noexcept {
    for (;;) {
      if (__i_ != 0) {
        const std::size_t __j = __i_ - 1;
        const std::size_t s = __g_->__skip[__j];
        if (s <= __j) { // j is an element (s == 0) or ends a run that does not reach slot 0
          __i_ = __j - s;
          return *this;
        }
      }
      __g_ = __g_->prev;
      __i_ = __g_->__hw;
    }
  }
  __hive_iter operator--(int) noexcept {
    __hive_iter t = *this;
    --*this;
    return t;
  }

  friend constexpr bool operator==(const __hive_iter& a, const __hive_iter& b) noexcept {
    return a.__g_ == b.__g_ && a.__i_ == b.__i_;
  }
  friend constexpr std::strong_ordering operator<=>(const __hive_iter& a, const __hive_iter& b) noexcept {
    if (a.__g_ == b.__g_)
      return a.__i_ <=> b.__i_;
    if (!a.__g_ || !b.__g_) // only singular iterators, or the empty hive's begin() and end()
      return a.__g_ ? std::strong_ordering::greater : std::strong_ordering::less;
    return a.__g_->__number <=> b.__g_->__number;
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp, class _Allocator = allocator<_Tp>>
class hive;

template <class _Tp, class _Allocator>
class hive {
  static_assert(__ycxx::__detail::__allocator_for<_Allocator, _Tp>,
                "std::hive: Allocator::value_type must be T ([container.alloc.reqmts])");

  using info = __ycxx::__detail::__alloc_info<_Allocator>;
  using __alloc_traits = allocator_traits<_Allocator>;
  using __group = __ycxx::__detail::__hive_group<_Tp>;
  using __slot = __ycxx::__detail::__hive_slot<_Tp>;
  using index = __ycxx::__detail::__hive_index;
  using __group_alloc = typename info::template rebind<__group>;
  using __slot_alloc = typename info::template rebind<__slot>;
  using __skip_alloc = typename info::template rebind<index>;
  using __ptr_alloc = typename info::template rebind<_Tp*>;
  using __size_alloc = typename info::template rebind<size_t>;
  static constexpr index none = __ycxx::__detail::__hive_none;

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
  using iterator = __ycxx::__adl_free::__hive_iter<_Tp, difference_type>;
  using const_iterator = __ycxx::__adl_free::__hive_iter<const _Tp, difference_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  __group* __first_ = nullptr;       // first active group
  __group* __last_ = nullptr;        // last active group
  __group* __free_groups_ = nullptr; // active groups with erased slots
  __group* __reserved_ = nullptr;    // reserved groups
  size_type __size_ = 0;
  size_type __capacity_ = 0; // active and reserved groups
  hive_limits __limits_;
  [[no_unique_address]] _Allocator __alloc_;

  static constexpr size_t __hard_min = 2;
  static constexpr size_t __hard_max = 0xffff;

  template <class _Pp, class _Up>
  static _Pp __alloc_ptr(_Up* p) noexcept {
    return __ycxx::__detail::__to_alloc_pointer<_Pp>(p);
  }

  // ---- groups ----
  // Allocates an empty group (not in any list).
  __group* __make_group(size_t __cap) {
    __group_alloc __ga(__alloc_);
    __group* __g = std::to_address(allocator_traits<__group_alloc>::allocate(__ga, 1));
    __ycxx::__detail::__rollback __rb_g{[&] {
      allocator_traits<__group_alloc>::deallocate(__ga, __alloc_ptr<typename allocator_traits<__group_alloc>::pointer>(__g), 1);
    }};
    __slot_alloc __sa(__alloc_);
    __slot* s = std::to_address(allocator_traits<__slot_alloc>::allocate(__sa, __cap));
    __ycxx::__detail::__rollback __rb_s{[&] {
      allocator_traits<__slot_alloc>::deallocate(__sa, __alloc_ptr<typename allocator_traits<__slot_alloc>::pointer>(s), __cap);
    }};
    __skip_alloc __ka(__alloc_);
    index* k = std::to_address(allocator_traits<__skip_alloc>::allocate(__ka, __cap + 1));
    __rb_s.release();
    __rb_g.release();
    ::new (static_cast<void*>(__g)) __group{s, k, nullptr, nullptr, nullptr, nullptr, 0, __cap, 0, 0, none};
    __capacity_ += static_cast<size_type>(__cap);
    return __g;
  }
  void __free_group(__group* __g) noexcept {
    __capacity_ -= static_cast<size_type>(__g->capacity);
    __skip_alloc __ka(__alloc_);
    allocator_traits<__skip_alloc>::deallocate(__ka, __alloc_ptr<typename allocator_traits<__skip_alloc>::pointer>(__g->__skip),
                                             __g->capacity + 1);
    __slot_alloc __sa(__alloc_);
    allocator_traits<__slot_alloc>::deallocate(__sa, __alloc_ptr<typename allocator_traits<__slot_alloc>::pointer>(__g->__slots),
                                             __g->capacity);
    __group_alloc __ga(__alloc_);
    allocator_traits<__group_alloc>::deallocate(__ga, __alloc_ptr<typename allocator_traits<__group_alloc>::pointer>(__g), 1);
  }
  // The capacity of a new group: the number of elements (so the total capacity about doubles),
  // within the current limits.
  size_t __growth_capacity() const noexcept {
    size_t c = static_cast<size_t>(__size_);
    if (c < __limits_.min)
      c = __limits_.min;
    if (c > __limits_.max)
      c = __limits_.max;
    return c;
  }
  // A group to store elements in: a reserved one, or a new one. Not yet active.
  __group* __take_group() {
    if (__reserved_) {
      __group* __g = __reserved_;
      __reserved_ = __g->next;
      return __g;
    }
    return __make_group(__growth_capacity());
  }
  // Undoes take_group for a group that received no element.
  void __return_group(__group* __g) noexcept {
    __g->next = __reserved_;
    __reserved_ = __g;
  }
  // Makes g (empty) the last active group.
  void __activate(__group* __g) noexcept {
    for (size_t i = 0; i <= __g->capacity; ++i)
      __g->__skip[i] = 0;
    __g->size = 0;
    __g->__hw = 0;
    __g->__free_head = none;
    __g->__free_next = __g->__free_prev = nullptr;
    __g->next = nullptr;
    __g->prev = __last_;
    __g->__number = __last_ ? __last_->__number + 1 : 0;
    if (__last_)
      __last_->next = __g;
    else
      __first_ = __g;
    __last_ = __g;
  }
  void __free_list_add(__group* __g) noexcept {
    __g->__free_prev = nullptr;
    __g->__free_next = __free_groups_;
    if (__free_groups_)
      __free_groups_->__free_prev = __g;
    __free_groups_ = __g;
  }
  void __free_list_remove(__group* __g) noexcept {
    if (__g->__free_prev)
      __g->__free_prev->__free_next = __g->__free_next;
    else
      __free_groups_ = __g->__free_next;
    if (__g->__free_next)
      __g->__free_next->__free_prev = __g->__free_prev;
    __g->__free_next = __g->__free_prev = nullptr;
  }
  // Takes an empty group out of the active list; it becomes a reserved group.
  void retire(__group* __g) noexcept {
    if (__g->__free_head != none)
      __free_list_remove(__g);
    if (__g->prev)
      __g->prev->next = __g->next;
    else
      __first_ = __g->next;
    if (__g->next)
      __g->next->prev = __g->prev;
    else
      __last_ = __g->prev;
    __return_group(__g);
  }

  // ---- free runs of a group ----
  using __run_links = __ycxx::__detail::__hive_run;
  static __run_links* __run_at(__group* __g, size_t i) noexcept {
    return std::launder(reinterpret_cast<__run_links*>(__g->__slots[i].__bytes));
  }
  // Begins the lifetime of the links of a run starting at slot i (no element lives there).
  static void __set_run(__group* __g, size_t i, __run_links r) noexcept {
    std::construct_at(reinterpret_cast<__run_links*>(__g->__slots[i].__bytes), r);
  }
  // The run whose links are r now starts at slot `to`: writes them there and fixes the
  // neighbours' links.
  void __relink_run(__group* __g, __run_links r, size_t to) noexcept {
    __set_run(__g, to, r);
    if (r.prev != none)
      __run_at(__g, r.prev)->next = static_cast<index>(to);
    else
      __g->__free_head = static_cast<index>(to);
    if (r.next != none)
      __run_at(__g, r.next)->prev = static_cast<index>(to);
  }
  // Takes the run whose links are r out of g's free list.
  void __unlink_run(__group* __g, __run_links r) noexcept {
    if (r.prev != none)
      __run_at(__g, r.prev)->next = r.next;
    else
      __g->__free_head = r.next;
    if (r.next != none)
      __run_at(__g, r.next)->prev = r.prev;
    if (__g->__free_head == none)
      __free_list_remove(__g);
  }
  void __push_run(__group* __g, size_t at) noexcept {
    const index __head = __g->__free_head;
    __set_run(__g, at, __run_links{none, __head});
    if (__head != none)
      __run_at(__g, __head)->prev = static_cast<index>(at);
    __g->__free_head = static_cast<index>(at);
    if (__head == none)
      __free_list_add(__g);
  }

  // ---- elements ----
  template <class... _Args>
  void __construct_at_slot(__group* __g, size_t i, _Args&&... __args) {
    __alloc_traits::construct(__alloc_, reinterpret_cast<_Tp*>(__g->__slots[i].__bytes), static_cast<_Args&&>(__args)...);
  }
  template <class... _Args>
  iterator __emplace_impl(_Args&&... __args) {
    if (__group* __g = __free_groups_) {
      // reuse the first slot of a free run
      const size_t s = __g->__free_head;
      const size_t __len = __g->__skip[s];
      const __run_links links = *__run_at(__g, s);
      {
        // a failed construction may have overwritten the links
        __ycxx::__detail::__rollback __rb{[&] { __set_run(__g, s, links); }};
        __construct_at_slot(__g, s, static_cast<_Args&&>(__args)...);
        __rb.release();
      }
      __g->__skip[s] = 0;
      if (__len == 1) {
        __unlink_run(__g, links);
      } else {
        __g->__skip[s + 1] = static_cast<index>(__len - 1);
        __g->__skip[s + __len - 1] = static_cast<index>(__len - 1);
        __relink_run(__g, links, s + 1);
      }
      ++__g->size;
      ++__size_;
      return iterator(__g, s);
    }
    if (__last_ && __last_->__hw < __last_->capacity) {
      __group* const __g = __last_;
      const size_t s = __g->__hw;
      __construct_at_slot(__g, s, static_cast<_Args&&>(__args)...);
      ++__g->__hw;
      ++__g->size;
      ++__size_;
      return iterator(__g, s);
    }
    if (__size_ == max_size())
      __ycxx::__detail::__throw_length_error("std::hive: max_size() exceeded");
    const bool __fresh = __reserved_ == nullptr;
    __group* const __g = __take_group();
    {
      // no effects: a group allocated for this element is freed again
      __ycxx::__detail::__rollback __rb{[&] {
        if (__fresh)
          __free_group(__g);
        else
          __return_group(__g);
      }};
      __construct_at_slot(__g, 0, static_cast<_Args&&>(__args)...);
      __rb.release();
    }
    __activate(__g);
    __g->__hw = 1;
    __g->size = 1;
    ++__size_;
    return iterator(__g, 0);
  }
  // Erases the element at slot i of g (the group stays active unless it becomes empty).
  void __erase_slot(__group* __g, size_t i) noexcept {
    __alloc_traits::destroy(__alloc_, __ycxx::__detail::__hive_elem(__g->__slots + i));
    --__size_;
    if (--__g->size == 0) {
      retire(__g);
      return;
    }
    index* const k = __g->__skip;
    const bool left = i > 0 && k[i - 1] != 0;
    const bool right = k[i + 1] != 0; // k[hw] is 0
    if (!left && !right) {
      k[i] = 1;
      __push_run(__g, i);
    } else if (left && !right) {
      const size_t __len = size_t{k[i - 1]} + 1;
      k[i + 1 - __len] = static_cast<index>(__len);
      k[i] = static_cast<index>(__len);
    } else if (!left) {
      const size_t __len = size_t{k[i + 1]} + 1;
      k[i] = static_cast<index>(__len);
      k[i + __len - 1] = static_cast<index>(__len);
      __relink_run(__g, *__run_at(__g, i + 1), i);
    } else {
      const size_t __left_len = k[i - 1];
      const size_t __right_len = k[i + 1];
      const size_t __len = __left_len + 1 + __right_len;
      __unlink_run(__g, *__run_at(__g, i + 1));
      k[i - __left_len] = static_cast<index>(__len);
      k[i + __right_len] = static_cast<index>(__len);
      k[i] = 1;
    }
  }
  // Destroys every element; the active groups become reserved.
  void __clear_impl() noexcept {
    __group* __g = __first_;
    while (__g) {
      __group* const next = __g->next;
      for (size_t i = __g->__skip[0]; i < __g->__hw;) {
        __alloc_traits::destroy(__alloc_, __ycxx::__detail::__hive_elem(__g->__slots + i));
        ++i;
        i += __g->__skip[i];
      }
      __return_group(__g);
      __g = next;
    }
    __first_ = __last_ = __free_groups_ = nullptr;
    __size_ = 0;
  }
  // Destroys every element and frees every group.
  void __free_all() noexcept {
    __clear_impl();
    trim_capacity();
  }
  void __steal(hive& __x) noexcept {
    __first_ = __x.__first_;
    __last_ = __x.__last_;
    __free_groups_ = __x.__free_groups_;
    __reserved_ = __x.__reserved_;
    __size_ = __x.__size_;
    __capacity_ = __x.__capacity_;
    __limits_ = __x.__limits_;
    __x.__first_ = __x.__last_ = __x.__free_groups_ = __x.__reserved_ = nullptr;
    __x.__size_ = 0;
    __x.__capacity_ = 0;
  }
  template <class _It, class _Sent>
  void __insert_elems(_It first, _Sent last) {
    for (; first != last; ++first)
      __emplace_impl(*first);
  }
  // Builds the contents in a constructor: on an exception everything is released.
  template <class _Fp>
  void __build(_Fp __f) {
    __ycxx::__detail::__rollback __rb{[this] { __free_all(); }};
    __f();
    __rb.release();
  }
  // [hive.overview]/5.4: limits outside the hard limits, or min > max, are erroneous; the
  // implementation-defined effect is that they are diagnosed (hardened) and clamped into the
  // hard limits.
  static constexpr hive_limits __checked_limits(hive_limits __lim) noexcept {
    __ycxx::__detail::__precondition(is_within_hard_limits(__lim),
                               "std::hive: block capacity limits outside the hard limits (erroneous)");
    if (__lim.min < __hard_min)
      __lim.min = __hard_min;
    if (__lim.min > __hard_max)
      __lim.min = __hard_max;
    if (__lim.max < __lim.min)
      __lim.max = __lim.min;
    if (__lim.max > __hard_max)
      __lim.max = __hard_max;
    return __lim;
  }
  static bool __within(const __group* __g, hive_limits __lim) noexcept {
    return __g->capacity >= __lim.min && __g->capacity <= __lim.max;
  }

public:
  // ---- [hive.cons] ----
  constexpr hive() noexcept(noexcept(_Allocator())) : hive(_Allocator()) {}
  constexpr explicit hive(const _Allocator& a) noexcept : __limits_(block_capacity_default_limits()), __alloc_(a) {}
  constexpr explicit hive(hive_limits __block_limits) : hive(__block_limits, _Allocator()) {}
  constexpr hive(hive_limits __block_limits, const _Allocator& a) : __limits_(__checked_limits(__block_limits)), __alloc_(a) {}
  explicit hive(size_type n, const _Allocator& a = _Allocator()) : hive(a) {
    __build([&] {
      for (size_type i = 0; i < n; ++i)
        __emplace_impl();
    });
  }
  hive(size_type n, hive_limits __block_limits, const _Allocator& a = _Allocator()) : hive(__block_limits, a) {
    __build([&] {
      for (size_type i = 0; i < n; ++i)
        __emplace_impl();
    });
  }
  hive(size_type n, const _Tp& value, const _Allocator& a = _Allocator()) : hive(a) {
    __build([&] { insert(n, value); });
  }
  hive(size_type n, const _Tp& value, hive_limits __block_limits, const _Allocator& a = _Allocator())
      : hive(__block_limits, a) {
    __build([&] { insert(n, value); });
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  hive(_InputIterator first, _InputIterator last, const _Allocator& a = _Allocator()) : hive(a) {
    __build([&] { __insert_elems(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last)); });
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  hive(_InputIterator first, _InputIterator last, hive_limits __block_limits, const _Allocator& a = _Allocator())
      : hive(__block_limits, a) {
    __build([&] { __insert_elems(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last)); });
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  hive(_Tag, _Rp&& __rg, const _Allocator& a = _Allocator()) : hive(a) {
    __build([&] { __insert_elems(ranges::begin(__rg), ranges::end(__rg)); });
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  hive(_Tag, _Rp&& __rg, hive_limits __block_limits, const _Allocator& a = _Allocator()) : hive(__block_limits, a) {
    __build([&] { __insert_elems(ranges::begin(__rg), ranges::end(__rg)); });
  }
  hive(const hive& __x) : hive(__x, __alloc_traits::select_on_container_copy_construction(__x.__alloc_)) {}
  hive(hive&& __x) noexcept : __limits_(__x.__limits_), __alloc_(static_cast<_Allocator&&>(__x.__alloc_)) { __steal(__x); }
  hive(const hive& __x, const type_identity_t<_Allocator>& a) : __limits_(__x.__limits_), __alloc_(a) {
    __build([&] {
      reserve(__x.__size_);
      __insert_elems(__x.begin(), __x.end());
    });
  }
  // noexcept when the allocators always compare equal (an extension: nothing is allocated).
  hive(hive&& __x, const type_identity_t<_Allocator>& a) noexcept(info::__always_equal) : __limits_(__x.__limits_), __alloc_(a) {
    if (info::__always_equal || __alloc_ == __x.__alloc_) {
      __steal(__x);
    } else {
      __build([&] {
        reserve(__x.__size_);
        __insert_elems(std::make_move_iterator(__x.begin()), std::make_move_iterator(__x.end()));
      });
      __x.clear();
    }
  }
  hive(initializer_list<_Tp> il, const _Allocator& a = _Allocator()) : hive(a) {
    __build([&] { __insert_elems(il.begin(), il.end()); });
  }
  hive(initializer_list<_Tp> il, hive_limits __block_limits, const _Allocator& a = _Allocator()) : hive(__block_limits, a) {
    __build([&] { __insert_elems(il.begin(), il.end()); });
  }
  ~hive() { __free_all(); }

  hive& operator=(const hive& __x) {
    if (this == __builtin_addressof(__x))
      return *this;
    if constexpr (info::__pocca) {
      if (!info::__always_equal && __alloc_ != __x.__alloc_)
        __free_all();
      __alloc_ = __x.__alloc_;
    }
    __clear_impl();
    __insert_elems(__x.begin(), __x.end());
    return *this;
  }
  hive& operator=(hive&& __x) noexcept(info::__pocma || info::__always_equal) {
    if (this == __builtin_addressof(__x))
      return *this;
    if (info::__pocma || info::__always_equal || __alloc_ == __x.__alloc_) {
      __free_all();
      if constexpr (info::__pocma)
        __alloc_ = static_cast<_Allocator&&>(__x.__alloc_);
      __steal(__x);
    } else {
      __clear_impl();
      __insert_elems(std::make_move_iterator(__x.begin()), std::make_move_iterator(__x.end()));
      __x.clear();
    }
    return *this;
  }
  hive& operator=(initializer_list<_Tp> il) {
    __clear_impl();
    __insert_elems(il.begin(), il.end());
    return *this;
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  void assign(_InputIterator first, _InputIterator last) {
    __clear_impl();
    __insert_elems(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  void assign_range(_Rp&& __rg) {
    __clear_impl();
    __insert_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  void assign(size_type n, const _Tp& t) {
    // t may be an element of *this: copy it before clearing.
    __ycxx::__detail::__alloc_temp<_Tp, _Allocator> __tmp(__alloc_, t);
    __clear_impl();
    insert(n, __tmp.__v);
  }
  void assign(initializer_list<_Tp> il) {
    __clear_impl();
    __insert_elems(il.begin(), il.end());
  }
  allocator_type get_allocator() const noexcept { return __alloc_; }

  // ---- iterators ----
  iterator begin() noexcept { return __first_ ? iterator(__first_, __first_->__skip[0]) : iterator(); }
  const_iterator begin() const noexcept { return __first_ ? const_iterator(__first_, __first_->__skip[0]) : const_iterator(); }
  iterator end() noexcept { return __last_ ? iterator(__last_, __last_->__hw) : iterator(); }
  const_iterator end() const noexcept { return __last_ ? const_iterator(__last_, __last_->__hw) : const_iterator(); }
  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }
  const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [hive.capacity] ----
  [[nodiscard]] bool empty() const noexcept { return __size_ == 0; }
  size_type size() const noexcept { return __size_; }
  size_type max_size() const noexcept {
    const auto a = static_cast<size_type>(__alloc_traits::max_size(__alloc_));
    const auto d = static_cast<size_type>(numeric_limits<difference_type>::max());
    return a < d ? a : d;
  }
  size_type capacity() const noexcept { return __capacity_; }
  void reserve(size_type n) {
    if (n <= __capacity_)
      return;
    if (n > max_size())
      __ycxx::__detail::__throw_length_error("std::hive::reserve: n > max_size()");
    while (__capacity_ < n) {
      size_t c = static_cast<size_t>(n - __capacity_);
      if (c < __limits_.min)
        c = __limits_.min;
      if (c > __limits_.max)
        c = __limits_.max;
      __return_group(__make_group(c));
    }
  }
  void shrink_to_fit() { trim_capacity(); }
  void trim_capacity() noexcept {
    while (__reserved_) {
      __group* const __g = __reserved_;
      __reserved_ = __g->next;
      __free_group(__g);
    }
  }
  void trim_capacity(size_type n) noexcept {
    __group** __link = &__reserved_;
    while (*__link && __capacity_ > n) {
      __group* const __g = *__link;
      if (__capacity_ - __g->capacity >= n) {
        *__link = __g->next;
        __free_group(__g);
      } else {
        __link = &__g->next;
      }
    }
  }
  constexpr hive_limits block_capacity_limits() const noexcept { return __limits_; }
  static constexpr hive_limits block_capacity_default_limits() noexcept {
    // Blocks of at most about 1 MiB, and at most 8192 elements.
    size_t __mx = (size_t(1) << 20) / sizeof(__slot);
    if (__mx > 8192)
      __mx = 8192;
    if (__mx < 64)
      __mx = 64;
    return hive_limits(8, __mx);
  }
  static constexpr hive_limits block_capacity_hard_limits() noexcept { return hive_limits(__hard_min, __hard_max); }
  static constexpr bool is_within_hard_limits(hive_limits __lim) noexcept {
    const hive_limits __hl = block_capacity_hard_limits();
    return __hl.min <= __lim.min && __lim.min <= __lim.max && __lim.max <= __hl.max;
  }
  void reshape(hive_limits __block_limits) {
    __block_limits = __checked_limits(__block_limits);
    // reserved groups outside the new limits go
    for (__group** __link = &__reserved_; *__link;) {
      __group* const __g = *__link;
      if (!__within(__g, __block_limits)) {
        *__link = __g->next;
        __free_group(__g);
      } else {
        __link = &__g->next;
      }
    }
    bool __fits = true;
    for (__group* __g = __first_; __g; __g = __g->next)
      __fits = __fits && __within(__g, __block_limits);
    if (!__fits) {
      // Reallocates every element into groups within the new limits, keeping their order.
      hive __tmp(__block_limits, __alloc_);
      __tmp.reserve(__size_);
      for (iterator i = begin(), e = end(); i != e; ++i)
        __tmp.__emplace_impl(static_cast<_Tp&&>(*i));
      __free_all();
      __steal(__tmp);
    }
    __limits_ = __block_limits;
  }

  // ---- [hive.modifiers] ----
  template <class... _Args>
  iterator emplace(_Args&&... __args) {
    return __emplace_impl(static_cast<_Args&&>(__args)...);
  }
  template <class... _Args>
  iterator emplace_hint(const_iterator, _Args&&... __args) {
    return __emplace_impl(static_cast<_Args&&>(__args)...);
  }
  iterator insert(const _Tp& __x) { return __emplace_impl(__x); }
  iterator insert(_Tp&& __x) { return __emplace_impl(static_cast<_Tp&&>(__x)); }
  iterator insert(const_iterator, const _Tp& __x) { return __emplace_impl(__x); }
  iterator insert(const_iterator, _Tp&& __x) { return __emplace_impl(static_cast<_Tp&&>(__x)); }
  void insert(initializer_list<_Tp> il) { __insert_elems(il.begin(), il.end()); }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  void insert_range(_Rp&& __rg) {
    __insert_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  void insert(_InputIterator first, _InputIterator last) {
    __insert_elems(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
  }
  void insert(size_type n, const _Tp& __x) {
    for (; n > 0; --n)
      __emplace_impl(__x);
  }
  iterator erase(const_iterator position) {
    __ycxx::__detail::__precondition(position != cend(), "std::hive::erase: end() iterator");
    __group* const __g = position.__g_;
    const_iterator next = position;
    ++next;
    __erase_slot(__g, position.__i_);
    if (next.__g_ == __g && __g->size == 0) // g was the last group and is gone
      return end();
    return iterator(next.__g_, next.__i_);
  }
  iterator erase(const_iterator first, const_iterator last) {
    while (first != last) {
      __group* const __g = first.__g_;
      const_iterator next = first;
      ++next;
      __erase_slot(__g, first.__i_);
      if (next.__g_ == __g && __g->size == 0) // g was the last group: last is end()
        return end();
      first = next;
    }
    return iterator(last.__g_, last.__i_);
  }
  void swap(hive& __x) noexcept(info::__pocs || info::__always_equal) {
    if (this == __builtin_addressof(__x))
      return;
    if constexpr (info::__pocs)
      __ycxx::__detail::__swap_adl::__do_swap(__alloc_, __x.__alloc_);
    else
      __ycxx::__detail::__precondition(info::__always_equal || __alloc_ == __x.__alloc_,
                                 "std::hive::swap: unequal allocators that do not propagate");
    auto exchange = [](auto& a, auto& b) noexcept {
      auto t = a;
      a = b;
      b = t;
    };
    exchange(__first_, __x.__first_);
    exchange(__last_, __x.__last_);
    exchange(__free_groups_, __x.__free_groups_);
    exchange(__reserved_, __x.__reserved_);
    exchange(__size_, __x.__size_);
    exchange(__capacity_, __x.__capacity_);
    exchange(__limits_, __x.__limits_);
  }
  void clear() noexcept { __clear_impl(); }

  // ---- [hive.operations] ----
  void splice(hive& __x) {
    __ycxx::__detail::__precondition(this != __builtin_addressof(__x), "std::hive::splice: x is *this (erroneous)");
    if (this == __builtin_addressof(__x))
      return;
    __ycxx::__detail::__precondition(info::__always_equal || __alloc_ == __x.__alloc_, "std::hive::splice: unequal allocators");
    size_t __moved_capacity = 0;
    for (__group* __g = __x.__first_; __g; __g = __g->next) {
      if (!__within(__g, __limits_))
        __ycxx::__detail::__throw_length_error("std::hive::splice: a block of x is outside the current limits");
      __moved_capacity += __g->capacity;
    }
    if (!__x.__first_)
      return;
    size_t __number = __last_ ? __last_->__number + 1 : 0;
    for (__group* __g = __x.__first_; __g; __g = __g->next)
      __g->__number = __number++;
    if (__last_) {
      __last_->next = __x.__first_;
      __x.__first_->prev = __last_;
    } else {
      __first_ = __x.__first_;
    }
    __last_ = __x.__last_;
    while (__group* __g = __x.__free_groups_) {
      __x.__free_list_remove(__g);
      __free_list_add(__g);
    }
    __size_ += __x.__size_;
    __capacity_ += static_cast<size_type>(__moved_capacity);
    __x.__capacity_ -= static_cast<size_type>(__moved_capacity);
    __x.__first_ = __x.__last_ = nullptr;
    __x.__size_ = 0;
  }
  void splice(hive&& __x) { splice(__x); }
  template <class _BinaryPredicate = equal_to<_Tp>>
  size_type unique(_BinaryPredicate __binary_pred = _BinaryPredicate()) {
    if (__size_ < 2)
      return 0;
    size_type __erased = 0;
    const_iterator prev = cbegin();
    const_iterator i = prev;
    ++i;
    while (i != cend()) {
      if (__binary_pred(*i, *prev)) {
        i = erase(i);
        ++__erased;
      } else {
        prev = i;
        ++i;
      }
    }
    return __erased;
  }
  template <class _Compare = less<_Tp>>
  void sort(_Compare comp = _Compare()) {
    const size_t n = static_cast<size_t>(__size_);
    if (n < 2)
      return;
    __ptr_alloc __pa(__alloc_);
    __size_alloc __ia(__alloc_);
    _Tp** const at = std::to_address(allocator_traits<__ptr_alloc>::allocate(__pa, n));
    __ycxx::__detail::__rollback __rb_at{[&] {
      allocator_traits<__ptr_alloc>::deallocate(__pa, __alloc_ptr<typename allocator_traits<__ptr_alloc>::pointer>(at), n);
    }};
    size_t* const __perm = std::to_address(allocator_traits<__size_alloc>::allocate(__ia, n));
    __ycxx::__detail::__rollback __rb_perm{[&] {
      allocator_traits<__size_alloc>::deallocate(__ia, __alloc_ptr<typename allocator_traits<__size_alloc>::pointer>(__perm), n);
    }};
    size_t k = 0;
    for (iterator i = begin(), e = end(); i != e; ++i, ++k) {
      at[k] = __builtin_addressof(*i);
      __perm[k] = k;
    }
    // perm[k]: the position whose element goes to position k
    std::sort(__perm, __perm + n, [&](size_t a, size_t b) -> bool { return static_cast<bool>(comp(*at[a], *at[b])); });
    for (size_t start = 0; start < n; ++start) {
      if (__perm[start] == start)
        continue;
      __ycxx::__detail::__alloc_temp<_Tp, _Allocator> __tmp(__alloc_, static_cast<_Tp&&>(*at[start]));
      size_t __j = start;
      for (;;) {
        const size_t __src = __perm[__j];
        __perm[__j] = __j;
        if (__src == start) {
          *at[__j] = static_cast<_Tp&&>(__tmp.__v);
          break;
        }
        *at[__j] = static_cast<_Tp&&>(*at[__src]);
        __j = __src;
      }
    }
  }
  iterator get_iterator(const_pointer p) noexcept {
    const_iterator i = static_cast<const hive&>(*this).get_iterator(p);
    return iterator(i.__g_, i.__i_);
  }
  const_iterator get_iterator(const_pointer p) const noexcept {
    const void* const a = static_cast<const void*>(std::to_address(p));
    for (__group* __g = __first_; __g; __g = __g->next) {
      const void* const __lo = static_cast<const void*>(__g->__slots);
      const void* const __hi = static_cast<const void*>(__g->__slots + __g->capacity);
      if (!less<const void*>()(a, __lo) && less<const void*>()(a, __hi))
        return const_iterator(__g, static_cast<size_t>(static_cast<const unsigned char*>(a) -
                                                     static_cast<const unsigned char*>(__lo)) /
                                     sizeof(__slot));
    }
    __ycxx::__detail::__precondition(false, "std::hive::get_iterator: p does not point to an element");
    return cend();
  }
};

// ---- deduction guides ----
template <class _InputIterator, class _Allocator = allocator<__ycxx::__detail::__iter_value_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
hive(_InputIterator, _InputIterator, _Allocator = _Allocator()) -> hive<__ycxx::__detail::__iter_value_type<_InputIterator>, _Allocator>;
template <class _InputIterator, class _Allocator = allocator<__ycxx::__detail::__iter_value_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
hive(_InputIterator, _InputIterator, hive_limits, _Allocator = _Allocator())
    -> hive<__ycxx::__detail::__iter_value_type<_InputIterator>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator = allocator<ranges::range_value_t<_Rp>>>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
hive(from_range_t, _Rp&&, _Allocator = _Allocator()) -> hive<ranges::range_value_t<_Rp>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator = allocator<ranges::range_value_t<_Rp>>>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
hive(from_range_t, _Rp&&, hive_limits, _Allocator = _Allocator()) -> hive<ranges::range_value_t<_Rp>, _Allocator>;

template <class _Tp, class _Allocator>
void swap(hive<_Tp, _Allocator>& __x, hive<_Tp, _Allocator>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}

// ---- [hive.erasure] ----
template <class _Tp, class _Allocator, class _Predicate>
typename hive<_Tp, _Allocator>::size_type erase_if(hive<_Tp, _Allocator>& c, _Predicate pred) {
  const auto __original_size = c.size();
  for (auto i = c.begin(); i != c.end();) {
    if (pred(*i))
      i = c.erase(i);
    else
      ++i;
  }
  return __original_size - c.size();
}
template <class _Tp, class _Allocator, class _Up = _Tp>
typename hive<_Tp, _Allocator>::size_type erase(hive<_Tp, _Allocator>& c, const _Up& value) {
  return std::erase_if(c, [&value](const auto& __elem) -> bool { return __elem == value; });
}

namespace pmr {
template <class _Tp>
using hive = std::hive<_Tp, polymorphic_allocator<_Tp>>;
} // namespace pmr

} // namespace std
