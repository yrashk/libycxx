// libycxx core: unordered_map and unordered_multimap ([unord.map], [unord.multimap]), their
// comparisons, swap, erasure, deduction guides and the pmr:: aliases. The table itself is
// ycxx::adl_free::hash_table (hash_table.hpp); these classes add the constructors, assignment,
// swap, merge and the map members (operator[], at, lookup, try_emplace, insert_or_assign,
// insert(P&&)).
#pragma once

#include <ycxx/core/hash_table.hpp>

namespace std {

template <class Key, class T, class Hash = hash<Key>, class Pred = equal_to<Key>,
          class Allocator = allocator<pair<const Key, T>>>
class unordered_map;
template <class Key, class T, class Hash = hash<Key>, class Pred = equal_to<Key>,
          class Allocator = allocator<pair<const Key, T>>>
class unordered_multimap;

template <class Key, class T, class Hash, class Pred, class Allocator>
class unordered_map : public ycxx::adl_free::hash_table<Key, pair<const Key, T>, Hash, Pred, Allocator, false> {
  static_assert(ycxx::detail::allocator_for<Allocator, pair<const Key, T>>,
                "std::unordered_map: Allocator::value_type must be pair<const Key, T> ([container.alloc.reqmts])");
  using base = ycxx::adl_free::hash_table<Key, pair<const Key, T>, Hash, Pred, Allocator, false>;
  using node = typename base::node;
  using node_base = typename base::node_base;

public:
  // ---- types ----
  using key_type = Key;
  using mapped_type = T;
  // Declared here, not inherited, so that the implicit deduction guides can deduce from
  // initializer_list<value_type>.
  using value_type = pair<const Key, T>;
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
  using insert_return_type = ycxx::adl_free::insert_return_type<iterator, node_type>;

  // ---- [unord.map.cnstr] ----
  // Constrained, and noexcept when nothing can throw (both strengthenings): the default
  // constructor allocates nothing.
  constexpr unordered_map() noexcept(base::nothrow_default)
    requires default_initializable<Hash> && default_initializable<Pred> && default_initializable<Allocator>
      : base() {}
  constexpr explicit unordered_map(size_type n, const hasher& hf = hasher(), const key_equal& eql = key_equal(),
                                   const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_map(InputIterator f, InputIterator l, size_type n = 0, const hasher& hf = hasher(),
                          const key_equal& eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {
    this->insert_elems(static_cast<InputIterator&&>(f), static_cast<InputIterator&&>(l));
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_map(Tag, R&& rg, size_type n = 0, const hasher& hf = hasher(), const key_equal& eql = key_equal(),
                          const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {
    this->insert_elems(ranges::begin(rg), ranges::end(rg));
  }
  constexpr unordered_map(const unordered_map&) = default;
  constexpr unordered_map(unordered_map&&) = default;
  constexpr explicit unordered_map(const Allocator& a) : base(0, hasher(), key_equal(), a) {}
  constexpr unordered_map(const unordered_map& x, const type_identity_t<Allocator>& a) : base(x, a) {}
  constexpr unordered_map(unordered_map&& x, const type_identity_t<Allocator>& a)
      noexcept(base::nothrow_move && allocator_traits<Allocator>::is_always_equal::value) : base(static_cast<base&&>(x), a) {}
  constexpr unordered_map(initializer_list<value_type> il, size_type n = 0, const hasher& hf = hasher(),
                          const key_equal& eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {
    this->insert_elems(il.begin(), il.end());
  }
  constexpr unordered_map(size_type n, const allocator_type& a) : unordered_map(n, hasher(), key_equal(), a) {}
  constexpr unordered_map(size_type n, const hasher& hf, const allocator_type& a)
      : unordered_map(n, hf, key_equal(), a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_map(InputIterator f, InputIterator l, size_type n, const allocator_type& a)
      : unordered_map(f, l, n, hasher(), key_equal(), a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_map(InputIterator f, InputIterator l, size_type n, const hasher& hf, const allocator_type& a)
      : unordered_map(f, l, n, hf, key_equal(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_map(Tag t, R&& rg, size_type n, const allocator_type& a)
      : unordered_map(t, static_cast<R&&>(rg), n, hasher(), key_equal(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_map(Tag t, R&& rg, size_type n, const hasher& hf, const allocator_type& a)
      : unordered_map(t, static_cast<R&&>(rg), n, hf, key_equal(), a) {}
  constexpr unordered_map(initializer_list<value_type> il, size_type n, const allocator_type& a)
      : unordered_map(il, n, hasher(), key_equal(), a) {}
  constexpr unordered_map(initializer_list<value_type> il, size_type n, const hasher& hf, const allocator_type& a)
      : unordered_map(il, n, hf, key_equal(), a) {}
  // Extensions (LWG 2713): the allocator-only forms the deduction guides already deduce from.
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_map(InputIterator f, InputIterator l, const allocator_type& a)
      : unordered_map(f, l, 0, hasher(), key_equal(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_map(Tag t, R&& rg, const allocator_type& a)
      : unordered_map(t, static_cast<R&&>(rg), 0, hasher(), key_equal(), a) {}
  constexpr unordered_map(initializer_list<value_type> il, const allocator_type& a)
      : unordered_map(il, 0, hasher(), key_equal(), a) {}
  constexpr ~unordered_map() = default;

  constexpr unordered_map& operator=(const unordered_map& x) {
    this->copy_assign(x);
    return *this;
  }
  constexpr unordered_map& operator=(unordered_map&& x) noexcept(allocator_traits<Allocator>::is_always_equal::value &&
                                                                  is_nothrow_move_assignable_v<Hash> &&
                                                                  is_nothrow_move_assignable_v<Pred>) {
    this->move_assign(x);
    return *this;
  }
  constexpr unordered_map& operator=(initializer_list<value_type> il) {
    this->assign_il(il);
    return *this;
  }

  // ---- [unord.map.modifiers] ----
  using base::insert;
  template <class P>
    requires is_constructible_v<value_type, P&&>
  constexpr pair<iterator, bool> insert(P&& obj) {
    return this->emplace(static_cast<P&&>(obj));
  }
  template <class P>
    requires is_constructible_v<value_type, P&&>
  constexpr iterator insert(const_iterator hint, P&& obj) {
    return this->emplace_hint(hint, static_cast<P&&>(obj));
  }

  template <class... Args>
  constexpr pair<iterator, bool> try_emplace(const key_type& k, Args&&... args) {
    return this->emplace_key(k, piecewise_construct, std::forward_as_tuple(k),
                             std::forward_as_tuple(static_cast<Args&&>(args)...));
  }
  template <class... Args>
  constexpr pair<iterator, bool> try_emplace(key_type&& k, Args&&... args) {
    return this->emplace_key(k, piecewise_construct, std::forward_as_tuple(static_cast<key_type&&>(k)),
                             std::forward_as_tuple(static_cast<Args&&>(args)...));
  }
  template <class K, class... Args>
    requires base::transparent && ycxx::detail::not_iterator_arg<K, iterator, const_iterator>
  constexpr pair<iterator, bool> try_emplace(K&& k, Args&&... args) {
    return this->emplace_key(k, piecewise_construct, std::forward_as_tuple(static_cast<K&&>(k)),
                             std::forward_as_tuple(static_cast<Args&&>(args)...));
  }
  template <class... Args>
  constexpr iterator try_emplace(const_iterator, const key_type& k, Args&&... args) {
    return try_emplace(k, static_cast<Args&&>(args)...).first;
  }
  template <class... Args>
  constexpr iterator try_emplace(const_iterator, key_type&& k, Args&&... args) {
    return try_emplace(static_cast<key_type&&>(k), static_cast<Args&&>(args)...).first;
  }
  template <class K, class... Args>
    requires base::transparent
  constexpr iterator try_emplace(const_iterator, K&& k, Args&&... args) {
    return this->emplace_key(k, piecewise_construct, std::forward_as_tuple(static_cast<K&&>(k)),
                             std::forward_as_tuple(static_cast<Args&&>(args)...))
        .first;
  }
  template <class M>
  constexpr pair<iterator, bool> insert_or_assign(const key_type& k, M&& obj) {
    return assign_key(k, k, static_cast<M&&>(obj));
  }
  template <class M>
  constexpr pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj) {
    return assign_key(k, static_cast<key_type&&>(k), static_cast<M&&>(obj));
  }
  template <class K, class M>
    requires base::transparent
  constexpr pair<iterator, bool> insert_or_assign(K&& k, M&& obj) {
    return assign_key(k, static_cast<K&&>(k), static_cast<M&&>(obj));
  }
  template <class M>
  constexpr iterator insert_or_assign(const_iterator, const key_type& k, M&& obj) {
    return assign_key(k, k, static_cast<M&&>(obj)).first;
  }
  template <class M>
  constexpr iterator insert_or_assign(const_iterator, key_type&& k, M&& obj) {
    return assign_key(k, static_cast<key_type&&>(k), static_cast<M&&>(obj)).first;
  }
  template <class K, class M>
    requires base::transparent
  constexpr iterator insert_or_assign(const_iterator, K&& k, M&& obj) {
    return assign_key(k, static_cast<K&&>(k), static_cast<M&&>(obj)).first;
  }

  constexpr void swap(unordered_map& x) noexcept(allocator_traits<Allocator>::is_always_equal::value &&
                                                 is_nothrow_swappable_v<Hash> && is_nothrow_swappable_v<Pred>) {
    this->swap_impl(x);
  }

  template <class H2, class P2>
  constexpr void merge(unordered_map<Key, T, H2, P2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class H2, class P2>
  constexpr void merge(unordered_map<Key, T, H2, P2, Allocator>&& source) {
    this->merge_from(source);
  }
  template <class H2, class P2>
  constexpr void merge(unordered_multimap<Key, T, H2, P2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class H2, class P2>
  constexpr void merge(unordered_multimap<Key, T, H2, P2, Allocator>&& source) {
    this->merge_from(source);
  }

  // ---- [unord.map.elem] ----
  constexpr mapped_type& operator[](const key_type& k) { return try_emplace(k).first->second; }
  constexpr mapped_type& operator[](key_type&& k) { return try_emplace(static_cast<key_type&&>(k)).first->second; }
  template <class K>
    requires base::transparent
  constexpr mapped_type& operator[](K&& k) {
    return this
        ->emplace_key(k, piecewise_construct, std::forward_as_tuple(static_cast<K&&>(k)), std::forward_as_tuple())
        .first->second;
  }
  constexpr mapped_type& at(const key_type& k) { return at_key(k); }
  constexpr const mapped_type& at(const key_type& k) const { return at_key(k); }
  template <class K>
    requires base::transparent
  constexpr mapped_type& at(const K& k) {
    return at_key(k);
  }
  template <class K>
    requires base::transparent
  constexpr const mapped_type& at(const K& k) const {
    return at_key(k);
  }
  constexpr optional<mapped_type&> lookup(const key_type& k) { return lookup_key<mapped_type&>(k); }
  constexpr optional<const mapped_type&> lookup(const key_type& k) const { return lookup_key<const mapped_type&>(k); }
  template <class K>
    requires base::transparent
  constexpr optional<mapped_type&> lookup(const K& k) {
    return lookup_key<mapped_type&>(k);
  }
  template <class K>
    requires base::transparent
  constexpr optional<const mapped_type&> lookup(const K& k) const {
    return lookup_key<const mapped_type&>(k);
  }

private:
  template <class K>
  constexpr mapped_type& at_key(const K& k) const {
    node_base* const n = this->find_node(k);
    if (!n)
      ycxx::detail::throw_out_of_range("std::unordered_map::at: key not found");
    return static_cast<node*>(n)->value.second;
  }
  template <class R, class K>
  constexpr optional<R> lookup_key(const K& k) const {
    node_base* const n = this->find_node(k);
    if (!n)
      return nullopt;
    return optional<R>(static_cast<node*>(n)->value.second);
  }
  // insert_or_assign: assigns to the mapped value of the element with key k, or inserts
  // value_type(key, obj) ([unord.map.modifiers]/18-33).
  template <class K, class KArg, class M>
  constexpr pair<iterator, bool> assign_key(const K& k, KArg&& key, M&& obj) {
    static_assert(is_assignable_v<mapped_type&, M&&>,
                  "std::unordered_map::insert_or_assign: mapped_type must be assignable from M");
    const size_t h = this->hash_of(k);
    if (node_base* const prev = this->find_prev(k, h)) {
      static_cast<node*>(prev->next)->value.second = static_cast<M&&>(obj);
      return {base::to_iter(prev->next), false};
    }
    typename base::node_guard g{this, this->make_node(static_cast<KArg&&>(key), static_cast<M&&>(obj))};
    this->link_new(g.n, h, nullptr);
    return {base::to_iter(g.release()), true};
  }
};

template <class Key, class T, class Hash, class Pred, class Allocator>
class unordered_multimap
    : public ycxx::adl_free::hash_table<Key, pair<const Key, T>, Hash, Pred, Allocator, true> {
  static_assert(ycxx::detail::allocator_for<Allocator, pair<const Key, T>>,
                "std::unordered_multimap: Allocator::value_type must be pair<const Key, T> ([container.alloc.reqmts])");
  using base = ycxx::adl_free::hash_table<Key, pair<const Key, T>, Hash, Pred, Allocator, true>;

public:
  // ---- types ----
  using key_type = Key;
  using mapped_type = T;
  // Declared here, not inherited, so that the implicit deduction guides can deduce from
  // initializer_list<value_type>.
  using value_type = pair<const Key, T>;
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
  constexpr unordered_multimap() noexcept(base::nothrow_default)
    requires default_initializable<Hash> && default_initializable<Pred> && default_initializable<Allocator>
      : base() {}
  constexpr explicit unordered_multimap(size_type n, const hasher& hf = hasher(), const key_equal& eql = key_equal(),
                                        const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_multimap(InputIterator f, InputIterator l, size_type n = 0, const hasher& hf = hasher(),
                               const key_equal& eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {
    this->insert_elems(static_cast<InputIterator&&>(f), static_cast<InputIterator&&>(l));
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_multimap(Tag, R&& rg, size_type n = 0, const hasher& hf = hasher(),
                               const key_equal& eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {
    this->insert_elems(ranges::begin(rg), ranges::end(rg));
  }
  constexpr unordered_multimap(const unordered_multimap&) = default;
  constexpr unordered_multimap(unordered_multimap&&) = default;
  constexpr explicit unordered_multimap(const Allocator& a) : base(0, hasher(), key_equal(), a) {}
  constexpr unordered_multimap(const unordered_multimap& x, const type_identity_t<Allocator>& a) : base(x, a) {}
  constexpr unordered_multimap(unordered_multimap&& x, const type_identity_t<Allocator>& a)
      noexcept(base::nothrow_move && allocator_traits<Allocator>::is_always_equal::value)
      : base(static_cast<base&&>(x), a) {}
  constexpr unordered_multimap(initializer_list<value_type> il, size_type n = 0, const hasher& hf = hasher(),
                               const key_equal& eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {
    this->insert_elems(il.begin(), il.end());
  }
  constexpr unordered_multimap(size_type n, const allocator_type& a)
      : unordered_multimap(n, hasher(), key_equal(), a) {}
  constexpr unordered_multimap(size_type n, const hasher& hf, const allocator_type& a)
      : unordered_multimap(n, hf, key_equal(), a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_multimap(InputIterator f, InputIterator l, size_type n, const allocator_type& a)
      : unordered_multimap(f, l, n, hasher(), key_equal(), a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_multimap(InputIterator f, InputIterator l, size_type n, const hasher& hf,
                               const allocator_type& a)
      : unordered_multimap(f, l, n, hf, key_equal(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_multimap(Tag t, R&& rg, size_type n, const allocator_type& a)
      : unordered_multimap(t, static_cast<R&&>(rg), n, hasher(), key_equal(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_multimap(Tag t, R&& rg, size_type n, const hasher& hf, const allocator_type& a)
      : unordered_multimap(t, static_cast<R&&>(rg), n, hf, key_equal(), a) {}
  constexpr unordered_multimap(initializer_list<value_type> il, size_type n, const allocator_type& a)
      : unordered_multimap(il, n, hasher(), key_equal(), a) {}
  constexpr unordered_multimap(initializer_list<value_type> il, size_type n, const hasher& hf,
                               const allocator_type& a)
      : unordered_multimap(il, n, hf, key_equal(), a) {}
  // Extensions (LWG 2713): the allocator-only forms the deduction guides already deduce from.
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_multimap(InputIterator f, InputIterator l, const allocator_type& a)
      : unordered_multimap(f, l, 0, hasher(), key_equal(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_multimap(Tag t, R&& rg, const allocator_type& a)
      : unordered_multimap(t, static_cast<R&&>(rg), 0, hasher(), key_equal(), a) {}
  constexpr unordered_multimap(initializer_list<value_type> il, const allocator_type& a)
      : unordered_multimap(il, 0, hasher(), key_equal(), a) {}
  constexpr ~unordered_multimap() = default;

  constexpr unordered_multimap& operator=(const unordered_multimap& x) {
    this->copy_assign(x);
    return *this;
  }
  constexpr unordered_multimap& operator=(unordered_multimap&& x) noexcept(
      allocator_traits<Allocator>::is_always_equal::value && is_nothrow_move_assignable_v<Hash> &&
      is_nothrow_move_assignable_v<Pred>) {
    this->move_assign(x);
    return *this;
  }
  constexpr unordered_multimap& operator=(initializer_list<value_type> il) {
    this->assign_il(il);
    return *this;
  }

  // ---- [unord.multimap.modifiers] ----
  using base::insert;
  template <class P>
    requires is_constructible_v<value_type, P&&>
  constexpr iterator insert(P&& obj) {
    return this->emplace(static_cast<P&&>(obj));
  }
  template <class P>
    requires is_constructible_v<value_type, P&&>
  constexpr iterator insert(const_iterator hint, P&& obj) {
    return this->emplace_hint(hint, static_cast<P&&>(obj));
  }

  constexpr void swap(unordered_multimap& x) noexcept(allocator_traits<Allocator>::is_always_equal::value &&
                                                      is_nothrow_swappable_v<Hash> && is_nothrow_swappable_v<Pred>) {
    this->swap_impl(x);
  }

  template <class H2, class P2>
  constexpr void merge(unordered_multimap<Key, T, H2, P2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class H2, class P2>
  constexpr void merge(unordered_multimap<Key, T, H2, P2, Allocator>&& source) {
    this->merge_from(source);
  }
  template <class H2, class P2>
  constexpr void merge(unordered_map<Key, T, H2, P2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class H2, class P2>
  constexpr void merge(unordered_map<Key, T, H2, P2, Allocator>&& source) {
    this->merge_from(source);
  }
};

// ---- deduction guides ([unord.map.overview], [unord.multimap.overview]) ----
template <class InputIterator, class Hash = hash<ycxx::detail::unord_iter_key_t<InputIterator>>,
          class Pred = equal_to<ycxx::detail::unord_iter_key_t<InputIterator>>,
          class Allocator = allocator<ycxx::detail::unord_iter_alloc_t<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::unord_hash_arg<Hash> &&
           ycxx::detail::unord_pred_arg<Pred> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_map(InputIterator, InputIterator, typename ycxx::detail::alloc_info<Allocator>::size_type = 0,
              Hash = Hash(), Pred = Pred(), Allocator = Allocator())
    -> unordered_map<ycxx::detail::unord_iter_key_t<InputIterator>, ycxx::detail::unord_iter_mapped_t<InputIterator>,
                     Hash, Pred, Allocator>;
template <ranges::input_range R, class Hash = hash<ycxx::detail::unord_range_key_t<R>>,
          class Pred = equal_to<ycxx::detail::unord_range_key_t<R>>,
          class Allocator = allocator<ycxx::detail::unord_range_alloc_t<R>>>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::unord_pred_arg<Pred> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
unordered_map(from_range_t, R&&, typename ycxx::detail::alloc_info<Allocator>::size_type = 0, Hash = Hash(),
              Pred = Pred(), Allocator = Allocator())
    -> unordered_map<ycxx::detail::unord_range_key_t<R>, ycxx::detail::unord_range_mapped_t<R>, Hash, Pred, Allocator>;
template <class Key, class T, class Hash = hash<Key>, class Pred = equal_to<Key>,
          class Allocator = allocator<pair<const Key, T>>>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::unord_pred_arg<Pred> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
unordered_map(initializer_list<pair<Key, T>>, typename ycxx::detail::alloc_info<Allocator>::size_type = 0,
              Hash = Hash(), Pred = Pred(), Allocator = Allocator()) -> unordered_map<Key, T, Hash, Pred, Allocator>;
template <class InputIterator, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_map(InputIterator, InputIterator, typename ycxx::detail::alloc_info<Allocator>::size_type, Allocator)
    -> unordered_map<ycxx::detail::unord_iter_key_t<InputIterator>, ycxx::detail::unord_iter_mapped_t<InputIterator>,
                     hash<ycxx::detail::unord_iter_key_t<InputIterator>>,
                     equal_to<ycxx::detail::unord_iter_key_t<InputIterator>>, Allocator>;
template <class InputIterator, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_map(InputIterator, InputIterator, Allocator)
    -> unordered_map<ycxx::detail::unord_iter_key_t<InputIterator>, ycxx::detail::unord_iter_mapped_t<InputIterator>,
                     hash<ycxx::detail::unord_iter_key_t<InputIterator>>,
                     equal_to<ycxx::detail::unord_iter_key_t<InputIterator>>, Allocator>;
template <class InputIterator, class Hash, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::unord_hash_arg<Hash> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
unordered_map(InputIterator, InputIterator, typename ycxx::detail::alloc_info<Allocator>::size_type, Hash, Allocator)
    -> unordered_map<ycxx::detail::unord_iter_key_t<InputIterator>, ycxx::detail::unord_iter_mapped_t<InputIterator>,
                     Hash, equal_to<ycxx::detail::unord_iter_key_t<InputIterator>>, Allocator>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
unordered_map(from_range_t, R&&, typename ycxx::detail::alloc_info<Allocator>::size_type, Allocator)
    -> unordered_map<ycxx::detail::unord_range_key_t<R>, ycxx::detail::unord_range_mapped_t<R>,
                     hash<ycxx::detail::unord_range_key_t<R>>, equal_to<ycxx::detail::unord_range_key_t<R>>, Allocator>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
unordered_map(from_range_t, R&&, Allocator)
    -> unordered_map<ycxx::detail::unord_range_key_t<R>, ycxx::detail::unord_range_mapped_t<R>,
                     hash<ycxx::detail::unord_range_key_t<R>>, equal_to<ycxx::detail::unord_range_key_t<R>>, Allocator>;
template <ranges::input_range R, class Hash, class Allocator>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_map(from_range_t, R&&, typename ycxx::detail::alloc_info<Allocator>::size_type, Hash, Allocator)
    -> unordered_map<ycxx::detail::unord_range_key_t<R>, ycxx::detail::unord_range_mapped_t<R>, Hash,
                     equal_to<ycxx::detail::unord_range_key_t<R>>, Allocator>;
template <class Key, class T, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
unordered_map(initializer_list<pair<Key, T>>, typename ycxx::detail::alloc_info<Allocator>::size_type, Allocator)
    -> unordered_map<Key, T, hash<Key>, equal_to<Key>, Allocator>;
template <class Key, class T, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
unordered_map(initializer_list<pair<Key, T>>, Allocator) -> unordered_map<Key, T, hash<Key>, equal_to<Key>, Allocator>;
template <class Key, class T, class Hash, class Allocator>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_map(initializer_list<pair<Key, T>>, typename ycxx::detail::alloc_info<Allocator>::size_type, Hash, Allocator)
    -> unordered_map<Key, T, Hash, equal_to<Key>, Allocator>;

template <class InputIterator, class Hash = hash<ycxx::detail::unord_iter_key_t<InputIterator>>,
          class Pred = equal_to<ycxx::detail::unord_iter_key_t<InputIterator>>,
          class Allocator = allocator<ycxx::detail::unord_iter_alloc_t<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::unord_hash_arg<Hash> &&
           ycxx::detail::unord_pred_arg<Pred> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multimap(InputIterator, InputIterator, typename ycxx::detail::alloc_info<Allocator>::size_type = 0,
                   Hash = Hash(), Pred = Pred(), Allocator = Allocator())
    -> unordered_multimap<ycxx::detail::unord_iter_key_t<InputIterator>,
                          ycxx::detail::unord_iter_mapped_t<InputIterator>, Hash, Pred, Allocator>;
template <ranges::input_range R, class Hash = hash<ycxx::detail::unord_range_key_t<R>>,
          class Pred = equal_to<ycxx::detail::unord_range_key_t<R>>,
          class Allocator = allocator<ycxx::detail::unord_range_alloc_t<R>>>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::unord_pred_arg<Pred> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multimap(from_range_t, R&&, typename ycxx::detail::alloc_info<Allocator>::size_type = 0, Hash = Hash(),
                   Pred = Pred(), Allocator = Allocator())
    -> unordered_multimap<ycxx::detail::unord_range_key_t<R>, ycxx::detail::unord_range_mapped_t<R>, Hash, Pred,
                          Allocator>;
template <class Key, class T, class Hash = hash<Key>, class Pred = equal_to<Key>,
          class Allocator = allocator<pair<const Key, T>>>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::unord_pred_arg<Pred> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multimap(initializer_list<pair<Key, T>>, typename ycxx::detail::alloc_info<Allocator>::size_type = 0,
                   Hash = Hash(), Pred = Pred(), Allocator = Allocator())
    -> unordered_multimap<Key, T, Hash, Pred, Allocator>;
template <class InputIterator, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multimap(InputIterator, InputIterator, typename ycxx::detail::alloc_info<Allocator>::size_type, Allocator)
    -> unordered_multimap<ycxx::detail::unord_iter_key_t<InputIterator>,
                          ycxx::detail::unord_iter_mapped_t<InputIterator>,
                          hash<ycxx::detail::unord_iter_key_t<InputIterator>>,
                          equal_to<ycxx::detail::unord_iter_key_t<InputIterator>>, Allocator>;
template <class InputIterator, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multimap(InputIterator, InputIterator, Allocator)
    -> unordered_multimap<ycxx::detail::unord_iter_key_t<InputIterator>,
                          ycxx::detail::unord_iter_mapped_t<InputIterator>,
                          hash<ycxx::detail::unord_iter_key_t<InputIterator>>,
                          equal_to<ycxx::detail::unord_iter_key_t<InputIterator>>, Allocator>;
template <class InputIterator, class Hash, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::unord_hash_arg<Hash> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multimap(InputIterator, InputIterator, typename ycxx::detail::alloc_info<Allocator>::size_type, Hash,
                   Allocator)
    -> unordered_multimap<ycxx::detail::unord_iter_key_t<InputIterator>,
                          ycxx::detail::unord_iter_mapped_t<InputIterator>, Hash,
                          equal_to<ycxx::detail::unord_iter_key_t<InputIterator>>, Allocator>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multimap(from_range_t, R&&, typename ycxx::detail::alloc_info<Allocator>::size_type, Allocator)
    -> unordered_multimap<ycxx::detail::unord_range_key_t<R>, ycxx::detail::unord_range_mapped_t<R>,
                          hash<ycxx::detail::unord_range_key_t<R>>, equal_to<ycxx::detail::unord_range_key_t<R>>,
                          Allocator>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multimap(from_range_t, R&&, Allocator)
    -> unordered_multimap<ycxx::detail::unord_range_key_t<R>, ycxx::detail::unord_range_mapped_t<R>,
                          hash<ycxx::detail::unord_range_key_t<R>>, equal_to<ycxx::detail::unord_range_key_t<R>>,
                          Allocator>;
template <ranges::input_range R, class Hash, class Allocator>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multimap(from_range_t, R&&, typename ycxx::detail::alloc_info<Allocator>::size_type, Hash, Allocator)
    -> unordered_multimap<ycxx::detail::unord_range_key_t<R>, ycxx::detail::unord_range_mapped_t<R>, Hash,
                          equal_to<ycxx::detail::unord_range_key_t<R>>, Allocator>;
template <class Key, class T, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multimap(initializer_list<pair<Key, T>>, typename ycxx::detail::alloc_info<Allocator>::size_type, Allocator)
    -> unordered_multimap<Key, T, hash<Key>, equal_to<Key>, Allocator>;
template <class Key, class T, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multimap(initializer_list<pair<Key, T>>, Allocator)
    -> unordered_multimap<Key, T, hash<Key>, equal_to<Key>, Allocator>;
template <class Key, class T, class Hash, class Allocator>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multimap(initializer_list<pair<Key, T>>, typename ycxx::detail::alloc_info<Allocator>::size_type, Hash,
                   Allocator) -> unordered_multimap<Key, T, Hash, equal_to<Key>, Allocator>;

// ---- comparisons, swap ----
template <class Key, class T, class Hash, class Pred, class Alloc>
constexpr bool operator==(const unordered_map<Key, T, Hash, Pred, Alloc>& a,
                          const unordered_map<Key, T, Hash, Pred, Alloc>& b) {
  return ycxx::detail::hash_table_access::equal(a, b);
}
template <class Key, class T, class Hash, class Pred, class Alloc>
constexpr bool operator==(const unordered_multimap<Key, T, Hash, Pred, Alloc>& a,
                          const unordered_multimap<Key, T, Hash, Pred, Alloc>& b) {
  return ycxx::detail::hash_table_access::equal(a, b);
}
template <class Key, class T, class Hash, class Pred, class Alloc>
constexpr void swap(unordered_map<Key, T, Hash, Pred, Alloc>& x,
                    unordered_map<Key, T, Hash, Pred, Alloc>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}
template <class Key, class T, class Hash, class Pred, class Alloc>
constexpr void swap(unordered_multimap<Key, T, Hash, Pred, Alloc>& x,
                    unordered_multimap<Key, T, Hash, Pred, Alloc>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

// ---- [unord.map.erasure], [unord.multimap.erasure] ----
template <class K, class T, class H, class P, class A, class Predicate>
constexpr typename unordered_map<K, T, H, P, A>::size_type erase_if(unordered_map<K, T, H, P, A>& c, Predicate pred) {
  return ycxx::detail::hash_table_access::erase_if(c, pred);
}
template <class K, class T, class H, class P, class A, class Predicate>
constexpr typename unordered_multimap<K, T, H, P, A>::size_type erase_if(unordered_multimap<K, T, H, P, A>& c,
                                                                         Predicate pred) {
  return ycxx::detail::hash_table_access::erase_if(c, pred);
}

namespace pmr {
template <class Key, class T, class Hash = hash<Key>, class Pred = equal_to<Key>>
using unordered_map = std::unordered_map<Key, T, Hash, Pred, polymorphic_allocator<pair<const Key, T>>>;
template <class Key, class T, class Hash = hash<Key>, class Pred = equal_to<Key>>
using unordered_multimap = std::unordered_multimap<Key, T, Hash, Pred, polymorphic_allocator<pair<const Key, T>>>;
} // namespace pmr

} // namespace std
