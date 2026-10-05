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

namespace [[gnu::visibility("hidden")]] std {
template <class T, class Allocator>
class forward_list;
}

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

struct fwd_node_base {
  fwd_node_base* next;
};

template <class T>
struct fwd_node : fwd_node_base {
  union {
    T value;
  };
  constexpr fwd_node() noexcept : fwd_node_base{nullptr} {}
  fwd_node(const fwd_node&) = delete;
  constexpr ~fwd_node() {}
};

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

// T is the element type, possibly const.
template <class T, class Diff>
class fwd_list_iter {
  using V = std::remove_const_t<T>;
  using base = ::ycxx::detail::fwd_node_base;
  base* n_ = nullptr;

  template <class, class>
  friend class fwd_list_iter;
  template <class, class>
  friend class std::forward_list;

  constexpr explicit fwd_list_iter(base* n) noexcept : n_(n) {}

public:
  using iterator_concept = std::forward_iterator_tag;
  using iterator_category = std::forward_iterator_tag;
  using value_type = V;
  using difference_type = Diff;
  using pointer = T*;
  using reference = T&;

  constexpr fwd_list_iter() noexcept = default;
  template <class U>
    requires std::is_same_v<const U, T> && (!std::is_same_v<U, T>)
  constexpr fwd_list_iter(const fwd_list_iter<U, Diff>& o) noexcept : n_(o.n_) {}

  constexpr reference operator*() const noexcept { return static_cast<::ycxx::detail::fwd_node<V>*>(n_)->value; }
  constexpr pointer operator->() const noexcept {
    return __builtin_addressof(static_cast<::ycxx::detail::fwd_node<V>*>(n_)->value);
  }
  constexpr fwd_list_iter& operator++() noexcept {
    n_ = n_->next;
    return *this;
  }
  constexpr fwd_list_iter operator++(int) noexcept {
    fwd_list_iter t = *this;
    n_ = n_->next;
    return t;
  }
  friend constexpr bool operator==(const fwd_list_iter& a, const fwd_list_iter& b) noexcept { return a.n_ == b.n_; }
};

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {

template <class T, class Allocator = allocator<T>>
class forward_list;

template <class T, class Allocator>
class forward_list {
  static_assert(ycxx::detail::allocator_for<Allocator, T>,
                "std::forward_list: Allocator::value_type must be T ([container.alloc.reqmts])");

  using info = ycxx::detail::alloc_info<Allocator>;
  using alloc_traits = allocator_traits<Allocator>;
  using node = ycxx::detail::fwd_node<T>;
  using node_base = ycxx::detail::fwd_node_base;
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
  using iterator = ycxx::adl_free::fwd_list_iter<T, difference_type>;
  using const_iterator = ycxx::adl_free::fwd_list_iter<const T, difference_type>;

private:
  static constexpr bool pocca = info::pocca;
  static constexpr bool pocma = info::pocma;
  static constexpr bool pocs = info::pocs;
  static constexpr bool always_equal = info::always_equal;

  node_base head_ = {nullptr};
  [[no_unique_address]] node_alloc na_;

  // ---- nodes ----
  constexpr node_base* head() const noexcept { return const_cast<node_base*>(__builtin_addressof(head_)); }
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
  // Frees a null-terminated chain.
  constexpr void free_chain(node_base* p) noexcept {
    while (p) {
      node_base* nx = p->next;
      free_node(p);
      p = nx;
    }
  }

  // A detached chain of nodes, null-terminated, freed unless linked in.
  struct chain {
    forward_list* owner;
    node_base* first = nullptr;
    node_base* last = nullptr;
    constexpr explicit chain(forward_list* o) noexcept : owner(o) {}
    chain(const chain&) = delete;
    constexpr ~chain() { owner->free_chain(first); }
    constexpr void push(node_base* x) noexcept {
      if (first)
        last->next = x;
      else
        first = x;
      last = x;
    }
  };
  // Links c after pos; returns the last linked node (pos if c is empty).
  static constexpr node_base* splice_chain(node_base* pos, chain& c) noexcept {
    if (!c.first)
      return pos;
    c.last->next = pos->next;
    pos->next = c.first;
    node_base* const l = c.last;
    c.first = c.last = nullptr;
    return l;
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
  // Frees the nodes after pos; returns pos.
  constexpr void erase_tail(node_base* pos) noexcept {
    node_base* p = pos->next;
    pos->next = nullptr;
    free_chain(p);
  }
  constexpr void check_alloc(const forward_list& x) const noexcept {
    if constexpr (!always_equal)
      ycxx::detail::precondition(na_ == x.na_,
                                 "std::forward_list: splice or merge between lists with unequal allocators");
  }
  template <class It, class Sent>
  constexpr void assign_elems(It first, Sent last) {
    node_base* prev = head();
    for (; first != last && prev->next; ++first, (void)(prev = prev->next))
      value(prev->next) = *first;
    if (first == last)
      erase_tail(prev);
    else
      insert_elems(prev, static_cast<It&&>(first), static_cast<Sent&&>(last));
  }
  // Destroys the nodes of a chain at scope exit.
  struct graveyard {
    forward_list* owner;
    node_base* head = nullptr;
    constexpr ~graveyard() { owner->free_chain(head); }
    constexpr void bury(node_base* n) noexcept {
      n->next = head;
      head = n;
    }
  };

public:
  // ---- [forward.list.cons] ----
  constexpr forward_list() noexcept(is_nothrow_default_constructible_v<Allocator>) : forward_list(Allocator()) {}
  constexpr explicit forward_list(const Allocator& a) noexcept : na_(a) {}
  constexpr explicit forward_list(size_type n, const Allocator& a = Allocator()) : forward_list(a) {
    insert_n(head(), n);
  }
  constexpr forward_list(size_type n, const T& value, const Allocator& a = Allocator()) : forward_list(a) {
    insert_n(head(), n, value);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr forward_list(InputIterator first, InputIterator last, const Allocator& a = Allocator())
      : forward_list(a) {
    insert_elems(head(), static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
  }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr forward_list(from_range_t, R&& rg, const Allocator& a = Allocator()) : forward_list(a) {
    insert_elems(head(), ranges::begin(rg), ranges::end(rg));
  }
  constexpr forward_list(const forward_list& x)
      : forward_list(alloc_traits::select_on_container_copy_construction(Allocator(x.na_))) {
    insert_elems(head(), x.begin(), x.end());
  }
  constexpr forward_list(forward_list&& x) noexcept(is_nothrow_move_constructible_v<node_alloc>) : na_(static_cast<node_alloc&&>(x.na_)) {
    head_.next = x.head_.next;
    x.head_.next = nullptr;
  }
  constexpr forward_list(const forward_list& x, const type_identity_t<Allocator>& a) : forward_list(a) {
    insert_elems(head(), x.begin(), x.end());
  }
  // noexcept when the allocators always compare equal (an extension: nothing is allocated).
  constexpr forward_list(forward_list&& x, const type_identity_t<Allocator>& a) noexcept(always_equal)
      : forward_list(a) {
    if (always_equal || na_ == x.na_) {
      head_.next = x.head_.next;
      x.head_.next = nullptr;
    } else {
      insert_elems(head(), std::make_move_iterator(x.begin()), std::make_move_iterator(x.end()));
    }
  }
  constexpr forward_list(initializer_list<T> il, const Allocator& a = Allocator()) : forward_list(a) {
    insert_elems(head(), il.begin(), il.end());
  }
  constexpr ~forward_list() { free_chain(head_.next); }

  constexpr forward_list& operator=(const forward_list& x) {
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
  constexpr forward_list& operator=(forward_list&& x) noexcept(always_equal) {
    if (this == __builtin_addressof(x))
      return *this;
    if constexpr (pocma || always_equal) {
      clear();
      if constexpr (pocma)
        na_ = static_cast<node_alloc&&>(x.na_);
      head_.next = x.head_.next;
      x.head_.next = nullptr;
    } else {
      if (na_ == x.na_) {
        clear();
        head_.next = x.head_.next;
        x.head_.next = nullptr;
      } else {
        assign_elems(std::make_move_iterator(x.begin()), std::make_move_iterator(x.end()));
      }
    }
    return *this;
  }
  constexpr forward_list& operator=(initializer_list<T> il) {
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
                  "std::forward_list::assign_range: T must be assignable from the range's reference type");
    assign_elems(ranges::begin(rg), ranges::end(rg));
  }
  constexpr void assign(size_type n, const T& t) {
    node_base* prev = head();
    for (; n > 0 && prev->next; --n, prev = prev->next)
      value(prev->next) = t;
    if (n == 0)
      erase_tail(prev);
    else
      insert_n(prev, n, t);
  }
  constexpr void assign(initializer_list<T> il) { assign_elems(il.begin(), il.end()); }
  constexpr allocator_type get_allocator() const noexcept { return allocator_type(na_); }

  // ---- [forward.list.iter] ----
  constexpr iterator before_begin() noexcept { return iterator(head()); }
  constexpr const_iterator before_begin() const noexcept { return const_iterator(head()); }
  constexpr iterator begin() noexcept { return iterator(head_.next); }
  constexpr const_iterator begin() const noexcept { return const_iterator(head_.next); }
  constexpr iterator end() noexcept { return iterator(nullptr); }
  constexpr const_iterator end() const noexcept { return const_iterator(nullptr); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cbefore_begin() const noexcept { return before_begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }

  // ---- capacity ----
  [[nodiscard]] constexpr bool empty() const noexcept { return head_.next == nullptr; }
  constexpr size_type max_size() const noexcept {
    const auto a = static_cast<size_type>(node_traits::max_size(na_));
    const auto d = static_cast<size_type>(numeric_limits<difference_type>::max());
    return a < d ? a : d;
  }

  // ---- [forward.list.access] ----
  constexpr reference front() {
    ycxx::detail::precondition(head_.next != nullptr, "std::forward_list::front: empty list");
    return value(head_.next);
  }
  constexpr const_reference front() const {
    ycxx::detail::precondition(head_.next != nullptr, "std::forward_list::front: empty list");
    return value(head_.next);
  }

  // ---- [forward.list.modifiers] ----
  template <class... Args>
  constexpr reference emplace_front(Args&&... args) {
    return *emplace_after(cbefore_begin(), static_cast<Args&&>(args)...);
  }
  constexpr void push_front(const T& x) { emplace_after(cbefore_begin(), x); }
  constexpr void push_front(T&& x) { emplace_after(cbefore_begin(), static_cast<T&&>(x)); }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr void prepend_range(R&& rg) {
    insert_elems(head(), ranges::begin(rg), ranges::end(rg));
  }
  constexpr void pop_front() {
    ycxx::detail::precondition(head_.next != nullptr, "std::forward_list::pop_front: empty list");
    erase_after(cbefore_begin());
  }
  template <class... Args>
  constexpr iterator emplace_after(const_iterator position, Args&&... args) {
    node* n = make_node(static_cast<Args&&>(args)...);
    n->next = position.n_->next;
    position.n_->next = n;
    return iterator(n);
  }
  constexpr iterator insert_after(const_iterator position, const T& x) { return emplace_after(position, x); }
  constexpr iterator insert_after(const_iterator position, T&& x) {
    return emplace_after(position, static_cast<T&&>(x));
  }
  constexpr iterator insert_after(const_iterator position, size_type n, const T& x) {
    return iterator(insert_n(position.n_, n, x));
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr iterator insert_after(const_iterator position, InputIterator first, InputIterator last) {
    return iterator(
        insert_elems(position.n_, static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last)));
  }
  constexpr iterator insert_after(const_iterator position, initializer_list<T> il) {
    return iterator(insert_elems(position.n_, il.begin(), il.end()));
  }
  template <ycxx::detail::container_compatible_range<T> R>
  constexpr iterator insert_range_after(const_iterator position, R&& rg) {
    return iterator(insert_elems(position.n_, ranges::begin(rg), ranges::end(rg)));
  }
  constexpr iterator erase_after(const_iterator position) {
    node_base* p = position.n_;
    node_base* n = p->next;
    p->next = n->next;
    free_node(n);
    return iterator(p->next);
  }
  constexpr iterator erase_after(const_iterator position, const_iterator last) {
    if (position == last)
      return iterator(last.n_);
    node_base* p = position.n_;
    node_base* n = p->next;
    p->next = last.n_;
    while (n != last.n_) {
      node_base* nx = n->next;
      free_node(n);
      n = nx;
    }
    return iterator(last.n_);
  }
  constexpr void swap(forward_list& x) noexcept(always_equal) {
    if (this == __builtin_addressof(x))
      return;
    if constexpr (pocs)
      ::ycxx::detail::swap_adl::do_swap(na_, x.na_);
    else
      ycxx::detail::precondition(always_equal || na_ == x.na_,
                                 "std::forward_list::swap: unequal allocators that do not propagate");
    node_base* const t = head_.next;
    head_.next = x.head_.next;
    x.head_.next = t;
  }
  constexpr void resize(size_type sz) {
    node_base* prev = head();
    for (; sz > 0 && prev->next; --sz)
      prev = prev->next;
    if (sz == 0)
      erase_tail(prev);
    else
      insert_n(prev, sz);
  }
  constexpr void resize(size_type sz, const value_type& c) {
    node_base* prev = head();
    for (; sz > 0 && prev->next; --sz)
      prev = prev->next;
    if (sz == 0)
      erase_tail(prev);
    else
      insert_n(prev, sz, c);
  }
  constexpr void clear() noexcept { erase_tail(head()); }

  // ---- [forward.list.ops] ----
  constexpr void splice_after(const_iterator position, forward_list& x) {
    ycxx::detail::precondition(this != __builtin_addressof(x), "std::forward_list::splice_after: x is *this");
    check_alloc(x);
    node_base* f = x.head_.next;
    if (!f)
      return;
    node_base* l = f;
    while (l->next)
      l = l->next;
    x.head_.next = nullptr;
    l->next = position.n_->next;
    position.n_->next = f;
  }
  constexpr void splice_after(const_iterator position, forward_list&& x) { splice_after(position, x); }
  constexpr void splice_after(const_iterator position, forward_list& x, const_iterator i) {
    check_alloc(x);
    node_base* const pos = position.n_;
    node_base* const p = i.n_;
    node_base* const n = p->next;
    if (pos == p || pos == n)
      return;
    p->next = n->next;
    n->next = pos->next;
    pos->next = n;
  }
  constexpr void splice_after(const_iterator position, forward_list&& x, const_iterator i) {
    splice_after(position, x, i);
  }
  constexpr void splice_after(const_iterator position, forward_list& x, const_iterator first, const_iterator last) {
    check_alloc(x);
    if (first == last)
      return;
    node_base* const f = first.n_;
    node_base* const b = f->next;
    if (b == last.n_)
      return;
    node_base* e = b;
    while (e->next != last.n_)
      e = e->next;
    f->next = last.n_;
    e->next = position.n_->next;
    position.n_->next = b;
  }
  constexpr void splice_after(const_iterator position, forward_list&& x, const_iterator first,
                              const_iterator last) {
    splice_after(position, x, first, last);
  }

  constexpr size_type remove(const T& value) {
    return remove_if([&value](const T& e) -> bool { return e == value; });
  }
  template <class Predicate>
  constexpr size_type remove_if(Predicate pred) {
    // Unlinked nodes are destroyed at the end: value may refer to one of them.
    graveyard g{this};
    size_type count = 0;
    node_base* prev = head();
    while (node_base* p = prev->next) {
      if (pred(forward_list::value(p))) {
        prev->next = p->next;
        g.bury(p);
        ++count;
      } else {
        prev = p;
      }
    }
    return count;
  }
  constexpr size_type unique() { return unique(equal_to<>()); }
  template <class BinaryPredicate>
  constexpr size_type unique(BinaryPredicate binary_pred) {
    graveyard g{this};
    size_type count = 0;
    node_base* kept = head_.next; // the last element kept
    if (!kept)
      return 0;
    // Walk the original sequence: prev is p's original predecessor (erased nodes stay alive
    // until the end, so it can still be read).
    node_base* prev = kept;
    node_base* p = kept->next;
    while (p) {
      node_base* nx = p->next;
      if (binary_pred(value(p), value(prev))) {
        kept->next = nx;
        g.bury(p);
        ++count;
      } else {
        kept = p;
      }
      prev = p;
      p = nx;
    }
    return count;
  }
  constexpr void merge(forward_list& x) { merge(x, less<>()); }
  constexpr void merge(forward_list&& x) { merge(x, less<>()); }
  template <class Compare>
  constexpr void merge(forward_list& x, Compare comp) {
    if (this == __builtin_addressof(x))
      return;
    check_alloc(x);
    node_base* p = head();
    while (p->next && x.head_.next) {
      node_base* q = x.head_.next;
      if (comp(value(q), value(p->next))) {
        x.head_.next = q->next;
        q->next = p->next;
        p->next = q;
      }
      p = p->next;
    }
    if (x.head_.next) {
      p->next = x.head_.next;
      x.head_.next = nullptr;
    }
  }
  template <class Compare>
  constexpr void merge(forward_list&& x, Compare comp) {
    merge(x, comp);
  }
  constexpr void sort() { sort(less<>()); }
  template <class Compare>
  constexpr void sort(Compare comp) {
    node_base* first = head_.next;
    if (!first || !first->next)
      return;
    head_.next = nullptr;
    auto val = [](node_base* n) -> T& { return forward_list::value(n); };
    node_base* const h = head();
    auto finish = [h](node_base* c) { h->next = c; };
    ycxx::detail::sort_chain(first, val, comp, finish);
  }
  constexpr void reverse() noexcept {
    node_base* r = nullptr;
    node_base* p = head_.next;
    while (p) {
      node_base* nx = p->next;
      p->next = r;
      r = p;
      p = nx;
    }
    head_.next = r;
  }
};

// ---- deduction guides ----
template <class InputIterator, class Allocator = allocator<ycxx::detail::iter_value_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
forward_list(InputIterator, InputIterator, Allocator = Allocator())
    -> forward_list<ycxx::detail::iter_value_type<InputIterator>, Allocator>;
template <ranges::input_range R, class Allocator = allocator<ranges::range_value_t<R>>>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
forward_list(from_range_t, R&&, Allocator = Allocator()) -> forward_list<ranges::range_value_t<R>, Allocator>;

// ---- comparisons ----
template <class T, class Allocator>
constexpr bool operator==(const forward_list<T, Allocator>& x, const forward_list<T, Allocator>& y) {
  auto i = x.begin(), j = y.begin();
  const auto ie = x.end(), je = y.end();
  for (; i != ie && j != je; ++i, (void)++j)
    if (!(*i == *j))
      return false;
  return i == ie && j == je;
}
template <class T, class Allocator>
constexpr ycxx::detail::synth_three_way_result<T> operator<=>(const forward_list<T, Allocator>& x,
                                                              const forward_list<T, Allocator>& y) {
  return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(),
                                                ycxx::detail::synth_three_way);
}

template <class T, class Allocator>
constexpr void swap(forward_list<T, Allocator>& x, forward_list<T, Allocator>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

// ---- [forward.list.erasure] ----
template <class T, class Allocator, class Predicate>
constexpr typename forward_list<T, Allocator>::size_type erase_if(forward_list<T, Allocator>& c, Predicate pred) {
  return c.remove_if(pred);
}
template <class T, class Allocator, class U = T>
constexpr typename forward_list<T, Allocator>::size_type erase(forward_list<T, Allocator>& c, const U& value) {
  return c.remove_if([&value](const auto& elem) -> bool { return elem == value; });
}

namespace pmr {
template <class T>
using forward_list = std::forward_list<T, polymorphic_allocator<T>>;
} // namespace pmr

} // namespace std
