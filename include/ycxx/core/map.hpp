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

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Key, class _Tp, class _Compare = less<_Key>, class _Allocator = allocator<pair<const _Key, _Tp>>>
class map;
template <class _Key, class _Tp, class _Compare = less<_Key>, class _Allocator = allocator<pair<const _Key, _Tp>>>
class multimap;

template <class _Key, class _Tp, class _Compare, class _Allocator>
class map : public __ycxx::__adl_free::__rb_tree<_Key, pair<const _Key, _Tp>, _Compare, _Allocator, true, false> {
  static_assert(__ycxx::__detail::__allocator_for<_Allocator, pair<const _Key, _Tp>>,
                "std::map: Allocator::value_type must be pair<const Key, T> ([container.alloc.reqmts])");

  using base = __ycxx::__adl_free::__rb_tree<_Key, pair<const _Key, _Tp>, _Compare, _Allocator, true, false>;
  using __alloc_traits = allocator_traits<_Allocator>;
  using __node_base = typename base::__node_base;
  static constexpr bool __always_equal = base::__always_equal;

  template <class, class, class, class>
  friend class map;
  template <class, class, class, class>
  friend class multimap;

public:
  // ---- types ----
  using key_type = _Key;
  using mapped_type = _Tp;
  using value_type = pair<const _Key, _Tp>;
  using key_compare = _Compare;
  using allocator_type = _Allocator;
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
  using insert_return_type = __ycxx::__adl_free::insert_return_type<iterator, node_type>;

  class value_compare {
    friend class map;

  protected:
    _Compare comp;
    constexpr value_compare(_Compare c) : comp(c) {}

  public:
    constexpr bool operator()(const value_type& __x, const value_type& y) const { return comp(__x.first, y.first); }
  };

  // ---- [map.cons] ----
  constexpr map() noexcept(base::__nothrow_default) : map(_Compare()) {}
  constexpr explicit map(const _Compare& comp, const _Allocator& a = _Allocator()) noexcept(
      is_nothrow_copy_constructible_v<_Compare>)
      : base(comp, a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr map(_InputIterator first, _InputIterator last, const _Compare& comp = _Compare(),
                const _Allocator& a = _Allocator())
      : base(comp, a) {
    this->__insert_elems(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr map(_Tag, _Rp&& __rg, const _Compare& comp = _Compare(), const _Allocator& a = _Allocator()) : base(comp, a) {
    this->__insert_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr map(const map& __x) : base(__x, __alloc_traits::select_on_container_copy_construction(__x.get_allocator())) {}
  constexpr map(map&& __x) noexcept(is_nothrow_copy_constructible_v<_Compare>) : base(static_cast<base&&>(__x)) {}
  constexpr explicit map(const _Allocator& a) : base(_Compare(), a) {}
  constexpr map(const map& __x, const type_identity_t<_Allocator>& a) : base(__x, a) {}
  constexpr map(map&& __x, const type_identity_t<_Allocator>& a) noexcept(base::__nothrow_move_alloc)
      : base(static_cast<base&&>(__x), a) {}
  constexpr map(initializer_list<value_type> il, const type_identity_t<_Compare>& comp = _Compare(),
                const type_identity_t<_Allocator>& a = _Allocator())
      : base(comp, a) {
    this->__insert_elems(il.begin(), il.end());
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr map(_InputIterator first, _InputIterator last, const _Allocator& a)
      : map(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last), _Compare(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr map(_Tag, _Rp&& __rg, const _Allocator& a) : map(from_range, static_cast<_Rp&&>(__rg), _Compare(), a) {}
  constexpr map(initializer_list<value_type> il, const type_identity_t<_Allocator>& a) : map(il, _Compare(), a) {}
  constexpr ~map() = default;

  constexpr map& operator=(const map& __x) {
    this->__copy_assign(__x);
    return *this;
  }
  constexpr map& operator=(map&& __x) noexcept(__always_equal && is_nothrow_move_assignable_v<_Compare>) {
    this->__move_assign(__x);
    return *this;
  }
  constexpr map& operator=(initializer_list<value_type> il) {
    this->clear();
    this->__insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- [map.access] ----
  constexpr mapped_type& operator[](const key_type& __x) { return try_emplace(__x).first->second; }
  constexpr mapped_type& operator[](key_type&& __x) { return try_emplace(static_cast<key_type&&>(__x)).first->second; }
  template <class _Kp>
    requires __ycxx::__detail::__transparent_compare<_Compare>
  constexpr mapped_type& operator[](_Kp&& __x) {
    return try_emplace(static_cast<_Kp&&>(__x)).first->second;
  }
  constexpr mapped_type& at(const key_type& __x) { return __at_impl(__x); }
  constexpr const mapped_type& at(const key_type& __x) const { return __at_impl(__x); }
  template <class _Kp>
    requires __ycxx::__detail::__transparent_compare<_Compare>
  constexpr mapped_type& at(const _Kp& __x) {
    return __at_impl(__x);
  }
  template <class _Kp>
    requires __ycxx::__detail::__transparent_compare<_Compare>
  constexpr const mapped_type& at(const _Kp& __x) const {
    return __at_impl(__x);
  }
  constexpr optional<mapped_type&> lookup(const key_type& __x) { return __lookup_impl<mapped_type&>(__x); }
  constexpr optional<const mapped_type&> lookup(const key_type& __x) const { return __lookup_impl<const mapped_type&>(__x); }
  template <class _Kp>
    requires __ycxx::__detail::__transparent_compare<_Compare>
  constexpr optional<mapped_type&> lookup(const _Kp& __x) {
    return __lookup_impl<mapped_type&>(__x);
  }
  template <class _Kp>
    requires __ycxx::__detail::__transparent_compare<_Compare>
  constexpr optional<const mapped_type&> lookup(const _Kp& __x) const {
    return __lookup_impl<const mapped_type&>(__x);
  }

  // ---- [map.modifiers] ----
  template <class... _Args>
  constexpr pair<iterator, bool> emplace(_Args&&... __args) {
    auto r = this->__emplace_unique(static_cast<_Args&&>(__args)...);
    return {iterator(r.first), r.second};
  }
  template <class... _Args>
  constexpr iterator emplace_hint(const_iterator position, _Args&&... __args) {
    return iterator(this->__emplace_hint_unique(base::__nd(position), static_cast<_Args&&>(__args)...));
  }
  constexpr pair<iterator, bool> insert(const value_type& __x) { return emplace(__x); }
  constexpr pair<iterator, bool> insert(value_type&& __x) { return emplace(static_cast<value_type&&>(__x)); }
  template <class _Pp>
    requires is_constructible_v<value_type, _Pp&&>
  constexpr pair<iterator, bool> insert(_Pp&& __x) {
    return emplace(static_cast<_Pp&&>(__x));
  }
  constexpr iterator insert(const_iterator position, const value_type& __x) { return emplace_hint(position, __x); }
  constexpr iterator insert(const_iterator position, value_type&& __x) {
    return emplace_hint(position, static_cast<value_type&&>(__x));
  }
  template <class _Pp>
    requires is_constructible_v<value_type, _Pp&&>
  constexpr iterator insert(const_iterator position, _Pp&& __x) {
    return emplace_hint(position, static_cast<_Pp&&>(__x));
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr void insert(_InputIterator first, _InputIterator last) {
    this->__insert_elems(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
  }
  template <__ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr void insert_range(_Rp&& __rg) {
    this->__insert_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr void insert(initializer_list<value_type> il) { this->__insert_elems(il.begin(), il.end()); }
  constexpr insert_return_type insert(node_type&& __nh) {
    auto r = this->__insert_handle_unique(nullptr, __nh);
    return {iterator(r.first), r.second, static_cast<node_type&&>(__nh)};
  }
  constexpr iterator insert(const_iterator __hint, node_type&& __nh) {
    return iterator(this->__insert_handle_unique(__hint_node(__hint), __nh).first);
  }

  template <class... _Args>
  constexpr pair<iterator, bool> try_emplace(const key_type& k, _Args&&... __args) {
    return __try_emplace_impl(nullptr, k, static_cast<_Args&&>(__args)...);
  }
  template <class... _Args>
  constexpr pair<iterator, bool> try_emplace(key_type&& k, _Args&&... __args) {
    return __try_emplace_impl(nullptr, static_cast<key_type&&>(k), static_cast<_Args&&>(__args)...);
  }
  template <class _Kp, class... _Args>
    requires __ycxx::__detail::__transparent_non_iter<_Compare, _Kp, iterator, const_iterator>
  constexpr pair<iterator, bool> try_emplace(_Kp&& k, _Args&&... __args) {
    return __try_emplace_impl(nullptr, static_cast<_Kp&&>(k), static_cast<_Args&&>(__args)...);
  }
  template <class... _Args>
  constexpr iterator try_emplace(const_iterator __hint, const key_type& k, _Args&&... __args) {
    return __try_emplace_impl(__hint_node(__hint), k, static_cast<_Args&&>(__args)...).first;
  }
  template <class... _Args>
  constexpr iterator try_emplace(const_iterator __hint, key_type&& k, _Args&&... __args) {
    return __try_emplace_impl(__hint_node(__hint), static_cast<key_type&&>(k), static_cast<_Args&&>(__args)...).first;
  }
  template <class _Kp, class... _Args>
    requires __ycxx::__detail::__transparent_compare<_Compare>
  constexpr iterator try_emplace(const_iterator __hint, _Kp&& k, _Args&&... __args) {
    return __try_emplace_impl(__hint_node(__hint), static_cast<_Kp&&>(k), static_cast<_Args&&>(__args)...).first;
  }
  template <class _Mp>
  constexpr pair<iterator, bool> insert_or_assign(const key_type& k, _Mp&& __obj) {
    return __insert_or_assign_impl(nullptr, k, static_cast<_Mp&&>(__obj));
  }
  template <class _Mp>
  constexpr pair<iterator, bool> insert_or_assign(key_type&& k, _Mp&& __obj) {
    return __insert_or_assign_impl(nullptr, static_cast<key_type&&>(k), static_cast<_Mp&&>(__obj));
  }
  template <class _Kp, class _Mp>
    requires __ycxx::__detail::__transparent_compare<_Compare>
  constexpr pair<iterator, bool> insert_or_assign(_Kp&& k, _Mp&& __obj) {
    return __insert_or_assign_impl(nullptr, static_cast<_Kp&&>(k), static_cast<_Mp&&>(__obj));
  }
  template <class _Mp>
  constexpr iterator insert_or_assign(const_iterator __hint, const key_type& k, _Mp&& __obj) {
    return __insert_or_assign_impl(__hint_node(__hint), k, static_cast<_Mp&&>(__obj)).first;
  }
  template <class _Mp>
  constexpr iterator insert_or_assign(const_iterator __hint, key_type&& k, _Mp&& __obj) {
    return __insert_or_assign_impl(__hint_node(__hint), static_cast<key_type&&>(k), static_cast<_Mp&&>(__obj)).first;
  }
  template <class _Kp, class _Mp>
    requires __ycxx::__detail::__transparent_compare<_Compare>
  constexpr iterator insert_or_assign(const_iterator __hint, _Kp&& k, _Mp&& __obj) {
    return __insert_or_assign_impl(__hint_node(__hint), static_cast<_Kp&&>(k), static_cast<_Mp&&>(__obj)).first;
  }

  constexpr void swap(map& __x) noexcept(is_nothrow_swappable_v<_Compare>) { this->__swap_tree(__x); }

  template <class _C2>
  constexpr void merge(map<_Key, _Tp, _C2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _C2>
  constexpr void merge(map<_Key, _Tp, _C2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }
  template <class _C2>
  constexpr void merge(multimap<_Key, _Tp, _C2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _C2>
  constexpr void merge(multimap<_Key, _Tp, _C2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }

  // ---- observers ----
  constexpr value_compare value_comp() const { return value_compare(this->__comp_); }

private:
  // The hint's node; the header (created if need be) for end().
  constexpr __node_base* __hint_node(const_iterator __hint) {
    __node_base* n = base::__nd(__hint);
    return n ? n : this->__header();
  }
  template <class _Kp>
  constexpr mapped_type& __at_impl(const _Kp& __x) const {
    __node_base* n = this->__find_node(__x);
    if (n == this->__hdr_)
      __ycxx::__detail::__throw_out_of_range("std::map::at: key not found");
    return base::value(n).second;
  }
  template <class _Rp, class _Kp>
  constexpr optional<_Rp> __lookup_impl(const _Kp& __x) const {
    __node_base* n = this->__find_node(__x);
    if (n == this->__hdr_)
      return nullopt;
    return optional<_Rp>(base::value(n).second);
  }
  template <class _Kp, class... _Args>
  constexpr pair<iterator, bool> __try_emplace_impl(__node_base* __hint, _Kp&& k, _Args&&... __args) {
    auto r = this->__emplace_key(__hint, k, piecewise_construct, std::forward_as_tuple(static_cast<_Kp&&>(k)),
                               std::forward_as_tuple(static_cast<_Args&&>(__args)...));
    return {iterator(r.first), r.second};
  }
  template <class _Kp, class _Mp>
  constexpr pair<iterator, bool> __insert_or_assign_impl(__node_base* __hint, _Kp&& k, _Mp&& __obj) {
    static_assert(is_assignable_v<mapped_type&, _Mp&&>,
                  "std::map::insert_or_assign: mapped_type& must be assignable from M&&");
    // emplace_key uses the arguments only when it inserts.
    auto r = this->__emplace_key(__hint, k, static_cast<_Kp&&>(k), static_cast<_Mp&&>(__obj));
    if (!r.second)
      base::value(r.first).second = static_cast<_Mp&&>(__obj);
    return {iterator(r.first), r.second};
  }
};

template <class _Key, class _Tp, class _Compare, class _Allocator>
class multimap : public __ycxx::__adl_free::__rb_tree<_Key, pair<const _Key, _Tp>, _Compare, _Allocator, true, true> {
  static_assert(__ycxx::__detail::__allocator_for<_Allocator, pair<const _Key, _Tp>>,
                "std::multimap: Allocator::value_type must be pair<const Key, T> ([container.alloc.reqmts])");

  using base = __ycxx::__adl_free::__rb_tree<_Key, pair<const _Key, _Tp>, _Compare, _Allocator, true, true>;
  using __alloc_traits = allocator_traits<_Allocator>;
  using __node_base = typename base::__node_base;
  static constexpr bool __always_equal = base::__always_equal;

  template <class, class, class, class>
  friend class map;
  template <class, class, class, class>
  friend class multimap;

public:
  // ---- types ----
  using key_type = _Key;
  using mapped_type = _Tp;
  using value_type = pair<const _Key, _Tp>;
  using key_compare = _Compare;
  using allocator_type = _Allocator;
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
    _Compare comp;
    constexpr value_compare(_Compare c) : comp(c) {}

  public:
    constexpr bool operator()(const value_type& __x, const value_type& y) const { return comp(__x.first, y.first); }
  };

  // ---- [multimap.cons] ----
  constexpr multimap() noexcept(base::__nothrow_default) : multimap(_Compare()) {}
  constexpr explicit multimap(const _Compare& comp, const _Allocator& a = _Allocator()) noexcept(
      is_nothrow_copy_constructible_v<_Compare>)
      : base(comp, a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr multimap(_InputIterator first, _InputIterator last, const _Compare& comp = _Compare(),
                     const _Allocator& a = _Allocator())
      : base(comp, a) {
    this->__insert_elems(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr multimap(_Tag, _Rp&& __rg, const _Compare& comp = _Compare(), const _Allocator& a = _Allocator())
      : base(comp, a) {
    this->__insert_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr multimap(const multimap& __x)
      : base(__x, __alloc_traits::select_on_container_copy_construction(__x.get_allocator())) {}
  constexpr multimap(multimap&& __x) noexcept(is_nothrow_copy_constructible_v<_Compare>) : base(static_cast<base&&>(__x)) {}
  constexpr explicit multimap(const _Allocator& a) : base(_Compare(), a) {}
  constexpr multimap(const multimap& __x, const type_identity_t<_Allocator>& a) : base(__x, a) {}
  constexpr multimap(multimap&& __x, const type_identity_t<_Allocator>& a) noexcept(base::__nothrow_move_alloc)
      : base(static_cast<base&&>(__x), a) {}
  constexpr multimap(initializer_list<value_type> il, const type_identity_t<_Compare>& comp = _Compare(),
                     const type_identity_t<_Allocator>& a = _Allocator())
      : base(comp, a) {
    this->__insert_elems(il.begin(), il.end());
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr multimap(_InputIterator first, _InputIterator last, const _Allocator& a)
      : multimap(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last), _Compare(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr multimap(_Tag, _Rp&& __rg, const _Allocator& a) : multimap(from_range, static_cast<_Rp&&>(__rg), _Compare(), a) {}
  constexpr multimap(initializer_list<value_type> il, const type_identity_t<_Allocator>& a)
      : multimap(il, _Compare(), a) {}
  constexpr ~multimap() = default;

  constexpr multimap& operator=(const multimap& __x) {
    this->__copy_assign(__x);
    return *this;
  }
  constexpr multimap& operator=(multimap&& __x) noexcept(__always_equal && is_nothrow_move_assignable_v<_Compare>) {
    this->__move_assign(__x);
    return *this;
  }
  constexpr multimap& operator=(initializer_list<value_type> il) {
    this->clear();
    this->__insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- [multimap.modifiers] ----
  template <class... _Args>
  constexpr iterator emplace(_Args&&... __args) {
    return iterator(this->__emplace_multi(static_cast<_Args&&>(__args)...));
  }
  template <class... _Args>
  constexpr iterator emplace_hint(const_iterator position, _Args&&... __args) {
    return iterator(this->__emplace_hint_multi(base::__nd(position), static_cast<_Args&&>(__args)...));
  }
  constexpr iterator insert(const value_type& __x) { return emplace(__x); }
  constexpr iterator insert(value_type&& __x) { return emplace(static_cast<value_type&&>(__x)); }
  template <class _Pp>
    requires is_constructible_v<value_type, _Pp&&>
  constexpr iterator insert(_Pp&& __x) {
    return emplace(static_cast<_Pp&&>(__x));
  }
  constexpr iterator insert(const_iterator position, const value_type& __x) { return emplace_hint(position, __x); }
  constexpr iterator insert(const_iterator position, value_type&& __x) {
    return emplace_hint(position, static_cast<value_type&&>(__x));
  }
  template <class _Pp>
    requires is_constructible_v<value_type, _Pp&&>
  constexpr iterator insert(const_iterator position, _Pp&& __x) {
    return emplace_hint(position, static_cast<_Pp&&>(__x));
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr void insert(_InputIterator first, _InputIterator last) {
    this->__insert_elems(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
  }
  template <__ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr void insert_range(_Rp&& __rg) {
    this->__insert_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr void insert(initializer_list<value_type> il) { this->__insert_elems(il.begin(), il.end()); }
  constexpr iterator insert(node_type&& __nh) { return iterator(this->__insert_handle_multi(nullptr, __nh)); }
  constexpr iterator insert(const_iterator __hint, node_type&& __nh) {
    __node_base* h = base::__nd(__hint);
    return iterator(this->__insert_handle_multi(h ? h : this->__header(), __nh));
  }

  constexpr void swap(multimap& __x) noexcept(is_nothrow_swappable_v<_Compare>) { this->__swap_tree(__x); }

  template <class _C2>
  constexpr void merge(multimap<_Key, _Tp, _C2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _C2>
  constexpr void merge(multimap<_Key, _Tp, _C2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }
  template <class _C2>
  constexpr void merge(map<_Key, _Tp, _C2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _C2>
  constexpr void merge(map<_Key, _Tp, _C2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }

  // ---- observers ----
  constexpr value_compare value_comp() const { return value_compare(this->__comp_); }
};

// ---- deduction guides ----
template <class _InputIterator, class _Compare = less<__ycxx::__detail::__iter_key_type<_InputIterator>>,
          class _Allocator = allocator<__ycxx::__detail::__iter_to_alloc_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__deducible_compare<_Compare> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
map(_InputIterator, _InputIterator, _Compare = _Compare(), _Allocator = _Allocator())
    -> map<__ycxx::__detail::__iter_key_type<_InputIterator>, __ycxx::__detail::__iter_mapped_type<_InputIterator>, _Compare,
           _Allocator>;
template <ranges::input_range _Rp, class _Compare = less<__ycxx::__detail::__range_key_type<_Rp>>,
          class _Allocator = allocator<__ycxx::__detail::__range_to_alloc_type<_Rp>>>
  requires __ycxx::__detail::__deducible_compare<_Compare> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
map(from_range_t, _Rp&&, _Compare = _Compare(), _Allocator = _Allocator())
    -> map<__ycxx::__detail::__range_key_type<_Rp>, __ycxx::__detail::__range_mapped_type<_Rp>, _Compare, _Allocator>;
template <class _Key, class _Tp, class _Compare = less<_Key>, class _Allocator = allocator<pair<const _Key, _Tp>>>
  requires __ycxx::__detail::__deducible_compare<_Compare> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
map(initializer_list<pair<_Key, _Tp>>, _Compare = _Compare(), _Allocator = _Allocator()) -> map<_Key, _Tp, _Compare, _Allocator>;
template <class _InputIterator, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
map(_InputIterator, _InputIterator, _Allocator)
    -> map<__ycxx::__detail::__iter_key_type<_InputIterator>, __ycxx::__detail::__iter_mapped_type<_InputIterator>,
           less<__ycxx::__detail::__iter_key_type<_InputIterator>>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
map(from_range_t, _Rp&&, _Allocator)
    -> map<__ycxx::__detail::__range_key_type<_Rp>, __ycxx::__detail::__range_mapped_type<_Rp>, less<__ycxx::__detail::__range_key_type<_Rp>>,
           _Allocator>;
template <class _Key, class _Tp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
map(initializer_list<pair<_Key, _Tp>>, _Allocator) -> map<_Key, _Tp, less<_Key>, _Allocator>;

template <class _InputIterator, class _Compare = less<__ycxx::__detail::__iter_key_type<_InputIterator>>,
          class _Allocator = allocator<__ycxx::__detail::__iter_to_alloc_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__deducible_compare<_Compare> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
multimap(_InputIterator, _InputIterator, _Compare = _Compare(), _Allocator = _Allocator())
    -> multimap<__ycxx::__detail::__iter_key_type<_InputIterator>, __ycxx::__detail::__iter_mapped_type<_InputIterator>, _Compare,
                _Allocator>;
template <ranges::input_range _Rp, class _Compare = less<__ycxx::__detail::__range_key_type<_Rp>>,
          class _Allocator = allocator<__ycxx::__detail::__range_to_alloc_type<_Rp>>>
  requires __ycxx::__detail::__deducible_compare<_Compare> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
multimap(from_range_t, _Rp&&, _Compare = _Compare(), _Allocator = _Allocator())
    -> multimap<__ycxx::__detail::__range_key_type<_Rp>, __ycxx::__detail::__range_mapped_type<_Rp>, _Compare, _Allocator>;
template <class _Key, class _Tp, class _Compare = less<_Key>, class _Allocator = allocator<pair<const _Key, _Tp>>>
  requires __ycxx::__detail::__deducible_compare<_Compare> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
multimap(initializer_list<pair<_Key, _Tp>>, _Compare = _Compare(), _Allocator = _Allocator())
    -> multimap<_Key, _Tp, _Compare, _Allocator>;
template <class _InputIterator, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
multimap(_InputIterator, _InputIterator, _Allocator)
    -> multimap<__ycxx::__detail::__iter_key_type<_InputIterator>, __ycxx::__detail::__iter_mapped_type<_InputIterator>,
                less<__ycxx::__detail::__iter_key_type<_InputIterator>>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
multimap(from_range_t, _Rp&&, _Allocator)
    -> multimap<__ycxx::__detail::__range_key_type<_Rp>, __ycxx::__detail::__range_mapped_type<_Rp>,
                less<__ycxx::__detail::__range_key_type<_Rp>>, _Allocator>;
template <class _Key, class _Tp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
multimap(initializer_list<pair<_Key, _Tp>>, _Allocator) -> multimap<_Key, _Tp, less<_Key>, _Allocator>;

// ---- comparisons ----
template <class _Key, class _Tp, class _Compare, class _Allocator>
constexpr bool operator==(const map<_Key, _Tp, _Compare, _Allocator>& __x, const map<_Key, _Tp, _Compare, _Allocator>& y) {
  return __x.size() == y.size() && std::equal(__x.begin(), __x.end(), y.begin());
}
template <class _Key, class _Tp, class _Compare, class _Allocator>
constexpr __ycxx::__detail::__synth_three_way_result<pair<const _Key, _Tp>>
operator<=>(const map<_Key, _Tp, _Compare, _Allocator>& __x, const map<_Key, _Tp, _Compare, _Allocator>& y) {
  return std::lexicographical_compare_three_way(__x.begin(), __x.end(), y.begin(), y.end(),
                                                __ycxx::__detail::__synth_three_way);
}
template <class _Key, class _Tp, class _Compare, class _Allocator>
constexpr bool operator==(const multimap<_Key, _Tp, _Compare, _Allocator>& __x,
                          const multimap<_Key, _Tp, _Compare, _Allocator>& y) {
  return __x.size() == y.size() && std::equal(__x.begin(), __x.end(), y.begin());
}
template <class _Key, class _Tp, class _Compare, class _Allocator>
constexpr __ycxx::__detail::__synth_three_way_result<pair<const _Key, _Tp>>
operator<=>(const multimap<_Key, _Tp, _Compare, _Allocator>& __x, const multimap<_Key, _Tp, _Compare, _Allocator>& y) {
  return std::lexicographical_compare_three_way(__x.begin(), __x.end(), y.begin(), y.end(),
                                                __ycxx::__detail::__synth_three_way);
}

// ---- specialized algorithms ----
template <class _Key, class _Tp, class _Compare, class _Allocator>
constexpr void swap(map<_Key, _Tp, _Compare, _Allocator>& __x, map<_Key, _Tp, _Compare, _Allocator>& y) noexcept(
    noexcept(__x.swap(y))) {
  __x.swap(y);
}
template <class _Key, class _Tp, class _Compare, class _Allocator>
constexpr void swap(multimap<_Key, _Tp, _Compare, _Allocator>& __x, multimap<_Key, _Tp, _Compare, _Allocator>& y) noexcept(
    noexcept(__x.swap(y))) {
  __x.swap(y);
}

// ---- [map.erasure], [multimap.erasure] ----
template <class _Key, class _Tp, class _Compare, class _Allocator, class _Predicate>
constexpr typename map<_Key, _Tp, _Compare, _Allocator>::size_type erase_if(map<_Key, _Tp, _Compare, _Allocator>& c,
                                                                       _Predicate pred) {
  auto __original_size = c.size();
  for (auto i = c.begin(), last = c.end(); i != last;) {
    if (pred(*i))
      i = c.erase(i);
    else
      ++i;
  }
  return __original_size - c.size();
}
template <class _Key, class _Tp, class _Compare, class _Allocator, class _Predicate>
constexpr typename multimap<_Key, _Tp, _Compare, _Allocator>::size_type erase_if(multimap<_Key, _Tp, _Compare, _Allocator>& c,
                                                                            _Predicate pred) {
  auto __original_size = c.size();
  for (auto i = c.begin(), last = c.end(); i != last;) {
    if (pred(*i))
      i = c.erase(i);
    else
      ++i;
  }
  return __original_size - c.size();
}

namespace pmr {
template <class _Key, class _Tp, class _Compare = less<_Key>>
using map = std::map<_Key, _Tp, _Compare, polymorphic_allocator<pair<const _Key, _Tp>>>;
template <class _Key, class _Tp, class _Compare = less<_Key>>
using multimap = std::multimap<_Key, _Tp, _Compare, polymorphic_allocator<pair<const _Key, _Tp>>>;
} // namespace pmr

}} // namespace std
