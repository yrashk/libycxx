// libycxx core: set and multiset ([set], [multiset]), their comparisons, erasure, deduction
// guides and the pmr:: aliases. Both are thin layers over the red-black tree of rb_tree.hpp;
// iterator and const_iterator are the same (constant) iterator type.
#pragma once

#include <initializer_list>
#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/memory_resource_fwd.hpp>
#include <ycxx/core/rb_tree.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Key, class _Compare = less<_Key>, class _Allocator = allocator<_Key>>
class set;
template <class _Key, class _Compare = less<_Key>, class _Allocator = allocator<_Key>>
class multiset;

template <class _Key, class _Compare, class _Allocator>
class set : public __ycxx::__adl_free::__rb_tree<_Key, _Key, _Compare, _Allocator, false, false> {
  static_assert(__ycxx::__detail::__allocator_for<_Allocator, _Key>,
                "std::set: Allocator::value_type must be Key ([container.alloc.reqmts])");

  using base = __ycxx::__adl_free::__rb_tree<_Key, _Key, _Compare, _Allocator, false, false>;
  using __alloc_traits = allocator_traits<_Allocator>;
  using __node_base = typename base::__node_base;
  static constexpr bool __always_equal = base::__always_equal;

public:
  // ---- types ----
  using key_type = _Key;
  using key_compare = _Compare;
  using value_type = _Key;
  using value_compare = _Compare;
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

  // ---- [set.cons] ----
  constexpr set() noexcept(base::__nothrow_default) : set(_Compare()) {}
  constexpr explicit set(const _Compare& comp, const _Allocator& a = _Allocator()) noexcept(
      is_nothrow_copy_constructible_v<_Compare>)
      : base(comp, a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr set(_InputIterator first, _InputIterator last, const _Compare& comp = _Compare(),
                const _Allocator& a = _Allocator())
      : base(comp, a) {
    this->__insert_elems(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr set(_Tag, _Rp&& __rg, const _Compare& comp = _Compare(), const _Allocator& a = _Allocator()) : base(comp, a) {
    this->__insert_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr set(const set& __x) : base(__x, __alloc_traits::select_on_container_copy_construction(__x.get_allocator())) {}
  constexpr set(set&& __x) noexcept(is_nothrow_copy_constructible_v<_Compare>) : base(static_cast<base&&>(__x)) {}
  constexpr explicit set(const _Allocator& a) : base(_Compare(), a) {}
  constexpr set(const set& __x, const type_identity_t<_Allocator>& a) : base(__x, a) {}
  constexpr set(set&& __x, const type_identity_t<_Allocator>& a) noexcept(base::__nothrow_move_alloc)
      : base(static_cast<base&&>(__x), a) {}
  constexpr set(initializer_list<value_type> il, const type_identity_t<_Compare>& comp = _Compare(),
                const type_identity_t<_Allocator>& a = _Allocator())
      : base(comp, a) {
    this->__insert_elems(il.begin(), il.end());
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr set(_InputIterator first, _InputIterator last, const _Allocator& a)
      : set(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last), _Compare(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr set(_Tag, _Rp&& __rg, const _Allocator& a) : set(from_range, static_cast<_Rp&&>(__rg), _Compare(), a) {}
  constexpr set(initializer_list<value_type> il, const type_identity_t<_Allocator>& a) : set(il, _Compare(), a) {}
  constexpr ~set() = default;

  constexpr set& operator=(const set& __x) {
    this->__copy_assign(__x);
    return *this;
  }
  constexpr set& operator=(set&& __x) noexcept(__always_equal && is_nothrow_move_assignable_v<_Compare>) {
    this->__move_assign(__x);
    return *this;
  }
  constexpr set& operator=(initializer_list<value_type> il) {
    this->clear();
    this->__insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- [set.modifiers] ----
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
  template <class _Kp>
    requires __ycxx::__detail::__transparent_compare<_Compare>
  constexpr pair<iterator, bool> insert(_Kp&& __x) {
    auto r = this->__emplace_key(nullptr, __x, static_cast<_Kp&&>(__x));
    return {iterator(r.first), r.second};
  }
  constexpr iterator insert(const_iterator position, const value_type& __x) { return emplace_hint(position, __x); }
  constexpr iterator insert(const_iterator position, value_type&& __x) {
    return emplace_hint(position, static_cast<value_type&&>(__x));
  }
  template <class _Kp>
    requires __ycxx::__detail::__transparent_non_iter<_Compare, _Kp, iterator, const_iterator>
  constexpr iterator insert(const_iterator position, _Kp&& __x) {
    __node_base* h = base::__nd(position);
    return iterator(this->__emplace_key(h ? h : this->__header(), __x, static_cast<_Kp&&>(__x)).first);
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
    __node_base* h = base::__nd(__hint);
    return iterator(this->__insert_handle_unique(h ? h : this->__header(), __nh).first);
  }

  constexpr void swap(set& __x) noexcept(is_nothrow_swappable_v<_Compare>) { this->__swap_tree(__x); }

  template <class _C2>
  constexpr void merge(set<_Key, _C2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _C2>
  constexpr void merge(set<_Key, _C2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }
  template <class _C2>
  constexpr void merge(multiset<_Key, _C2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _C2>
  constexpr void merge(multiset<_Key, _C2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }

  // ---- observers ----
  constexpr value_compare value_comp() const { return this->__comp_; }
};

template <class _Key, class _Compare, class _Allocator>
class multiset : public __ycxx::__adl_free::__rb_tree<_Key, _Key, _Compare, _Allocator, false, true> {
  static_assert(__ycxx::__detail::__allocator_for<_Allocator, _Key>,
                "std::multiset: Allocator::value_type must be Key ([container.alloc.reqmts])");

  using base = __ycxx::__adl_free::__rb_tree<_Key, _Key, _Compare, _Allocator, false, true>;
  using __alloc_traits = allocator_traits<_Allocator>;
  using __node_base = typename base::__node_base;
  static constexpr bool __always_equal = base::__always_equal;

public:
  // ---- types ----
  using key_type = _Key;
  using key_compare = _Compare;
  using value_type = _Key;
  using value_compare = _Compare;
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

  // ---- [multiset.cons] ----
  constexpr multiset() noexcept(base::__nothrow_default) : multiset(_Compare()) {}
  constexpr explicit multiset(const _Compare& comp, const _Allocator& a = _Allocator()) noexcept(
      is_nothrow_copy_constructible_v<_Compare>)
      : base(comp, a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr multiset(_InputIterator first, _InputIterator last, const _Compare& comp = _Compare(),
                     const _Allocator& a = _Allocator())
      : base(comp, a) {
    this->__insert_elems(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr multiset(_Tag, _Rp&& __rg, const _Compare& comp = _Compare(), const _Allocator& a = _Allocator())
      : base(comp, a) {
    this->__insert_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr multiset(const multiset& __x)
      : base(__x, __alloc_traits::select_on_container_copy_construction(__x.get_allocator())) {}
  constexpr multiset(multiset&& __x) noexcept(is_nothrow_copy_constructible_v<_Compare>) : base(static_cast<base&&>(__x)) {}
  constexpr explicit multiset(const _Allocator& a) : base(_Compare(), a) {}
  constexpr multiset(const multiset& __x, const type_identity_t<_Allocator>& a) : base(__x, a) {}
  constexpr multiset(multiset&& __x, const type_identity_t<_Allocator>& a) noexcept(base::__nothrow_move_alloc)
      : base(static_cast<base&&>(__x), a) {}
  constexpr multiset(initializer_list<value_type> il, const type_identity_t<_Compare>& comp = _Compare(),
                     const type_identity_t<_Allocator>& a = _Allocator())
      : base(comp, a) {
    this->__insert_elems(il.begin(), il.end());
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr multiset(_InputIterator first, _InputIterator last, const _Allocator& a)
      : multiset(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last), _Compare(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr multiset(_Tag, _Rp&& __rg, const _Allocator& a) : multiset(from_range, static_cast<_Rp&&>(__rg), _Compare(), a) {}
  constexpr multiset(initializer_list<value_type> il, const type_identity_t<_Allocator>& a)
      : multiset(il, _Compare(), a) {}
  constexpr ~multiset() = default;

  constexpr multiset& operator=(const multiset& __x) {
    this->__copy_assign(__x);
    return *this;
  }
  constexpr multiset& operator=(multiset&& __x) noexcept(__always_equal && is_nothrow_move_assignable_v<_Compare>) {
    this->__move_assign(__x);
    return *this;
  }
  constexpr multiset& operator=(initializer_list<value_type> il) {
    this->clear();
    this->__insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- modifiers ----
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
  constexpr iterator insert(const_iterator position, const value_type& __x) { return emplace_hint(position, __x); }
  constexpr iterator insert(const_iterator position, value_type&& __x) {
    return emplace_hint(position, static_cast<value_type&&>(__x));
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

  constexpr void swap(multiset& __x) noexcept(is_nothrow_swappable_v<_Compare>) { this->__swap_tree(__x); }

  template <class _C2>
  constexpr void merge(multiset<_Key, _C2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _C2>
  constexpr void merge(multiset<_Key, _C2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }
  template <class _C2>
  constexpr void merge(set<_Key, _C2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _C2>
  constexpr void merge(set<_Key, _C2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }

  // ---- observers ----
  constexpr value_compare value_comp() const { return this->__comp_; }
};

// ---- deduction guides ----
template <class _InputIterator, class _Compare = less<__ycxx::__detail::__iter_value_type<_InputIterator>>,
          class _Allocator = allocator<__ycxx::__detail::__iter_value_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__deducible_compare<_Compare> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
set(_InputIterator, _InputIterator, _Compare = _Compare(), _Allocator = _Allocator())
    -> set<__ycxx::__detail::__iter_value_type<_InputIterator>, _Compare, _Allocator>;
template <ranges::input_range _Rp, class _Compare = less<ranges::range_value_t<_Rp>>,
          class _Allocator = allocator<ranges::range_value_t<_Rp>>>
  requires __ycxx::__detail::__deducible_compare<_Compare> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
set(from_range_t, _Rp&&, _Compare = _Compare(), _Allocator = _Allocator())
    -> set<ranges::range_value_t<_Rp>, _Compare, _Allocator>;
template <class _Key, class _Compare = less<_Key>, class _Allocator = allocator<_Key>>
  requires __ycxx::__detail::__deducible_compare<_Compare> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
set(initializer_list<_Key>, _Compare = _Compare(), _Allocator = _Allocator()) -> set<_Key, _Compare, _Allocator>;
template <class _InputIterator, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
set(_InputIterator, _InputIterator, _Allocator)
    -> set<__ycxx::__detail::__iter_value_type<_InputIterator>, less<__ycxx::__detail::__iter_value_type<_InputIterator>>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
set(from_range_t, _Rp&&, _Allocator) -> set<ranges::range_value_t<_Rp>, less<ranges::range_value_t<_Rp>>, _Allocator>;
template <class _Key, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
set(initializer_list<_Key>, _Allocator) -> set<_Key, less<_Key>, _Allocator>;

template <class _InputIterator, class _Compare = less<__ycxx::__detail::__iter_value_type<_InputIterator>>,
          class _Allocator = allocator<__ycxx::__detail::__iter_value_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__deducible_compare<_Compare> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
multiset(_InputIterator, _InputIterator, _Compare = _Compare(), _Allocator = _Allocator())
    -> multiset<__ycxx::__detail::__iter_value_type<_InputIterator>, _Compare, _Allocator>;
template <ranges::input_range _Rp, class _Compare = less<ranges::range_value_t<_Rp>>,
          class _Allocator = allocator<ranges::range_value_t<_Rp>>>
  requires __ycxx::__detail::__deducible_compare<_Compare> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
multiset(from_range_t, _Rp&&, _Compare = _Compare(), _Allocator = _Allocator())
    -> multiset<ranges::range_value_t<_Rp>, _Compare, _Allocator>;
template <class _Key, class _Compare = less<_Key>, class _Allocator = allocator<_Key>>
  requires __ycxx::__detail::__deducible_compare<_Compare> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
multiset(initializer_list<_Key>, _Compare = _Compare(), _Allocator = _Allocator()) -> multiset<_Key, _Compare, _Allocator>;
template <class _InputIterator, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
multiset(_InputIterator, _InputIterator, _Allocator)
    -> multiset<__ycxx::__detail::__iter_value_type<_InputIterator>, less<__ycxx::__detail::__iter_value_type<_InputIterator>>,
                _Allocator>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
multiset(from_range_t, _Rp&&, _Allocator)
    -> multiset<ranges::range_value_t<_Rp>, less<ranges::range_value_t<_Rp>>, _Allocator>;
template <class _Key, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
multiset(initializer_list<_Key>, _Allocator) -> multiset<_Key, less<_Key>, _Allocator>;

// ---- comparisons ----
template <class _Key, class _Compare, class _Allocator>
constexpr bool operator==(const set<_Key, _Compare, _Allocator>& __x, const set<_Key, _Compare, _Allocator>& y) {
  return __x.size() == y.size() && std::equal(__x.begin(), __x.end(), y.begin());
}
template <class _Key, class _Compare, class _Allocator>
constexpr __ycxx::__detail::__synth_three_way_result<_Key> operator<=>(const set<_Key, _Compare, _Allocator>& __x,
                                                                const set<_Key, _Compare, _Allocator>& y) {
  return std::lexicographical_compare_three_way(__x.begin(), __x.end(), y.begin(), y.end(),
                                                __ycxx::__detail::__synth_three_way);
}
template <class _Key, class _Compare, class _Allocator>
constexpr bool operator==(const multiset<_Key, _Compare, _Allocator>& __x, const multiset<_Key, _Compare, _Allocator>& y) {
  return __x.size() == y.size() && std::equal(__x.begin(), __x.end(), y.begin());
}
template <class _Key, class _Compare, class _Allocator>
constexpr __ycxx::__detail::__synth_three_way_result<_Key> operator<=>(const multiset<_Key, _Compare, _Allocator>& __x,
                                                                const multiset<_Key, _Compare, _Allocator>& y) {
  return std::lexicographical_compare_three_way(__x.begin(), __x.end(), y.begin(), y.end(),
                                                __ycxx::__detail::__synth_three_way);
}

// ---- specialized algorithms ----
template <class _Key, class _Compare, class _Allocator>
constexpr void swap(set<_Key, _Compare, _Allocator>& __x, set<_Key, _Compare, _Allocator>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}
template <class _Key, class _Compare, class _Allocator>
constexpr void swap(multiset<_Key, _Compare, _Allocator>& __x, multiset<_Key, _Compare, _Allocator>& y) noexcept(
    noexcept(__x.swap(y))) {
  __x.swap(y);
}

// ---- [set.erasure], [multiset.erasure] ----
template <class _Key, class _Compare, class _Allocator, class _Predicate>
constexpr typename set<_Key, _Compare, _Allocator>::size_type erase_if(set<_Key, _Compare, _Allocator>& c, _Predicate pred) {
  auto __original_size = c.size();
  for (auto i = c.begin(), last = c.end(); i != last;) {
    if (pred(*i))
      i = c.erase(i);
    else
      ++i;
  }
  return __original_size - c.size();
}
template <class _Key, class _Compare, class _Allocator, class _Predicate>
constexpr typename multiset<_Key, _Compare, _Allocator>::size_type erase_if(multiset<_Key, _Compare, _Allocator>& c,
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
template <class _Key, class _Compare = less<_Key>>
using set = std::set<_Key, _Compare, polymorphic_allocator<_Key>>;
template <class _Key, class _Compare = less<_Key>>
using multiset = std::multiset<_Key, _Compare, polymorphic_allocator<_Key>>;
} // namespace pmr

}} // namespace std
