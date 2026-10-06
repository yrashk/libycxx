// libycxx core: list ([list]), its comparisons, erasure and the pmr:: alias.
//
// Representation: a circular doubly linked list through a sentinel node ({prev, next}; end()
// is the sentinel), the element count and the node allocator (the allocator rebound to the node
// type; elements are constructed and destroyed through it). sent_ points to the sentinel, or is
// null while the list has never held an element. At run time the sentinel is head_, a member;
// during constant evaluation it is allocated (with std::allocator) instead, because GCC 16
// mis-evaluates objects reached through pointers into a returned (NRVO) object, which an
// embedded sentinel always is. Moves and swaps exchange node rings, never sentinels.
// A node keeps its element in a union member, so the node can exist before and after its
// element. Multi-element insertions build a detached chain first and splice it in, so an
// exception leaves the list unchanged. sort is a bottom-up merge sort of the nodes
// (sequence_support.hpp) that allocates nothing. T may be incomplete until a member is used.
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

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _Tp, class _Allocator>
class list;
}

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

struct __list_node_base {
  __list_node_base* prev;
  __list_node_base* next;
};

template <class _Tp>
struct __list_node : __list_node_base {
  union {
    _Tp value;
  };
  constexpr __list_node() noexcept : __list_node_base{nullptr, nullptr} {}
  __list_node(const __list_node&) = delete;
  constexpr ~__list_node() {}
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// T is the element type, possibly const.
template <class _Tp, class _Diff>
class __list_iter {
  using _Vp = std::remove_const_t<_Tp>;
  using base = ::__ycxx::__detail::__list_node_base;
  base* __n_ = nullptr;

  template <class, class>
  friend class __list_iter;
  template <class, class>
  friend class std::list;

  constexpr explicit __list_iter(base* n) noexcept : __n_(n) {}

public:
  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = _Vp;
  using difference_type = _Diff;
  using pointer = _Tp*;
  using reference = _Tp&;

  constexpr __list_iter() noexcept = default;
  template <class _Up>
    requires std::is_same_v<const _Up, _Tp> && (!std::is_same_v<_Up, _Tp>)
  constexpr __list_iter(const __list_iter<_Up, _Diff>& __o) noexcept : __n_(__o.__n_) {}

  constexpr reference operator*() const noexcept { return static_cast<::__ycxx::__detail::__list_node<_Vp>*>(__n_)->value; }
  constexpr pointer operator->() const noexcept {
    return __builtin_addressof(static_cast<::__ycxx::__detail::__list_node<_Vp>*>(__n_)->value);
  }
  constexpr __list_iter& operator++() noexcept {
    __n_ = __n_->next;
    return *this;
  }
  constexpr __list_iter operator++(int) noexcept {
    __list_iter t = *this;
    __n_ = __n_->next;
    return t;
  }
  constexpr __list_iter& operator--() noexcept {
    __n_ = __n_->prev;
    return *this;
  }
  constexpr __list_iter operator--(int) noexcept {
    __list_iter t = *this;
    __n_ = __n_->prev;
    return t;
  }
  friend constexpr bool operator==(const __list_iter& a, const __list_iter& b) noexcept { return a.__n_ == b.__n_; }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp, class _Allocator = allocator<_Tp>>
class list;

template <class _Tp, class _Allocator>
class list {
  static_assert(__ycxx::__detail::__allocator_for<_Allocator, _Tp>,
                "std::list: Allocator::value_type must be T ([container.alloc.reqmts])");

  using info = __ycxx::__detail::__alloc_info<_Allocator>;
  using __alloc_traits = allocator_traits<_Allocator>;
  using node = __ycxx::__detail::__list_node<_Tp>;
  using __node_base = __ycxx::__detail::__list_node_base;
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
  using iterator = __ycxx::__adl_free::__list_iter<_Tp, difference_type>;
  using const_iterator = __ycxx::__adl_free::__list_iter<const _Tp, difference_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  static constexpr bool __pocca = info::__pocca;
  static constexpr bool __pocma = info::__pocma;
  static constexpr bool __pocs = info::__pocs;
  static constexpr bool __always_equal = info::__always_equal;

  __node_base __head_ = {nullptr, nullptr};
  __node_base* __sent_ = nullptr;
  size_type __size_ = 0;
  [[no_unique_address]] __node_alloc __na_;

  // ---- nodes ----
  constexpr __node_base* __end_node() const noexcept { return __sent_; }
  constexpr __node_base* __first_node() const noexcept { return __sent_ ? __sent_->next : nullptr; }
  constexpr __node_base* sentinel() {
    if (!__sent_) {
      if consteval {
        __sent_ = allocator<__node_base>().allocate(1);
        std::construct_at(__sent_);
      } else {
        __sent_ = __builtin_addressof(__head_);
      }
      __sent_->prev = __sent_->next = __sent_;
    }
    return __sent_;
  }
  // The node of position p (the sentinel, created if need be, for end()).
  constexpr __node_base* __pos_node(const_iterator p) { return p.__n_ ? p.__n_ : sentinel(); }
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

  // The nodes of *this, detached: *this becomes empty.
  struct __ring {
    __node_base* first = nullptr;
    __node_base* last = nullptr;
    size_type n = 0;
  };
  constexpr __ring detach() noexcept {
    if (__size_ == 0)
      return {};
    __ring r{__sent_->next, __sent_->prev, __size_};
    __sent_->prev = __sent_->next = __sent_;
    __size_ = 0;
    return r;
  }
  // Links r into *this, which must be empty.
  constexpr void __attach(__ring r) {
    if (r.n == 0)
      return;
    __node_base* s = sentinel();
    r.first->prev = s;
    r.last->next = s;
    s->next = r.first;
    s->prev = r.last;
    __size_ = r.n;
  }
  static constexpr void __link_before(__node_base* __pos, __node_base* n) noexcept {
    n->prev = __pos->prev;
    n->next = __pos;
    __pos->prev->next = n;
    __pos->prev = n;
  }
  static constexpr void __unlink(__node_base* n) noexcept {
    n->prev->next = n->next;
    n->next->prev = n->prev;
  }
  // Moves the nodes [f, l) before pos; pos is not in [f, l).
  static constexpr void __transfer(__node_base* __pos, __node_base* __f, __node_base* __l) noexcept {
    if (__f == __l || __pos == __l)
      return;
    __node_base* last = __l->prev;
    __f->prev->next = __l;
    __l->prev = __f->prev;
    __node_base* p = __pos->prev;
    p->next = __f;
    __f->prev = p;
    last->next = __pos;
    __pos->prev = last;
  }
  // Takes over o's nodes; *this owns none.
  constexpr void take(list& __o) noexcept { __attach(__o.detach()); }

  // A detached chain of nodes, linked through prev/next from first to last.
  struct __chain {
    list* __owner;
    __node_base* first = nullptr;
    __node_base* last = nullptr;
    size_type n = 0;
    constexpr explicit __chain(list* __o) noexcept : __owner(__o) {}
    __chain(const __chain&) = delete;
    constexpr ~__chain() {
      while (first) {
        __node_base* __nx = first == last ? nullptr : first->next;
        __owner->__free_node(first);
        first = __nx;
      }
    }
    constexpr void push(__node_base* __x) noexcept {
      if (first) {
        last->next = __x;
        __x->prev = last;
      } else {
        first = __x;
      }
      last = __x;
      ++n;
    }
  };
  // Links c before pos (null: end()); returns the first linked node (pos if c is empty).
  constexpr __node_base* __splice_chain(__node_base* __pos, __chain& c) {
    __node_base* const __f = c.first;
    if (!__f)
      return __pos;
    if (!__pos)
      __pos = sentinel();
    __node_base* p = __pos->prev;
    p->next = __f;
    __f->prev = p;
    c.last->next = __pos;
    __pos->prev = c.last;
    __size_ += c.n;
    c.first = c.last = nullptr;
    c.n = 0;
    return __f;
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
  constexpr void __check_alloc(const list& __x) const noexcept {
    if constexpr (!__always_equal)
      __ycxx::__detail::__precondition(__na_ == __x.__na_, "std::list: splice or merge between lists with unequal allocators");
  }

  // The node at index i (i <= size_), walking from the closer end.
  constexpr __node_base* __node_at(size_type i) const noexcept {
    __node_base* p;
    if (i <= __size_ / 2) {
      p = __first_node();
      for (; i > 0; --i)
        p = p->next;
    } else {
      p = __end_node();
      for (i = __size_ - i; i > 0; --i)
        p = p->prev;
    }
    return p;
  }

  // Destroys the nodes of a null-terminated chain linked through next.
  struct __graveyard {
    list* __owner;
    __node_base* __head = nullptr;
    constexpr ~__graveyard() {
      while (__head) {
        __node_base* __nx = __head->next;
        __owner->__free_node(__head);
        __head = __nx;
      }
    }
    constexpr void __bury(__node_base* n) noexcept {
      n->next = __head;
      __head = n;
    }
  };

  template <class _It, class _Sent>
  constexpr void __assign_elems(_It first, _Sent last) {
    __node_base* p = __first_node();
    for (; first != last && p != __end_node(); ++first, (void)(p = p->next))
      value(p) = *first;
    if (first == last)
      erase(const_iterator(p), cend());
    else
      __insert_elems(__end_node(), static_cast<_It&&>(first), static_cast<_Sent&&>(last));
  }

public:
  // ---- [list.cons] ----
  constexpr list() noexcept(is_nothrow_default_constructible_v<_Allocator>) : list(_Allocator()) {}
  constexpr explicit list(const _Allocator& a) noexcept : __na_(a) {}
  constexpr explicit list(size_type n, const _Allocator& a = _Allocator()) : list(a) { __insert_n(__end_node(), n); }
  constexpr list(size_type n, const _Tp& value, const _Allocator& a = _Allocator()) : list(a) {
    __insert_n(__end_node(), n, value);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr list(_InputIterator first, _InputIterator last, const _Allocator& a = _Allocator()) : list(a) {
    __insert_elems(__end_node(), static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr list(from_range_t, _Rp&& __rg, const _Allocator& a = _Allocator()) : list(a) {
    __insert_elems(__end_node(), ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr list(const list& __x) : list(__alloc_traits::select_on_container_copy_construction(_Allocator(__x.__na_))) {
    __insert_elems(__end_node(), __x.begin(), __x.end());
  }
  constexpr list(list&& __x) noexcept(is_nothrow_move_constructible_v<__node_alloc>) : __na_(static_cast<__node_alloc&&>(__x.__na_)) { take(__x); }
  constexpr list(const list& __x, const type_identity_t<_Allocator>& a) : list(a) {
    __insert_elems(__end_node(), __x.begin(), __x.end());
  }
  // noexcept when the allocators always compare equal (an extension: nothing is allocated).
  constexpr list(list&& __x, const type_identity_t<_Allocator>& a) noexcept(__always_equal) : list(a) {
    if (__always_equal || __na_ == __x.__na_)
      take(__x);
    else
      __insert_elems(__end_node(), std::make_move_iterator(__x.begin()), std::make_move_iterator(__x.end()));
  }
  constexpr list(initializer_list<_Tp> il, const _Allocator& a = _Allocator()) : list(a) {
    __insert_elems(__end_node(), il.begin(), il.end());
  }
  constexpr ~list() {
    clear();
    if (__sent_ && __sent_ != __builtin_addressof(__head_)) {
      std::destroy_at(__sent_);
      allocator<__node_base>().deallocate(__sent_, 1);
    }
  }

  constexpr list& operator=(const list& __x) {
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
  constexpr list& operator=(list&& __x) noexcept(__always_equal) {
    if (this == __builtin_addressof(__x))
      return *this;
    if constexpr (__pocma || __always_equal) {
      clear();
      if constexpr (__pocma)
        __na_ = static_cast<__node_alloc&&>(__x.__na_);
      take(__x);
    } else {
      if (__na_ == __x.__na_) {
        clear();
        take(__x);
      } else {
        __assign_elems(std::make_move_iterator(__x.begin()), std::make_move_iterator(__x.end()));
      }
    }
    return *this;
  }
  constexpr list& operator=(initializer_list<_Tp> il) {
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
                  "std::list::assign_range: T must be assignable from the range's reference type");
    __assign_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr void assign(size_type n, const _Tp& t) {
    __node_base* p = __first_node();
    for (; n > 0 && p != __end_node(); --n, p = p->next)
      value(p) = t;
    if (n == 0)
      erase(const_iterator(p), cend());
    else
      __insert_n(__end_node(), n, t);
  }
  constexpr void assign(initializer_list<_Tp> il) { __assign_elems(il.begin(), il.end()); }
  constexpr allocator_type get_allocator() const noexcept { return allocator_type(__na_); }

  // ---- iterators ----
  constexpr iterator begin() noexcept { return iterator(__first_node()); }
  constexpr const_iterator begin() const noexcept { return const_iterator(__first_node()); }
  constexpr iterator end() noexcept { return iterator(__end_node()); }
  constexpr const_iterator end() const noexcept { return const_iterator(__end_node()); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [list.capacity] ----
  [[nodiscard]] constexpr bool empty() const noexcept { return __size_ == 0; }
  constexpr size_type size() const noexcept { return __size_; }
  constexpr size_type max_size() const noexcept {
    const auto a = static_cast<size_type>(__node_traits::max_size(__na_));
    const auto d = static_cast<size_type>(numeric_limits<difference_type>::max());
    return a < d ? a : d;
  }
  constexpr void resize(size_type __sz) {
    if (__sz < __size_)
      erase(const_iterator(__node_at(__sz)), cend());
    else
      __insert_n(__end_node(), __sz - __size_);
  }
  constexpr void resize(size_type __sz, const _Tp& c) {
    if (__sz < __size_)
      erase(const_iterator(__node_at(__sz)), cend());
    else
      __insert_n(__end_node(), __sz - __size_, c);
  }

  // ---- element access ----
  constexpr reference front() {
    __ycxx::__detail::__precondition(__size_ != 0, "std::list::front: empty list");
    return value(__sent_->next);
  }
  constexpr const_reference front() const {
    __ycxx::__detail::__precondition(__size_ != 0, "std::list::front: empty list");
    return value(__sent_->next);
  }
  constexpr reference back() {
    __ycxx::__detail::__precondition(__size_ != 0, "std::list::back: empty list");
    return value(__sent_->prev);
  }
  constexpr const_reference back() const {
    __ycxx::__detail::__precondition(__size_ != 0, "std::list::back: empty list");
    return value(__sent_->prev);
  }

  // ---- [list.modifiers] ----
  template <class... _Args>
  constexpr reference emplace_front(_Args&&... __args) {
    return *emplace(cbegin(), static_cast<_Args&&>(__args)...);
  }
  template <class... _Args>
  constexpr reference emplace_back(_Args&&... __args) {
    return *emplace(cend(), static_cast<_Args&&>(__args)...);
  }
  constexpr void push_front(const _Tp& __x) { emplace(cbegin(), __x); }
  constexpr void push_front(_Tp&& __x) { emplace(cbegin(), static_cast<_Tp&&>(__x)); }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr void prepend_range(_Rp&& __rg) {
    __insert_elems(__first_node(), ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr void pop_front() {
    __ycxx::__detail::__precondition(__size_ != 0, "std::list::pop_front: empty list");
    erase(cbegin());
  }
  constexpr void push_back(const _Tp& __x) { emplace(cend(), __x); }
  constexpr void push_back(_Tp&& __x) { emplace(cend(), static_cast<_Tp&&>(__x)); }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr void append_range(_Rp&& __rg) {
    __insert_elems(__end_node(), ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr void pop_back() {
    __ycxx::__detail::__precondition(__size_ != 0, "std::list::pop_back: empty list");
    erase(const_iterator(__sent_->prev));
  }
  template <class... _Args>
  constexpr iterator emplace(const_iterator position, _Args&&... __args) {
    if (__size_ == max_size())
      __ycxx::__detail::__throw_length_error("std::list: size would exceed max_size()");
    __node_base* const __pos = __pos_node(position);
    node* n = __make_node(static_cast<_Args&&>(__args)...);
    __link_before(__pos, n);
    ++__size_;
    return iterator(n);
  }
  constexpr iterator insert(const_iterator position, const _Tp& __x) { return emplace(position, __x); }
  constexpr iterator insert(const_iterator position, _Tp&& __x) { return emplace(position, static_cast<_Tp&&>(__x)); }
  constexpr iterator insert(const_iterator position, size_type n, const _Tp& __x) {
    return iterator(__insert_n(position.__n_, n, __x));
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr iterator insert(const_iterator position, _InputIterator first, _InputIterator last) {
    return iterator(
        __insert_elems(position.__n_, static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last)));
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr iterator insert_range(const_iterator position, _Rp&& __rg) {
    return iterator(__insert_elems(position.__n_, ranges::begin(__rg), ranges::end(__rg)));
  }
  constexpr iterator insert(const_iterator position, initializer_list<_Tp> il) {
    return iterator(__insert_elems(position.__n_, il.begin(), il.end()));
  }
  constexpr iterator erase(const_iterator position) {
    __node_base* n = position.__n_;
    __node_base* __nx = n->next;
    __unlink(n);
    --__size_;
    __free_node(n);
    return iterator(__nx);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    __node_base* p = first.__n_;
    while (p != last.__n_) {
      __node_base* __nx = p->next;
      __unlink(p);
      --__size_;
      __free_node(p);
      p = __nx;
    }
    return iterator(last.__n_);
  }
  constexpr void swap(list& __x) noexcept(__always_equal) {
    if (this == __builtin_addressof(__x))
      return;
    if constexpr (__pocs)
      ::__ycxx::__detail::__swap_adl::__do_swap(__na_, __x.__na_);
    else
      __ycxx::__detail::__precondition(__always_equal || __na_ == __x.__na_,
                                 "std::list::swap: unequal allocators that do not propagate");
    const __ring a = detach(), b = __x.detach();
    __attach(b);
    __x.__attach(a);
  }
  constexpr void clear() noexcept {
    if (!__sent_)
      return;
    __node_base* p = __sent_->next;
    while (p != __sent_) {
      __node_base* __nx = p->next;
      __free_node(p);
      p = __nx;
    }
    __sent_->prev = __sent_->next = __sent_;
    __size_ = 0;
  }

  // ---- [list.ops] ----
  constexpr void splice(const_iterator position, list& __x) {
    __ycxx::__detail::__precondition(this != __builtin_addressof(__x), "std::list::splice: x is *this");
    __check_alloc(__x);
    if (__x.__size_ == 0)
      return;
    __transfer(__pos_node(position), __x.__sent_->next, __x.__sent_);
    __size_ += __x.__size_;
    __x.__size_ = 0;
  }
  constexpr void splice(const_iterator position, list&& __x) { splice(position, __x); }
  constexpr void splice(const_iterator position, list& __x, const_iterator i) {
    __check_alloc(__x);
    __node_base* n = i.__n_;
    __node_base* const __pos = __pos_node(position);
    if (__pos == n || __pos == n->next)
      return;
    __transfer(__pos, n, n->next);
    if (this != __builtin_addressof(__x)) {
      --__x.__size_;
      ++__size_;
    }
  }
  constexpr void splice(const_iterator position, list&& __x, const_iterator i) { splice(position, __x, i); }
  constexpr void splice(const_iterator position, list& __x, const_iterator first, const_iterator last) {
    __check_alloc(__x);
    if (first == last)
      return;
    if (this != __builtin_addressof(__x)) {
      size_type n = 0;
      for (__node_base* p = first.__n_; p != last.__n_; p = p->next)
        ++n;
      __x.__size_ -= n;
      __size_ += n;
    }
    __transfer(__pos_node(position), first.__n_, last.__n_);
  }
  constexpr void splice(const_iterator position, list&& __x, const_iterator first, const_iterator last) {
    splice(position, __x, first, last);
  }

  constexpr size_type remove(const _Tp& value) {
    return remove_if([&value](const _Tp& e) -> bool { return e == value; });
  }
  template <class _Predicate>
  constexpr size_type remove_if(_Predicate pred) {
    // Unlinked nodes are destroyed at the end: value may refer to one of them.
    __graveyard __g{this};
    size_type count = 0;
    if (__size_ == 0)
      return 0;
    for (__node_base* p = __sent_->next; p != __sent_;) {
      __node_base* __nx = p->next;
      if (pred(list::value(p))) {
        __unlink(p);
        --__size_;
        __g.__bury(p);
        ++count;
      }
      p = __nx;
    }
    return count;
  }
  constexpr size_type unique() { return unique(equal_to<>()); }
  template <class _BinaryPredicate>
  constexpr size_type unique(_BinaryPredicate __binary_pred) {
    __graveyard __g{this};
    size_type count = 0;
    if (__size_ < 2)
      return 0;
    __node_base* prev = __sent_->next; // the element before p in the original sequence
    for (__node_base* p = prev->next; p != __end_node();) {
      __node_base* __nx = p->next;
      if (__binary_pred(value(p), value(prev))) {
        // Erased nodes stay alive until the end: the next comparison still reads this one.
        __unlink(p);
        --__size_;
        __g.__bury(p);
        ++count;
      }
      prev = p;
      p = __nx;
    }
    return count;
  }
  constexpr void merge(list& __x) { merge(__x, less<>()); }
  constexpr void merge(list&& __x) { merge(__x, less<>()); }
  template <class _Compare>
  constexpr void merge(list& __x, _Compare comp) {
    if (this == __builtin_addressof(__x) || __x.__size_ == 0)
      return;
    __check_alloc(__x);
    __node_base* const e = sentinel();
    __node_base* p = e->next;
    __node_base* __q = __x.__sent_->next;
    __node_base* const __xe = __x.__sent_;
    while (p != e && __q != __xe) {
      if (comp(value(__q), value(p))) {
        __node_base* __nq = __q->next;
        __unlink(__q);
        --__x.__size_;
        __link_before(p, __q);
        ++__size_;
        __q = __nq;
      } else {
        p = p->next;
      }
    }
    if (__q != __xe) {
      __transfer(e, __q, __xe);
      __size_ += __x.__size_;
      __x.__size_ = 0;
    }
  }
  template <class _Compare>
  constexpr void merge(list&& __x, _Compare comp) {
    merge(__x, comp);
  }
  constexpr void sort() { sort(less<>()); }
  template <class _Compare>
  constexpr void sort(_Compare comp) {
    if (__size_ < 2)
      return;
    __node_base* const h = __sent_;
    __node_base* first = h->next;
    h->prev->next = nullptr;
    auto __val = [](__node_base* n) -> _Tp& { return list::value(n); };
    auto finish = [h](__node_base* c) {
      __node_base* prev = h;
      for (; c; c = c->next) {
        c->prev = prev;
        prev->next = c;
        prev = c;
      }
      prev->next = h;
      h->prev = prev;
    };
    __ycxx::__detail::__sort_chain(first, __val, comp, finish);
  }
  constexpr void reverse() noexcept {
    if (!__sent_)
      return;
    __node_base* p = __sent_;
    do {
      __node_base* __nx = p->next;
      p->next = p->prev;
      p->prev = __nx;
      p = __nx;
    } while (p != __end_node());
  }
};

// ---- deduction guides ----
template <class _InputIterator, class _Allocator = allocator<__ycxx::__detail::__iter_value_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
list(_InputIterator, _InputIterator, _Allocator = _Allocator())
    -> list<__ycxx::__detail::__iter_value_type<_InputIterator>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator = allocator<ranges::range_value_t<_Rp>>>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
list(from_range_t, _Rp&&, _Allocator = _Allocator()) -> list<ranges::range_value_t<_Rp>, _Allocator>;

// ---- comparisons ----
template <class _Tp, class _Allocator>
constexpr bool operator==(const list<_Tp, _Allocator>& __x, const list<_Tp, _Allocator>& y) {
  return __x.size() == y.size() && std::equal(__x.begin(), __x.end(), y.begin());
}
template <class _Tp, class _Allocator>
constexpr __ycxx::__detail::__synth_three_way_result<_Tp> operator<=>(const list<_Tp, _Allocator>& __x,
                                                              const list<_Tp, _Allocator>& y) {
  return std::lexicographical_compare_three_way(__x.begin(), __x.end(), y.begin(), y.end(),
                                                __ycxx::__detail::__synth_three_way);
}

template <class _Tp, class _Allocator>
constexpr void swap(list<_Tp, _Allocator>& __x, list<_Tp, _Allocator>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}

// ---- [list.erasure] ----
template <class _Tp, class _Allocator, class _Predicate>
constexpr typename list<_Tp, _Allocator>::size_type erase_if(list<_Tp, _Allocator>& c, _Predicate pred) {
  return c.remove_if(pred);
}
template <class _Tp, class _Allocator, class _Up = _Tp>
constexpr typename list<_Tp, _Allocator>::size_type erase(list<_Tp, _Allocator>& c, const _Up& value) {
  return c.remove_if([&value](const auto& __elem) -> bool { return __elem == value; });
}

namespace pmr {
template <class _Tp>
using list = std::list<_Tp, polymorphic_allocator<_Tp>>;
} // namespace pmr

} // namespace std
