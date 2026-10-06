// libycxx core: unordered_set and unordered_multiset ([unord.set], [unord.multiset]), their
// comparisons, swap, erasure, deduction guides and the pmr:: aliases. The table itself is
// __ycxx::__adl_free::__hash_table (hash_table.hpp); iterator and const_iterator are the same
// constant iterator type ([unord.req.general]/8).
#pragma once

#include <ycxx/core/hash_table.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Key, class _Hash = hash<_Key>, class _Pred = equal_to<_Key>, class _Allocator = allocator<_Key>>
class unordered_set;
template <class _Key, class _Hash = hash<_Key>, class _Pred = equal_to<_Key>, class _Allocator = allocator<_Key>>
class unordered_multiset;

template <class _Key, class _Hash, class _Pred, class _Allocator>
class unordered_set : public __ycxx::__adl_free::__hash_table<_Key, _Key, _Hash, _Pred, _Allocator, false> {
  static_assert(__ycxx::__detail::__allocator_for<_Allocator, _Key>,
                "std::unordered_set: Allocator::value_type must be Key ([container.alloc.reqmts])");
  using base = __ycxx::__adl_free::__hash_table<_Key, _Key, _Hash, _Pred, _Allocator, false>;

public:
  // ---- types ----
  using key_type = _Key;
  // Declared here, not inherited, so that the implicit deduction guides can deduce from
  // initializer_list<value_type>.
  using value_type = _Key;
  using typename base::hasher;
  using typename base::key_equal;
  using typename base::allocator_type;
  using typename base::pointer;
  using typename base::const_pointer;
  using typename base::reference;
  using typename base::const_reference;
  using typename base::size_type;
  using typename base::difference_type;
  using typename base::iterator;
  using typename base::const_iterator;
  using typename base::local_iterator;
  using typename base::const_local_iterator;
  using typename base::node_type;
  using insert_return_type = __ycxx::__adl_free::insert_return_type<iterator, node_type>;

  // ---- [unord.set.cnstr] ----
  // Constrained, and noexcept when nothing can throw (both strengthenings): the default
  // constructor allocates nothing.
  constexpr unordered_set() noexcept(base::__nothrow_default)
    requires default_initializable<_Hash> && default_initializable<_Pred> && default_initializable<_Allocator>
      : base() {}
  constexpr explicit unordered_set(size_type n, const hasher& __hf = hasher(), const key_equal& __eql = key_equal(),
                                   const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_set(_InputIterator __f, _InputIterator __l, size_type n = 0, const hasher& __hf = hasher(),
                          const key_equal& __eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {
    this->__insert_elems(static_cast<_InputIterator&&>(__f), static_cast<_InputIterator&&>(__l));
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_set(_Tag, _Rp&& __rg, size_type n = 0, const hasher& __hf = hasher(), const key_equal& __eql = key_equal(),
                          const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {
    this->__insert_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr unordered_set(const unordered_set&) = default;
  constexpr unordered_set(unordered_set&&) = default;
  constexpr explicit unordered_set(const _Allocator& a) : base(0, hasher(), key_equal(), a) {}
  constexpr unordered_set(const unordered_set& __x, const type_identity_t<_Allocator>& a) : base(__x, a) {}
  constexpr unordered_set(unordered_set&& __x, const type_identity_t<_Allocator>& a)
      noexcept(base::__nothrow_move && allocator_traits<_Allocator>::is_always_equal::value) : base(static_cast<base&&>(__x), a) {}
  constexpr unordered_set(initializer_list<value_type> il, size_type n = 0, const hasher& __hf = hasher(),
                          const key_equal& __eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {
    this->__insert_elems(il.begin(), il.end());
  }
  constexpr unordered_set(size_type n, const allocator_type& a) : unordered_set(n, hasher(), key_equal(), a) {}
  constexpr unordered_set(size_type n, const hasher& __hf, const allocator_type& a)
      : unordered_set(n, __hf, key_equal(), a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_set(_InputIterator __f, _InputIterator __l, size_type n, const allocator_type& a)
      : unordered_set(__f, __l, n, hasher(), key_equal(), a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_set(_InputIterator __f, _InputIterator __l, size_type n, const hasher& __hf, const allocator_type& a)
      : unordered_set(__f, __l, n, __hf, key_equal(), a) {}
  constexpr unordered_set(initializer_list<value_type> il, size_type n, const allocator_type& a)
      : unordered_set(il, n, hasher(), key_equal(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_set(_Tag t, _Rp&& __rg, size_type n, const allocator_type& a)
      : unordered_set(t, static_cast<_Rp&&>(__rg), n, hasher(), key_equal(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_set(_Tag t, _Rp&& __rg, size_type n, const hasher& __hf, const allocator_type& a)
      : unordered_set(t, static_cast<_Rp&&>(__rg), n, __hf, key_equal(), a) {}
  constexpr unordered_set(initializer_list<value_type> il, size_type n, const hasher& __hf, const allocator_type& a)
      : unordered_set(il, n, __hf, key_equal(), a) {}
  // Extensions (LWG 2713): the allocator-only forms the deduction guides already deduce from.
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_set(_InputIterator __f, _InputIterator __l, const allocator_type& a)
      : unordered_set(__f, __l, 0, hasher(), key_equal(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_set(_Tag t, _Rp&& __rg, const allocator_type& a)
      : unordered_set(t, static_cast<_Rp&&>(__rg), 0, hasher(), key_equal(), a) {}
  constexpr unordered_set(initializer_list<value_type> il, const allocator_type& a)
      : unordered_set(il, 0, hasher(), key_equal(), a) {}
  constexpr ~unordered_set() = default;

  constexpr unordered_set& operator=(const unordered_set& __x) {
    this->__copy_assign(__x);
    return *this;
  }
  constexpr unordered_set& operator=(unordered_set&& __x) noexcept(allocator_traits<_Allocator>::is_always_equal::value &&
                                                                  is_nothrow_move_assignable_v<_Hash> &&
                                                                  is_nothrow_move_assignable_v<_Pred>) {
    this->__move_assign(__x);
    return *this;
  }
  constexpr unordered_set& operator=(initializer_list<value_type> il) {
    this->__assign_il(il);
    return *this;
  }

  // ---- [unord.set.modifiers] ----
  using base::insert;
  template <class _Kp>
    requires base::__transparent
  constexpr pair<iterator, bool> insert(_Kp&& __obj) {
    return this->__emplace_key(__obj, static_cast<_Kp&&>(__obj));
  }
  template <class _Kp>
    requires base::__transparent && __ycxx::__detail::__not_iterator_arg<_Kp, iterator, const_iterator>
  constexpr iterator insert(const_iterator, _Kp&& __obj) {
    return this->__emplace_key(__obj, static_cast<_Kp&&>(__obj)).first;
  }

  constexpr void swap(unordered_set& __x) noexcept(allocator_traits<_Allocator>::is_always_equal::value &&
                                                 is_nothrow_swappable_v<_Hash> && is_nothrow_swappable_v<_Pred>) {
    this->__swap_impl(__x);
  }

  template <class _H2, class _P2>
  constexpr void merge(unordered_set<_Key, _H2, _P2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _H2, class _P2>
  constexpr void merge(unordered_set<_Key, _H2, _P2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }
  template <class _H2, class _P2>
  constexpr void merge(unordered_multiset<_Key, _H2, _P2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _H2, class _P2>
  constexpr void merge(unordered_multiset<_Key, _H2, _P2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }
};

template <class _Key, class _Hash, class _Pred, class _Allocator>
class unordered_multiset : public __ycxx::__adl_free::__hash_table<_Key, _Key, _Hash, _Pred, _Allocator, true> {
  static_assert(__ycxx::__detail::__allocator_for<_Allocator, _Key>,
                "std::unordered_multiset: Allocator::value_type must be Key ([container.alloc.reqmts])");
  using base = __ycxx::__adl_free::__hash_table<_Key, _Key, _Hash, _Pred, _Allocator, true>;

public:
  // ---- types ----
  using key_type = _Key;
  // Declared here, not inherited, so that the implicit deduction guides can deduce from
  // initializer_list<value_type>.
  using value_type = _Key;
  using typename base::hasher;
  using typename base::key_equal;
  using typename base::allocator_type;
  using typename base::pointer;
  using typename base::const_pointer;
  using typename base::reference;
  using typename base::const_reference;
  using typename base::size_type;
  using typename base::difference_type;
  using typename base::iterator;
  using typename base::const_iterator;
  using typename base::local_iterator;
  using typename base::const_local_iterator;
  using typename base::node_type;

  // ---- [unord.multiset.cnstr] ----
  // Constrained, and noexcept when nothing can throw (both strengthenings): the default
  // constructor allocates nothing.
  constexpr unordered_multiset() noexcept(base::__nothrow_default)
    requires default_initializable<_Hash> && default_initializable<_Pred> && default_initializable<_Allocator>
      : base() {}
  constexpr explicit unordered_multiset(size_type n, const hasher& __hf = hasher(), const key_equal& __eql = key_equal(),
                                        const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_multiset(_InputIterator __f, _InputIterator __l, size_type n = 0, const hasher& __hf = hasher(),
                               const key_equal& __eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {
    this->__insert_elems(static_cast<_InputIterator&&>(__f), static_cast<_InputIterator&&>(__l));
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_multiset(_Tag, _Rp&& __rg, size_type n = 0, const hasher& __hf = hasher(),
                               const key_equal& __eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {
    this->__insert_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr unordered_multiset(const unordered_multiset&) = default;
  constexpr unordered_multiset(unordered_multiset&&) = default;
  constexpr explicit unordered_multiset(const _Allocator& a) : base(0, hasher(), key_equal(), a) {}
  constexpr unordered_multiset(const unordered_multiset& __x, const type_identity_t<_Allocator>& a) : base(__x, a) {}
  constexpr unordered_multiset(unordered_multiset&& __x, const type_identity_t<_Allocator>& a)
      noexcept(base::__nothrow_move && allocator_traits<_Allocator>::is_always_equal::value)
      : base(static_cast<base&&>(__x), a) {}
  constexpr unordered_multiset(initializer_list<value_type> il, size_type n = 0, const hasher& __hf = hasher(),
                               const key_equal& __eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {
    this->__insert_elems(il.begin(), il.end());
  }
  constexpr unordered_multiset(size_type n, const allocator_type& a)
      : unordered_multiset(n, hasher(), key_equal(), a) {}
  constexpr unordered_multiset(size_type n, const hasher& __hf, const allocator_type& a)
      : unordered_multiset(n, __hf, key_equal(), a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_multiset(_InputIterator __f, _InputIterator __l, size_type n, const allocator_type& a)
      : unordered_multiset(__f, __l, n, hasher(), key_equal(), a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_multiset(_InputIterator __f, _InputIterator __l, size_type n, const hasher& __hf,
                               const allocator_type& a)
      : unordered_multiset(__f, __l, n, __hf, key_equal(), a) {}
  constexpr unordered_multiset(initializer_list<value_type> il, size_type n, const allocator_type& a)
      : unordered_multiset(il, n, hasher(), key_equal(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_multiset(_Tag t, _Rp&& __rg, size_type n, const allocator_type& a)
      : unordered_multiset(t, static_cast<_Rp&&>(__rg), n, hasher(), key_equal(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_multiset(_Tag t, _Rp&& __rg, size_type n, const hasher& __hf, const allocator_type& a)
      : unordered_multiset(t, static_cast<_Rp&&>(__rg), n, __hf, key_equal(), a) {}
  constexpr unordered_multiset(initializer_list<value_type> il, size_type n, const hasher& __hf,
                               const allocator_type& a)
      : unordered_multiset(il, n, __hf, key_equal(), a) {}
  // Extensions (LWG 2713): the allocator-only forms the deduction guides already deduce from.
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_multiset(_InputIterator __f, _InputIterator __l, const allocator_type& a)
      : unordered_multiset(__f, __l, 0, hasher(), key_equal(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_multiset(_Tag t, _Rp&& __rg, const allocator_type& a)
      : unordered_multiset(t, static_cast<_Rp&&>(__rg), 0, hasher(), key_equal(), a) {}
  constexpr unordered_multiset(initializer_list<value_type> il, const allocator_type& a)
      : unordered_multiset(il, 0, hasher(), key_equal(), a) {}
  constexpr ~unordered_multiset() = default;

  constexpr unordered_multiset& operator=(const unordered_multiset& __x) {
    this->__copy_assign(__x);
    return *this;
  }
  constexpr unordered_multiset& operator=(unordered_multiset&& __x) noexcept(
      allocator_traits<_Allocator>::is_always_equal::value && is_nothrow_move_assignable_v<_Hash> &&
      is_nothrow_move_assignable_v<_Pred>) {
    this->__move_assign(__x);
    return *this;
  }
  constexpr unordered_multiset& operator=(initializer_list<value_type> il) {
    this->__assign_il(il);
    return *this;
  }

  constexpr void swap(unordered_multiset& __x) noexcept(allocator_traits<_Allocator>::is_always_equal::value &&
                                                      is_nothrow_swappable_v<_Hash> && is_nothrow_swappable_v<_Pred>) {
    this->__swap_impl(__x);
  }

  template <class _H2, class _P2>
  constexpr void merge(unordered_multiset<_Key, _H2, _P2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _H2, class _P2>
  constexpr void merge(unordered_multiset<_Key, _H2, _P2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }
  template <class _H2, class _P2>
  constexpr void merge(unordered_set<_Key, _H2, _P2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _H2, class _P2>
  constexpr void merge(unordered_set<_Key, _H2, _P2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }
};

// ---- deduction guides ([unord.set.overview], [unord.multiset.overview]) ----
template <class _InputIterator, class _Hash = hash<__ycxx::__detail::__iter_value_type<_InputIterator>>,
          class _Pred = equal_to<__ycxx::__detail::__iter_value_type<_InputIterator>>,
          class _Allocator = allocator<__ycxx::__detail::__iter_value_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__unord_hash_arg<_Hash> &&
           __ycxx::__detail::__unord_pred_arg<_Pred> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_set(_InputIterator, _InputIterator, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type = 0,
              _Hash = _Hash(), _Pred = _Pred(), _Allocator = _Allocator())
    -> unordered_set<__ycxx::__detail::__iter_value_type<_InputIterator>, _Hash, _Pred, _Allocator>;
template <ranges::input_range _Rp, class _Hash = hash<ranges::range_value_t<_Rp>>,
          class _Pred = equal_to<ranges::range_value_t<_Rp>>, class _Allocator = allocator<ranges::range_value_t<_Rp>>>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__unord_pred_arg<_Pred> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_set(from_range_t, _Rp&&, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type = 0, _Hash = _Hash(),
              _Pred = _Pred(), _Allocator = _Allocator()) -> unordered_set<ranges::range_value_t<_Rp>, _Hash, _Pred, _Allocator>;
template <class _Tp, class _Hash = hash<_Tp>, class _Pred = equal_to<_Tp>, class _Allocator = allocator<_Tp>>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__unord_pred_arg<_Pred> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_set(initializer_list<_Tp>, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type = 0, _Hash = _Hash(),
              _Pred = _Pred(), _Allocator = _Allocator()) -> unordered_set<_Tp, _Hash, _Pred, _Allocator>;
template <class _InputIterator, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_set(_InputIterator, _InputIterator, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Allocator)
    -> unordered_set<__ycxx::__detail::__iter_value_type<_InputIterator>, hash<__ycxx::__detail::__iter_value_type<_InputIterator>>,
                     equal_to<__ycxx::__detail::__iter_value_type<_InputIterator>>, _Allocator>;
template <class _InputIterator, class _Hash, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__unord_hash_arg<_Hash> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_set(_InputIterator, _InputIterator, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Hash, _Allocator)
    -> unordered_set<__ycxx::__detail::__iter_value_type<_InputIterator>, _Hash,
                     equal_to<__ycxx::__detail::__iter_value_type<_InputIterator>>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_set(from_range_t, _Rp&&, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Allocator)
    -> unordered_set<ranges::range_value_t<_Rp>, hash<ranges::range_value_t<_Rp>>, equal_to<ranges::range_value_t<_Rp>>,
                     _Allocator>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_set(from_range_t, _Rp&&, _Allocator)
    -> unordered_set<ranges::range_value_t<_Rp>, hash<ranges::range_value_t<_Rp>>, equal_to<ranges::range_value_t<_Rp>>,
                     _Allocator>;
template <ranges::input_range _Rp, class _Hash, class _Allocator>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_set(from_range_t, _Rp&&, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Hash, _Allocator)
    -> unordered_set<ranges::range_value_t<_Rp>, _Hash, equal_to<ranges::range_value_t<_Rp>>, _Allocator>;
template <class _Tp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_set(initializer_list<_Tp>, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Allocator)
    -> unordered_set<_Tp, hash<_Tp>, equal_to<_Tp>, _Allocator>;
template <class _Tp, class _Hash, class _Allocator>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_set(initializer_list<_Tp>, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Hash, _Allocator)
    -> unordered_set<_Tp, _Hash, equal_to<_Tp>, _Allocator>;

template <class _InputIterator, class _Hash = hash<__ycxx::__detail::__iter_value_type<_InputIterator>>,
          class _Pred = equal_to<__ycxx::__detail::__iter_value_type<_InputIterator>>,
          class _Allocator = allocator<__ycxx::__detail::__iter_value_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__unord_hash_arg<_Hash> &&
           __ycxx::__detail::__unord_pred_arg<_Pred> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multiset(_InputIterator, _InputIterator, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type = 0,
                   _Hash = _Hash(), _Pred = _Pred(), _Allocator = _Allocator())
    -> unordered_multiset<__ycxx::__detail::__iter_value_type<_InputIterator>, _Hash, _Pred, _Allocator>;
template <ranges::input_range _Rp, class _Hash = hash<ranges::range_value_t<_Rp>>,
          class _Pred = equal_to<ranges::range_value_t<_Rp>>, class _Allocator = allocator<ranges::range_value_t<_Rp>>>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__unord_pred_arg<_Pred> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multiset(from_range_t, _Rp&&, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type = 0, _Hash = _Hash(),
                   _Pred = _Pred(), _Allocator = _Allocator())
    -> unordered_multiset<ranges::range_value_t<_Rp>, _Hash, _Pred, _Allocator>;
template <class _Tp, class _Hash = hash<_Tp>, class _Pred = equal_to<_Tp>, class _Allocator = allocator<_Tp>>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__unord_pred_arg<_Pred> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multiset(initializer_list<_Tp>, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type = 0, _Hash = _Hash(),
                   _Pred = _Pred(), _Allocator = _Allocator()) -> unordered_multiset<_Tp, _Hash, _Pred, _Allocator>;
template <class _InputIterator, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multiset(_InputIterator, _InputIterator, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Allocator)
    -> unordered_multiset<__ycxx::__detail::__iter_value_type<_InputIterator>,
                          hash<__ycxx::__detail::__iter_value_type<_InputIterator>>,
                          equal_to<__ycxx::__detail::__iter_value_type<_InputIterator>>, _Allocator>;
template <class _InputIterator, class _Hash, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__unord_hash_arg<_Hash> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multiset(_InputIterator, _InputIterator, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Hash,
                   _Allocator)
    -> unordered_multiset<__ycxx::__detail::__iter_value_type<_InputIterator>, _Hash,
                          equal_to<__ycxx::__detail::__iter_value_type<_InputIterator>>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multiset(from_range_t, _Rp&&, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Allocator)
    -> unordered_multiset<ranges::range_value_t<_Rp>, hash<ranges::range_value_t<_Rp>>,
                          equal_to<ranges::range_value_t<_Rp>>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multiset(from_range_t, _Rp&&, _Allocator)
    -> unordered_multiset<ranges::range_value_t<_Rp>, hash<ranges::range_value_t<_Rp>>,
                          equal_to<ranges::range_value_t<_Rp>>, _Allocator>;
template <ranges::input_range _Rp, class _Hash, class _Allocator>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multiset(from_range_t, _Rp&&, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Hash, _Allocator)
    -> unordered_multiset<ranges::range_value_t<_Rp>, _Hash, equal_to<ranges::range_value_t<_Rp>>, _Allocator>;
template <class _Tp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multiset(initializer_list<_Tp>, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Allocator)
    -> unordered_multiset<_Tp, hash<_Tp>, equal_to<_Tp>, _Allocator>;
template <class _Tp, class _Hash, class _Allocator>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multiset(initializer_list<_Tp>, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Hash, _Allocator)
    -> unordered_multiset<_Tp, _Hash, equal_to<_Tp>, _Allocator>;

// ---- comparisons, swap ----
template <class _Key, class _Hash, class _Pred, class _Alloc>
constexpr bool operator==(const unordered_set<_Key, _Hash, _Pred, _Alloc>& a, const unordered_set<_Key, _Hash, _Pred, _Alloc>& b) {
  return __ycxx::__detail::__hash_table_access::equal(a, b);
}
template <class _Key, class _Hash, class _Pred, class _Alloc>
constexpr bool operator==(const unordered_multiset<_Key, _Hash, _Pred, _Alloc>& a,
                          const unordered_multiset<_Key, _Hash, _Pred, _Alloc>& b) {
  return __ycxx::__detail::__hash_table_access::equal(a, b);
}
template <class _Key, class _Hash, class _Pred, class _Alloc>
constexpr void swap(unordered_set<_Key, _Hash, _Pred, _Alloc>& __x,
                    unordered_set<_Key, _Hash, _Pred, _Alloc>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}
template <class _Key, class _Hash, class _Pred, class _Alloc>
constexpr void swap(unordered_multiset<_Key, _Hash, _Pred, _Alloc>& __x,
                    unordered_multiset<_Key, _Hash, _Pred, _Alloc>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}

// ---- [unord.set.erasure], [unord.multiset.erasure] ----
template <class _Kp, class _Hp, class _Pp, class _Ap, class _Predicate>
constexpr typename unordered_set<_Kp, _Hp, _Pp, _Ap>::size_type erase_if(unordered_set<_Kp, _Hp, _Pp, _Ap>& c, _Predicate pred) {
  return __ycxx::__detail::__hash_table_access::erase_if(c, pred);
}
template <class _Kp, class _Hp, class _Pp, class _Ap, class _Predicate>
constexpr typename unordered_multiset<_Kp, _Hp, _Pp, _Ap>::size_type erase_if(unordered_multiset<_Kp, _Hp, _Pp, _Ap>& c,
                                                                      _Predicate pred) {
  return __ycxx::__detail::__hash_table_access::erase_if(c, pred);
}

namespace pmr {
template <class _Key, class _Hash = hash<_Key>, class _Pred = equal_to<_Key>>
using unordered_set = std::unordered_set<_Key, _Hash, _Pred, polymorphic_allocator<_Key>>;
template <class _Key, class _Hash = hash<_Key>, class _Pred = equal_to<_Key>>
using unordered_multiset = std::unordered_multiset<_Key, _Hash, _Pred, polymorphic_allocator<_Key>>;
} // namespace pmr

} // namespace std
