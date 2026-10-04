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

namespace std {

struct hive_limits {
  size_t min;
  size_t max;
  constexpr hive_limits(size_t minimum, size_t maximum) noexcept : min(minimum), max(maximum) {}
};

template <class T, class Allocator>
class hive;

} // namespace std

namespace ycxx::detail {

using hive_index = std::uint16_t;
inline constexpr hive_index hive_none = 0xffff; // no slot (a free list's end)

// The links of a free run, kept in the run's first slot.
struct hive_run {
  hive_index prev, next;
};

// Storage for one element, or for a hive_run (an implicit-lifetime type: the allocated array
// needs no construction).
template <class T>
struct hive_slot {
  alignas(T) alignas(hive_run) unsigned char bytes[sizeof(T) > sizeof(hive_run) ? sizeof(T) : sizeof(hive_run)];
};
template <class T>
inline T* hive_elem(hive_slot<T>* s) noexcept {
  return std::launder(reinterpret_cast<T*>(s->bytes));
}

template <class T>
struct hive_group {
  hive_slot<T>* slots;
  hive_index* skip; // capacity + 1 entries
  hive_group* next; // active list, or the reserved list
  hive_group* prev;
  hive_group* free_next; // active groups with erased slots
  hive_group* free_prev;
  std::size_t number; // increasing along the active list
  std::size_t capacity;
  std::size_t size; // elements
  std::size_t hw;   // high-water mark
  hive_index free_head; // first free run, or hive_none
};

} // namespace ycxx::detail

namespace ycxx::adl_free {

// T is the element type, possibly const.
template <class T, class Diff>
class hive_iter {
  using V = std::remove_const_t<T>;
  using group = ::ycxx::detail::hive_group<V>;
  group* g_ = nullptr;
  std::size_t i_ = 0;

  template <class, class>
  friend class hive_iter;
  template <class, class>
  friend class std::hive;

  constexpr hive_iter(group* g, std::size_t i) noexcept : g_(g), i_(i) {}

public:
  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = V;
  using difference_type = Diff;
  using pointer = T*;
  using reference = T&;

  constexpr hive_iter() noexcept = default;
  template <class U>
    requires std::is_same_v<const U, T> && (!std::is_same_v<U, T>)
  constexpr hive_iter(const hive_iter<U, Diff>& o) noexcept : g_(o.g_), i_(o.i_) {}

  reference operator*() const noexcept { return *::ycxx::detail::hive_elem(g_->slots + i_); }
  pointer operator->() const noexcept { return ::ycxx::detail::hive_elem(g_->slots + i_); }

  hive_iter& operator++() noexcept {
    ++i_;
    i_ += g_->skip[i_];
    if (i_ == g_->hw && g_->next) {
      g_ = g_->next;
      i_ = g_->skip[0];
    }
    return *this;
  }
  hive_iter operator++(int) noexcept {
    hive_iter t = *this;
    ++*this;
    return t;
  }
  hive_iter& operator--() noexcept {
    for (;;) {
      if (i_ != 0) {
        const std::size_t j = i_ - 1;
        const std::size_t s = g_->skip[j];
        if (s <= j) { // j is an element (s == 0) or ends a run that does not reach slot 0
          i_ = j - s;
          return *this;
        }
      }
      g_ = g_->prev;
      i_ = g_->hw;
    }
  }
  hive_iter operator--(int) noexcept {
    hive_iter t = *this;
    --*this;
    return t;
  }

  friend constexpr bool operator==(const hive_iter& a, const hive_iter& b) noexcept {
    return a.g_ == b.g_ && a.i_ == b.i_;
  }
  friend constexpr std::strong_ordering operator<=>(const hive_iter& a, const hive_iter& b) noexcept {
    if (a.g_ == b.g_)
      return a.i_ <=> b.i_;
    if (!a.g_ || !b.g_) // only singular iterators, or the empty hive's begin() and end()
      return a.g_ ? std::strong_ordering::greater : std::strong_ordering::less;
    return a.g_->number <=> b.g_->number;
  }
};

} // namespace ycxx::adl_free

namespace std {

template <class T, class Allocator = allocator<T>>
class hive;

template <class T, class Allocator>
class hive {
  static_assert(ycxx::detail::allocator_for<Allocator, T>,
                "std::hive: Allocator::value_type must be T ([container.alloc.reqmts])");

  using info = ycxx::detail::alloc_info<Allocator>;
  using alloc_traits = allocator_traits<Allocator>;
  using group = ycxx::detail::hive_group<T>;
  using slot = ycxx::detail::hive_slot<T>;
  using index = ycxx::detail::hive_index;
  using group_alloc = typename info::template rebind<group>;
  using slot_alloc = typename info::template rebind<slot>;
  using skip_alloc = typename info::template rebind<index>;
  using ptr_alloc = typename info::template rebind<T*>;
  using size_alloc = typename info::template rebind<size_t>;
  static constexpr index none = ycxx::detail::hive_none;

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
  using iterator = ycxx::adl_free::hive_iter<T, difference_type>;
  using const_iterator = ycxx::adl_free::hive_iter<const T, difference_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  group* first_ = nullptr;       // first active group
  group* last_ = nullptr;        // last active group
  group* free_groups_ = nullptr; // active groups with erased slots
  group* reserved_ = nullptr;    // reserved groups
  size_type size_ = 0;
  size_type capacity_ = 0; // active and reserved groups
  hive_limits limits_;
  [[no_unique_address]] Allocator alloc_;

  static constexpr size_t hard_min = 2;
  static constexpr size_t hard_max = 0xffff;

  template <class P, class U>
  static P alloc_ptr(U* p) noexcept {
    return ycxx::detail::to_alloc_pointer<P>(p);
  }

  // ---- groups ----
  // Allocates an empty group (not in any list).
  group* make_group(size_t cap) {
    group_alloc ga(alloc_);
    group* g = std::to_address(allocator_traits<group_alloc>::allocate(ga, 1));
    ycxx::detail::rollback rb_g{[&] {
      allocator_traits<group_alloc>::deallocate(ga, alloc_ptr<typename allocator_traits<group_alloc>::pointer>(g), 1);
    }};
    slot_alloc sa(alloc_);
    slot* s = std::to_address(allocator_traits<slot_alloc>::allocate(sa, cap));
    ycxx::detail::rollback rb_s{[&] {
      allocator_traits<slot_alloc>::deallocate(sa, alloc_ptr<typename allocator_traits<slot_alloc>::pointer>(s), cap);
    }};
    skip_alloc ka(alloc_);
    index* k = std::to_address(allocator_traits<skip_alloc>::allocate(ka, cap + 1));
    rb_s.release();
    rb_g.release();
    ::new (static_cast<void*>(g)) group{s, k, nullptr, nullptr, nullptr, nullptr, 0, cap, 0, 0, none};
    capacity_ += static_cast<size_type>(cap);
    return g;
  }
  void free_group(group* g) noexcept {
    capacity_ -= static_cast<size_type>(g->capacity);
    skip_alloc ka(alloc_);
    allocator_traits<skip_alloc>::deallocate(ka, alloc_ptr<typename allocator_traits<skip_alloc>::pointer>(g->skip),
                                             g->capacity + 1);
    slot_alloc sa(alloc_);
    allocator_traits<slot_alloc>::deallocate(sa, alloc_ptr<typename allocator_traits<slot_alloc>::pointer>(g->slots),
                                             g->capacity);
    group_alloc ga(alloc_);
    allocator_traits<group_alloc>::deallocate(ga, alloc_ptr<typename allocator_traits<group_alloc>::pointer>(g), 1);
  }
  // The capacity of a new group: the number of elements (so the total capacity about doubles),
  // within the current limits.
  size_t growth_capacity() const noexcept {
    size_t c = static_cast<size_t>(size_);
    if (c < limits_.min)
      c = limits_.min;
    if (c > limits_.max)
      c = limits_.max;
    return c;
  }
  // A group to store elements in: a reserved one, or a new one. Not yet active.
  group* take_group() {
    if (reserved_) {
      group* g = reserved_;
      reserved_ = g->next;
      return g;
    }
    return make_group(growth_capacity());
  }
  // Undoes take_group for a group that received no element.
  void return_group(group* g) noexcept {
    g->next = reserved_;
    reserved_ = g;
  }
  // Makes g (empty) the last active group.
  void activate(group* g) noexcept {
    for (size_t i = 0; i <= g->capacity; ++i)
      g->skip[i] = 0;
    g->size = 0;
    g->hw = 0;
    g->free_head = none;
    g->free_next = g->free_prev = nullptr;
    g->next = nullptr;
    g->prev = last_;
    g->number = last_ ? last_->number + 1 : 0;
    if (last_)
      last_->next = g;
    else
      first_ = g;
    last_ = g;
  }
  void free_list_add(group* g) noexcept {
    g->free_prev = nullptr;
    g->free_next = free_groups_;
    if (free_groups_)
      free_groups_->free_prev = g;
    free_groups_ = g;
  }
  void free_list_remove(group* g) noexcept {
    if (g->free_prev)
      g->free_prev->free_next = g->free_next;
    else
      free_groups_ = g->free_next;
    if (g->free_next)
      g->free_next->free_prev = g->free_prev;
    g->free_next = g->free_prev = nullptr;
  }
  // Takes an empty group out of the active list; it becomes a reserved group.
  void retire(group* g) noexcept {
    if (g->free_head != none)
      free_list_remove(g);
    if (g->prev)
      g->prev->next = g->next;
    else
      first_ = g->next;
    if (g->next)
      g->next->prev = g->prev;
    else
      last_ = g->prev;
    return_group(g);
  }

  // ---- free runs of a group ----
  using run_links = ycxx::detail::hive_run;
  static run_links* run_at(group* g, size_t i) noexcept {
    return std::launder(reinterpret_cast<run_links*>(g->slots[i].bytes));
  }
  // Begins the lifetime of the links of a run starting at slot i (no element lives there).
  static void set_run(group* g, size_t i, run_links r) noexcept {
    std::construct_at(reinterpret_cast<run_links*>(g->slots[i].bytes), r);
  }
  // The run whose links are r now starts at slot `to`: writes them there and fixes the
  // neighbours' links.
  void relink_run(group* g, run_links r, size_t to) noexcept {
    set_run(g, to, r);
    if (r.prev != none)
      run_at(g, r.prev)->next = static_cast<index>(to);
    else
      g->free_head = static_cast<index>(to);
    if (r.next != none)
      run_at(g, r.next)->prev = static_cast<index>(to);
  }
  // Takes the run whose links are r out of g's free list.
  void unlink_run(group* g, run_links r) noexcept {
    if (r.prev != none)
      run_at(g, r.prev)->next = r.next;
    else
      g->free_head = r.next;
    if (r.next != none)
      run_at(g, r.next)->prev = r.prev;
    if (g->free_head == none)
      free_list_remove(g);
  }
  void push_run(group* g, size_t at) noexcept {
    const index head = g->free_head;
    set_run(g, at, run_links{none, head});
    if (head != none)
      run_at(g, head)->prev = static_cast<index>(at);
    g->free_head = static_cast<index>(at);
    if (head == none)
      free_list_add(g);
  }

  // ---- elements ----
  template <class... Args>
  void construct_at_slot(group* g, size_t i, Args&&... args) {
    alloc_traits::construct(alloc_, reinterpret_cast<T*>(g->slots[i].bytes), static_cast<Args&&>(args)...);
  }
  template <class... Args>
  iterator emplace_impl(Args&&... args) {
    if (group* g = free_groups_) {
      // reuse the first slot of a free run
      const size_t s = g->free_head;
      const size_t len = g->skip[s];
      const run_links links = *run_at(g, s);
      {
        // a failed construction may have overwritten the links
        ycxx::detail::rollback rb{[&] { set_run(g, s, links); }};
        construct_at_slot(g, s, static_cast<Args&&>(args)...);
        rb.release();
      }
      g->skip[s] = 0;
      if (len == 1) {
        unlink_run(g, links);
      } else {
        g->skip[s + 1] = static_cast<index>(len - 1);
        g->skip[s + len - 1] = static_cast<index>(len - 1);
        relink_run(g, links, s + 1);
      }
      ++g->size;
      ++size_;
      return iterator(g, s);
    }
    if (last_ && last_->hw < last_->capacity) {
      group* const g = last_;
      const size_t s = g->hw;
      construct_at_slot(g, s, static_cast<Args&&>(args)...);
      ++g->hw;
      ++g->size;
      ++size_;
      return iterator(g, s);
    }
    if (size_ == max_size())
      ycxx::detail::throw_length_error("std::hive: max_size() exceeded");
    const bool fresh = reserved_ == nullptr;
    group* const g = take_group();
    {
      // no effects: a group allocated for this element is freed again
      ycxx::detail::rollback rb{[&] {
        if (fresh)
          free_group(g);
        else
          return_group(g);
      }};
      construct_at_slot(g, 0, static_cast<Args&&>(args)...);
      rb.release();
    }
    activate(g);
    g->hw = 1;
    g->size = 1;
    ++size_;
    return iterator(g, 0);
  }
  // Erases the element at slot i of g (the group stays active unless it becomes empty).
  void erase_slot(group* g, size_t i) noexcept {
    alloc_traits::destroy(alloc_, ycxx::detail::hive_elem(g->slots + i));
    --size_;
    if (--g->size == 0) {
      retire(g);
      return;
    }
    index* const k = g->skip;
    const bool left = i > 0 && k[i - 1] != 0;
    const bool right = k[i + 1] != 0; // k[hw] is 0
    if (!left && !right) {
      k[i] = 1;
      push_run(g, i);
    } else if (left && !right) {
      const size_t len = size_t{k[i - 1]} + 1;
      k[i + 1 - len] = static_cast<index>(len);
      k[i] = static_cast<index>(len);
    } else if (!left) {
      const size_t len = size_t{k[i + 1]} + 1;
      k[i] = static_cast<index>(len);
      k[i + len - 1] = static_cast<index>(len);
      relink_run(g, *run_at(g, i + 1), i);
    } else {
      const size_t left_len = k[i - 1];
      const size_t right_len = k[i + 1];
      const size_t len = left_len + 1 + right_len;
      unlink_run(g, *run_at(g, i + 1));
      k[i - left_len] = static_cast<index>(len);
      k[i + right_len] = static_cast<index>(len);
      k[i] = 1;
    }
  }
  // Destroys every element; the active groups become reserved.
  void clear_impl() noexcept {
    group* g = first_;
    while (g) {
      group* const next = g->next;
      for (size_t i = g->skip[0]; i < g->hw;) {
        alloc_traits::destroy(alloc_, ycxx::detail::hive_elem(g->slots + i));
        ++i;
        i += g->skip[i];
      }
      return_group(g);
      g = next;
    }
    first_ = last_ = free_groups_ = nullptr;
    size_ = 0;
  }
  // Destroys every element and frees every group.
  void free_all() noexcept {
    clear_impl();
    trim_capacity();
  }
  void steal(hive& x) noexcept {
    first_ = x.first_;
    last_ = x.last_;
    free_groups_ = x.free_groups_;
    reserved_ = x.reserved_;
    size_ = x.size_;
    capacity_ = x.capacity_;
    limits_ = x.limits_;
    x.first_ = x.last_ = x.free_groups_ = x.reserved_ = nullptr;
    x.size_ = 0;
    x.capacity_ = 0;
  }
  template <class It, class Sent>
  void insert_elems(It first, Sent last) {
    for (; first != last; ++first)
      emplace_impl(*first);
  }
  // Builds the contents in a constructor: on an exception everything is released.
  template <class F>
  void build(F f) {
    ycxx::detail::rollback rb{[this] { free_all(); }};
    f();
    rb.release();
  }
  // [hive.overview]/5.4: limits outside the hard limits, or min > max, are erroneous; the
  // implementation-defined effect is that they are diagnosed (hardened) and clamped into the
  // hard limits.
  static constexpr hive_limits checked_limits(hive_limits lim) noexcept {
    ycxx::detail::precondition(is_within_hard_limits(lim),
                               "std::hive: block capacity limits outside the hard limits (erroneous)");
    if (lim.min < hard_min)
      lim.min = hard_min;
    if (lim.min > hard_max)
      lim.min = hard_max;
    if (lim.max < lim.min)
      lim.max = lim.min;
    if (lim.max > hard_max)
      lim.max = hard_max;
    return lim;
  }
  static bool within(const group* g, hive_limits lim) noexcept {
    return g->capacity >= lim.min && g->capacity <= lim.max;
  }

public:
  // ---- [hive.cons] ----
  constexpr hive() noexcept(noexcept(Allocator())) : hive(Allocator()) {}
  constexpr explicit hive(const Allocator& a) noexcept : limits_(block_capacity_default_limits()), alloc_(a) {}
  constexpr explicit hive(hive_limits block_limits) : hive(block_limits, Allocator()) {}
  constexpr hive(hive_limits block_limits, const Allocator& a) : limits_(checked_limits(block_limits)), alloc_(a) {}
  explicit hive(size_type n, const Allocator& a = Allocator()) : hive(a) {
    build([&] {
      for (size_type i = 0; i < n; ++i)
        emplace_impl();
    });
  }
  hive(size_type n, hive_limits block_limits, const Allocator& a = Allocator()) : hive(block_limits, a) {
    build([&] {
      for (size_type i = 0; i < n; ++i)
        emplace_impl();
    });
  }
  hive(size_type n, const T& value, const Allocator& a = Allocator()) : hive(a) {
    build([&] { insert(n, value); });
  }
  hive(size_type n, const T& value, hive_limits block_limits, const Allocator& a = Allocator())
      : hive(block_limits, a) {
    build([&] { insert(n, value); });
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  hive(InputIterator first, InputIterator last, const Allocator& a = Allocator()) : hive(a) {
    build([&] { insert_elems(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last)); });
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  hive(InputIterator first, InputIterator last, hive_limits block_limits, const Allocator& a = Allocator())
      : hive(block_limits, a) {
    build([&] { insert_elems(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last)); });
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<T> R>
  hive(Tag, R&& rg, const Allocator& a = Allocator()) : hive(a) {
    build([&] { insert_elems(ranges::begin(rg), ranges::end(rg)); });
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<T> R>
  hive(Tag, R&& rg, hive_limits block_limits, const Allocator& a = Allocator()) : hive(block_limits, a) {
    build([&] { insert_elems(ranges::begin(rg), ranges::end(rg)); });
  }
  hive(const hive& x) : hive(x, alloc_traits::select_on_container_copy_construction(x.alloc_)) {}
  hive(hive&& x) noexcept : limits_(x.limits_), alloc_(static_cast<Allocator&&>(x.alloc_)) { steal(x); }
  hive(const hive& x, const type_identity_t<Allocator>& a) : limits_(x.limits_), alloc_(a) {
    build([&] {
      reserve(x.size_);
      insert_elems(x.begin(), x.end());
    });
  }
  // noexcept when the allocators always compare equal (an extension: nothing is allocated).
  hive(hive&& x, const type_identity_t<Allocator>& a) noexcept(info::always_equal) : limits_(x.limits_), alloc_(a) {
    if (info::always_equal || alloc_ == x.alloc_) {
      steal(x);
    } else {
      build([&] {
        reserve(x.size_);
        insert_elems(std::make_move_iterator(x.begin()), std::make_move_iterator(x.end()));
      });
      x.clear();
    }
  }
  hive(initializer_list<T> il, const Allocator& a = Allocator()) : hive(a) {
    build([&] { insert_elems(il.begin(), il.end()); });
  }
  hive(initializer_list<T> il, hive_limits block_limits, const Allocator& a = Allocator()) : hive(block_limits, a) {
    build([&] { insert_elems(il.begin(), il.end()); });
  }
  ~hive() { free_all(); }

  hive& operator=(const hive& x) {
    if (this == __builtin_addressof(x))
      return *this;
    if constexpr (info::pocca) {
      if (!info::always_equal && alloc_ != x.alloc_)
        free_all();
      alloc_ = x.alloc_;
    }
    clear_impl();
    insert_elems(x.begin(), x.end());
    return *this;
  }
  hive& operator=(hive&& x) noexcept(info::pocma || info::always_equal) {
    if (this == __builtin_addressof(x))
      return *this;
    if (info::pocma || info::always_equal || alloc_ == x.alloc_) {
      free_all();
      if constexpr (info::pocma)
        alloc_ = static_cast<Allocator&&>(x.alloc_);
      steal(x);
    } else {
      clear_impl();
      insert_elems(std::make_move_iterator(x.begin()), std::make_move_iterator(x.end()));
      x.clear();
    }
    return *this;
  }
  hive& operator=(initializer_list<T> il) {
    clear_impl();
    insert_elems(il.begin(), il.end());
    return *this;
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  void assign(InputIterator first, InputIterator last) {
    clear_impl();
    insert_elems(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
  }
  template <ycxx::detail::container_compatible_range<T> R>
  void assign_range(R&& rg) {
    clear_impl();
    insert_elems(ranges::begin(rg), ranges::end(rg));
  }
  void assign(size_type n, const T& t) {
    // t may be an element of *this: copy it before clearing.
    ycxx::detail::alloc_temp<T, Allocator> tmp(alloc_, t);
    clear_impl();
    insert(n, tmp.v);
  }
  void assign(initializer_list<T> il) {
    clear_impl();
    insert_elems(il.begin(), il.end());
  }
  allocator_type get_allocator() const noexcept { return alloc_; }

  // ---- iterators ----
  iterator begin() noexcept { return first_ ? iterator(first_, first_->skip[0]) : iterator(); }
  const_iterator begin() const noexcept { return first_ ? const_iterator(first_, first_->skip[0]) : const_iterator(); }
  iterator end() noexcept { return last_ ? iterator(last_, last_->hw) : iterator(); }
  const_iterator end() const noexcept { return last_ ? const_iterator(last_, last_->hw) : const_iterator(); }
  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }
  const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [hive.capacity] ----
  [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
  size_type size() const noexcept { return size_; }
  size_type max_size() const noexcept {
    const auto a = static_cast<size_type>(alloc_traits::max_size(alloc_));
    const auto d = static_cast<size_type>(numeric_limits<difference_type>::max());
    return a < d ? a : d;
  }
  size_type capacity() const noexcept { return capacity_; }
  void reserve(size_type n) {
    if (n <= capacity_)
      return;
    if (n > max_size())
      ycxx::detail::throw_length_error("std::hive::reserve: n > max_size()");
    while (capacity_ < n) {
      size_t c = static_cast<size_t>(n - capacity_);
      if (c < limits_.min)
        c = limits_.min;
      if (c > limits_.max)
        c = limits_.max;
      return_group(make_group(c));
    }
  }
  void shrink_to_fit() { trim_capacity(); }
  void trim_capacity() noexcept {
    while (reserved_) {
      group* const g = reserved_;
      reserved_ = g->next;
      free_group(g);
    }
  }
  void trim_capacity(size_type n) noexcept {
    group** link = &reserved_;
    while (*link && capacity_ > n) {
      group* const g = *link;
      if (capacity_ - g->capacity >= n) {
        *link = g->next;
        free_group(g);
      } else {
        link = &g->next;
      }
    }
  }
  constexpr hive_limits block_capacity_limits() const noexcept { return limits_; }
  static constexpr hive_limits block_capacity_default_limits() noexcept {
    // Blocks of at most about 1 MiB, and at most 8192 elements.
    size_t mx = (size_t(1) << 20) / sizeof(slot);
    if (mx > 8192)
      mx = 8192;
    if (mx < 64)
      mx = 64;
    return hive_limits(8, mx);
  }
  static constexpr hive_limits block_capacity_hard_limits() noexcept { return hive_limits(hard_min, hard_max); }
  static constexpr bool is_within_hard_limits(hive_limits lim) noexcept {
    const hive_limits hl = block_capacity_hard_limits();
    return hl.min <= lim.min && lim.min <= lim.max && lim.max <= hl.max;
  }
  void reshape(hive_limits block_limits) {
    block_limits = checked_limits(block_limits);
    // reserved groups outside the new limits go
    for (group** link = &reserved_; *link;) {
      group* const g = *link;
      if (!within(g, block_limits)) {
        *link = g->next;
        free_group(g);
      } else {
        link = &g->next;
      }
    }
    bool fits = true;
    for (group* g = first_; g; g = g->next)
      fits = fits && within(g, block_limits);
    if (!fits) {
      // Reallocates every element into groups within the new limits, keeping their order.
      hive tmp(block_limits, alloc_);
      tmp.reserve(size_);
      for (iterator i = begin(), e = end(); i != e; ++i)
        tmp.emplace_impl(static_cast<T&&>(*i));
      free_all();
      steal(tmp);
    }
    limits_ = block_limits;
  }

  // ---- [hive.modifiers] ----
  template <class... Args>
  iterator emplace(Args&&... args) {
    return emplace_impl(static_cast<Args&&>(args)...);
  }
  template <class... Args>
  iterator emplace_hint(const_iterator, Args&&... args) {
    return emplace_impl(static_cast<Args&&>(args)...);
  }
  iterator insert(const T& x) { return emplace_impl(x); }
  iterator insert(T&& x) { return emplace_impl(static_cast<T&&>(x)); }
  iterator insert(const_iterator, const T& x) { return emplace_impl(x); }
  iterator insert(const_iterator, T&& x) { return emplace_impl(static_cast<T&&>(x)); }
  void insert(initializer_list<T> il) { insert_elems(il.begin(), il.end()); }
  template <ycxx::detail::container_compatible_range<T> R>
  void insert_range(R&& rg) {
    insert_elems(ranges::begin(rg), ranges::end(rg));
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  void insert(InputIterator first, InputIterator last) {
    insert_elems(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
  }
  void insert(size_type n, const T& x) {
    for (; n > 0; --n)
      emplace_impl(x);
  }
  iterator erase(const_iterator position) {
    ycxx::detail::precondition(position != cend(), "std::hive::erase: end() iterator");
    group* const g = position.g_;
    const_iterator next = position;
    ++next;
    erase_slot(g, position.i_);
    if (next.g_ == g && g->size == 0) // g was the last group and is gone
      return end();
    return iterator(next.g_, next.i_);
  }
  iterator erase(const_iterator first, const_iterator last) {
    while (first != last) {
      group* const g = first.g_;
      const_iterator next = first;
      ++next;
      erase_slot(g, first.i_);
      if (next.g_ == g && g->size == 0) // g was the last group: last is end()
        return end();
      first = next;
    }
    return iterator(last.g_, last.i_);
  }
  void swap(hive& x) noexcept(info::pocs || info::always_equal) {
    if (this == __builtin_addressof(x))
      return;
    if constexpr (info::pocs)
      ycxx::detail::swap_adl::do_swap(alloc_, x.alloc_);
    else
      ycxx::detail::precondition(info::always_equal || alloc_ == x.alloc_,
                                 "std::hive::swap: unequal allocators that do not propagate");
    auto exchange = [](auto& a, auto& b) noexcept {
      auto t = a;
      a = b;
      b = t;
    };
    exchange(first_, x.first_);
    exchange(last_, x.last_);
    exchange(free_groups_, x.free_groups_);
    exchange(reserved_, x.reserved_);
    exchange(size_, x.size_);
    exchange(capacity_, x.capacity_);
    exchange(limits_, x.limits_);
  }
  void clear() noexcept { clear_impl(); }

  // ---- [hive.operations] ----
  void splice(hive& x) {
    ycxx::detail::precondition(this != __builtin_addressof(x), "std::hive::splice: x is *this (erroneous)");
    if (this == __builtin_addressof(x))
      return;
    ycxx::detail::precondition(info::always_equal || alloc_ == x.alloc_, "std::hive::splice: unequal allocators");
    size_t moved_capacity = 0;
    for (group* g = x.first_; g; g = g->next) {
      if (!within(g, limits_))
        ycxx::detail::throw_length_error("std::hive::splice: a block of x is outside the current limits");
      moved_capacity += g->capacity;
    }
    if (!x.first_)
      return;
    size_t number = last_ ? last_->number + 1 : 0;
    for (group* g = x.first_; g; g = g->next)
      g->number = number++;
    if (last_) {
      last_->next = x.first_;
      x.first_->prev = last_;
    } else {
      first_ = x.first_;
    }
    last_ = x.last_;
    while (group* g = x.free_groups_) {
      x.free_list_remove(g);
      free_list_add(g);
    }
    size_ += x.size_;
    capacity_ += static_cast<size_type>(moved_capacity);
    x.capacity_ -= static_cast<size_type>(moved_capacity);
    x.first_ = x.last_ = nullptr;
    x.size_ = 0;
  }
  void splice(hive&& x) { splice(x); }
  template <class BinaryPredicate = equal_to<T>>
  size_type unique(BinaryPredicate binary_pred = BinaryPredicate()) {
    if (size_ < 2)
      return 0;
    size_type erased = 0;
    const_iterator prev = cbegin();
    const_iterator i = prev;
    ++i;
    while (i != cend()) {
      if (binary_pred(*i, *prev)) {
        i = erase(i);
        ++erased;
      } else {
        prev = i;
        ++i;
      }
    }
    return erased;
  }
  template <class Compare = less<T>>
  void sort(Compare comp = Compare()) {
    const size_t n = static_cast<size_t>(size_);
    if (n < 2)
      return;
    ptr_alloc pa(alloc_);
    size_alloc ia(alloc_);
    T** const at = std::to_address(allocator_traits<ptr_alloc>::allocate(pa, n));
    ycxx::detail::rollback rb_at{[&] {
      allocator_traits<ptr_alloc>::deallocate(pa, alloc_ptr<typename allocator_traits<ptr_alloc>::pointer>(at), n);
    }};
    size_t* const perm = std::to_address(allocator_traits<size_alloc>::allocate(ia, n));
    ycxx::detail::rollback rb_perm{[&] {
      allocator_traits<size_alloc>::deallocate(ia, alloc_ptr<typename allocator_traits<size_alloc>::pointer>(perm), n);
    }};
    size_t k = 0;
    for (iterator i = begin(), e = end(); i != e; ++i, ++k) {
      at[k] = __builtin_addressof(*i);
      perm[k] = k;
    }
    // perm[k]: the position whose element goes to position k
    std::sort(perm, perm + n, [&](size_t a, size_t b) -> bool { return static_cast<bool>(comp(*at[a], *at[b])); });
    for (size_t start = 0; start < n; ++start) {
      if (perm[start] == start)
        continue;
      ycxx::detail::alloc_temp<T, Allocator> tmp(alloc_, static_cast<T&&>(*at[start]));
      size_t j = start;
      for (;;) {
        const size_t src = perm[j];
        perm[j] = j;
        if (src == start) {
          *at[j] = static_cast<T&&>(tmp.v);
          break;
        }
        *at[j] = static_cast<T&&>(*at[src]);
        j = src;
      }
    }
  }
  iterator get_iterator(const_pointer p) noexcept {
    const_iterator i = static_cast<const hive&>(*this).get_iterator(p);
    return iterator(i.g_, i.i_);
  }
  const_iterator get_iterator(const_pointer p) const noexcept {
    const void* const a = static_cast<const void*>(std::to_address(p));
    for (group* g = first_; g; g = g->next) {
      const void* const lo = static_cast<const void*>(g->slots);
      const void* const hi = static_cast<const void*>(g->slots + g->capacity);
      if (!less<const void*>()(a, lo) && less<const void*>()(a, hi))
        return const_iterator(g, static_cast<size_t>(static_cast<const unsigned char*>(a) -
                                                     static_cast<const unsigned char*>(lo)) /
                                     sizeof(slot));
    }
    ycxx::detail::precondition(false, "std::hive::get_iterator: p does not point to an element");
    return cend();
  }
};

// ---- deduction guides ----
template <class InputIterator, class Allocator = allocator<ycxx::detail::iter_value_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
hive(InputIterator, InputIterator, Allocator = Allocator()) -> hive<ycxx::detail::iter_value_type<InputIterator>, Allocator>;
template <class InputIterator, class Allocator = allocator<ycxx::detail::iter_value_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
hive(InputIterator, InputIterator, hive_limits, Allocator = Allocator())
    -> hive<ycxx::detail::iter_value_type<InputIterator>, Allocator>;
template <ranges::input_range R, class Allocator = allocator<ranges::range_value_t<R>>>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
hive(from_range_t, R&&, Allocator = Allocator()) -> hive<ranges::range_value_t<R>, Allocator>;
template <ranges::input_range R, class Allocator = allocator<ranges::range_value_t<R>>>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
hive(from_range_t, R&&, hive_limits, Allocator = Allocator()) -> hive<ranges::range_value_t<R>, Allocator>;

template <class T, class Allocator>
void swap(hive<T, Allocator>& x, hive<T, Allocator>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

// ---- [hive.erasure] ----
template <class T, class Allocator, class Predicate>
typename hive<T, Allocator>::size_type erase_if(hive<T, Allocator>& c, Predicate pred) {
  const auto original_size = c.size();
  for (auto i = c.begin(); i != c.end();) {
    if (pred(*i))
      i = c.erase(i);
    else
      ++i;
  }
  return original_size - c.size();
}
template <class T, class Allocator, class U = T>
typename hive<T, Allocator>::size_type erase(hive<T, Allocator>& c, const U& value) {
  return std::erase_if(c, [&value](const auto& elem) -> bool { return elem == value; });
}

namespace pmr {
template <class T>
using hive = std::hive<T, polymorphic_allocator<T>>;
} // namespace pmr

} // namespace std
