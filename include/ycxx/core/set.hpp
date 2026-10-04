// libycxx core: set and multiset ([set], [multiset]), their comparisons, erasure, deduction
// guides and the pmr:: aliases. Both are thin layers over the red-black tree of rb_tree.hpp;
// iterator and const_iterator are the same (constant) iterator type.
#pragma once

#include <initializer_list>
#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/memory_resource_fwd.hpp>
#include <ycxx/core/rb_tree.hpp>

namespace std {

template <class Key, class Compare = less<Key>, class Allocator = allocator<Key>>
class set;
template <class Key, class Compare = less<Key>, class Allocator = allocator<Key>>
class multiset;

template <class Key, class Compare, class Allocator>
class set : public ycxx::adl_free::rb_tree<Key, Compare, Allocator, false, false> {
  static_assert(ycxx::detail::allocator_for<Allocator, Key>,
                "std::set: Allocator::value_type must be Key ([container.alloc.reqmts])");

  using base = ycxx::adl_free::rb_tree<Key, Compare, Allocator, false, false>;
  using alloc_traits = allocator_traits<Allocator>;
  using node_base = typename base::node_base;
  static constexpr bool always_equal = base::always_equal;

public:
  // ---- types ----
  using key_type = Key;
  using key_compare = Compare;
  using value_type = Key;
  using value_compare = Compare;
  using allocator_type = Allocator;
  using typename base::const_pointer;
  using typename base::const_reference;
  using typename base::const_iterator;
  using typename base::const_reverse_iterator;
  using typename base::difference_type;
  using typename base::iterator;
  using typename base::node_type;
  using typename base::pointer;
  using typename base::reference;
  using typename base::reverse_iterator;
  using typename base::size_type;
  using insert_return_type = ycxx::adl_free::insert_return_type<iterator, node_type>;

  // ---- [set.cons] ----
  constexpr set() : set(Compare()) {}
  constexpr explicit set(const Compare& comp, const Allocator& a = Allocator()) : base(comp, a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr set(InputIterator first, InputIterator last, const Compare& comp = Compare(),
                const Allocator& a = Allocator())
      : base(comp, a) {
    this->insert_elems(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr set(Tag, R&& rg, const Compare& comp = Compare(), const Allocator& a = Allocator()) : base(comp, a) {
    this->insert_elems(ranges::begin(rg), ranges::end(rg));
  }
  constexpr set(const set& x) : base(x, alloc_traits::select_on_container_copy_construction(x.get_allocator())) {}
  constexpr set(set&& x) noexcept(is_nothrow_copy_constructible_v<Compare>) : base(static_cast<base&&>(x)) {}
  constexpr explicit set(const Allocator& a) : base(Compare(), a) {}
  constexpr set(const set& x, const type_identity_t<Allocator>& a) : base(x, a) {}
  constexpr set(set&& x, const type_identity_t<Allocator>& a) : base(static_cast<base&&>(x), a) {}
  constexpr set(initializer_list<value_type> il, const Compare& comp = Compare(), const Allocator& a = Allocator())
      : base(comp, a) {
    this->insert_elems(il.begin(), il.end());
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr set(InputIterator first, InputIterator last, const Allocator& a)
      : set(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last), Compare(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr set(Tag, R&& rg, const Allocator& a) : set(from_range, static_cast<R&&>(rg), Compare(), a) {}
  constexpr set(initializer_list<value_type> il, const Allocator& a) : set(il, Compare(), a) {}
  constexpr ~set() = default;

  constexpr set& operator=(const set& x) {
    this->copy_assign(x);
    return *this;
  }
  constexpr set& operator=(set&& x) noexcept(always_equal && is_nothrow_move_assignable_v<Compare>) {
    this->move_assign(x);
    return *this;
  }
  constexpr set& operator=(initializer_list<value_type> il) {
    this->clear();
    this->insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- [set.modifiers] ----
  template <class... Args>
  constexpr pair<iterator, bool> emplace(Args&&... args) {
    auto r = this->emplace_unique(static_cast<Args&&>(args)...);
    return {iterator(r.first), r.second};
  }
  template <class... Args>
  constexpr iterator emplace_hint(const_iterator position, Args&&... args) {
    return iterator(this->emplace_hint_unique(base::nd(position), static_cast<Args&&>(args)...));
  }
  constexpr pair<iterator, bool> insert(const value_type& x) { return emplace(x); }
  constexpr pair<iterator, bool> insert(value_type&& x) { return emplace(static_cast<value_type&&>(x)); }
  template <class K>
    requires ycxx::detail::transparent_compare<Compare>
  constexpr pair<iterator, bool> insert(K&& x) {
    auto r = this->emplace_key(nullptr, x, static_cast<K&&>(x));
    return {iterator(r.first), r.second};
  }
  constexpr iterator insert(const_iterator position, const value_type& x) { return emplace_hint(position, x); }
  constexpr iterator insert(const_iterator position, value_type&& x) {
    return emplace_hint(position, static_cast<value_type&&>(x));
  }
  template <class K>
    requires ycxx::detail::transparent_non_iter<Compare, K, iterator, const_iterator>
  constexpr iterator insert(const_iterator position, K&& x) {
    node_base* h = base::nd(position);
    return iterator(this->emplace_key(h ? h : this->header(), x, static_cast<K&&>(x)).first);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr void insert(InputIterator first, InputIterator last) {
    this->insert_elems(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
  }
  template <ycxx::detail::container_compatible_range<value_type> R>
  constexpr void insert_range(R&& rg) {
    this->insert_elems(ranges::begin(rg), ranges::end(rg));
  }
  constexpr void insert(initializer_list<value_type> il) { this->insert_elems(il.begin(), il.end()); }
  constexpr insert_return_type insert(node_type&& nh) {
    auto r = this->insert_handle_unique(nullptr, nh);
    return {iterator(r.first), r.second, static_cast<node_type&&>(nh)};
  }
  constexpr iterator insert(const_iterator hint, node_type&& nh) {
    node_base* h = base::nd(hint);
    return iterator(this->insert_handle_unique(h ? h : this->header(), nh).first);
  }

  constexpr void swap(set& x) noexcept(always_equal && is_nothrow_swappable_v<Compare>) { this->swap_tree(x); }

  template <class C2>
  constexpr void merge(set<Key, C2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class C2>
  constexpr void merge(set<Key, C2, Allocator>&& source) {
    this->merge_from(source);
  }
  template <class C2>
  constexpr void merge(multiset<Key, C2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class C2>
  constexpr void merge(multiset<Key, C2, Allocator>&& source) {
    this->merge_from(source);
  }

  // ---- observers ----
  constexpr value_compare value_comp() const { return this->comp_; }
};

template <class Key, class Compare, class Allocator>
class multiset : public ycxx::adl_free::rb_tree<Key, Compare, Allocator, false, true> {
  static_assert(ycxx::detail::allocator_for<Allocator, Key>,
                "std::multiset: Allocator::value_type must be Key ([container.alloc.reqmts])");

  using base = ycxx::adl_free::rb_tree<Key, Compare, Allocator, false, true>;
  using alloc_traits = allocator_traits<Allocator>;
  using node_base = typename base::node_base;
  static constexpr bool always_equal = base::always_equal;

public:
  // ---- types ----
  using key_type = Key;
  using key_compare = Compare;
  using value_type = Key;
  using value_compare = Compare;
  using allocator_type = Allocator;
  using typename base::const_pointer;
  using typename base::const_reference;
  using typename base::const_iterator;
  using typename base::const_reverse_iterator;
  using typename base::difference_type;
  using typename base::iterator;
  using typename base::node_type;
  using typename base::pointer;
  using typename base::reference;
  using typename base::reverse_iterator;
  using typename base::size_type;

  // ---- [multiset.cons] ----
  constexpr multiset() : multiset(Compare()) {}
  constexpr explicit multiset(const Compare& comp, const Allocator& a = Allocator()) : base(comp, a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr multiset(InputIterator first, InputIterator last, const Compare& comp = Compare(),
                     const Allocator& a = Allocator())
      : base(comp, a) {
    this->insert_elems(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr multiset(Tag, R&& rg, const Compare& comp = Compare(), const Allocator& a = Allocator())
      : base(comp, a) {
    this->insert_elems(ranges::begin(rg), ranges::end(rg));
  }
  constexpr multiset(const multiset& x)
      : base(x, alloc_traits::select_on_container_copy_construction(x.get_allocator())) {}
  constexpr multiset(multiset&& x) noexcept(is_nothrow_copy_constructible_v<Compare>) : base(static_cast<base&&>(x)) {}
  constexpr explicit multiset(const Allocator& a) : base(Compare(), a) {}
  constexpr multiset(const multiset& x, const type_identity_t<Allocator>& a) : base(x, a) {}
  constexpr multiset(multiset&& x, const type_identity_t<Allocator>& a) : base(static_cast<base&&>(x), a) {}
  constexpr multiset(initializer_list<value_type> il, const Compare& comp = Compare(),
                     const Allocator& a = Allocator())
      : base(comp, a) {
    this->insert_elems(il.begin(), il.end());
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr multiset(InputIterator first, InputIterator last, const Allocator& a)
      : multiset(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last), Compare(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr multiset(Tag, R&& rg, const Allocator& a) : multiset(from_range, static_cast<R&&>(rg), Compare(), a) {}
  constexpr multiset(initializer_list<value_type> il, const Allocator& a) : multiset(il, Compare(), a) {}
  constexpr ~multiset() = default;

  constexpr multiset& operator=(const multiset& x) {
    this->copy_assign(x);
    return *this;
  }
  constexpr multiset& operator=(multiset&& x) noexcept(always_equal && is_nothrow_move_assignable_v<Compare>) {
    this->move_assign(x);
    return *this;
  }
  constexpr multiset& operator=(initializer_list<value_type> il) {
    this->clear();
    this->insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- modifiers ----
  template <class... Args>
  constexpr iterator emplace(Args&&... args) {
    return iterator(this->emplace_multi(static_cast<Args&&>(args)...));
  }
  template <class... Args>
  constexpr iterator emplace_hint(const_iterator position, Args&&... args) {
    return iterator(this->emplace_hint_multi(base::nd(position), static_cast<Args&&>(args)...));
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
    this->insert_elems(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
  }
  template <ycxx::detail::container_compatible_range<value_type> R>
  constexpr void insert_range(R&& rg) {
    this->insert_elems(ranges::begin(rg), ranges::end(rg));
  }
  constexpr void insert(initializer_list<value_type> il) { this->insert_elems(il.begin(), il.end()); }
  constexpr iterator insert(node_type&& nh) { return iterator(this->insert_handle_multi(nullptr, nh)); }
  constexpr iterator insert(const_iterator hint, node_type&& nh) {
    node_base* h = base::nd(hint);
    return iterator(this->insert_handle_multi(h ? h : this->header(), nh));
  }

  constexpr void swap(multiset& x) noexcept(always_equal && is_nothrow_swappable_v<Compare>) { this->swap_tree(x); }

  template <class C2>
  constexpr void merge(multiset<Key, C2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class C2>
  constexpr void merge(multiset<Key, C2, Allocator>&& source) {
    this->merge_from(source);
  }
  template <class C2>
  constexpr void merge(set<Key, C2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class C2>
  constexpr void merge(set<Key, C2, Allocator>&& source) {
    this->merge_from(source);
  }

  // ---- observers ----
  constexpr value_compare value_comp() const { return this->comp_; }
};

// ---- deduction guides ----
template <class InputIterator, class Compare = less<ycxx::detail::iter_value_type<InputIterator>>,
          class Allocator = allocator<ycxx::detail::iter_value_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::deducible_compare<Compare> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
set(InputIterator, InputIterator, Compare = Compare(), Allocator = Allocator())
    -> set<ycxx::detail::iter_value_type<InputIterator>, Compare, Allocator>;
template <ranges::input_range R, class Compare = less<ranges::range_value_t<R>>,
          class Allocator = allocator<ranges::range_value_t<R>>>
  requires ycxx::detail::deducible_compare<Compare> && ycxx::detail::qualifies_as_allocator<Allocator>
set(from_range_t, R&&, Compare = Compare(), Allocator = Allocator()) -> set<ranges::range_value_t<R>, Compare, Allocator>;
template <class Key, class Compare = less<Key>, class Allocator = allocator<Key>>
  requires ycxx::detail::deducible_compare<Compare> && ycxx::detail::qualifies_as_allocator<Allocator>
set(initializer_list<Key>, Compare = Compare(), Allocator = Allocator()) -> set<Key, Compare, Allocator>;
template <class InputIterator, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
set(InputIterator, InputIterator, Allocator)
    -> set<ycxx::detail::iter_value_type<InputIterator>, less<ycxx::detail::iter_value_type<InputIterator>>, Allocator>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
set(from_range_t, R&&, Allocator) -> set<ranges::range_value_t<R>, less<ranges::range_value_t<R>>, Allocator>;
template <class Key, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
set(initializer_list<Key>, Allocator) -> set<Key, less<Key>, Allocator>;

template <class InputIterator, class Compare = less<ycxx::detail::iter_value_type<InputIterator>>,
          class Allocator = allocator<ycxx::detail::iter_value_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::deducible_compare<Compare> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
multiset(InputIterator, InputIterator, Compare = Compare(), Allocator = Allocator())
    -> multiset<ycxx::detail::iter_value_type<InputIterator>, Compare, Allocator>;
template <ranges::input_range R, class Compare = less<ranges::range_value_t<R>>,
          class Allocator = allocator<ranges::range_value_t<R>>>
  requires ycxx::detail::deducible_compare<Compare> && ycxx::detail::qualifies_as_allocator<Allocator>
multiset(from_range_t, R&&, Compare = Compare(), Allocator = Allocator())
    -> multiset<ranges::range_value_t<R>, Compare, Allocator>;
template <class Key, class Compare = less<Key>, class Allocator = allocator<Key>>
  requires ycxx::detail::deducible_compare<Compare> && ycxx::detail::qualifies_as_allocator<Allocator>
multiset(initializer_list<Key>, Compare = Compare(), Allocator = Allocator()) -> multiset<Key, Compare, Allocator>;
template <class InputIterator, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
multiset(InputIterator, InputIterator, Allocator)
    -> multiset<ycxx::detail::iter_value_type<InputIterator>, less<ycxx::detail::iter_value_type<InputIterator>>,
                Allocator>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
multiset(from_range_t, R&&, Allocator)
    -> multiset<ranges::range_value_t<R>, less<ranges::range_value_t<R>>, Allocator>;
template <class Key, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
multiset(initializer_list<Key>, Allocator) -> multiset<Key, less<Key>, Allocator>;

// ---- comparisons ----
template <class Key, class Compare, class Allocator>
constexpr bool operator==(const set<Key, Compare, Allocator>& x, const set<Key, Compare, Allocator>& y) {
  return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}
template <class Key, class Compare, class Allocator>
constexpr ycxx::detail::synth_three_way_result<Key> operator<=>(const set<Key, Compare, Allocator>& x,
                                                                const set<Key, Compare, Allocator>& y) {
  return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(),
                                                ycxx::detail::synth_three_way);
}
template <class Key, class Compare, class Allocator>
constexpr bool operator==(const multiset<Key, Compare, Allocator>& x, const multiset<Key, Compare, Allocator>& y) {
  return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}
template <class Key, class Compare, class Allocator>
constexpr ycxx::detail::synth_three_way_result<Key> operator<=>(const multiset<Key, Compare, Allocator>& x,
                                                                const multiset<Key, Compare, Allocator>& y) {
  return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(),
                                                ycxx::detail::synth_three_way);
}

// ---- specialized algorithms ----
template <class Key, class Compare, class Allocator>
constexpr void swap(set<Key, Compare, Allocator>& x, set<Key, Compare, Allocator>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}
template <class Key, class Compare, class Allocator>
constexpr void swap(multiset<Key, Compare, Allocator>& x, multiset<Key, Compare, Allocator>& y) noexcept(
    noexcept(x.swap(y))) {
  x.swap(y);
}

// ---- [set.erasure], [multiset.erasure] ----
template <class Key, class Compare, class Allocator, class Predicate>
constexpr typename set<Key, Compare, Allocator>::size_type erase_if(set<Key, Compare, Allocator>& c, Predicate pred) {
  auto original_size = c.size();
  for (auto i = c.begin(), last = c.end(); i != last;) {
    if (pred(*i))
      i = c.erase(i);
    else
      ++i;
  }
  return original_size - c.size();
}
template <class Key, class Compare, class Allocator, class Predicate>
constexpr typename multiset<Key, Compare, Allocator>::size_type erase_if(multiset<Key, Compare, Allocator>& c,
                                                                         Predicate pred) {
  auto original_size = c.size();
  for (auto i = c.begin(), last = c.end(); i != last;) {
    if (pred(*i))
      i = c.erase(i);
    else
      ++i;
  }
  return original_size - c.size();
}

namespace pmr {
template <class Key, class Compare = less<Key>>
using set = std::set<Key, Compare, polymorphic_allocator<Key>>;
template <class Key, class Compare = less<Key>>
using multiset = std::multiset<Key, Compare, polymorphic_allocator<Key>>;
} // namespace pmr

} // namespace std
