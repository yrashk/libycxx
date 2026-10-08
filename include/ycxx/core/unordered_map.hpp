// libycxx core: unordered_map and unordered_multimap ([unord.map], [unord.multimap]), their
// comparisons, swap, erasure, deduction guides and the pmr:: aliases. The table itself is
// __ycxx::__adl_free::__hash_table (hash_table.hpp); these classes add the constructors, assignment,
// swap, merge and the map members (operator[], at, lookup, try_emplace, insert_or_assign,
// insert(P&&)).
#pragma once

#include <ycxx/core/hash_table.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Key, class _Tp, class _Hash = hash<_Key>, class _Pred = equal_to<_Key>,
          class _Allocator = allocator<pair<const _Key, _Tp>>>
class unordered_map;
template <class _Key, class _Tp, class _Hash = hash<_Key>, class _Pred = equal_to<_Key>,
          class _Allocator = allocator<pair<const _Key, _Tp>>>
class unordered_multimap;

template <class _Key, class _Tp, class _Hash, class _Pred, class _Allocator>
class unordered_map : public __ycxx::__adl_free::__hash_table<_Key, pair<const _Key, _Tp>, _Hash, _Pred, _Allocator, false> {
  static_assert(__ycxx::__detail::__allocator_for<_Allocator, pair<const _Key, _Tp>>,
                "std::unordered_map: Allocator::value_type must be pair<const Key, T> ([container.alloc.reqmts])");
  using base = __ycxx::__adl_free::__hash_table<_Key, pair<const _Key, _Tp>, _Hash, _Pred, _Allocator, false>;
  using node = typename base::node;
  using __node_base = typename base::__node_base;

public:
  // ---- types ----
  using key_type = _Key;
  using mapped_type = _Tp;
  // Declared here, not inherited, so that the implicit deduction guides can deduce from
  // initializer_list<value_type>.
  using value_type = pair<const _Key, _Tp>;
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

  // ---- [unord.map.cnstr] ----
  // Constrained, and noexcept when nothing can throw (both strengthenings): the default
  // constructor allocates nothing.
  constexpr unordered_map() noexcept(base::__nothrow_default)
    requires default_initializable<_Hash> && default_initializable<_Pred> && default_initializable<_Allocator>
      : base() {}
  constexpr explicit unordered_map(size_type n, const hasher& __hf = hasher(), const key_equal& __eql = key_equal(),
                                   const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_map(_InputIterator __f, _InputIterator __l, size_type n = 0, const hasher& __hf = hasher(),
                          const key_equal& __eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {
    this->__insert_elems(static_cast<_InputIterator&&>(__f), static_cast<_InputIterator&&>(__l));
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_map(_Tag, _Rp&& __rg, size_type n = 0, const hasher& __hf = hasher(), const key_equal& __eql = key_equal(),
                          const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {
    this->__insert_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr unordered_map(const unordered_map&) = default;
  constexpr unordered_map(unordered_map&&) = default;
  constexpr explicit unordered_map(const _Allocator& a) : base(0, hasher(), key_equal(), a) {}
  constexpr unordered_map(const unordered_map& __x, const type_identity_t<_Allocator>& a) : base(__x, a) {}
  constexpr unordered_map(unordered_map&& __x, const type_identity_t<_Allocator>& a)
      noexcept(base::__nothrow_move && allocator_traits<_Allocator>::is_always_equal::value) : base(static_cast<base&&>(__x), a) {}
  constexpr unordered_map(initializer_list<value_type> il, size_type n = 0, const hasher& __hf = hasher(),
                          const key_equal& __eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {
    this->__insert_elems(il.begin(), il.end());
  }
  constexpr unordered_map(size_type n, const allocator_type& a) : unordered_map(n, hasher(), key_equal(), a) {}
  constexpr unordered_map(size_type n, const hasher& __hf, const allocator_type& a)
      : unordered_map(n, __hf, key_equal(), a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_map(_InputIterator __f, _InputIterator __l, size_type n, const allocator_type& a)
      : unordered_map(__f, __l, n, hasher(), key_equal(), a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_map(_InputIterator __f, _InputIterator __l, size_type n, const hasher& __hf, const allocator_type& a)
      : unordered_map(__f, __l, n, __hf, key_equal(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_map(_Tag t, _Rp&& __rg, size_type n, const allocator_type& a)
      : unordered_map(t, static_cast<_Rp&&>(__rg), n, hasher(), key_equal(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_map(_Tag t, _Rp&& __rg, size_type n, const hasher& __hf, const allocator_type& a)
      : unordered_map(t, static_cast<_Rp&&>(__rg), n, __hf, key_equal(), a) {}
  constexpr unordered_map(initializer_list<value_type> il, size_type n, const allocator_type& a)
      : unordered_map(il, n, hasher(), key_equal(), a) {}
  constexpr unordered_map(initializer_list<value_type> il, size_type n, const hasher& __hf, const allocator_type& a)
      : unordered_map(il, n, __hf, key_equal(), a) {}
  // Extensions (LWG 2713): the allocator-only forms the deduction guides already deduce from.
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_map(_InputIterator __f, _InputIterator __l, const allocator_type& a)
      : unordered_map(__f, __l, 0, hasher(), key_equal(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_map(_Tag t, _Rp&& __rg, const allocator_type& a)
      : unordered_map(t, static_cast<_Rp&&>(__rg), 0, hasher(), key_equal(), a) {}
  constexpr unordered_map(initializer_list<value_type> il, const allocator_type& a)
      : unordered_map(il, 0, hasher(), key_equal(), a) {}
  constexpr ~unordered_map() = default;

  constexpr unordered_map& operator=(const unordered_map& __x) {
    this->__copy_assign(__x);
    return *this;
  }
  constexpr unordered_map& operator=(unordered_map&& __x) noexcept(allocator_traits<_Allocator>::is_always_equal::value &&
                                                                  is_nothrow_move_assignable_v<_Hash> &&
                                                                  is_nothrow_move_assignable_v<_Pred>) {
    this->__move_assign(__x);
    return *this;
  }
  constexpr unordered_map& operator=(initializer_list<value_type> il) {
    this->__assign_il(il);
    return *this;
  }

  // ---- [unord.map.modifiers] ----
  using base::insert;
  template <class _Pp>
    requires is_constructible_v<value_type, _Pp&&>
  constexpr pair<iterator, bool> insert(_Pp&& __obj) {
    return this->emplace(static_cast<_Pp&&>(__obj));
  }
  template <class _Pp>
    requires is_constructible_v<value_type, _Pp&&>
  constexpr iterator insert(const_iterator __hint, _Pp&& __obj) {
    return this->emplace_hint(__hint, static_cast<_Pp&&>(__obj));
  }

  template <class... _Args>
  constexpr pair<iterator, bool> try_emplace(const key_type& k, _Args&&... __args) {
    return this->__emplace_key(k, piecewise_construct, std::forward_as_tuple(k),
                             std::forward_as_tuple(static_cast<_Args&&>(__args)...));
  }
  template <class... _Args>
  constexpr pair<iterator, bool> try_emplace(key_type&& k, _Args&&... __args) {
    return this->__emplace_key(k, piecewise_construct, std::forward_as_tuple(static_cast<key_type&&>(k)),
                             std::forward_as_tuple(static_cast<_Args&&>(__args)...));
  }
  template <class _Kp, class... _Args>
    requires base::__transparent && __ycxx::__detail::__not_iterator_arg<_Kp, iterator, const_iterator>
  constexpr pair<iterator, bool> try_emplace(_Kp&& k, _Args&&... __args) {
    return this->__emplace_key(k, piecewise_construct, std::forward_as_tuple(static_cast<_Kp&&>(k)),
                             std::forward_as_tuple(static_cast<_Args&&>(__args)...));
  }
  template <class... _Args>
  constexpr iterator try_emplace(const_iterator, const key_type& k, _Args&&... __args) {
    return try_emplace(k, static_cast<_Args&&>(__args)...).first;
  }
  template <class... _Args>
  constexpr iterator try_emplace(const_iterator, key_type&& k, _Args&&... __args) {
    return try_emplace(static_cast<key_type&&>(k), static_cast<_Args&&>(__args)...).first;
  }
  template <class _Kp, class... _Args>
    requires base::__transparent
  constexpr iterator try_emplace(const_iterator, _Kp&& k, _Args&&... __args) {
    return this->__emplace_key(k, piecewise_construct, std::forward_as_tuple(static_cast<_Kp&&>(k)),
                             std::forward_as_tuple(static_cast<_Args&&>(__args)...))
        .first;
  }
  template <class _Mp>
  constexpr pair<iterator, bool> insert_or_assign(const key_type& k, _Mp&& __obj) {
    return __assign_key(k, k, static_cast<_Mp&&>(__obj));
  }
  template <class _Mp>
  constexpr pair<iterator, bool> insert_or_assign(key_type&& k, _Mp&& __obj) {
    return __assign_key(k, static_cast<key_type&&>(k), static_cast<_Mp&&>(__obj));
  }
  template <class _Kp, class _Mp>
    requires base::__transparent
  constexpr pair<iterator, bool> insert_or_assign(_Kp&& k, _Mp&& __obj) {
    return __assign_key(k, static_cast<_Kp&&>(k), static_cast<_Mp&&>(__obj));
  }
  template <class _Mp>
  constexpr iterator insert_or_assign(const_iterator, const key_type& k, _Mp&& __obj) {
    return __assign_key(k, k, static_cast<_Mp&&>(__obj)).first;
  }
  template <class _Mp>
  constexpr iterator insert_or_assign(const_iterator, key_type&& k, _Mp&& __obj) {
    return __assign_key(k, static_cast<key_type&&>(k), static_cast<_Mp&&>(__obj)).first;
  }
  template <class _Kp, class _Mp>
    requires base::__transparent
  constexpr iterator insert_or_assign(const_iterator, _Kp&& k, _Mp&& __obj) {
    return __assign_key(k, static_cast<_Kp&&>(k), static_cast<_Mp&&>(__obj)).first;
  }

  constexpr void swap(unordered_map& __x) noexcept(allocator_traits<_Allocator>::is_always_equal::value &&
                                                 is_nothrow_swappable_v<_Hash> && is_nothrow_swappable_v<_Pred>) {
    this->__swap_impl(__x);
  }

  template <class _H2, class _P2>
  constexpr void merge(unordered_map<_Key, _Tp, _H2, _P2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _H2, class _P2>
  constexpr void merge(unordered_map<_Key, _Tp, _H2, _P2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }
  template <class _H2, class _P2>
  constexpr void merge(unordered_multimap<_Key, _Tp, _H2, _P2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _H2, class _P2>
  constexpr void merge(unordered_multimap<_Key, _Tp, _H2, _P2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }

  // ---- [unord.map.elem] ----
  constexpr mapped_type& operator[](const key_type& k) { return try_emplace(k).first->second; }
  constexpr mapped_type& operator[](key_type&& k) { return try_emplace(static_cast<key_type&&>(k)).first->second; }
  template <class _Kp>
    requires base::__transparent
  constexpr mapped_type& operator[](_Kp&& k) {
    return this
        ->__emplace_key(k, piecewise_construct, std::forward_as_tuple(static_cast<_Kp&&>(k)), std::forward_as_tuple())
        .first->second;
  }
  constexpr mapped_type& at(const key_type& k) { return __at_key(k); }
  constexpr const mapped_type& at(const key_type& k) const { return __at_key(k); }
  template <class _Kp>
    requires base::__transparent
  constexpr mapped_type& at(const _Kp& k) {
    return __at_key(k);
  }
  template <class _Kp>
    requires base::__transparent
  constexpr const mapped_type& at(const _Kp& k) const {
    return __at_key(k);
  }
  constexpr optional<mapped_type&> lookup(const key_type& k) { return __lookup_key<mapped_type&>(k); }
  constexpr optional<const mapped_type&> lookup(const key_type& k) const { return __lookup_key<const mapped_type&>(k); }
  template <class _Kp>
    requires base::__transparent
  constexpr optional<mapped_type&> lookup(const _Kp& k) {
    return __lookup_key<mapped_type&>(k);
  }
  template <class _Kp>
    requires base::__transparent
  constexpr optional<const mapped_type&> lookup(const _Kp& k) const {
    return __lookup_key<const mapped_type&>(k);
  }

private:
  template <class _Kp>
  constexpr mapped_type& __at_key(const _Kp& k) const {
    __node_base* const n = this->__find_node(k);
    if (!n)
      __ycxx::__detail::__throw_out_of_range("std::unordered_map::at: key not found");
    return static_cast<node*>(n)->value.second;
  }
  template <class _Rp, class _Kp>
  constexpr optional<_Rp> __lookup_key(const _Kp& k) const {
    __node_base* const n = this->__find_node(k);
    if (!n)
      return nullopt;
    return optional<_Rp>(static_cast<node*>(n)->value.second);
  }
  // insert_or_assign: assigns to the mapped value of the element with key k, or inserts
  // value_type(key, obj) ([unord.map.modifiers]/18-33).
  template <class _Kp, class _KArg, class _Mp>
  constexpr pair<iterator, bool> __assign_key(const _Kp& k, _KArg&& key, _Mp&& __obj) {
    static_assert(is_assignable_v<mapped_type&, _Mp&&>,
                  "std::unordered_map::insert_or_assign: mapped_type must be assignable from M");
    const size_t h = this->__hash_of(k);
    if (__node_base* const prev = this->__find_prev(k, h)) {
      static_cast<node*>(prev->next)->value.second = static_cast<_Mp&&>(__obj);
      return {base::__to_iter(prev->next), false};
    }
    typename base::__node_guard __g{this, this->__make_node(static_cast<_KArg&&>(key), static_cast<_Mp&&>(__obj))};
    this->__link_new(__g.n, h, nullptr);
    return {base::__to_iter(__g.release()), true};
  }
};

template <class _Key, class _Tp, class _Hash, class _Pred, class _Allocator>
class unordered_multimap
    : public __ycxx::__adl_free::__hash_table<_Key, pair<const _Key, _Tp>, _Hash, _Pred, _Allocator, true> {
  static_assert(__ycxx::__detail::__allocator_for<_Allocator, pair<const _Key, _Tp>>,
                "std::unordered_multimap: Allocator::value_type must be pair<const Key, T> ([container.alloc.reqmts])");
  using base = __ycxx::__adl_free::__hash_table<_Key, pair<const _Key, _Tp>, _Hash, _Pred, _Allocator, true>;

public:
  // ---- types ----
  using key_type = _Key;
  using mapped_type = _Tp;
  // Declared here, not inherited, so that the implicit deduction guides can deduce from
  // initializer_list<value_type>.
  using value_type = pair<const _Key, _Tp>;
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

  // ---- [unord.multimap.cnstr] ----
  // Constrained, and noexcept when nothing can throw (both strengthenings): the default
  // constructor allocates nothing.
  constexpr unordered_multimap() noexcept(base::__nothrow_default)
    requires default_initializable<_Hash> && default_initializable<_Pred> && default_initializable<_Allocator>
      : base() {}
  constexpr explicit unordered_multimap(size_type n, const hasher& __hf = hasher(), const key_equal& __eql = key_equal(),
                                        const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_multimap(_InputIterator __f, _InputIterator __l, size_type n = 0, const hasher& __hf = hasher(),
                               const key_equal& __eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {
    this->__insert_elems(static_cast<_InputIterator&&>(__f), static_cast<_InputIterator&&>(__l));
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_multimap(_Tag, _Rp&& __rg, size_type n = 0, const hasher& __hf = hasher(),
                               const key_equal& __eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {
    this->__insert_elems(ranges::begin(__rg), ranges::end(__rg));
  }
  constexpr unordered_multimap(const unordered_multimap&) = default;
  constexpr unordered_multimap(unordered_multimap&&) = default;
  constexpr explicit unordered_multimap(const _Allocator& a) : base(0, hasher(), key_equal(), a) {}
  constexpr unordered_multimap(const unordered_multimap& __x, const type_identity_t<_Allocator>& a) : base(__x, a) {}
  constexpr unordered_multimap(unordered_multimap&& __x, const type_identity_t<_Allocator>& a)
      noexcept(base::__nothrow_move && allocator_traits<_Allocator>::is_always_equal::value)
      : base(static_cast<base&&>(__x), a) {}
  constexpr unordered_multimap(initializer_list<value_type> il, size_type n = 0, const hasher& __hf = hasher(),
                               const key_equal& __eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, __hf, __eql, a) {
    this->__insert_elems(il.begin(), il.end());
  }
  constexpr unordered_multimap(size_type n, const allocator_type& a)
      : unordered_multimap(n, hasher(), key_equal(), a) {}
  constexpr unordered_multimap(size_type n, const hasher& __hf, const allocator_type& a)
      : unordered_multimap(n, __hf, key_equal(), a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_multimap(_InputIterator __f, _InputIterator __l, size_type n, const allocator_type& a)
      : unordered_multimap(__f, __l, n, hasher(), key_equal(), a) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_multimap(_InputIterator __f, _InputIterator __l, size_type n, const hasher& __hf,
                               const allocator_type& a)
      : unordered_multimap(__f, __l, n, __hf, key_equal(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_multimap(_Tag t, _Rp&& __rg, size_type n, const allocator_type& a)
      : unordered_multimap(t, static_cast<_Rp&&>(__rg), n, hasher(), key_equal(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_multimap(_Tag t, _Rp&& __rg, size_type n, const hasher& __hf, const allocator_type& a)
      : unordered_multimap(t, static_cast<_Rp&&>(__rg), n, __hf, key_equal(), a) {}
  constexpr unordered_multimap(initializer_list<value_type> il, size_type n, const allocator_type& a)
      : unordered_multimap(il, n, hasher(), key_equal(), a) {}
  constexpr unordered_multimap(initializer_list<value_type> il, size_type n, const hasher& __hf,
                               const allocator_type& a)
      : unordered_multimap(il, n, __hf, key_equal(), a) {}
  // Extensions (LWG 2713): the allocator-only forms the deduction guides already deduce from.
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr unordered_multimap(_InputIterator __f, _InputIterator __l, const allocator_type& a)
      : unordered_multimap(__f, __l, 0, hasher(), key_equal(), a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr unordered_multimap(_Tag t, _Rp&& __rg, const allocator_type& a)
      : unordered_multimap(t, static_cast<_Rp&&>(__rg), 0, hasher(), key_equal(), a) {}
  constexpr unordered_multimap(initializer_list<value_type> il, const allocator_type& a)
      : unordered_multimap(il, 0, hasher(), key_equal(), a) {}
  constexpr ~unordered_multimap() = default;

  constexpr unordered_multimap& operator=(const unordered_multimap& __x) {
    this->__copy_assign(__x);
    return *this;
  }
  constexpr unordered_multimap& operator=(unordered_multimap&& __x) noexcept(
      allocator_traits<_Allocator>::is_always_equal::value && is_nothrow_move_assignable_v<_Hash> &&
      is_nothrow_move_assignable_v<_Pred>) {
    this->__move_assign(__x);
    return *this;
  }
  constexpr unordered_multimap& operator=(initializer_list<value_type> il) {
    this->__assign_il(il);
    return *this;
  }

  // ---- [unord.multimap.modifiers] ----
  using base::insert;
  template <class _Pp>
    requires is_constructible_v<value_type, _Pp&&>
  constexpr iterator insert(_Pp&& __obj) {
    return this->emplace(static_cast<_Pp&&>(__obj));
  }
  template <class _Pp>
    requires is_constructible_v<value_type, _Pp&&>
  constexpr iterator insert(const_iterator __hint, _Pp&& __obj) {
    return this->emplace_hint(__hint, static_cast<_Pp&&>(__obj));
  }

  constexpr void swap(unordered_multimap& __x) noexcept(allocator_traits<_Allocator>::is_always_equal::value &&
                                                      is_nothrow_swappable_v<_Hash> && is_nothrow_swappable_v<_Pred>) {
    this->__swap_impl(__x);
  }

  template <class _H2, class _P2>
  constexpr void merge(unordered_multimap<_Key, _Tp, _H2, _P2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _H2, class _P2>
  constexpr void merge(unordered_multimap<_Key, _Tp, _H2, _P2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }
  template <class _H2, class _P2>
  constexpr void merge(unordered_map<_Key, _Tp, _H2, _P2, _Allocator>& __source) {
    this->__merge_from(__source);
  }
  template <class _H2, class _P2>
  constexpr void merge(unordered_map<_Key, _Tp, _H2, _P2, _Allocator>&& __source) {
    this->__merge_from(__source);
  }
};

// ---- deduction guides ([unord.map.overview], [unord.multimap.overview]) ----
template <class _InputIterator, class _Hash = hash<__ycxx::__detail::__unord_iter_key_t<_InputIterator>>,
          class _Pred = equal_to<__ycxx::__detail::__unord_iter_key_t<_InputIterator>>,
          class _Allocator = allocator<__ycxx::__detail::__unord_iter_alloc_t<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__unord_hash_arg<_Hash> &&
           __ycxx::__detail::__unord_pred_arg<_Pred> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_map(_InputIterator, _InputIterator, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type = 0,
              _Hash = _Hash(), _Pred = _Pred(), _Allocator = _Allocator())
    -> unordered_map<__ycxx::__detail::__unord_iter_key_t<_InputIterator>, __ycxx::__detail::__unord_iter_mapped_t<_InputIterator>,
                     _Hash, _Pred, _Allocator>;
template <ranges::input_range _Rp, class _Hash = hash<__ycxx::__detail::__unord_range_key_t<_Rp>>,
          class _Pred = equal_to<__ycxx::__detail::__unord_range_key_t<_Rp>>,
          class _Allocator = allocator<__ycxx::__detail::__unord_range_alloc_t<_Rp>>>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__unord_pred_arg<_Pred> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_map(from_range_t, _Rp&&, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type = 0, _Hash = _Hash(),
              _Pred = _Pred(), _Allocator = _Allocator())
    -> unordered_map<__ycxx::__detail::__unord_range_key_t<_Rp>, __ycxx::__detail::__unord_range_mapped_t<_Rp>, _Hash, _Pred, _Allocator>;
template <class _Key, class _Tp, class _Hash = hash<_Key>, class _Pred = equal_to<_Key>,
          class _Allocator = allocator<pair<const _Key, _Tp>>>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__unord_pred_arg<_Pred> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_map(initializer_list<pair<_Key, _Tp>>, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type = 0,
              _Hash = _Hash(), _Pred = _Pred(), _Allocator = _Allocator()) -> unordered_map<_Key, _Tp, _Hash, _Pred, _Allocator>;
template <class _InputIterator, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_map(_InputIterator, _InputIterator, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Allocator)
    -> unordered_map<__ycxx::__detail::__unord_iter_key_t<_InputIterator>, __ycxx::__detail::__unord_iter_mapped_t<_InputIterator>,
                     hash<__ycxx::__detail::__unord_iter_key_t<_InputIterator>>,
                     equal_to<__ycxx::__detail::__unord_iter_key_t<_InputIterator>>, _Allocator>;
template <class _InputIterator, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_map(_InputIterator, _InputIterator, _Allocator)
    -> unordered_map<__ycxx::__detail::__unord_iter_key_t<_InputIterator>, __ycxx::__detail::__unord_iter_mapped_t<_InputIterator>,
                     hash<__ycxx::__detail::__unord_iter_key_t<_InputIterator>>,
                     equal_to<__ycxx::__detail::__unord_iter_key_t<_InputIterator>>, _Allocator>;
template <class _InputIterator, class _Hash, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__unord_hash_arg<_Hash> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_map(_InputIterator, _InputIterator, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Hash, _Allocator)
    -> unordered_map<__ycxx::__detail::__unord_iter_key_t<_InputIterator>, __ycxx::__detail::__unord_iter_mapped_t<_InputIterator>,
                     _Hash, equal_to<__ycxx::__detail::__unord_iter_key_t<_InputIterator>>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_map(from_range_t, _Rp&&, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Allocator)
    -> unordered_map<__ycxx::__detail::__unord_range_key_t<_Rp>, __ycxx::__detail::__unord_range_mapped_t<_Rp>,
                     hash<__ycxx::__detail::__unord_range_key_t<_Rp>>, equal_to<__ycxx::__detail::__unord_range_key_t<_Rp>>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_map(from_range_t, _Rp&&, _Allocator)
    -> unordered_map<__ycxx::__detail::__unord_range_key_t<_Rp>, __ycxx::__detail::__unord_range_mapped_t<_Rp>,
                     hash<__ycxx::__detail::__unord_range_key_t<_Rp>>, equal_to<__ycxx::__detail::__unord_range_key_t<_Rp>>, _Allocator>;
template <ranges::input_range _Rp, class _Hash, class _Allocator>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_map(from_range_t, _Rp&&, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Hash, _Allocator)
    -> unordered_map<__ycxx::__detail::__unord_range_key_t<_Rp>, __ycxx::__detail::__unord_range_mapped_t<_Rp>, _Hash,
                     equal_to<__ycxx::__detail::__unord_range_key_t<_Rp>>, _Allocator>;
template <class _Key, class _Tp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_map(initializer_list<pair<_Key, _Tp>>, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Allocator)
    -> unordered_map<_Key, _Tp, hash<_Key>, equal_to<_Key>, _Allocator>;
template <class _Key, class _Tp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_map(initializer_list<pair<_Key, _Tp>>, _Allocator) -> unordered_map<_Key, _Tp, hash<_Key>, equal_to<_Key>, _Allocator>;
template <class _Key, class _Tp, class _Hash, class _Allocator>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_map(initializer_list<pair<_Key, _Tp>>, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Hash, _Allocator)
    -> unordered_map<_Key, _Tp, _Hash, equal_to<_Key>, _Allocator>;

template <class _InputIterator, class _Hash = hash<__ycxx::__detail::__unord_iter_key_t<_InputIterator>>,
          class _Pred = equal_to<__ycxx::__detail::__unord_iter_key_t<_InputIterator>>,
          class _Allocator = allocator<__ycxx::__detail::__unord_iter_alloc_t<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__unord_hash_arg<_Hash> &&
           __ycxx::__detail::__unord_pred_arg<_Pred> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multimap(_InputIterator, _InputIterator, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type = 0,
                   _Hash = _Hash(), _Pred = _Pred(), _Allocator = _Allocator())
    -> unordered_multimap<__ycxx::__detail::__unord_iter_key_t<_InputIterator>,
                          __ycxx::__detail::__unord_iter_mapped_t<_InputIterator>, _Hash, _Pred, _Allocator>;
template <ranges::input_range _Rp, class _Hash = hash<__ycxx::__detail::__unord_range_key_t<_Rp>>,
          class _Pred = equal_to<__ycxx::__detail::__unord_range_key_t<_Rp>>,
          class _Allocator = allocator<__ycxx::__detail::__unord_range_alloc_t<_Rp>>>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__unord_pred_arg<_Pred> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multimap(from_range_t, _Rp&&, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type = 0, _Hash = _Hash(),
                   _Pred = _Pred(), _Allocator = _Allocator())
    -> unordered_multimap<__ycxx::__detail::__unord_range_key_t<_Rp>, __ycxx::__detail::__unord_range_mapped_t<_Rp>, _Hash, _Pred,
                          _Allocator>;
template <class _Key, class _Tp, class _Hash = hash<_Key>, class _Pred = equal_to<_Key>,
          class _Allocator = allocator<pair<const _Key, _Tp>>>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__unord_pred_arg<_Pred> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multimap(initializer_list<pair<_Key, _Tp>>, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type = 0,
                   _Hash = _Hash(), _Pred = _Pred(), _Allocator = _Allocator())
    -> unordered_multimap<_Key, _Tp, _Hash, _Pred, _Allocator>;
template <class _InputIterator, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multimap(_InputIterator, _InputIterator, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Allocator)
    -> unordered_multimap<__ycxx::__detail::__unord_iter_key_t<_InputIterator>,
                          __ycxx::__detail::__unord_iter_mapped_t<_InputIterator>,
                          hash<__ycxx::__detail::__unord_iter_key_t<_InputIterator>>,
                          equal_to<__ycxx::__detail::__unord_iter_key_t<_InputIterator>>, _Allocator>;
template <class _InputIterator, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multimap(_InputIterator, _InputIterator, _Allocator)
    -> unordered_multimap<__ycxx::__detail::__unord_iter_key_t<_InputIterator>,
                          __ycxx::__detail::__unord_iter_mapped_t<_InputIterator>,
                          hash<__ycxx::__detail::__unord_iter_key_t<_InputIterator>>,
                          equal_to<__ycxx::__detail::__unord_iter_key_t<_InputIterator>>, _Allocator>;
template <class _InputIterator, class _Hash, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__unord_hash_arg<_Hash> &&
           __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multimap(_InputIterator, _InputIterator, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Hash,
                   _Allocator)
    -> unordered_multimap<__ycxx::__detail::__unord_iter_key_t<_InputIterator>,
                          __ycxx::__detail::__unord_iter_mapped_t<_InputIterator>, _Hash,
                          equal_to<__ycxx::__detail::__unord_iter_key_t<_InputIterator>>, _Allocator>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multimap(from_range_t, _Rp&&, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Allocator)
    -> unordered_multimap<__ycxx::__detail::__unord_range_key_t<_Rp>, __ycxx::__detail::__unord_range_mapped_t<_Rp>,
                          hash<__ycxx::__detail::__unord_range_key_t<_Rp>>, equal_to<__ycxx::__detail::__unord_range_key_t<_Rp>>,
                          _Allocator>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multimap(from_range_t, _Rp&&, _Allocator)
    -> unordered_multimap<__ycxx::__detail::__unord_range_key_t<_Rp>, __ycxx::__detail::__unord_range_mapped_t<_Rp>,
                          hash<__ycxx::__detail::__unord_range_key_t<_Rp>>, equal_to<__ycxx::__detail::__unord_range_key_t<_Rp>>,
                          _Allocator>;
template <ranges::input_range _Rp, class _Hash, class _Allocator>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multimap(from_range_t, _Rp&&, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Hash, _Allocator)
    -> unordered_multimap<__ycxx::__detail::__unord_range_key_t<_Rp>, __ycxx::__detail::__unord_range_mapped_t<_Rp>, _Hash,
                          equal_to<__ycxx::__detail::__unord_range_key_t<_Rp>>, _Allocator>;
template <class _Key, class _Tp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multimap(initializer_list<pair<_Key, _Tp>>, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Allocator)
    -> unordered_multimap<_Key, _Tp, hash<_Key>, equal_to<_Key>, _Allocator>;
template <class _Key, class _Tp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multimap(initializer_list<pair<_Key, _Tp>>, _Allocator)
    -> unordered_multimap<_Key, _Tp, hash<_Key>, equal_to<_Key>, _Allocator>;
template <class _Key, class _Tp, class _Hash, class _Allocator>
  requires __ycxx::__detail::__unord_hash_arg<_Hash> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
unordered_multimap(initializer_list<pair<_Key, _Tp>>, typename __ycxx::__detail::__alloc_info<_Allocator>::size_type, _Hash,
                   _Allocator) -> unordered_multimap<_Key, _Tp, _Hash, equal_to<_Key>, _Allocator>;

// ---- comparisons, swap ----
template <class _Key, class _Tp, class _Hash, class _Pred, class _Alloc>
constexpr bool operator==(const unordered_map<_Key, _Tp, _Hash, _Pred, _Alloc>& a,
                          const unordered_map<_Key, _Tp, _Hash, _Pred, _Alloc>& b) {
  return __ycxx::__detail::__hash_table_access::equal(a, b);
}
template <class _Key, class _Tp, class _Hash, class _Pred, class _Alloc>
constexpr bool operator==(const unordered_multimap<_Key, _Tp, _Hash, _Pred, _Alloc>& a,
                          const unordered_multimap<_Key, _Tp, _Hash, _Pred, _Alloc>& b) {
  return __ycxx::__detail::__hash_table_access::equal(a, b);
}
template <class _Key, class _Tp, class _Hash, class _Pred, class _Alloc>
constexpr void swap(unordered_map<_Key, _Tp, _Hash, _Pred, _Alloc>& __x,
                    unordered_map<_Key, _Tp, _Hash, _Pred, _Alloc>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}
template <class _Key, class _Tp, class _Hash, class _Pred, class _Alloc>
constexpr void swap(unordered_multimap<_Key, _Tp, _Hash, _Pred, _Alloc>& __x,
                    unordered_multimap<_Key, _Tp, _Hash, _Pred, _Alloc>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}

// ---- [unord.map.erasure], [unord.multimap.erasure] ----
template <class _Kp, class _Tp, class _Hp, class _Pp, class _Ap, class _Predicate>
constexpr typename unordered_map<_Kp, _Tp, _Hp, _Pp, _Ap>::size_type erase_if(unordered_map<_Kp, _Tp, _Hp, _Pp, _Ap>& c, _Predicate pred) {
  return __ycxx::__detail::__hash_table_access::erase_if(c, pred);
}
template <class _Kp, class _Tp, class _Hp, class _Pp, class _Ap, class _Predicate>
constexpr typename unordered_multimap<_Kp, _Tp, _Hp, _Pp, _Ap>::size_type erase_if(unordered_multimap<_Kp, _Tp, _Hp, _Pp, _Ap>& c,
                                                                         _Predicate pred) {
  return __ycxx::__detail::__hash_table_access::erase_if(c, pred);
}

namespace pmr {
template <class _Key, class _Tp, class _Hash = hash<_Key>, class _Pred = equal_to<_Key>>
using unordered_map = std::unordered_map<_Key, _Tp, _Hash, _Pred, polymorphic_allocator<pair<const _Key, _Tp>>>;
template <class _Key, class _Tp, class _Hash = hash<_Key>, class _Pred = equal_to<_Key>>
using unordered_multimap = std::unordered_multimap<_Key, _Tp, _Hash, _Pred, polymorphic_allocator<pair<const _Key, _Tp>>>;
} // namespace pmr

}} // namespace std
