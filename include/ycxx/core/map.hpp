// libycxx core: map and multimap ([map], [multimap]), their comparisons, erasure, deduction
// guides and the pmr:: aliases. Both are thin layers over the red-black tree of rb_tree.hpp,
// which also provides the members they share with set and multiset.
#pragma once

#include <initializer_list>
#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/memory_resource_fwd.hpp>
#include <ycxx/core/optional.hpp>
#include <ycxx/core/rb_tree.hpp>
#include <ycxx/core/tuple.hpp>

namespace [[gnu::visibility("hidden")]] std {

template <class Key, class T, class Compare = less<Key>, class Allocator = allocator<pair<const Key, T>>>
class map;
template <class Key, class T, class Compare = less<Key>, class Allocator = allocator<pair<const Key, T>>>
class multimap;

template <class Key, class T, class Compare, class Allocator>
class map : public ycxx::adl_free::rb_tree<Key, pair<const Key, T>, Compare, Allocator, true, false> {
  static_assert(ycxx::detail::allocator_for<Allocator, pair<const Key, T>>,
                "std::map: Allocator::value_type must be pair<const Key, T> ([container.alloc.reqmts])");

  using base = ycxx::adl_free::rb_tree<Key, pair<const Key, T>, Compare, Allocator, true, false>;
  using alloc_traits = allocator_traits<Allocator>;
  using node_base = typename base::node_base;
  static constexpr bool always_equal = base::always_equal;

  template <class, class, class, class>
  friend class map;
  template <class, class, class, class>
  friend class multimap;

public:
  // ---- types ----
  using key_type = Key;
  using mapped_type = T;
  using value_type = pair<const Key, T>;
  using key_compare = Compare;
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

  class value_compare {
    friend class map;

  protected:
    Compare comp;
    constexpr value_compare(Compare c) : comp(c) {}

  public:
    constexpr bool operator()(const value_type& x, const value_type& y) const { return comp(x.first, y.first); }
  };

  // ---- [map.cons] ----
  constexpr map() noexcept(base::nothrow_default) : map(Compare()) {}
  constexpr explicit map(const Compare& comp, const Allocator& a = Allocator()) noexcept(
      is_nothrow_copy_constructible_v<Compare>)
      : base(comp, a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr map(InputIterator first, InputIterator last, const Compare& comp = Compare(),
                const Allocator& a = Allocator())
      : base(comp, a) {
    this->insert_elems(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr map(Tag, R&& rg, const Compare& comp = Compare(), const Allocator& a = Allocator()) : base(comp, a) {
    this->insert_elems(ranges::begin(rg), ranges::end(rg));
  }
  constexpr map(const map& x) : base(x, alloc_traits::select_on_container_copy_construction(x.get_allocator())) {}
  constexpr map(map&& x) noexcept(is_nothrow_copy_constructible_v<Compare>) : base(static_cast<base&&>(x)) {}
  constexpr explicit map(const Allocator& a) : base(Compare(), a) {}
  constexpr map(const map& x, const type_identity_t<Allocator>& a) : base(x, a) {}
  constexpr map(map&& x, const type_identity_t<Allocator>& a) noexcept(base::nothrow_move_alloc)
      : base(static_cast<base&&>(x), a) {}
  constexpr map(initializer_list<value_type> il, const type_identity_t<Compare>& comp = Compare(),
                const type_identity_t<Allocator>& a = Allocator())
      : base(comp, a) {
    this->insert_elems(il.begin(), il.end());
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr map(InputIterator first, InputIterator last, const Allocator& a)
      : map(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last), Compare(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr map(Tag, R&& rg, const Allocator& a) : map(from_range, static_cast<R&&>(rg), Compare(), a) {}
  constexpr map(initializer_list<value_type> il, const type_identity_t<Allocator>& a) : map(il, Compare(), a) {}
  constexpr ~map() = default;

  constexpr map& operator=(const map& x) {
    this->copy_assign(x);
    return *this;
  }
  constexpr map& operator=(map&& x) noexcept(always_equal && is_nothrow_move_assignable_v<Compare>) {
    this->move_assign(x);
    return *this;
  }
  constexpr map& operator=(initializer_list<value_type> il) {
    this->clear();
    this->insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- [map.access] ----
  constexpr mapped_type& operator[](const key_type& x) { return try_emplace(x).first->second; }
  constexpr mapped_type& operator[](key_type&& x) { return try_emplace(static_cast<key_type&&>(x)).first->second; }
  template <class K>
    requires ycxx::detail::transparent_compare<Compare>
  constexpr mapped_type& operator[](K&& x) {
    return try_emplace(static_cast<K&&>(x)).first->second;
  }
  constexpr mapped_type& at(const key_type& x) { return at_impl(x); }
  constexpr const mapped_type& at(const key_type& x) const { return at_impl(x); }
  template <class K>
    requires ycxx::detail::transparent_compare<Compare>
  constexpr mapped_type& at(const K& x) {
    return at_impl(x);
  }
  template <class K>
    requires ycxx::detail::transparent_compare<Compare>
  constexpr const mapped_type& at(const K& x) const {
    return at_impl(x);
  }
  constexpr optional<mapped_type&> lookup(const key_type& x) { return lookup_impl<mapped_type&>(x); }
  constexpr optional<const mapped_type&> lookup(const key_type& x) const { return lookup_impl<const mapped_type&>(x); }
  template <class K>
    requires ycxx::detail::transparent_compare<Compare>
  constexpr optional<mapped_type&> lookup(const K& x) {
    return lookup_impl<mapped_type&>(x);
  }
  template <class K>
    requires ycxx::detail::transparent_compare<Compare>
  constexpr optional<const mapped_type&> lookup(const K& x) const {
    return lookup_impl<const mapped_type&>(x);
  }

  // ---- [map.modifiers] ----
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
  template <class P>
    requires is_constructible_v<value_type, P&&>
  constexpr pair<iterator, bool> insert(P&& x) {
    return emplace(static_cast<P&&>(x));
  }
  constexpr iterator insert(const_iterator position, const value_type& x) { return emplace_hint(position, x); }
  constexpr iterator insert(const_iterator position, value_type&& x) {
    return emplace_hint(position, static_cast<value_type&&>(x));
  }
  template <class P>
    requires is_constructible_v<value_type, P&&>
  constexpr iterator insert(const_iterator position, P&& x) {
    return emplace_hint(position, static_cast<P&&>(x));
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
    return iterator(this->insert_handle_unique(hint_node(hint), nh).first);
  }

  template <class... Args>
  constexpr pair<iterator, bool> try_emplace(const key_type& k, Args&&... args) {
    return try_emplace_impl(nullptr, k, static_cast<Args&&>(args)...);
  }
  template <class... Args>
  constexpr pair<iterator, bool> try_emplace(key_type&& k, Args&&... args) {
    return try_emplace_impl(nullptr, static_cast<key_type&&>(k), static_cast<Args&&>(args)...);
  }
  template <class K, class... Args>
    requires ycxx::detail::transparent_non_iter<Compare, K, iterator, const_iterator>
  constexpr pair<iterator, bool> try_emplace(K&& k, Args&&... args) {
    return try_emplace_impl(nullptr, static_cast<K&&>(k), static_cast<Args&&>(args)...);
  }
  template <class... Args>
  constexpr iterator try_emplace(const_iterator hint, const key_type& k, Args&&... args) {
    return try_emplace_impl(hint_node(hint), k, static_cast<Args&&>(args)...).first;
  }
  template <class... Args>
  constexpr iterator try_emplace(const_iterator hint, key_type&& k, Args&&... args) {
    return try_emplace_impl(hint_node(hint), static_cast<key_type&&>(k), static_cast<Args&&>(args)...).first;
  }
  template <class K, class... Args>
    requires ycxx::detail::transparent_compare<Compare>
  constexpr iterator try_emplace(const_iterator hint, K&& k, Args&&... args) {
    return try_emplace_impl(hint_node(hint), static_cast<K&&>(k), static_cast<Args&&>(args)...).first;
  }
  template <class M>
  constexpr pair<iterator, bool> insert_or_assign(const key_type& k, M&& obj) {
    return insert_or_assign_impl(nullptr, k, static_cast<M&&>(obj));
  }
  template <class M>
  constexpr pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj) {
    return insert_or_assign_impl(nullptr, static_cast<key_type&&>(k), static_cast<M&&>(obj));
  }
  template <class K, class M>
    requires ycxx::detail::transparent_compare<Compare>
  constexpr pair<iterator, bool> insert_or_assign(K&& k, M&& obj) {
    return insert_or_assign_impl(nullptr, static_cast<K&&>(k), static_cast<M&&>(obj));
  }
  template <class M>
  constexpr iterator insert_or_assign(const_iterator hint, const key_type& k, M&& obj) {
    return insert_or_assign_impl(hint_node(hint), k, static_cast<M&&>(obj)).first;
  }
  template <class M>
  constexpr iterator insert_or_assign(const_iterator hint, key_type&& k, M&& obj) {
    return insert_or_assign_impl(hint_node(hint), static_cast<key_type&&>(k), static_cast<M&&>(obj)).first;
  }
  template <class K, class M>
    requires ycxx::detail::transparent_compare<Compare>
  constexpr iterator insert_or_assign(const_iterator hint, K&& k, M&& obj) {
    return insert_or_assign_impl(hint_node(hint), static_cast<K&&>(k), static_cast<M&&>(obj)).first;
  }

  constexpr void swap(map& x) noexcept(is_nothrow_swappable_v<Compare>) { this->swap_tree(x); }

  template <class C2>
  constexpr void merge(map<Key, T, C2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class C2>
  constexpr void merge(map<Key, T, C2, Allocator>&& source) {
    this->merge_from(source);
  }
  template <class C2>
  constexpr void merge(multimap<Key, T, C2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class C2>
  constexpr void merge(multimap<Key, T, C2, Allocator>&& source) {
    this->merge_from(source);
  }

  // ---- observers ----
  constexpr value_compare value_comp() const { return value_compare(this->comp_); }

private:
  // The hint's node; the header (created if need be) for end().
  constexpr node_base* hint_node(const_iterator hint) {
    node_base* n = base::nd(hint);
    return n ? n : this->header();
  }
  template <class K>
  constexpr mapped_type& at_impl(const K& x) const {
    node_base* n = this->find_node(x);
    if (n == this->hdr_)
      ycxx::detail::throw_out_of_range("std::map::at: key not found");
    return base::value(n).second;
  }
  template <class R, class K>
  constexpr optional<R> lookup_impl(const K& x) const {
    node_base* n = this->find_node(x);
    if (n == this->hdr_)
      return nullopt;
    return optional<R>(base::value(n).second);
  }
  template <class K, class... Args>
  constexpr pair<iterator, bool> try_emplace_impl(node_base* hint, K&& k, Args&&... args) {
    auto r = this->emplace_key(hint, k, piecewise_construct, std::forward_as_tuple(static_cast<K&&>(k)),
                               std::forward_as_tuple(static_cast<Args&&>(args)...));
    return {iterator(r.first), r.second};
  }
  template <class K, class M>
  constexpr pair<iterator, bool> insert_or_assign_impl(node_base* hint, K&& k, M&& obj) {
    static_assert(is_assignable_v<mapped_type&, M&&>,
                  "std::map::insert_or_assign: mapped_type& must be assignable from M&&");
    // emplace_key uses the arguments only when it inserts.
    auto r = this->emplace_key(hint, k, static_cast<K&&>(k), static_cast<M&&>(obj));
    if (!r.second)
      base::value(r.first).second = static_cast<M&&>(obj);
    return {iterator(r.first), r.second};
  }
};

template <class Key, class T, class Compare, class Allocator>
class multimap : public ycxx::adl_free::rb_tree<Key, pair<const Key, T>, Compare, Allocator, true, true> {
  static_assert(ycxx::detail::allocator_for<Allocator, pair<const Key, T>>,
                "std::multimap: Allocator::value_type must be pair<const Key, T> ([container.alloc.reqmts])");

  using base = ycxx::adl_free::rb_tree<Key, pair<const Key, T>, Compare, Allocator, true, true>;
  using alloc_traits = allocator_traits<Allocator>;
  using node_base = typename base::node_base;
  static constexpr bool always_equal = base::always_equal;

  template <class, class, class, class>
  friend class map;
  template <class, class, class, class>
  friend class multimap;

public:
  // ---- types ----
  using key_type = Key;
  using mapped_type = T;
  using value_type = pair<const Key, T>;
  using key_compare = Compare;
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

  class value_compare {
    friend class multimap;

  protected:
    Compare comp;
    constexpr value_compare(Compare c) : comp(c) {}

  public:
    constexpr bool operator()(const value_type& x, const value_type& y) const { return comp(x.first, y.first); }
  };

  // ---- [multimap.cons] ----
  constexpr multimap() noexcept(base::nothrow_default) : multimap(Compare()) {}
  constexpr explicit multimap(const Compare& comp, const Allocator& a = Allocator()) noexcept(
      is_nothrow_copy_constructible_v<Compare>)
      : base(comp, a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr multimap(InputIterator first, InputIterator last, const Compare& comp = Compare(),
                     const Allocator& a = Allocator())
      : base(comp, a) {
    this->insert_elems(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last));
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr multimap(Tag, R&& rg, const Compare& comp = Compare(), const Allocator& a = Allocator())
      : base(comp, a) {
    this->insert_elems(ranges::begin(rg), ranges::end(rg));
  }
  constexpr multimap(const multimap& x)
      : base(x, alloc_traits::select_on_container_copy_construction(x.get_allocator())) {}
  constexpr multimap(multimap&& x) noexcept(is_nothrow_copy_constructible_v<Compare>) : base(static_cast<base&&>(x)) {}
  constexpr explicit multimap(const Allocator& a) : base(Compare(), a) {}
  constexpr multimap(const multimap& x, const type_identity_t<Allocator>& a) : base(x, a) {}
  constexpr multimap(multimap&& x, const type_identity_t<Allocator>& a) noexcept(base::nothrow_move_alloc)
      : base(static_cast<base&&>(x), a) {}
  constexpr multimap(initializer_list<value_type> il, const type_identity_t<Compare>& comp = Compare(),
                     const type_identity_t<Allocator>& a = Allocator())
      : base(comp, a) {
    this->insert_elems(il.begin(), il.end());
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr multimap(InputIterator first, InputIterator last, const Allocator& a)
      : multimap(static_cast<InputIterator&&>(first), static_cast<InputIterator&&>(last), Compare(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr multimap(Tag, R&& rg, const Allocator& a) : multimap(from_range, static_cast<R&&>(rg), Compare(), a) {}
  constexpr multimap(initializer_list<value_type> il, const type_identity_t<Allocator>& a)
      : multimap(il, Compare(), a) {}
  constexpr ~multimap() = default;

  constexpr multimap& operator=(const multimap& x) {
    this->copy_assign(x);
    return *this;
  }
  constexpr multimap& operator=(multimap&& x) noexcept(always_equal && is_nothrow_move_assignable_v<Compare>) {
    this->move_assign(x);
    return *this;
  }
  constexpr multimap& operator=(initializer_list<value_type> il) {
    this->clear();
    this->insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- [multimap.modifiers] ----
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
  template <class P>
    requires is_constructible_v<value_type, P&&>
  constexpr iterator insert(P&& x) {
    return emplace(static_cast<P&&>(x));
  }
  constexpr iterator insert(const_iterator position, const value_type& x) { return emplace_hint(position, x); }
  constexpr iterator insert(const_iterator position, value_type&& x) {
    return emplace_hint(position, static_cast<value_type&&>(x));
  }
  template <class P>
    requires is_constructible_v<value_type, P&&>
  constexpr iterator insert(const_iterator position, P&& x) {
    return emplace_hint(position, static_cast<P&&>(x));
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

  constexpr void swap(multimap& x) noexcept(is_nothrow_swappable_v<Compare>) { this->swap_tree(x); }

  template <class C2>
  constexpr void merge(multimap<Key, T, C2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class C2>
  constexpr void merge(multimap<Key, T, C2, Allocator>&& source) {
    this->merge_from(source);
  }
  template <class C2>
  constexpr void merge(map<Key, T, C2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class C2>
  constexpr void merge(map<Key, T, C2, Allocator>&& source) {
    this->merge_from(source);
  }

  // ---- observers ----
  constexpr value_compare value_comp() const { return value_compare(this->comp_); }
};

// ---- deduction guides ----
template <class InputIterator, class Compare = less<ycxx::detail::iter_key_type<InputIterator>>,
          class Allocator = allocator<ycxx::detail::iter_to_alloc_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::deducible_compare<Compare> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
map(InputIterator, InputIterator, Compare = Compare(), Allocator = Allocator())
    -> map<ycxx::detail::iter_key_type<InputIterator>, ycxx::detail::iter_mapped_type<InputIterator>, Compare,
           Allocator>;
template <ranges::input_range R, class Compare = less<ycxx::detail::range_key_type<R>>,
          class Allocator = allocator<ycxx::detail::range_to_alloc_type<R>>>
  requires ycxx::detail::deducible_compare<Compare> && ycxx::detail::qualifies_as_allocator<Allocator>
map(from_range_t, R&&, Compare = Compare(), Allocator = Allocator())
    -> map<ycxx::detail::range_key_type<R>, ycxx::detail::range_mapped_type<R>, Compare, Allocator>;
template <class Key, class T, class Compare = less<Key>, class Allocator = allocator<pair<const Key, T>>>
  requires ycxx::detail::deducible_compare<Compare> && ycxx::detail::qualifies_as_allocator<Allocator>
map(initializer_list<pair<Key, T>>, Compare = Compare(), Allocator = Allocator()) -> map<Key, T, Compare, Allocator>;
template <class InputIterator, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
map(InputIterator, InputIterator, Allocator)
    -> map<ycxx::detail::iter_key_type<InputIterator>, ycxx::detail::iter_mapped_type<InputIterator>,
           less<ycxx::detail::iter_key_type<InputIterator>>, Allocator>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
map(from_range_t, R&&, Allocator)
    -> map<ycxx::detail::range_key_type<R>, ycxx::detail::range_mapped_type<R>, less<ycxx::detail::range_key_type<R>>,
           Allocator>;
template <class Key, class T, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
map(initializer_list<pair<Key, T>>, Allocator) -> map<Key, T, less<Key>, Allocator>;

template <class InputIterator, class Compare = less<ycxx::detail::iter_key_type<InputIterator>>,
          class Allocator = allocator<ycxx::detail::iter_to_alloc_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::deducible_compare<Compare> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
multimap(InputIterator, InputIterator, Compare = Compare(), Allocator = Allocator())
    -> multimap<ycxx::detail::iter_key_type<InputIterator>, ycxx::detail::iter_mapped_type<InputIterator>, Compare,
                Allocator>;
template <ranges::input_range R, class Compare = less<ycxx::detail::range_key_type<R>>,
          class Allocator = allocator<ycxx::detail::range_to_alloc_type<R>>>
  requires ycxx::detail::deducible_compare<Compare> && ycxx::detail::qualifies_as_allocator<Allocator>
multimap(from_range_t, R&&, Compare = Compare(), Allocator = Allocator())
    -> multimap<ycxx::detail::range_key_type<R>, ycxx::detail::range_mapped_type<R>, Compare, Allocator>;
template <class Key, class T, class Compare = less<Key>, class Allocator = allocator<pair<const Key, T>>>
  requires ycxx::detail::deducible_compare<Compare> && ycxx::detail::qualifies_as_allocator<Allocator>
multimap(initializer_list<pair<Key, T>>, Compare = Compare(), Allocator = Allocator())
    -> multimap<Key, T, Compare, Allocator>;
template <class InputIterator, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
multimap(InputIterator, InputIterator, Allocator)
    -> multimap<ycxx::detail::iter_key_type<InputIterator>, ycxx::detail::iter_mapped_type<InputIterator>,
                less<ycxx::detail::iter_key_type<InputIterator>>, Allocator>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
multimap(from_range_t, R&&, Allocator)
    -> multimap<ycxx::detail::range_key_type<R>, ycxx::detail::range_mapped_type<R>,
                less<ycxx::detail::range_key_type<R>>, Allocator>;
template <class Key, class T, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
multimap(initializer_list<pair<Key, T>>, Allocator) -> multimap<Key, T, less<Key>, Allocator>;

// ---- comparisons ----
template <class Key, class T, class Compare, class Allocator>
constexpr bool operator==(const map<Key, T, Compare, Allocator>& x, const map<Key, T, Compare, Allocator>& y) {
  return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}
template <class Key, class T, class Compare, class Allocator>
constexpr ycxx::detail::synth_three_way_result<pair<const Key, T>>
operator<=>(const map<Key, T, Compare, Allocator>& x, const map<Key, T, Compare, Allocator>& y) {
  return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(),
                                                ycxx::detail::synth_three_way);
}
template <class Key, class T, class Compare, class Allocator>
constexpr bool operator==(const multimap<Key, T, Compare, Allocator>& x,
                          const multimap<Key, T, Compare, Allocator>& y) {
  return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
}
template <class Key, class T, class Compare, class Allocator>
constexpr ycxx::detail::synth_three_way_result<pair<const Key, T>>
operator<=>(const multimap<Key, T, Compare, Allocator>& x, const multimap<Key, T, Compare, Allocator>& y) {
  return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(),
                                                ycxx::detail::synth_three_way);
}

// ---- specialized algorithms ----
template <class Key, class T, class Compare, class Allocator>
constexpr void swap(map<Key, T, Compare, Allocator>& x, map<Key, T, Compare, Allocator>& y) noexcept(
    noexcept(x.swap(y))) {
  x.swap(y);
}
template <class Key, class T, class Compare, class Allocator>
constexpr void swap(multimap<Key, T, Compare, Allocator>& x, multimap<Key, T, Compare, Allocator>& y) noexcept(
    noexcept(x.swap(y))) {
  x.swap(y);
}

// ---- [map.erasure], [multimap.erasure] ----
template <class Key, class T, class Compare, class Allocator, class Predicate>
constexpr typename map<Key, T, Compare, Allocator>::size_type erase_if(map<Key, T, Compare, Allocator>& c,
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
template <class Key, class T, class Compare, class Allocator, class Predicate>
constexpr typename multimap<Key, T, Compare, Allocator>::size_type erase_if(multimap<Key, T, Compare, Allocator>& c,
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
template <class Key, class T, class Compare = less<Key>>
using map = std::map<Key, T, Compare, polymorphic_allocator<pair<const Key, T>>>;
template <class Key, class T, class Compare = less<Key>>
using multimap = std::multimap<Key, T, Compare, polymorphic_allocator<pair<const Key, T>>>;
} // namespace pmr

} // namespace std
