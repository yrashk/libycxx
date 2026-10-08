// libycxx core: forward_list ([forward.list]), its comparisons, erasure and the pmr:: alias.
//
// Representation: a null-terminated singly linked list whose head node ({next}) is a member;
// before_begin() is the head and end() is null. Nodes never point back at the head, so moving a
// forward_list only moves the head's pointer. The node allocator and the node layout follow
// list.hpp: elements are constructed through the allocator rebound to the node type, multi-
// element insertions build a detached chain first (no effects on an exception), and sort is the
// allocation-free merge sort of sequence_support.hpp. T may be incomplete until a member is used.
#pragma once

#include <initializer_list>
#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/memory_resource_fwd.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/utility_base.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
template <class _Tp, class _Allocator>
class forward_list;
}}

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

struct __fwd_node_base {
  __fwd_node_base* next;
};

template <class _Tp>
struct __fwd_node : __fwd_node_base {
  union {
    _Tp value;
  };
  constexpr __fwd_node() noexcept : __fwd_node_base{nullptr} {}
  __fwd_node(const __fwd_node&) = delete;
  constexpr ~__fwd_node() {}
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {

// T is the element type, possibly const.
template <class _Tp, class _Diff>
class __fwd_list_iter {
  using _Vp = std::remove_const_t<_Tp>;
  using base = ::__ycxx::__detail::__fwd_node_base;
  base* __n_ = nullptr;

  template <class, class>
  friend class __fwd_list_iter;
  template <class, class>
  friend class std::forward_list;

  constexpr explicit __fwd_list_iter(base* n) noexcept : __n_(n) {}

public:
  using iterator_concept = std::forward_iterator_tag;
  using iterator_category = std::forward_iterator_tag;
  using value_type = _Vp;
  using difference_type = _Diff;
  using pointer = _Tp*;
  using reference = _Tp&;

  constexpr __fwd_list_iter() noexcept = default;
  template <class _Up>
    requires std::is_same_v<const _Up, _Tp> && (!std::is_same_v<_Up, _Tp>)
  constexpr __fwd_list_iter(const __fwd_list_iter<_Up, _Diff>& __o) noexcept : __n_(__o.__n_) {}

  constexpr reference operator*() const noexcept { return static_cast<::__ycxx::__detail::__fwd_node<_Vp>*>(__n_)->value; }
  constexpr pointer operator->() const noexcept {
    return __builtin_addressof(static_cast<::__ycxx::__detail::__fwd_node<_Vp>*>(__n_)->value);
  }
  constexpr __fwd_list_iter& operator++() noexcept {
    __n_ = __n_->next;
    return *this;
  }
  constexpr __fwd_list_iter operator++(int) noexcept {
    __fwd_list_iter t = *this;
    __n_ = __n_->next;
    return t;
  }
  friend constexpr bool operator==(const __fwd_list_iter& a, const __fwd_list_iter& b) noexcept { return a.__n_ == b.__n_; }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp, class _Allocator = allocator<_Tp>>
class forward_list;

template <class _Tp, class _Allocator>
class forward_list {
  static_assert(__ycxx::__detail::__allocator_for<_Allocator, _Tp>,
                "std::forward_list: Allocator::value_type must be T ([container.alloc.reqmts])");

  using info = __ycxx::__detail::__alloc_info<_Allocator>;
  using __alloc_traits = allocator_traits<_Allocator>;
  using node = __ycxx::__detail::__fwd_node<_Tp>;
  using __node_base = __ycxx::__detail::__fwd_node_base;
  using __node_alloc = typename info::template rebind<node>;
  using __node_traits = allocator_traits<__node_alloc>;

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
  using iterator = __ycxx::__adl_free::__fwd_list_iter<_Tp, difference_type>;
  using const_iterator = __ycxx::__adl_free::__fwd_list_iter<const _Tp, difference_type>;

private:
  static constexpr bool __pocca = info::__pocca;
  static constexpr bool __pocma = info::__pocma;
  static constexpr bool __pocs = info::__pocs;
  static constexpr bool __always_equal = info::__always_equal;

  __node_base __head_ = {nullptr};
  [[no_unique_address]] __node_alloc __na_;

  // ---- nodes ----
  constexpr __node_base* __head() const noexcept { return const_cast<__node_base*>(__builtin_addressof(__head_)); }
  static constexpr _Tp& value(__node_base* n) noexcept { return static_cast<node*>(n)->value; }

  template <class... _Args>
  constexpr node* __make_node(_Args&&... __args) {
    node* n = std::to_address(__node_traits::allocate(__na_, 1));
    std::construct_at(n);
    __ycxx::__detail::__rollback __rb{[&] {
      std::destroy_at(n);
      __node_traits::deallocate(__na_, __ycxx::__detail::__to_alloc_pointer<typename __node_traits::pointer>(n), 1);
    }};
    __node_traits::construct(__na_, __builtin_addressof(n->value), static_cast<_Args&&>(__args)...);
    __rb.release();
    return n;
  }
  constexpr void __free_node(__node_base* b) noexcept {
    node* n = static_cast<node*>(b);
    __node_traits::destroy(__na_, __builtin_addressof(n->value));
    std::destroy_at(n);
    __node_traits::deallocate(__na_, __ycxx::__detail::__to_alloc_pointer<typename __node_traits::pointer>(n), 1);
  }
  // Frees a null-terminated chain.
  constexpr void __free_chain(__node_base* p) noexcept {
    while (p) {
      __node_base* __nx = p->next;
      __free_node(p);
      p = __nx;
    }
  }

  // A detached chain of nodes, null-terminated, freed unless linked in.
  struct __chain {
    forward_list* __owner;
    __node_base* first = nullptr;
    __node_base* last = nullptr;
    constexpr explicit __chain(forward_list* __o) noexcept : __owner(__o) {}
    __chain(const __chain&) = delete;
    constexpr ~__chain() { __owner->__free_chain(first); }
    constexpr void push(__node_base* __x) noexcept {
      if (first)
        last->next = __x;
      else
        first = __x;
      last = __x;
    }
  };
  // Links c after pos; returns the last linked node (pos if c is empty).
  static constexpr __node_base* __splice_chain(__node_base* __pos, __chain& c) noexcept {
    if (!c.first)
      return __pos;
    c.last->next = __pos->next;
    __pos->next = c.first;
    __node_base* const __l = c.last;
    c.first = c.last = nullptr;
    return __l;
  }
  template <class _It, class _Sent>
  constexpr __node_base* __insert_elems(__node_base* __pos, _It first, _Sent last) {
    __chain c(this);
    for (; first != last; ++first)
      c.push(__make_node(*first));
    return __splice_chain(__pos, c);
  }
  template <class... _Args>
  constexpr __node_base* __insert_n(__node_base* __pos, size_type n, const _Args&... __args) {
    __chain c(this);
    for (size_type i = 0; i < n; ++i)
      c.push(__make_node(__args...));
    return __splice_chain(__pos, c);
  }
  // Frees the nodes after pos; returns pos.
  constexpr void __erase_tail(__node_base* __pos) noexcept {
    __node_base* p = __pos->next;
    __pos->next = nullptr;
    __free_chain(p);
  }
  constexpr void __check_alloc(const forward_list& __x) const noexcept {
    if constexpr (!__always_equal)
      __ycxx::__detail::__precondition(__na_ == __x.__na_,
                                 "std::forward_list: splice or merge between lists with unequal allocators");
  }
  template <class _It, class _Sent>
  constexpr void __assign_elems(_It first, _Sent last) {
    __node_base* prev = __head();
    for (; first != last && prev->next; ++first, (void)(prev = prev->next))
      value(prev->next) = *first;
    if (first == last)
      __erase_tail(prev);
    else
      __insert_elems(prev, static_cast<_It&&>(first), static_cast<_Sent&&>(last));
  }
  // Destroys the nodes of a chain at scope exit.
  struct __graveyard {
    forward_list* __owner;
    __node_base* __head = nullptr;
    constexpr ~__graveyard() { __owner->__free_chain(__head); }
    constexpr void __bury(__node_base* n) noexcept {
      n->next = __head;
      __head = n;
    }
  };

public:
  // ---- [forward.list.cons] ----
  constexpr forward_list() noexcept(is_nothrow_default_constructible_v<_Allocator>) : forward_list(_Allocator()) {}
  constexpr explicit forward_list(const _Allocator& a) noexcept : __na_(a) {}
  constexpr explicit forward_list(size_type n, const _Allocator& a = _Allocator()) : forward_list(a) {
    __insert_n(__head(), n);
  }
  constexpr forward_list(size_type n, const _Tp& value, const _Allocator& a = _Allocator()) : forward_list(a) {
    __insert_n(__head(), n, value);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr forward_list(_InputIterator first, _InputIterator last, const _Allocator& a = _Allocator())
      : forward_list(a) {
    __insert_elems(__head(), static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr forward_list(from_range_t, _Rp&& __rg, const _Allocator& a = _Allocator()) : forward_list(a) {
    __insert_elems(__head(), ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr forward_list(const forward_list& __x)
      : forward_list(__alloc_traits::select_on_container_copy_construction(_Allocator(__x.__na_))) {
    __insert_elems(__head(), __x.begin(), __x.end());
  }
  constexpr forward_list(forward_list&& __x) noexcept(is_nothrow_move_constructible_v<__node_alloc>) : __na_(static_cast<__node_alloc&&>(__x.__na_)) {
    __head_.next = __x.__head_.next;
    __x.__head_.next = nullptr;
  }
  constexpr forward_list(const forward_list& __x, const type_identity_t<_Allocator>& a) : forward_list(a) {
    __insert_elems(__head(), __x.begin(), __x.end());
  }
  // noexcept when the allocators always compare equal (an extension: nothing is allocated).
  constexpr forward_list(forward_list&& __x, const type_identity_t<_Allocator>& a) noexcept(__always_equal)
      : forward_list(a) {
    if (__always_equal || __na_ == __x.__na_) {
      __head_.next = __x.__head_.next;
      __x.__head_.next = nullptr;
    } else {
      __insert_elems(__head(), std::make_move_iterator(__x.begin()), std::make_move_iterator(__x.end()));
    }
  }
  constexpr forward_list(initializer_list<_Tp> il, const _Allocator& a = _Allocator()) : forward_list(a) {
    __insert_elems(__head(), il.begin(), il.end());
  }
  constexpr ~forward_list() { __free_chain(__head_.next); }

  constexpr forward_list& operator=(const forward_list& __x) {
    if (this == __builtin_addressof(__x))
      return *this;
    if constexpr (__pocca) {
      if (!__always_equal && __na_ != __x.__na_)
        clear();
      __na_ = __x.__na_;
    }
    __assign_elems(__x.begin(), __x.end());
    return *this;
  }
  constexpr forward_list& operator=(forward_list&& __x) noexcept(__always_equal) {
    if (this == __builtin_addressof(__x))
      return *this;
    if constexpr (__pocma || __always_equal) {
      clear();
      if constexpr (__pocma)
        __na_ = static_cast<__node_alloc&&>(__x.__na_);
      __head_.next = __x.__head_.next;
      __x.__head_.next = nullptr;
    } else {
      if (__na_ == __x.__na_) {
        clear();
        __head_.next = __x.__head_.next;
        __x.__head_.next = nullptr;
      } else {
        __assign_elems(std::make_move_iterator(__x.begin()), std::make_move_iterator(__x.end()));
      }
    }
    return *this;
  }
  constexpr forward_list& operator=(initializer_list<_Tp> il) {
    __assign_elems(il.begin(), il.end());
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
                  "std::forward_list::assign_range: T must be assignable from the range's reference type");
    __assign_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr void assign(size_type n, const _Tp& t) {
    __node_base* prev = __head();
    for (; n > 0 && prev->next; --n, prev = prev->next)
      value(prev->next) = t;
    if (n == 0)
      __erase_tail(prev);
    else
      __insert_n(prev, n, t);
  }
  constexpr void assign(initializer_list<_Tp> il) { __assign_elems(il.begin(), il.end()); }
  constexpr allocator_type get_allocator() const noexcept { return allocator_type(__na_); }

  // ---- [forward.list.iter] ----
  constexpr iterator before_begin() noexcept { return iterator(__head()); }
  constexpr const_iterator before_begin() const noexcept { return const_iterator(__head()); }
  constexpr iterator begin() noexcept { return iterator(__head_.next); }
  constexpr const_iterator begin() const noexcept { return const_iterator(__head_.next); }
  constexpr iterator end() noexcept { return iterator(nullptr); }
  constexpr const_iterator end() const noexcept { return const_iterator(nullptr); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cbefore_begin() const noexcept { return before_begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }

  // ---- capacity ----
  [[nodiscard]] constexpr bool empty() const noexcept { return __head_.next == nullptr; }
  constexpr size_type max_size() const noexcept {
    const auto a = static_cast<size_type>(__node_traits::max_size(__na_));
    const auto d = static_cast<size_type>(numeric_limits<difference_type>::max());
    return a < d ? a : d;
  }

  // ---- [forward.list.access] ----
  constexpr reference front() {
    __ycxx::__detail::__precondition(__head_.next != nullptr, "std::forward_list::front: empty list");
    return value(__head_.next);
  }
  constexpr const_reference front() const {
    __ycxx::__detail::__precondition(__head_.next != nullptr, "std::forward_list::front: empty list");
    return value(__head_.next);
  }

  // ---- [forward.list.modifiers] ----
  template <class... _Args>
  constexpr reference emplace_front(_Args&&... __args) {
    return *emplace_after(cbefore_begin(), static_cast<_Args&&>(__args)...);
  }
  constexpr void push_front(const _Tp& __x) { emplace_after(cbefore_begin(), __x); }
  constexpr void push_front(_Tp&& __x) { emplace_after(cbefore_begin(), static_cast<_Tp&&>(__x)); }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr void prepend_range(_Rp&& __rg) {
    __insert_elems(__head(), ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr void pop_front() {
    __ycxx::__detail::__precondition(__head_.next != nullptr, "std::forward_list::pop_front: empty list");
    erase_after(cbefore_begin());
  }
  template <class... _Args>
  constexpr iterator emplace_after(const_iterator position, _Args&&... __args) {
    node* n = __make_node(static_cast<_Args&&>(__args)...);
    n->next = position.__n_->next;
    position.__n_->next = n;
    return iterator(n);
  }
  constexpr iterator insert_after(const_iterator position, const _Tp& __x) { return emplace_after(position, __x); }
  constexpr iterator insert_after(const_iterator position, _Tp&& __x) {
    return emplace_after(position, static_cast<_Tp&&>(__x));
  }
  constexpr iterator insert_after(const_iterator position, size_type n, const _Tp& __x) {
    return iterator(__insert_n(position.__n_, n, __x));
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr iterator insert_after(const_iterator position, _InputIterator first, _InputIterator last) {
    return iterator(
        __insert_elems(position.__n_, static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last)));
  }
  constexpr iterator insert_after(const_iterator position, initializer_list<_Tp> il) {
    return iterator(__insert_elems(position.__n_, il.begin(), il.end()));
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr iterator insert_range_after(const_iterator position, _Rp&& __rg) {
    return iterator(__insert_elems(position.__n_, ranges::begin(__rg), ranges::end(__rg)));
  }
  constexpr iterator erase_after(const_iterator position) {
    __node_base* p = position.__n_;
    __node_base* n = p->next;
    p->next = n->next;
    __free_node(n);
    return iterator(p->next);
  }
  constexpr iterator erase_after(const_iterator position, const_iterator last) {
    if (position == last)
      return iterator(last.__n_);
    __node_base* p = position.__n_;
    __node_base* n = p->next;
    p->next = last.__n_;
    while (n != last.__n_) {
      __node_base* __nx = n->next;
      __free_node(n);
      n = __nx;
    }
    return iterator(last.__n_);
  }
  constexpr void swap(forward_list& __x) noexcept(__always_equal) {
    if (this == __builtin_addressof(__x))
      return;
    if constexpr (__pocs)
      ::__ycxx::__detail::__swap_adl::__do_swap(__na_, __x.__na_);
    else
      __ycxx::__detail::__precondition(__always_equal || __na_ == __x.__na_,
                                 "std::forward_list::swap: unequal allocators that do not propagate");
    __node_base* const t = __head_.next;
    __head_.next = __x.__head_.next;
    __x.__head_.next = t;
  }
  constexpr void resize(size_type __sz) {
    __node_base* prev = __head();
    for (; __sz > 0 && prev->next; --__sz)
      prev = prev->next;
    if (__sz == 0)
      __erase_tail(prev);
    else
      __insert_n(prev, __sz);
  }
  constexpr void resize(size_type __sz, const value_type& c) {
    __node_base* prev = __head();
    for (; __sz > 0 && prev->next; --__sz)
      prev = prev->next;
    if (__sz == 0)
      __erase_tail(prev);
    else
      __insert_n(prev, __sz, c);
  }
  constexpr void clear() noexcept { __erase_tail(__head()); }

  // ---- [forward.list.ops] ----
  constexpr void splice_after(const_iterator position, forward_list& __x) {
    __ycxx::__detail::__precondition(this != __builtin_addressof(__x), "std::forward_list::splice_after: x is *this");
    __check_alloc(__x);
    __node_base* __f = __x.__head_.next;
    if (!__f)
      return;
    __node_base* __l = __f;
    while (__l->next)
      __l = __l->next;
    __x.__head_.next = nullptr;
    __l->next = position.__n_->next;
    position.__n_->next = __f;
  }
  constexpr void splice_after(const_iterator position, forward_list&& __x) { splice_after(position, __x); }
  constexpr void splice_after(const_iterator position, forward_list& __x, const_iterator i) {
    __check_alloc(__x);
    __node_base* const __pos = position.__n_;
    __node_base* const p = i.__n_;
    __node_base* const n = p->next;
    if (__pos == p || __pos == n)
      return;
    p->next = n->next;
    n->next = __pos->next;
    __pos->next = n;
  }
  constexpr void splice_after(const_iterator position, forward_list&& __x, const_iterator i) {
    splice_after(position, __x, i);
  }
  constexpr void splice_after(const_iterator position, forward_list& __x, const_iterator first, const_iterator last) {
    __check_alloc(__x);
    if (first == last)
      return;
    __node_base* const __f = first.__n_;
    __node_base* const b = __f->next;
    if (b == last.__n_)
      return;
    __node_base* e = b;
    while (e->next != last.__n_)
      e = e->next;
    __f->next = last.__n_;
    e->next = position.__n_->next;
    position.__n_->next = b;
  }
  constexpr void splice_after(const_iterator position, forward_list&& __x, const_iterator first,
                              const_iterator last) {
    splice_after(position, __x, first, last);
  }

  constexpr size_type remove(const _Tp& value) {
    return remove_if([&value](const _Tp& e) -> bool { return e == value; });
  }
  template <class _Predicate>
  constexpr size_type remove_if(_Predicate pred) {
    // Unlinked nodes are destroyed at the end: value may refer to one of them.
    __graveyard __g{this};
    size_type count = 0;
    __node_base* prev = __head();
    while (__node_base* p = prev->next) {
      if (pred(forward_list::value(p))) {
        prev->next = p->next;
        __g.__bury(p);
        ++count;
      } else {
        prev = p;
      }
    }
    return count;
  }
  constexpr size_type unique() { return unique(equal_to<>()); }
  template <class _BinaryPredicate>
  constexpr size_type unique(_BinaryPredicate __binary_pred) {
    __graveyard __g{this};
    size_type count = 0;
    __node_base* __kept = __head_.next; // the last element kept
    if (!__kept)
      return 0;
    // Walk the original sequence: prev is p's original predecessor (erased nodes stay alive
    // until the end, so it can still be read).
    __node_base* prev = __kept;
    __node_base* p = __kept->next;
    while (p) {
      __node_base* __nx = p->next;
      if (__binary_pred(value(p), value(prev))) {
        __kept->next = __nx;
        __g.__bury(p);
        ++count;
      } else {
        __kept = p;
      }
      prev = p;
      p = __nx;
    }
    return count;
  }
  constexpr void merge(forward_list& __x) { merge(__x, less<>()); }
  constexpr void merge(forward_list&& __x) { merge(__x, less<>()); }
  template <class _Compare>
  constexpr void merge(forward_list& __x, _Compare comp) {
    if (this == __builtin_addressof(__x))
      return;
    __check_alloc(__x);
    __node_base* p = __head();
    while (p->next && __x.__head_.next) {
      __node_base* __q = __x.__head_.next;
      if (comp(value(__q), value(p->next))) {
        __x.__head_.next = __q->next;
        __q->next = p->next;
        p->next = __q;
      }
      p = p->next;
    }
    if (__x.__head_.next) {
      p->next = __x.__head_.next;
      __x.__head_.next = nullptr;
    }
  }
  template <class _Compare>
  constexpr void merge(forward_list&& __x, _Compare comp) {
    merge(__x, comp);
  }
  constexpr void sort() { sort(less<>()); }
  template <class _Compare>
  constexpr void sort(_Compare comp) {
    __node_base* first = __head_.next;
    if (!first || !first->next)
      return;
    __head_.next = nullptr;
    auto __val = [](__node_base* n) -> _Tp& { return forward_list::value(n); };
    __node_base* const h = __head();
    auto finish = [h](__node_base* c) { h->next = c; };
    __ycxx::__detail::__sort_chain(first, __val, comp, finish);
  }
  constexpr void reverse() noexcept {
    __node_base* r = nullptr;
    __node_base* p = __head_.next;
    while (p) {
      __node_base* __nx = p->next;
      p->next = r;
      r = p;
      p = __nx;
    }
    __head_.next = r;
  }
};

// ---- deduction guides ----
template <class _InputIterator, class _Allocator = allocator<__ycxx::__detail::__iter_value_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
forward_list(_InputIterator, _InputIterator, _Allocator = _Allocator())
    -> forward_list<__ycxx::__detail::__iter_value_type<_InputIterator>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator = allocator<ranges::range_value_t<_Rp>>>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
forward_list(from_range_t, _Rp&&, _Allocator = _Allocator()) -> forward_list<ranges::range_value_t<_Rp>, _Allocator>;

// ---- comparisons ----
template <class _Tp, class _Allocator>
constexpr bool operator==(const forward_list<_Tp, _Allocator>& __x, const forward_list<_Tp, _Allocator>& y) {
  auto i = __x.begin(), __j = y.begin();
  const auto __ie = __x.end(), __je = y.end();
  for (; i != __ie && __j != __je; ++i, (void)++__j)
    if (!(*i == *__j))
      return false;
  return i == __ie && __j == __je;
}
template <class _Tp, class _Allocator>
constexpr __ycxx::__detail::__synth_three_way_result<_Tp> operator<=>(const forward_list<_Tp, _Allocator>& __x,
                                                              const forward_list<_Tp, _Allocator>& y) {
  return std::lexicographical_compare_three_way(__x.begin(), __x.end(), y.begin(), y.end(),
                                                __ycxx::__detail::__synth_three_way);
}

template <class _Tp, class _Allocator>
constexpr void swap(forward_list<_Tp, _Allocator>& __x, forward_list<_Tp, _Allocator>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}

// ---- [forward.list.erasure] ----
template <class _Tp, class _Allocator, class _Predicate>
constexpr typename forward_list<_Tp, _Allocator>::size_type erase_if(forward_list<_Tp, _Allocator>& c, _Predicate pred) {
  return c.remove_if(pred);
}
template <class _Tp, class _Allocator, class _Up = _Tp>
constexpr typename forward_list<_Tp, _Allocator>::size_type erase(forward_list<_Tp, _Allocator>& c, const _Up& value) {
  return c.remove_if([&value](const auto& __elem) -> bool { return __elem == value; });
}

namespace pmr {
template <class _Tp>
using forward_list = std::forward_list<_Tp, polymorphic_allocator<_Tp>>;
} // namespace pmr

}} // namespace std
