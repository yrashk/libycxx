// libycxx core: flat_set and flat_multiset ([flat.set], [flat.multiset]), their erasure,
// deduction guides and uses_allocator specializations.
//
// The keys live in one sequence container c_; flat_set_base holds it and the members both
// adaptors share. iterator and const_iterator are both the container's const_iterator. As for
// flat_map, every modification is covered by a flat_guard (flat_support.hpp) that restores the
// sorted invariant ([flat.set.overview]/5-6) if it exits via an exception.
#pragma once

#include <initializer_list>
#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/flat_support.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/tuple.hpp>
#include <ycxx/core/vector.hpp>

namespace ycxx::adl_free {

// The members flat_set (Multi false) and flat_multiset share.
template <class Key, class Compare, class KC, bool Multi>
class flat_set_base {
  static_assert(std::is_same_v<Key, typename KC::value_type>,
                "flat_set: Key must be KeyContainer::value_type ([flat.set.overview]/8)");

public:
  // ---- types ----
  using key_type = Key;
  using value_type = Key;
  using key_compare = Compare;
  using value_compare = Compare;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = typename KC::size_type;
  using difference_type = typename KC::difference_type;
  using iterator = typename KC::const_iterator;
  using const_iterator = typename KC::const_iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using container_type = KC;

protected:
  container_type c_;
  [[no_unique_address]] key_compare compare_;

  // ---- helpers ----
  template <class A, class B>
  constexpr bool lt(const A& a, const B& b) const {
    return static_cast<bool>(static_cast<const key_compare&>(compare_)(a, b));
  }
  constexpr auto less_fn() const noexcept {
    return [this](const auto& a, const auto& b) -> bool { return lt(a, b); };
  }
  constexpr const key_type& key_at(std::size_t i) const { return ::ycxx::detail::row_at(c_, i); }
  constexpr const_iterator it_at(std::size_t i) const noexcept {
    return c_.cbegin() + static_cast<difference_type>(i);
  }
  constexpr std::size_t index_of(const_iterator it) const noexcept {
    return static_cast<std::size_t>(it - c_.cbegin());
  }
  constexpr auto guard() noexcept { return ::ycxx::detail::flat_guard(c_); }

  template <class K>
  constexpr std::size_t lower_index(const K& x, std::size_t lo, std::size_t hi) const {
    std::size_t n = hi - lo;
    while (n > 0) {
      const std::size_t half = n / 2;
      if (lt(key_at(lo + half), x)) {
        lo += half + 1;
        n -= half + 1;
      } else {
        n = half;
      }
    }
    return lo;
  }
  template <class K>
  constexpr std::size_t lower_index(const K& x) const {
    return lower_index(x, 0, c_.size());
  }
  template <class K>
  constexpr std::size_t upper_index(const K& x, std::size_t lo, std::size_t hi) const {
    std::size_t n = hi - lo;
    while (n > 0) {
      const std::size_t half = n / 2;
      if (!lt(x, key_at(lo + half))) {
        lo += half + 1;
        n -= half + 1;
      } else {
        n = half;
      }
    }
    return lo;
  }
  template <class K>
  constexpr std::size_t upper_index(const K& x) const {
    return upper_index(x, 0, c_.size());
  }
  template <class K>
  constexpr std::size_t find_index(const K& x) const {
    const std::size_t i = lower_index(x);
    if (i == c_.size() || lt(x, key_at(i)))
      return c_.size();
    return i;
  }
  template <class K>
  constexpr std::pair<std::size_t, bool> unique_pos(const K& x) const {
    const std::size_t i = lower_index(x);
    return {i, i != c_.size() && !lt(x, key_at(i))};
  }
  template <class K>
  constexpr std::pair<std::size_t, bool> unique_pos_hint(const_iterator hint, const K& x) const {
    const std::size_t h = index_of(hint);
    if ((h == 0 || lt(key_at(h - 1), x)) && (h == c_.size() || lt(x, key_at(h))))
      return {h, false};
    return unique_pos(x);
  }
  template <class K>
  constexpr std::size_t multi_pos_hint(const_iterator hint, const K& x) const {
    const std::size_t h = index_of(hint);
    if (h > 0 && lt(x, key_at(h - 1)))
      return upper_index(x, 0, h - 1);
    if (h < c_.size() && lt(key_at(h), x))
      return lower_index(x, h + 1, c_.size());
    return h;
  }

  template <class... Args>
  constexpr iterator insert_at(std::size_t i, Args&&... args) {
    auto g = guard();
    c_.emplace(it_at(i), static_cast<Args&&>(args)...);
    g.release();
    return it_at(i);
  }
  constexpr std::pair<iterator, bool> insert_unique(value_type&& t) {
    const auto p = unique_pos(t);
    if (p.second)
      return {it_at(p.first), false};
    return {insert_at(p.first, static_cast<value_type&&>(t)), true};
  }
  constexpr iterator insert_unique_hint(const_iterator hint, value_type&& t) {
    const auto p = unique_pos_hint(hint, t);
    if (p.second)
      return it_at(p.first);
    return insert_at(p.first, static_cast<value_type&&>(t));
  }
  constexpr iterator insert_multi(value_type&& t) {
    return insert_at(upper_index(t), static_cast<value_type&&>(t));
  }
  constexpr iterator insert_multi_hint(const_iterator hint, value_type&& t) {
    return insert_at(multi_pos_hint(hint, t), static_cast<value_type&&>(t));
  }

  constexpr void sort_from(std::size_t from) {
    auto less = less_fn();
    ::ycxx::detail::sort_rows(less, from, c_);
    if constexpr (!Multi)
      ::ycxx::detail::unique_rows(less, c_);
  }
  constexpr void sort_all() {
    auto g = guard();
    sort_from(0);
    g.release();
  }
  // [flat.set.modifiers]/5: c.insert(c.end(), first, last), then sorts the new keys in.
  template <class It>
  constexpr void insert_elems(It first, It last) {
    const std::size_t old = c_.size();
    auto g = guard();
    c_.insert(c_.end(), first, last);
    sort_from(old);
    g.release();
  }
  // [flat.set.modifiers]/10: each element is inserted at the end, then the new keys sorted in.
  template <class R>
  constexpr void insert_range_elems(R&& rg) {
    const std::size_t old = c_.size();
    auto g = guard();
    for (auto&& e : rg)
      c_.insert(c_.end(), value_type(static_cast<decltype(e)&&>(e)));
    sort_from(old);
    g.release();
  }

  static constexpr container_type take(flat_set_base& o) {
    auto g = o.guard();
    return static_cast<container_type&&>(o.c_);
  }
  template <class A>
  static constexpr container_type take(flat_set_base& o, const A& a) {
    auto g = o.guard();
    return std::make_obj_using_allocator<container_type>(a, static_cast<container_type&&>(o.c_));
  }

  // ---- construction and assignment ----
  constexpr explicit flat_set_base(const key_compare& comp) : c_(), compare_(comp) {}
  template <class A>
  constexpr flat_set_base(const key_compare& comp, const A& a)
      : c_(std::make_obj_using_allocator<container_type>(a)), compare_(comp) {}
  constexpr flat_set_base(container_type&& c, const key_compare& comp)
      : c_(static_cast<container_type&&>(c)), compare_(comp) {}
  template <class A>
  constexpr flat_set_base(const container_type& c, const key_compare& comp, const A& a)
      : c_(std::make_obj_using_allocator<container_type>(a, c)), compare_(comp) {}
  template <class It>
  constexpr flat_set_base(It first, It last, const key_compare& comp) : c_(first, last), compare_(comp) {}
  template <class It, class A>
  constexpr flat_set_base(It first, It last, const key_compare& comp, const A& a)
      : c_(std::make_obj_using_allocator<container_type>(a, first, last)), compare_(comp) {}
  constexpr flat_set_base(const flat_set_base&) = default;
  constexpr flat_set_base(flat_set_base&& o) : c_(take(o)), compare_(o.compare_) {}
  template <class A>
  constexpr flat_set_base(const flat_set_base& o, const A& a)
      : c_(std::make_obj_using_allocator<container_type>(a, o.c_)), compare_(o.compare_) {}
  template <class A>
  constexpr flat_set_base(flat_set_base&& o, const A& a) : c_(take(o, a)), compare_(o.compare_) {}

  constexpr void copy_assign(const flat_set_base& o) {
    if (this == __builtin_addressof(o))
      return;
    auto g = guard();
    c_ = o.c_;
    compare_ = o.compare_;
    g.release();
  }
  constexpr void move_assign(flat_set_base& o) {
    if (this == __builtin_addressof(o))
      return;
    auto go = o.guard(); // o is emptied in any case
    auto g = guard();
    c_ = static_cast<container_type&&>(o.c_);
    compare_ = static_cast<key_compare&&>(o.compare_);
    g.release();
  }
  constexpr void swap_impl(flat_set_base& y) {
    auto g = guard();
    auto gy = y.guard();
    ::ycxx::detail::swap_adl::do_swap(compare_, y.compare_);
    ::ycxx::detail::swap_adl::do_swap(c_, y.c_);
    gy.release();
    g.release();
  }

public:
  // ---- iterators ----
  constexpr iterator begin() noexcept { return c_.cbegin(); }
  constexpr const_iterator begin() const noexcept { return c_.cbegin(); }
  constexpr iterator end() noexcept { return c_.cend(); }
  constexpr const_iterator end() const noexcept { return c_.cend(); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- capacity ----
  [[nodiscard]] constexpr bool empty() const noexcept { return c_.empty(); }
  constexpr size_type size() const noexcept { return c_.size(); }
  constexpr size_type max_size() const noexcept { return c_.max_size(); }

  // ---- modifiers ----
  constexpr container_type extract() && {
    auto g = guard();
    return static_cast<container_type&&>(c_);
  }
  constexpr void replace(container_type&& cont) {
    auto g = guard();
    c_ = static_cast<container_type&&>(cont);
    g.release();
  }
  constexpr iterator erase(const_iterator position) {
    const std::size_t i = index_of(position);
    ::ycxx::detail::precondition(i < c_.size(), "flat_set::erase: end() iterator");
    auto g = guard();
    c_.erase(position);
    g.release();
    return it_at(i);
  }
  constexpr size_type erase(const key_type& x) { return erase_key(x); }
  template <class K>
    requires ::ycxx::detail::transparent_non_iter<Compare, K, iterator, const_iterator>
  constexpr size_type erase(K&& x) {
    return erase_key(x);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    const std::size_t i = index_of(first);
    if (first != last) {
      auto g = guard();
      c_.erase(first, last);
      g.release();
    }
    return it_at(i);
  }
  constexpr void clear() noexcept { c_.clear(); }

  // ---- observers ----
  constexpr key_compare key_comp() const { return compare_; }
  constexpr value_compare value_comp() const { return compare_; }

  // ---- set operations ----
  constexpr iterator find(const key_type& x) { return it_at(find_index(x)); }
  constexpr const_iterator find(const key_type& x) const { return it_at(find_index(x)); }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr iterator find(const K& x) {
    return it_at(find_index(x));
  }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr const_iterator find(const K& x) const {
    return it_at(find_index(x));
  }
  constexpr size_type count(const key_type& x) const {
    if constexpr (Multi)
      return static_cast<size_type>(upper_index(x) - lower_index(x));
    else
      return find_index(x) == c_.size() ? 0 : 1;
  }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr size_type count(const K& x) const {
    return static_cast<size_type>(upper_index(x) - lower_index(x));
  }
  constexpr bool contains(const key_type& x) const { return find_index(x) != c_.size(); }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr bool contains(const K& x) const {
    return find_index(x) != c_.size();
  }
  constexpr iterator lower_bound(const key_type& x) { return it_at(lower_index(x)); }
  constexpr const_iterator lower_bound(const key_type& x) const { return it_at(lower_index(x)); }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr iterator lower_bound(const K& x) {
    return it_at(lower_index(x));
  }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr const_iterator lower_bound(const K& x) const {
    return it_at(lower_index(x));
  }
  constexpr iterator upper_bound(const key_type& x) { return it_at(upper_index(x)); }
  constexpr const_iterator upper_bound(const key_type& x) const { return it_at(upper_index(x)); }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr iterator upper_bound(const K& x) {
    return it_at(upper_index(x));
  }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr const_iterator upper_bound(const K& x) const {
    return it_at(upper_index(x));
  }
  constexpr std::pair<iterator, iterator> equal_range(const key_type& x) {
    return {it_at(lower_index(x)), it_at(upper_index(x))};
  }
  constexpr std::pair<const_iterator, const_iterator> equal_range(const key_type& x) const {
    return {it_at(lower_index(x)), it_at(upper_index(x))};
  }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr std::pair<iterator, iterator> equal_range(const K& x) {
    return {it_at(lower_index(x)), it_at(upper_index(x))};
  }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr std::pair<const_iterator, const_iterator> equal_range(const K& x) const {
    return {it_at(lower_index(x)), it_at(upper_index(x))};
  }

private:
  template <class K>
  constexpr size_type erase_key(const K& x) {
    const std::size_t i = lower_index(x), j = upper_index(x);
    erase(it_at(i), it_at(j));
    return static_cast<size_type>(j - i);
  }
};

} // namespace ycxx::adl_free

namespace std {

template <class Key, class Compare = less<Key>, class KeyContainer = vector<Key>>
class flat_set : public ycxx::adl_free::flat_set_base<Key, Compare, KeyContainer, false> {
  using base = ycxx::adl_free::flat_set_base<Key, Compare, KeyContainer, false>;

public:
  // ---- types ----
  using typename base::const_iterator;
  using typename base::const_reference;
  using typename base::const_reverse_iterator;
  using typename base::container_type;
  using typename base::difference_type;
  using typename base::iterator;
  using typename base::key_compare;
  using typename base::key_type;
  using typename base::reference;
  using typename base::reverse_iterator;
  using typename base::size_type;
  using typename base::value_compare;
  using typename base::value_type;

  // ---- [flat.set.cons] ----
  constexpr flat_set() : flat_set(key_compare()) {}
  constexpr flat_set(const flat_set&) = default;
  constexpr flat_set(flat_set&& x) : base(static_cast<base&&>(x)) {}
  constexpr flat_set& operator=(const flat_set& x) {
    this->copy_assign(x);
    return *this;
  }
  constexpr flat_set& operator=(flat_set&& x) {
    this->move_assign(x);
    return *this;
  }
  constexpr explicit flat_set(const key_compare& comp) : base(comp) {}
  constexpr explicit flat_set(container_type cont, const key_compare& comp = key_compare())
      : base(static_cast<container_type&&>(cont), comp) {
    this->sort_all();
  }
  constexpr flat_set(sorted_unique_t, container_type cont, const key_compare& comp = key_compare())
      : base(static_cast<container_type&&>(cont), comp) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr flat_set(InputIterator first, InputIterator last, const key_compare& comp = key_compare()) : base(comp) {
    this->insert_elems(first, last);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr flat_set(sorted_unique_t, InputIterator first, InputIterator last, const key_compare& comp = key_compare())
      : base(first, last, comp) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr flat_set(Tag, R&& rg) : flat_set(from_range, static_cast<R&&>(rg), key_compare()) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr flat_set(Tag, R&& rg, const key_compare& comp) : base(comp) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  constexpr flat_set(initializer_list<value_type> il, const key_compare& comp = key_compare())
      : flat_set(il.begin(), il.end(), comp) {}
  constexpr flat_set(sorted_unique_t, initializer_list<value_type> il, const key_compare& comp = key_compare())
      : flat_set(sorted_unique, il.begin(), il.end(), comp) {}

  // ---- [flat.set.cons.alloc] ----
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr explicit flat_set(const Alloc& a) : base(key_compare(), a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(const key_compare& comp, const Alloc& a) : base(comp, a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(const container_type& cont, const Alloc& a) : base(cont, key_compare(), a) {
    this->sort_all();
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(const container_type& cont, const key_compare& comp, const Alloc& a) : base(cont, comp, a) {
    this->sort_all();
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(sorted_unique_t, const container_type& cont, const Alloc& a) : base(cont, key_compare(), a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(sorted_unique_t, const container_type& cont, const key_compare& comp, const Alloc& a)
      : base(cont, comp, a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(const flat_set& x, const Alloc& a) : base(x, a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(flat_set&& x, const Alloc& a) : base(static_cast<base&&>(x), a) {}
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(InputIterator first, InputIterator last, const Alloc& a) : base(key_compare(), a) {
    this->insert_elems(first, last);
  }
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(InputIterator first, InputIterator last, const key_compare& comp, const Alloc& a)
      : base(comp, a) {
    this->insert_elems(first, last);
  }
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(sorted_unique_t, InputIterator first, InputIterator last, const Alloc& a)
      : base(first, last, key_compare(), a) {}
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(sorted_unique_t, InputIterator first, InputIterator last, const key_compare& comp,
                     const Alloc& a)
      : base(first, last, comp, a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R, class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(Tag, R&& rg, const Alloc& a) : base(key_compare(), a) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R, class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(Tag, R&& rg, const key_compare& comp, const Alloc& a) : base(comp, a) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(initializer_list<value_type> il, const Alloc& a) : base(key_compare(), a) {
    this->insert_elems(il.begin(), il.end());
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(initializer_list<value_type> il, const key_compare& comp, const Alloc& a) : base(comp, a) {
    this->insert_elems(il.begin(), il.end());
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(sorted_unique_t, initializer_list<value_type> il, const Alloc& a)
      : base(il.begin(), il.end(), key_compare(), a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_set(sorted_unique_t, initializer_list<value_type> il, const key_compare& comp, const Alloc& a)
      : base(il.begin(), il.end(), comp, a) {}

  constexpr flat_set& operator=(initializer_list<value_type> il) {
    this->clear();
    this->insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- [flat.set.modifiers] ----
  template <class... Args>
    requires is_constructible_v<value_type, Args...>
  constexpr pair<iterator, bool> emplace(Args&&... args) {
    return this->insert_unique(value_type(static_cast<Args&&>(args)...));
  }
  template <class... Args>
    requires is_constructible_v<value_type, Args...>
  constexpr iterator emplace_hint(const_iterator position, Args&&... args) {
    return this->insert_unique_hint(position, value_type(static_cast<Args&&>(args)...));
  }
  constexpr pair<iterator, bool> insert(const value_type& x) { return emplace(x); }
  constexpr pair<iterator, bool> insert(value_type&& x) { return emplace(static_cast<value_type&&>(x)); }
  template <class K>
    requires ycxx::detail::transparent_compare<Compare> && is_constructible_v<value_type, K>
  constexpr pair<iterator, bool> insert(K&& x) {
    const auto p = this->unique_pos(x);
    if (p.second)
      return {this->it_at(p.first), false};
    return {this->insert_at(p.first, static_cast<K&&>(x)), true};
  }
  constexpr iterator insert(const_iterator position, const value_type& x) { return emplace_hint(position, x); }
  constexpr iterator insert(const_iterator position, value_type&& x) {
    return emplace_hint(position, static_cast<value_type&&>(x));
  }
  template <class K>
    requires ycxx::detail::transparent_compare<Compare> && is_constructible_v<value_type, K>
  constexpr iterator insert(const_iterator hint, K&& x) {
    const auto p = this->unique_pos_hint(hint, x);
    if (p.second)
      return this->it_at(p.first);
    return this->insert_at(p.first, static_cast<K&&>(x));
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr void insert(InputIterator first, InputIterator last) {
    this->insert_elems(first, last);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr void insert(sorted_unique_t, InputIterator first, InputIterator last) {
    this->insert_elems(first, last);
  }
  template <ycxx::detail::container_compatible_range<value_type> R>
  constexpr void insert_range(R&& rg) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  template <ycxx::detail::container_compatible_range<value_type> R>
  constexpr void insert_range(sorted_unique_t, R&& rg) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  constexpr void insert(initializer_list<value_type> il) { insert(il.begin(), il.end()); }
  constexpr void insert(sorted_unique_t, initializer_list<value_type> il) {
    insert(sorted_unique, il.begin(), il.end());
  }

  constexpr void swap(flat_set& y) noexcept(is_nothrow_swappable_v<container_type> &&
                                            is_nothrow_swappable_v<key_compare>) {
    this->swap_impl(y);
  }

  friend constexpr bool operator==(const flat_set& x, const flat_set& y) {
    return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
  }
  // A template, so that the return type is formed only when the operator is used.
  template <class V = value_type>
  friend constexpr ycxx::detail::synth_three_way_result<V> operator<=>(const flat_set& x, const flat_set& y) {
    return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(),
                                                  ycxx::detail::synth_three_way);
  }
  friend constexpr void swap(flat_set& x, flat_set& y) noexcept(noexcept(x.swap(y))) { x.swap(y); }
};

template <class Key, class Compare = less<Key>, class KeyContainer = vector<Key>>
class flat_multiset : public ycxx::adl_free::flat_set_base<Key, Compare, KeyContainer, true> {
  using base = ycxx::adl_free::flat_set_base<Key, Compare, KeyContainer, true>;

public:
  // ---- types ----
  using typename base::const_iterator;
  using typename base::const_reference;
  using typename base::const_reverse_iterator;
  using typename base::container_type;
  using typename base::difference_type;
  using typename base::iterator;
  using typename base::key_compare;
  using typename base::key_type;
  using typename base::reference;
  using typename base::reverse_iterator;
  using typename base::size_type;
  using typename base::value_compare;
  using typename base::value_type;

  // ---- [flat.multiset.cons] ----
  constexpr flat_multiset() : flat_multiset(key_compare()) {}
  constexpr flat_multiset(const flat_multiset&) = default;
  constexpr flat_multiset(flat_multiset&& x) : base(static_cast<base&&>(x)) {}
  constexpr flat_multiset& operator=(const flat_multiset& x) {
    this->copy_assign(x);
    return *this;
  }
  constexpr flat_multiset& operator=(flat_multiset&& x) {
    this->move_assign(x);
    return *this;
  }
  constexpr explicit flat_multiset(const key_compare& comp) : base(comp) {}
  constexpr explicit flat_multiset(container_type cont, const key_compare& comp = key_compare())
      : base(static_cast<container_type&&>(cont), comp) {
    this->sort_all();
  }
  constexpr flat_multiset(sorted_equivalent_t, container_type cont, const key_compare& comp = key_compare())
      : base(static_cast<container_type&&>(cont), comp) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr flat_multiset(InputIterator first, InputIterator last, const key_compare& comp = key_compare())
      : base(comp) {
    this->insert_elems(first, last);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr flat_multiset(sorted_equivalent_t, InputIterator first, InputIterator last,
                          const key_compare& comp = key_compare())
      : base(first, last, comp) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr flat_multiset(Tag, R&& rg) : flat_multiset(from_range, static_cast<R&&>(rg), key_compare()) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr flat_multiset(Tag, R&& rg, const key_compare& comp) : base(comp) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  constexpr flat_multiset(initializer_list<value_type> il, const key_compare& comp = key_compare())
      : flat_multiset(il.begin(), il.end(), comp) {}
  constexpr flat_multiset(sorted_equivalent_t, initializer_list<value_type> il,
                          const key_compare& comp = key_compare())
      : flat_multiset(sorted_equivalent, il.begin(), il.end(), comp) {}

  // ---- [flat.multiset.cons.alloc] ----
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr explicit flat_multiset(const Alloc& a) : base(key_compare(), a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(const key_compare& comp, const Alloc& a) : base(comp, a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(const container_type& cont, const Alloc& a) : base(cont, key_compare(), a) {
    this->sort_all();
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(const container_type& cont, const key_compare& comp, const Alloc& a)
      : base(cont, comp, a) {
    this->sort_all();
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(sorted_equivalent_t, const container_type& cont, const Alloc& a)
      : base(cont, key_compare(), a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(sorted_equivalent_t, const container_type& cont, const key_compare& comp, const Alloc& a)
      : base(cont, comp, a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(const flat_multiset& x, const Alloc& a) : base(x, a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(flat_multiset&& x, const Alloc& a) : base(static_cast<base&&>(x), a) {}
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(InputIterator first, InputIterator last, const Alloc& a) : base(key_compare(), a) {
    this->insert_elems(first, last);
  }
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(InputIterator first, InputIterator last, const key_compare& comp, const Alloc& a)
      : base(comp, a) {
    this->insert_elems(first, last);
  }
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(sorted_equivalent_t, InputIterator first, InputIterator last, const Alloc& a)
      : base(first, last, key_compare(), a) {}
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(sorted_equivalent_t, InputIterator first, InputIterator last, const key_compare& comp,
                          const Alloc& a)
      : base(first, last, comp, a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R, class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(Tag, R&& rg, const Alloc& a) : base(key_compare(), a) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R, class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(Tag, R&& rg, const key_compare& comp, const Alloc& a) : base(comp, a) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(initializer_list<value_type> il, const Alloc& a) : base(key_compare(), a) {
    this->insert_elems(il.begin(), il.end());
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(initializer_list<value_type> il, const key_compare& comp, const Alloc& a)
      : base(comp, a) {
    this->insert_elems(il.begin(), il.end());
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(sorted_equivalent_t, initializer_list<value_type> il, const Alloc& a)
      : base(il.begin(), il.end(), key_compare(), a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer>
  constexpr flat_multiset(sorted_equivalent_t, initializer_list<value_type> il, const key_compare& comp,
                          const Alloc& a)
      : base(il.begin(), il.end(), comp, a) {}

  constexpr flat_multiset& operator=(initializer_list<value_type> il) {
    this->clear();
    this->insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- [flat.multiset.modifiers] ----
  template <class... Args>
    requires is_constructible_v<value_type, Args...>
  constexpr iterator emplace(Args&&... args) {
    return this->insert_multi(value_type(static_cast<Args&&>(args)...));
  }
  template <class... Args>
    requires is_constructible_v<value_type, Args...>
  constexpr iterator emplace_hint(const_iterator position, Args&&... args) {
    return this->insert_multi_hint(position, value_type(static_cast<Args&&>(args)...));
  }
  constexpr iterator insert(const value_type& x) { return emplace(x); }
  constexpr iterator insert(value_type&& x) { return emplace(static_cast<value_type&&>(x)); }
  constexpr iterator insert(const_iterator position, const value_type& x) { return emplace_hint(position, x); }
  constexpr iterator insert(const_iterator position, value_type&& x) {
    return emplace_hint(position, static_cast<value_type&&>(x));
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr void insert(InputIterator first, InputIterator last) {
    this->insert_elems(first, last);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr void insert(sorted_equivalent_t, InputIterator first, InputIterator last) {
    this->insert_elems(first, last);
  }
  template <ycxx::detail::container_compatible_range<value_type> R>
  constexpr void insert_range(R&& rg) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  template <ycxx::detail::container_compatible_range<value_type> R>
  constexpr void insert_range(sorted_equivalent_t, R&& rg) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  constexpr void insert(initializer_list<value_type> il) { insert(il.begin(), il.end()); }
  constexpr void insert(sorted_equivalent_t, initializer_list<value_type> il) {
    insert(sorted_equivalent, il.begin(), il.end());
  }

  constexpr void swap(flat_multiset& y) noexcept(is_nothrow_swappable_v<container_type> &&
                                                 is_nothrow_swappable_v<key_compare>) {
    this->swap_impl(y);
  }

  friend constexpr bool operator==(const flat_multiset& x, const flat_multiset& y) {
    return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
  }
  // A template, so that the return type is formed only when the operator is used.
  template <class V = value_type>
  friend constexpr ycxx::detail::synth_three_way_result<V> operator<=>(const flat_multiset& x, const flat_multiset& y) {
    return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(),
                                                  ycxx::detail::synth_three_way);
  }
  friend constexpr void swap(flat_multiset& x, flat_multiset& y) noexcept(noexcept(x.swap(y))) { x.swap(y); }
};

// ---- deduction guides ([container.adaptors.general]/6) ----
template <class KeyContainer, class Compare = less<typename KeyContainer::value_type>>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_compare_for<Compare, KeyContainer>
flat_set(KeyContainer, Compare = Compare())
    -> flat_set<typename KeyContainer::value_type, Compare, KeyContainer>;
template <class KeyContainer, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && uses_allocator_v<KeyContainer, Allocator>
flat_set(KeyContainer, Allocator)
    -> flat_set<typename KeyContainer::value_type, less<typename KeyContainer::value_type>, KeyContainer>;
template <class KeyContainer, class Compare, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_compare_for<Compare, KeyContainer> &&
           uses_allocator_v<KeyContainer, Allocator>
flat_set(KeyContainer, Compare, Allocator) -> flat_set<typename KeyContainer::value_type, Compare, KeyContainer>;
template <class KeyContainer, class Compare = less<typename KeyContainer::value_type>>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_compare_for<Compare, KeyContainer>
flat_set(sorted_unique_t, KeyContainer, Compare = Compare())
    -> flat_set<typename KeyContainer::value_type, Compare, KeyContainer>;
template <class KeyContainer, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && uses_allocator_v<KeyContainer, Allocator>
flat_set(sorted_unique_t, KeyContainer, Allocator)
    -> flat_set<typename KeyContainer::value_type, less<typename KeyContainer::value_type>, KeyContainer>;
template <class KeyContainer, class Compare, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_compare_for<Compare, KeyContainer> &&
           uses_allocator_v<KeyContainer, Allocator>
flat_set(sorted_unique_t, KeyContainer, Compare, Allocator)
    -> flat_set<typename KeyContainer::value_type, Compare, KeyContainer>;
template <class InputIterator, class Compare = less<ycxx::detail::iter_value_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::deducible_compare<Compare>
flat_set(InputIterator, InputIterator, Compare = Compare())
    -> flat_set<ycxx::detail::iter_value_type<InputIterator>, Compare>;
template <class InputIterator, class Compare = less<ycxx::detail::iter_value_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::deducible_compare<Compare>
flat_set(sorted_unique_t, InputIterator, InputIterator, Compare = Compare())
    -> flat_set<ycxx::detail::iter_value_type<InputIterator>, Compare>;
template <ranges::input_range R, class Compare = less<ranges::range_value_t<R>>,
          class Allocator = allocator<ranges::range_value_t<R>>>
  requires ycxx::detail::deducible_compare<Compare> && ycxx::detail::qualifies_as_allocator<Allocator>
flat_set(from_range_t, R&&, Compare = Compare(), Allocator = Allocator())
    -> flat_set<ranges::range_value_t<R>, Compare,
                vector<ranges::range_value_t<R>,
                       ycxx::detail::rebound_alloc<Allocator, ranges::range_value_t<R>>>>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
flat_set(from_range_t, R&&, Allocator)
    -> flat_set<ranges::range_value_t<R>, less<ranges::range_value_t<R>>,
                vector<ranges::range_value_t<R>,
                       ycxx::detail::rebound_alloc<Allocator, ranges::range_value_t<R>>>>;
template <class Key, class Compare = less<Key>>
  requires ycxx::detail::deducible_compare<Compare>
flat_set(initializer_list<Key>, Compare = Compare()) -> flat_set<Key, Compare>;
template <class Key, class Compare = less<Key>>
  requires ycxx::detail::deducible_compare<Compare>
flat_set(sorted_unique_t, initializer_list<Key>, Compare = Compare()) -> flat_set<Key, Compare>;

template <class KeyContainer, class Compare = less<typename KeyContainer::value_type>>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_compare_for<Compare, KeyContainer>
flat_multiset(KeyContainer, Compare = Compare())
    -> flat_multiset<typename KeyContainer::value_type, Compare, KeyContainer>;
template <class KeyContainer, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && uses_allocator_v<KeyContainer, Allocator>
flat_multiset(KeyContainer, Allocator)
    -> flat_multiset<typename KeyContainer::value_type, less<typename KeyContainer::value_type>, KeyContainer>;
template <class KeyContainer, class Compare, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_compare_for<Compare, KeyContainer> &&
           uses_allocator_v<KeyContainer, Allocator>
flat_multiset(KeyContainer, Compare, Allocator)
    -> flat_multiset<typename KeyContainer::value_type, Compare, KeyContainer>;
template <class KeyContainer, class Compare = less<typename KeyContainer::value_type>>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_compare_for<Compare, KeyContainer>
flat_multiset(sorted_equivalent_t, KeyContainer, Compare = Compare())
    -> flat_multiset<typename KeyContainer::value_type, Compare, KeyContainer>;
template <class KeyContainer, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && uses_allocator_v<KeyContainer, Allocator>
flat_multiset(sorted_equivalent_t, KeyContainer, Allocator)
    -> flat_multiset<typename KeyContainer::value_type, less<typename KeyContainer::value_type>, KeyContainer>;
template <class KeyContainer, class Compare, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_compare_for<Compare, KeyContainer> &&
           uses_allocator_v<KeyContainer, Allocator>
flat_multiset(sorted_equivalent_t, KeyContainer, Compare, Allocator)
    -> flat_multiset<typename KeyContainer::value_type, Compare, KeyContainer>;
template <class InputIterator, class Compare = less<ycxx::detail::iter_value_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::deducible_compare<Compare>
flat_multiset(InputIterator, InputIterator, Compare = Compare())
    -> flat_multiset<ycxx::detail::iter_value_type<InputIterator>, Compare>;
template <class InputIterator, class Compare = less<ycxx::detail::iter_value_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::deducible_compare<Compare>
flat_multiset(sorted_equivalent_t, InputIterator, InputIterator, Compare = Compare())
    -> flat_multiset<ycxx::detail::iter_value_type<InputIterator>, Compare>;
template <ranges::input_range R, class Compare = less<ranges::range_value_t<R>>,
          class Allocator = allocator<ranges::range_value_t<R>>>
  requires ycxx::detail::deducible_compare<Compare> && ycxx::detail::qualifies_as_allocator<Allocator>
flat_multiset(from_range_t, R&&, Compare = Compare(), Allocator = Allocator())
    -> flat_multiset<ranges::range_value_t<R>, Compare,
                     vector<ranges::range_value_t<R>,
                            ycxx::detail::rebound_alloc<Allocator, ranges::range_value_t<R>>>>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
flat_multiset(from_range_t, R&&, Allocator)
    -> flat_multiset<ranges::range_value_t<R>, less<ranges::range_value_t<R>>,
                     vector<ranges::range_value_t<R>,
                            ycxx::detail::rebound_alloc<Allocator, ranges::range_value_t<R>>>>;
template <class Key, class Compare = less<Key>>
  requires ycxx::detail::deducible_compare<Compare>
flat_multiset(initializer_list<Key>, Compare = Compare()) -> flat_multiset<Key, Compare>;
template <class Key, class Compare = less<Key>>
  requires ycxx::detail::deducible_compare<Compare>
flat_multiset(sorted_equivalent_t, initializer_list<Key>, Compare = Compare()) -> flat_multiset<Key, Compare>;

// ---- uses_allocator ----
template <class Key, class Compare, class KeyContainer, class Allocator>
struct uses_allocator<flat_set<Key, Compare, KeyContainer>, Allocator>
    : bool_constant<uses_allocator_v<KeyContainer, Allocator>> {};
template <class Key, class Compare, class KeyContainer, class Allocator>
struct uses_allocator<flat_multiset<Key, Compare, KeyContainer>, Allocator>
    : bool_constant<uses_allocator_v<KeyContainer, Allocator>> {};

// ---- [flat.set.erasure], [flat.multiset.erasure] ----
template <class Key, class Compare, class KeyContainer, class Predicate>
constexpr typename flat_set<Key, Compare, KeyContainer>::size_type erase_if(flat_set<Key, Compare, KeyContainer>& c,
                                                                            Predicate pred) {
  // The container is taken out (c is empty if pred throws) and put back.
  KeyContainer cont = static_cast<flat_set<Key, Compare, KeyContainer>&&>(c).extract();
  auto test = [&](size_t i) -> bool {
    return static_cast<bool>(pred(static_cast<const Key&>(ycxx::detail::row_at(cont, i))));
  };
  const auto n = ycxx::detail::erase_rows_if(test, cont);
  c.replace(static_cast<KeyContainer&&>(cont));
  return static_cast<typename flat_set<Key, Compare, KeyContainer>::size_type>(n);
}
template <class Key, class Compare, class KeyContainer, class Predicate>
constexpr typename flat_multiset<Key, Compare, KeyContainer>::size_type
erase_if(flat_multiset<Key, Compare, KeyContainer>& c, Predicate pred) {
  KeyContainer cont = static_cast<flat_multiset<Key, Compare, KeyContainer>&&>(c).extract();
  auto test = [&](size_t i) -> bool {
    return static_cast<bool>(pred(static_cast<const Key&>(ycxx::detail::row_at(cont, i))));
  };
  const auto n = ycxx::detail::erase_rows_if(test, cont);
  c.replace(static_cast<KeyContainer&&>(cont));
  return static_cast<typename flat_multiset<Key, Compare, KeyContainer>::size_type>(n);
}

} // namespace std
