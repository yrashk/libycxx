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

namespace [[gnu::visibility("hidden")]] std {
template <class T, class Allocator>
class list;
}

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

struct list_node_base {
  list_node_base* prev;
  list_node_base* next;
};

template <class T>
struct list_node : list_node_base {
  union {
    T value;
  };
  constexpr list_node() noexcept : list_node_base{nullptr, nullptr} {}
  list_node(const list_node&) = delete;
  constexpr ~list_node() {}
};

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

// T is the element type, possibly const.
template <class T, class Diff>
class list_iter {
  using V = std::remove_const_t<T>;
  using base = ::ycxx::detail::list_node_base;
  base* n_ = nullptr;

  template <class, class>
  friend class list_iter;
  template <class, class>
  friend class std::list;

  constexpr explicit list_iter(base* n) noexcept : n_(n) {}

public:
  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = V;
  using difference_type = Diff;
  using pointer = T*;
  using reference = T&;

  constexpr list_iter() noexcept = default;
  template <class U>
    requires std::is_same_v<const U, T> && (!std::is_same_v<U, T>)
  constexpr list_iter(const list_iter<U, Diff>& o) noexcept : n_(o.n_) {}

  constexpr reference operator*() const noexcept { return static_cast<::ycxx::detail::list_node<V>*>(n_)->value; }
  constexpr pointer operator->() const noexcept {
    return __builtin_addressof(static_cast<::ycxx::detail::list_node<V>*>(n_)->value);
  }
  constexpr list_iter& operator++() noexcept {
    n_ = n_->next;
    return *this;
  }
  constexpr list_iter operator++(int) noexcept {
    list_iter t = *this;
    n_ = n_->next;
    return t;
  }
  constexpr list_iter& operator--() noexcept {
    n_ = n_->prev;
    return *this;
  }
  constexpr list_iter operator--(int) noexcept {
    list_iter t = *this;
    n_ = n_->prev;
    return t;
  }
  friend constexpr bool operator==(const list_iter& a, const list_iter& b) noexcept { return a.n_ == b.n_; }
};

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {

template <class T, class Allocator = allocator<T>>
class list;

template <class T, class Allocator>
class list {
  static_assert(ycxx::detail::allocator_for<Allocator, T>,
                "std::list: Allocator::value_type must be T ([container.alloc.reqmts])");

  using info = ycxx::detail::alloc_info<Allocator>;
  using alloc_traits = allocator_traits<Allocator>;
  using node = ycxx::detail::list_node<T>;
  using node_base = ycxx::detail::list_node_base;
  using node_alloc = typename info::template rebind<node>;
  using node_traits = allocator_traits<node_alloc>;

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
  using iterator = ycxx::adl_free::list_iter<T, difference_type>;
  using const_iterator = ycxx::adl_free::list_iter<const T, difference_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  static constexpr bool pocca = info::pocca;
  static constexpr bool pocma = info::pocma;
  static constexpr bool pocs = info::pocs;
  static constexpr bool always_equal = info::always_equal;

  node_base head_ = {nullptr, nullptr};
  node_base* sent_ = nullptr;
  size_type size_ = 0;
  [[no_unique_address]] node_alloc na_;

  // ---- nodes ----
  constexpr node_base* end_node() const noexcept { return sent_; }
  constexpr node_base* first_node() const noexcept { return sent_ ? sent_->next : nullptr; }
  constexpr node_base* sentinel() {
    if (!sent_) {
      if consteval {
        sent_ = allocator<node_base>().allocate(1);
        std::construct_at(sent_);
      } else {
        sent_ = __builtin_addressof(head_);
      }
      sent_->prev = sent_->next = sent_;
    }
    return sent_;
  }
  // The node of position p (the sentinel, created if need be, for end()).
  constexpr node_base* pos_node(const_iterator p) { return p.n_ ? p.n_ : sentinel(); }
  static constexpr T& value(node_base* n) noexcept { return static_cast<node*>(n)->value; }

  template <class... Args>
  constexpr node* make_node(Args&&... args) {
    node* n = std::to_address(node_traits::allocate(na_, 1));
    std::construct_at(n);
    ycxx::detail::rollback rb{[&] {
      std::destroy_at(n);
      node_traits::deallocate(na_, ycxx::detail::to_alloc_pointer<typename node_traits::pointer>(n), 1);
    }};
    node_traits::construct(na_, __builtin_addressof(n->value), static_cast<Args&&>(args)...);
    rb.release();
    return n;
  }
  constexpr void free_node(node_base* b) noexcept {
    node* n = static_cast<node*>(b);
    node_traits::destroy(na_, __builtin_addressof(n->value));
    std::destroy_at(n);
    node_traits::deallocate(na_, ycxx::detail::to_alloc_pointer<typename node_traits::pointer>(n), 1);
  }

  // The nodes of *this, detached: *this becomes empty.
  struct ring {
    node_base* first = nullptr;
    node_base* last = nullptr;
    size_type n = 0;
  };
  constexpr ring detach() noexcept {
    if (size_ == 0)
      return {};
    ring r{sent_->next, sent_->prev, size_};
    sent_->prev = sent_->next = sent_;
    size_ = 0;
    return r;
  }
  // Links r into *this, which must be empty.
  constexpr void attach(ring r) {
    if (r.n == 0)
      return;
    node_base* s = sentinel();
    r.first->prev = s;
    r.last->next = s;
    s->next = r.first;
    s->prev = r.last;
    size_ = r.n;
  }
  static constexpr void link_before(node_base* pos, node_base* n) noexcept {
    n->prev = pos->prev;
    n->next = pos;
    pos->prev->next = n;
    pos->prev = n;
  }
  static constexpr void unlink(node_base* n) noexcept {
    n->prev->next = n->next;
    n->next->prev = n->prev;
  }
  // Moves the nodes [f, l) before pos; pos is not in [f, l).
  static constexpr void transfer(node_base* pos, node_base* f, node_base* l) noexcept {
    if (f == l || pos == l)
      return;
    node_base* last = l->prev;
    f->prev->next = l;
    l->prev = f->prev;
    node_base* p = pos->prev;
    p->next = f;
    f->prev = p;
    last->next = pos;
    pos->prev = last;
  }
  // Takes over o's nodes; *this owns none.
  constexpr void take(list& o) noexcept { attach(o.detach()); }

  // A detached chain of nodes, linked through prev/next from first to last.
  struct chain {
    list* owner;
    node_base* first = nullptr;
    node_base* last = nullptr;
    size_type n = 0;
    constexpr explicit chain(list* o) noexcept : owner(o) {}
    chain(const chain&) = delete;
    constexpr ~chain() {
      while (first) {
        node_base* nx = first == last ? nullptr : first->next;
        owner->free_node(first);
        first = nx;
      }
    }
    constexpr void push(node_base* x) noexcept {
      if (first) {
        last->next = x;
        x->prev = last;
      } else {
        first = x;
      }
      last = x;
      ++n;
    }
  };
  // Links c before pos (null: end()); returns the first linked node (pos if c is empty).
  constexpr node_base* splice_chain(node_base* pos, chain& c) {
    node_base* const f = c.first;
    if (!f)
      return pos;
    if (!pos)
      pos = sentinel();
    node_base* p = pos->prev;
    p->next = f;
    f->prev = p;
    c.last->next = pos;
    pos->prev = c.last;
    size_ += c.n;
    c.first = c.last = nullptr;
    c.n = 0;
    return f;
  }

  template <class It, class Sent>
  constexpr node_base* insert_elems(node_base* pos, It first, Sent last) {
    chain c(this);
    for (; first != last; ++first)
      c.push(make_node(*first));
    return splice_chain(pos, c);
  }
  template <class... Args>
  constexpr node_base* insert_n(node_base* pos, size_type n, const Args&... args) {
    chain c(this);
    for (size_type i = 0; i < n; ++i)
      c.push(make_node(args...));
    return splice_chain(pos, c);
  }
  constexpr void check_alloc(const list& x) const noexcept {
    if constexpr (!always_equal)
      ycxx::detail::precondition(na_ == x.na_, "std::list: splice or merge between lists with unequal allocators");
  }

  // The node at index i (i <= size_), walking from the closer end.
  constexpr node_base* node_at(size_type i) const noexcept {
    node_base* p;
    if (i <= size_ / 2) {
      p = first_node();
      for (; i > 0; --i)
        p = p->next;
    } else {
      p = end_node();
      for (i = size_ - i; i > 0; --i)
        p = p->prev;
    }
    return p;
  }

  // Destroys the nodes of a null-terminated chain linked through next.
  struct graveyard {
    list* owner;
    node_base* head = nullptr;
    constexpr ~graveyard() {
      while (head) {
        node_base* nx = head->next;
        owner->free_node(head);
        head = nx;
      }
    }
    constexpr void bury(node_base* n) noexcept {
      n->next = head;
      head = n;
    }
  };

  template <class It, class Sent>
  constexpr void assign_elems(It first, Sent last) {
    node_base* p = first_node();
    for (; first != last && p != end_node(); ++first, (void)(p = p->next))
      value(p) = *first;
    if (first == last)
      erase(const_iterator(p), cend());
    else
      insert_elems(end_node(), static_cast<It&&>(first), static_cast<Sent&&>(last));
  }

public:
  // ---- [list.cons] ----
  constexpr list() noexcept(is_nothrow_default_constructible_v<Allocator>) : list(Allocator()) {}
  constexpr explicit list(const Allocator& a) noexcept : na_(a) {}
  constexpr explicit list(size_type n, const Allocator& a = Allocator()) : list(a) { insert_n(end_node(), n); }
  constexpr list(size_type n, const T& value, const Allocator& a = Allocator()) : list(a) {
    insert_n(end_node(), n, value);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr list(InputIterator first, InputIterator last, const Allocator& a = Allocator()) : list(a) {
    insert_elems(end_node(), static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
  }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr list(from_range_t, R&& rg, const Allocator& a = Allocator()) : list(a) {
    insert_elems(end_node(), ranges::begin(rg), ranges::end(rg));
  }
  constexpr list(const list& x) : list(alloc_traits::select_on_container_copy_construction(Allocator(x.na_))) {
    insert_elems(end_node(), x.begin(), x.end());
  }
  constexpr list(list&& x) noexcept(is_nothrow_move_constructible_v<node_alloc>) : na_(static_cast<node_alloc&&>(x.na_)) { take(x); }
  constexpr list(const list& x, const type_identity_t<Allocator>& a) : list(a) {
    insert_elems(end_node(), x.begin(), x.end());
  }
  // noexcept when the allocators always compare equal (an extension: nothing is allocated).
  constexpr list(list&& x, const type_identity_t<Allocator>& a) noexcept(always_equal) : list(a) {
    if (always_equal || na_ == x.na_)
      take(x);
    else
      insert_elems(end_node(), std::make_move_iterator(x.begin()), std::make_move_iterator(x.end()));
  }
  constexpr list(initializer_list<T> il, const Allocator& a = Allocator()) : list(a) {
    insert_elems(end_node(), il.begin(), il.end());
  }
  constexpr ~list() {
    clear();
    if (sent_ && sent_ != __builtin_addressof(head_)) {
      std::destroy_at(sent_);
      allocator<node_base>().deallocate(sent_, 1);
    }
  }

  constexpr list& operator=(const list& x) {
    if (this == __builtin_addressof(x))
      return *this;
    if constexpr (pocca) {
      if (!always_equal && na_ != x.na_)
        clear();
      na_ = x.na_;
    }
    assign_elems(x.begin(), x.end());
    return *this;
  }
  constexpr list& operator=(list&& x) noexcept(always_equal) {
    if (this == __builtin_addressof(x))
      return *this;
    if constexpr (pocma || always_equal) {
      clear();
      if constexpr (pocma)
        na_ = static_cast<node_alloc&&>(x.na_);
      take(x);
    } else {
      if (na_ == x.na_) {
        clear();
        take(x);
      } else {
        assign_elems(std::make_move_iterator(x.begin()), std::make_move_iterator(x.end()));
      }
    }
    return *this;
  }
  constexpr list& operator=(initializer_list<T> il) {
    assign_elems(il.begin(), il.end());
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
                  "std::list::assign_range: T must be assignable from the range's reference type");
    assign_elems(ranges::begin(rg), ranges::end(rg));
  }
  constexpr void assign(size_type n, const T& t) {
    node_base* p = first_node();
    for (; n > 0 && p != end_node(); --n, p = p->next)
      value(p) = t;
    if (n == 0)
      erase(const_iterator(p), cend());
    else
      insert_n(end_node(), n, t);
  }
  constexpr void assign(initializer_list<T> il) { assign_elems(il.begin(), il.end()); }
  constexpr allocator_type get_allocator() const noexcept { return allocator_type(na_); }

  // ---- iterators ----
  constexpr iterator begin() noexcept { return iterator(first_node()); }
  constexpr const_iterator begin() const noexcept { return const_iterator(first_node()); }
  constexpr iterator end() noexcept { return iterator(end_node()); }
  constexpr const_iterator end() const noexcept { return const_iterator(end_node()); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [list.capacity] ----
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
  constexpr size_type size() const noexcept { return size_; }
  constexpr size_type max_size() const noexcept {
    const auto a = static_cast<size_type>(node_traits::max_size(na_));
    const auto d = static_cast<size_type>(numeric_limits<difference_type>::max());
    return a < d ? a : d;
  }
  constexpr void resize(size_type sz) {
    if (sz < size_)
      erase(const_iterator(node_at(sz)), cend());
    else
      insert_n(end_node(), sz - size_);
  }
  constexpr void resize(size_type sz, const T& c) {
    if (sz < size_)
      erase(const_iterator(node_at(sz)), cend());
    else
      insert_n(end_node(), sz - size_, c);
  }

  // ---- element access ----
  constexpr reference front() {
    ycxx::detail::precondition(size_ != 0, "std::list::front: empty list");
    return value(sent_->next);
  }
  constexpr const_reference front() const {
    ycxx::detail::precondition(size_ != 0, "std::list::front: empty list");
    return value(sent_->next);
  }
  constexpr reference back() {
    ycxx::detail::precondition(size_ != 0, "std::list::back: empty list");
    return value(sent_->prev);
  }
  constexpr const_reference back() const {
    ycxx::detail::precondition(size_ != 0, "std::list::back: empty list");
    return value(sent_->prev);
  }

  // ---- [list.modifiers] ----
  template <class... Args>
  constexpr reference emplace_front(Args&&... args) {
    return *emplace(cbegin(), static_cast<Args&&>(args)...);
  }
  template <class... Args>
  constexpr reference emplace_back(Args&&... args) {
    return *emplace(cend(), static_cast<Args&&>(args)...);
  }
  constexpr void push_front(const T& x) { emplace(cbegin(), x); }
  constexpr void push_front(T&& x) { emplace(cbegin(), static_cast<T&&>(x)); }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr void prepend_range(R&& rg) {
    insert_elems(first_node(), ranges::begin(rg), ranges::end(rg));
  }
  constexpr void pop_front() {
    ycxx::detail::precondition(size_ != 0, "std::list::pop_front: empty list");
    erase(cbegin());
  }
  constexpr void push_back(const T& x) { emplace(cend(), x); }
  constexpr void push_back(T&& x) { emplace(cend(), static_cast<T&&>(x)); }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr void append_range(R&& rg) {
    insert_elems(end_node(), ranges::begin(rg), ranges::end(rg));
  }
  constexpr void pop_back() {
    ycxx::detail::precondition(size_ != 0, "std::list::pop_back: empty list");
    erase(const_iterator(sent_->prev));
  }
  template <class... Args>
  constexpr iterator emplace(const_iterator position, Args&&... args) {
    if (size_ == max_size())
      ycxx::detail::throw_length_error("std::list: size would exceed max_size()");
    node_base* const pos = pos_node(position);
    node* n = make_node(static_cast<Args&&>(args)...);
    link_before(pos, n);
    ++size_;
    return iterator(n);
  }
  constexpr iterator insert(const_iterator position, const T& x) { return emplace(position, x); }
  constexpr iterator insert(const_iterator position, T&& x) { return emplace(position, static_cast<T&&>(x)); }
  constexpr iterator insert(const_iterator position, size_type n, const T& x) {
    return iterator(insert_n(position.n_, n, x));
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr iterator insert(const_iterator position, InputIterator first, InputIterator last) {
    return iterator(
        insert_elems(position.n_, static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last)));
  }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr iterator insert_range(const_iterator position, R&& rg) {
    return iterator(insert_elems(position.n_, ranges::begin(rg), ranges::end(rg)));
  }
  constexpr iterator insert(const_iterator position, initializer_list<T> il) {
    return iterator(insert_elems(position.n_, il.begin(), il.end()));
  }
  constexpr iterator erase(const_iterator position) {
    node_base* n = position.n_;
    node_base* nx = n->next;
    unlink(n);
    --size_;
    free_node(n);
    return iterator(nx);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    node_base* p = first.n_;
    while (p != last.n_) {
      node_base* nx = p->next;
      unlink(p);
      --size_;
      free_node(p);
      p = nx;
    }
    return iterator(last.n_);
  }
  constexpr void swap(list& x) noexcept(always_equal) {
    if (this == __builtin_addressof(x))
      return;
    if constexpr (pocs)
      ::ycxx::detail::swap_adl::do_swap(na_, x.na_);
    else
      ycxx::detail::precondition(always_equal || na_ == x.na_,
                                 "std::list::swap: unequal allocators that do not propagate");
    const ring a = detach(), b = x.detach();
    attach(b);
    x.attach(a);
  }
  constexpr void clear() noexcept {
    if (!sent_)
      return;
    node_base* p = sent_->next;
    while (p != sent_) {
      node_base* nx = p->next;
      free_node(p);
      p = nx;
    }
    sent_->prev = sent_->next = sent_;
    size_ = 0;
  }

  // ---- [list.ops] ----
  constexpr void splice(const_iterator position, list& x) {
    ycxx::detail::precondition(this != __builtin_addressof(x), "std::list::splice: x is *this");
    check_alloc(x);
    if (x.size_ == 0)
      return;
    transfer(pos_node(position), x.sent_->next, x.sent_);
    size_ += x.size_;
    x.size_ = 0;
  }
  constexpr void splice(const_iterator position, list&& x) { splice(position, x); }
  constexpr void splice(const_iterator position, list& x, const_iterator i) {
    check_alloc(x);
    node_base* n = i.n_;
    node_base* const pos = pos_node(position);
    if (pos == n || pos == n->next)
      return;
    transfer(pos, n, n->next);
    if (this != __builtin_addressof(x)) {
      --x.size_;
      ++size_;
    }
  }
  constexpr void splice(const_iterator position, list&& x, const_iterator i) { splice(position, x, i); }
  constexpr void splice(const_iterator position, list& x, const_iterator first, const_iterator last) {
    check_alloc(x);
    if (first == last)
      return;
    if (this != __builtin_addressof(x)) {
      size_type n = 0;
      for (node_base* p = first.n_; p != last.n_; p = p->next)
        ++n;
      x.size_ -= n;
      size_ += n;
    }
    transfer(pos_node(position), first.n_, last.n_);
  }
  constexpr void splice(const_iterator position, list&& x, const_iterator first, const_iterator last) {
    splice(position, x, first, last);
  }

  constexpr size_type remove(const T& value) {
    return remove_if([&value](const T& e) -> bool { return e == value; });
  }
  template <class Predicate>
  constexpr size_type remove_if(Predicate pred) {
    // Unlinked nodes are destroyed at the end: value may refer to one of them.
    graveyard g{this};
    size_type count = 0;
    if (size_ == 0)
      return 0;
    for (node_base* p = sent_->next; p != sent_;) {
      node_base* nx = p->next;
      if (pred(list::value(p))) {
        unlink(p);
        --size_;
        g.bury(p);
        ++count;
      }
      p = nx;
    }
    return count;
  }
  constexpr size_type unique() { return unique(equal_to<>()); }
  template <class BinaryPredicate>
  constexpr size_type unique(BinaryPredicate binary_pred) {
    graveyard g{this};
    size_type count = 0;
    if (size_ < 2)
      return 0;
    node_base* prev = sent_->next; // the element before p in the original sequence
    for (node_base* p = prev->next; p != end_node();) {
      node_base* nx = p->next;
      if (binary_pred(value(p), value(prev))) {
        // Erased nodes stay alive until the end: the next comparison still reads this one.
        unlink(p);
        --size_;
        g.bury(p);
        ++count;
      }
      prev = p;
      p = nx;
    }
    return count;
  }
  constexpr void merge(list& x) { merge(x, less<>()); }
  constexpr void merge(list&& x) { merge(x, less<>()); }
  template <class Compare>
  constexpr void merge(list& x, Compare comp) {
    if (this == __builtin_addressof(x) || x.size_ == 0)
      return;
    check_alloc(x);
    node_base* const e = sentinel();
    node_base* p = e->next;
    node_base* q = x.sent_->next;
    node_base* const xe = x.sent_;
    while (p != e && q != xe) {
      if (comp(value(q), value(p))) {
        node_base* nq = q->next;
        unlink(q);
        --x.size_;
        link_before(p, q);
        ++size_;
        q = nq;
      } else {
        p = p->next;
      }
    }
    if (q != xe) {
      transfer(e, q, xe);
      size_ += x.size_;
      x.size_ = 0;
    }
  }
  template <class Compare>
  constexpr void merge(list&& x, Compare comp) {
    merge(x, comp);
  }
  constexpr void sort() { sort(less<>()); }
  template <class Compare>
  constexpr void sort(Compare comp) {
    if (size_ < 2)
      return;
    node_base* const h = sent_;
    node_base* first = h->next;
    h->prev->next = nullptr;
    auto val = [](node_base* n) -> T& { return list::value(n); };
    auto finish = [h](node_base* c) {
      node_base* prev = h;
      for (; c; c = c->next) {
        c->prev = prev;
        prev->next = c;
        prev = c;
      }
      prev->next = h;
      h->prev = prev;
    };
    ycxx::detail::sort_chain(first, val, comp, finish);
  }
  constexpr void reverse() noexcept {
    if (!sent_)
      return;
    node_base* p = sent_;
    do {
      node_base* nx = p->next;
      p->next = p->prev;
      p->prev = nx;
      p = nx;
    } while (p != end_node());
  }
};

// ---- deduction guides ----
template <class InputIterator, class Allocator = allocator<ycxx::detail::iter_value_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
list(InputIterator, InputIterator, Allocator = Allocator())
    -> list<ycxx::detail::iter_value_type<InputIterator>, Allocator>;
template <ranges::input_range R, class Allocator = allocator<ranges::range_value_t<R>>>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
list(from_range_t, R&&, Allocator = Allocator()) -> list<ranges::range_value_t<R>, Allocator>;

// ---- comparisons ----
template <class T, class Allocator>
constexpr bool operator==(const list<T, Allocator>& x, const list<T, Allocator>& y) {
  return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}
template <class T, class Allocator>
constexpr ycxx::detail::synth_three_way_result<T> operator<=>(const list<T, Allocator>& x,
                                                              const list<T, Allocator>& y) {
  return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(),
                                                ycxx::detail::synth_three_way);
}

template <class T, class Allocator>
constexpr void swap(list<T, Allocator>& x, list<T, Allocator>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

// ---- [list.erasure] ----
template <class T, class Allocator, class Predicate>
constexpr typename list<T, Allocator>::size_type erase_if(list<T, Allocator>& c, Predicate pred) {
  return c.remove_if(pred);
}
template <class T, class Allocator, class U = T>
constexpr typename list<T, Allocator>::size_type erase(list<T, Allocator>& c, const U& value) {
  return c.remove_if([&value](const auto& elem) -> bool { return elem == value; });
}

namespace pmr {
template <class T>
using list = std::list<T, polymorphic_allocator<T>>;
} // namespace pmr

} // namespace std
