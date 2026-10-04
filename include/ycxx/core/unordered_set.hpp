// libycxx core: unordered_set and unordered_multiset ([unord.set], [unord.multiset]), their
// comparisons, swap, erasure, deduction guides and the pmr:: aliases. The table itself is
// ycxx::adl_free::hash_table (hash_table.hpp); iterator and const_iterator are the same
// constant iterator type ([unord.req.general]/8).
#pragma once

#include <ycxx/core/hash_table.hpp>

namespace std {

template <class Key, class Hash = hash<Key>, class Pred = equal_to<Key>, class Allocator = allocator<Key>>
class unordered_set;
template <class Key, class Hash = hash<Key>, class Pred = equal_to<Key>, class Allocator = allocator<Key>>
class unordered_multiset;

template <class Key, class Hash, class Pred, class Allocator>
class unordered_set : public ycxx::adl_free::hash_table<Key, Key, Hash, Pred, Allocator, false> {
  static_assert(ycxx::detail::allocator_for<Allocator, Key>,
                "std::unordered_set: Allocator::value_type must be Key ([container.alloc.reqmts])");
  using base = ycxx::adl_free::hash_table<Key, Key, Hash, Pred, Allocator, false>;

public:
  // ---- types ----
  using key_type = Key;
  // Declared here, not inherited, so that the implicit deduction guides can deduce from
  // initializer_list<value_type>.
  using value_type = Key;
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

  // ---- [unord.set.cnstr] ----
  // Constrained, and noexcept when nothing can throw (both strengthenings): the default
  // constructor allocates nothing.
  constexpr unordered_set() noexcept(base::nothrow_default)
    requires default_initializable<Hash> && default_initializable<Pred> && default_initializable<Allocator>
      : base() {}
  constexpr explicit unordered_set(size_type n, const hasher& hf = hasher(), const key_equal& eql = key_equal(),
                                   const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_set(InputIterator f, InputIterator l, size_type n = 0, const hasher& hf = hasher(),
                          const key_equal& eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {
    this->insert_elems(static_cast<InputIterator&&>(f), static_cast<InputIterator&&>(l));
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_set(Tag, R&& rg, size_type n = 0, const hasher& hf = hasher(), const key_equal& eql = key_equal(),
                          const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {
    this->insert_elems(ranges::begin(rg), ranges::end(rg));
  }
  constexpr unordered_set(const unordered_set&) = default;
  constexpr unordered_set(unordered_set&&) = default;
  constexpr explicit unordered_set(const Allocator& a) : base(0, hasher(), key_equal(), a) {}
  constexpr unordered_set(const unordered_set& x, const type_identity_t<Allocator>& a) : base(x, a) {}
  constexpr unordered_set(unordered_set&& x, const type_identity_t<Allocator>& a)
      noexcept(base::nothrow_move && allocator_traits<Allocator>::is_always_equal::value) : base(static_cast<base&&>(x), a) {}
  constexpr unordered_set(initializer_list<value_type> il, size_type n = 0, const hasher& hf = hasher(),
                          const key_equal& eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {
    this->insert_elems(il.begin(), il.end());
  }
  constexpr unordered_set(size_type n, const allocator_type& a) : unordered_set(n, hasher(), key_equal(), a) {}
  constexpr unordered_set(size_type n, const hasher& hf, const allocator_type& a)
      : unordered_set(n, hf, key_equal(), a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_set(InputIterator f, InputIterator l, size_type n, const allocator_type& a)
      : unordered_set(f, l, n, hasher(), key_equal(), a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_set(InputIterator f, InputIterator l, size_type n, const hasher& hf, const allocator_type& a)
      : unordered_set(f, l, n, hf, key_equal(), a) {}
  constexpr unordered_set(initializer_list<value_type> il, size_type n, const allocator_type& a)
      : unordered_set(il, n, hasher(), key_equal(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_set(Tag t, R&& rg, size_type n, const allocator_type& a)
      : unordered_set(t, static_cast<R&&>(rg), n, hasher(), key_equal(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_set(Tag t, R&& rg, size_type n, const hasher& hf, const allocator_type& a)
      : unordered_set(t, static_cast<R&&>(rg), n, hf, key_equal(), a) {}
  constexpr unordered_set(initializer_list<value_type> il, size_type n, const hasher& hf, const allocator_type& a)
      : unordered_set(il, n, hf, key_equal(), a) {}
  // Extensions (LWG 2713): the allocator-only forms the deduction guides already deduce from.
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_set(InputIterator f, InputIterator l, const allocator_type& a)
      : unordered_set(f, l, 0, hasher(), key_equal(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_set(Tag t, R&& rg, const allocator_type& a)
      : unordered_set(t, static_cast<R&&>(rg), 0, hasher(), key_equal(), a) {}
  constexpr unordered_set(initializer_list<value_type> il, const allocator_type& a)
      : unordered_set(il, 0, hasher(), key_equal(), a) {}
  constexpr ~unordered_set() = default;

  constexpr unordered_set& operator=(const unordered_set& x) {
    this->copy_assign(x);
    return *this;
  }
  constexpr unordered_set& operator=(unordered_set&& x) noexcept(allocator_traits<Allocator>::is_always_equal::value &&
                                                                  is_nothrow_move_assignable_v<Hash> &&
                                                                  is_nothrow_move_assignable_v<Pred>) {
    this->move_assign(x);
    return *this;
  }
  constexpr unordered_set& operator=(initializer_list<value_type> il) {
    this->assign_il(il);
    return *this;
  }

  // ---- [unord.set.modifiers] ----
  using base::insert;
  template <class K>
    requires base::transparent
  constexpr pair<iterator, bool> insert(K&& obj) {
    return this->emplace_key(obj, static_cast<K&&>(obj));
  }
  template <class K>
    requires base::transparent && ycxx::detail::not_iterator_arg<K, iterator, const_iterator>
  constexpr iterator insert(const_iterator, K&& obj) {
    return this->emplace_key(obj, static_cast<K&&>(obj)).first;
  }

  constexpr void swap(unordered_set& x) noexcept(allocator_traits<Allocator>::is_always_equal::value &&
                                                 is_nothrow_swappable_v<Hash> && is_nothrow_swappable_v<Pred>) {
    this->swap_impl(x);
  }

  template <class H2, class P2>
  constexpr void merge(unordered_set<Key, H2, P2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class H2, class P2>
  constexpr void merge(unordered_set<Key, H2, P2, Allocator>&& source) {
    this->merge_from(source);
  }
  template <class H2, class P2>
  constexpr void merge(unordered_multiset<Key, H2, P2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class H2, class P2>
  constexpr void merge(unordered_multiset<Key, H2, P2, Allocator>&& source) {
    this->merge_from(source);
  }
};

template <class Key, class Hash, class Pred, class Allocator>
class unordered_multiset : public ycxx::adl_free::hash_table<Key, Key, Hash, Pred, Allocator, true> {
  static_assert(ycxx::detail::allocator_for<Allocator, Key>,
                "std::unordered_multiset: Allocator::value_type must be Key ([container.alloc.reqmts])");
  using base = ycxx::adl_free::hash_table<Key, Key, Hash, Pred, Allocator, true>;

public:
  // ---- types ----
  using key_type = Key;
  // Declared here, not inherited, so that the implicit deduction guides can deduce from
  // initializer_list<value_type>.
  using value_type = Key;
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
  constexpr unordered_multiset() noexcept(base::nothrow_default)
    requires default_initializable<Hash> && default_initializable<Pred> && default_initializable<Allocator>
      : base() {}
  constexpr explicit unordered_multiset(size_type n, const hasher& hf = hasher(), const key_equal& eql = key_equal(),
                                        const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_multiset(InputIterator f, InputIterator l, size_type n = 0, const hasher& hf = hasher(),
                               const key_equal& eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {
    this->insert_elems(static_cast<InputIterator&&>(f), static_cast<InputIterator&&>(l));
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_multiset(Tag, R&& rg, size_type n = 0, const hasher& hf = hasher(),
                               const key_equal& eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {
    this->insert_elems(ranges::begin(rg), ranges::end(rg));
  }
  constexpr unordered_multiset(const unordered_multiset&) = default;
  constexpr unordered_multiset(unordered_multiset&&) = default;
  constexpr explicit unordered_multiset(const Allocator& a) : base(0, hasher(), key_equal(), a) {}
  constexpr unordered_multiset(const unordered_multiset& x, const type_identity_t<Allocator>& a) : base(x, a) {}
  constexpr unordered_multiset(unordered_multiset&& x, const type_identity_t<Allocator>& a)
      noexcept(base::nothrow_move && allocator_traits<Allocator>::is_always_equal::value)
      : base(static_cast<base&&>(x), a) {}
  constexpr unordered_multiset(initializer_list<value_type> il, size_type n = 0, const hasher& hf = hasher(),
                               const key_equal& eql = key_equal(), const allocator_type& a = allocator_type())
      : base(n, hf, eql, a) {
    this->insert_elems(il.begin(), il.end());
  }
  constexpr unordered_multiset(size_type n, const allocator_type& a)
      : unordered_multiset(n, hasher(), key_equal(), a) {}
  constexpr unordered_multiset(size_type n, const hasher& hf, const allocator_type& a)
      : unordered_multiset(n, hf, key_equal(), a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_multiset(InputIterator f, InputIterator l, size_type n, const allocator_type& a)
      : unordered_multiset(f, l, n, hasher(), key_equal(), a) {}
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_multiset(InputIterator f, InputIterator l, size_type n, const hasher& hf,
                               const allocator_type& a)
      : unordered_multiset(f, l, n, hf, key_equal(), a) {}
  constexpr unordered_multiset(initializer_list<value_type> il, size_type n, const allocator_type& a)
      : unordered_multiset(il, n, hasher(), key_equal(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_multiset(Tag t, R&& rg, size_type n, const allocator_type& a)
      : unordered_multiset(t, static_cast<R&&>(rg), n, hasher(), key_equal(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_multiset(Tag t, R&& rg, size_type n, const hasher& hf, const allocator_type& a)
      : unordered_multiset(t, static_cast<R&&>(rg), n, hf, key_equal(), a) {}
  constexpr unordered_multiset(initializer_list<value_type> il, size_type n, const hasher& hf,
                               const allocator_type& a)
      : unordered_multiset(il, n, hf, key_equal(), a) {}
  // Extensions (LWG 2713): the allocator-only forms the deduction guides already deduce from.
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr unordered_multiset(InputIterator f, InputIterator l, const allocator_type& a)
      : unordered_multiset(f, l, 0, hasher(), key_equal(), a) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr unordered_multiset(Tag t, R&& rg, const allocator_type& a)
      : unordered_multiset(t, static_cast<R&&>(rg), 0, hasher(), key_equal(), a) {}
  constexpr unordered_multiset(initializer_list<value_type> il, const allocator_type& a)
      : unordered_multiset(il, 0, hasher(), key_equal(), a) {}
  constexpr ~unordered_multiset() = default;

  constexpr unordered_multiset& operator=(const unordered_multiset& x) {
    this->copy_assign(x);
    return *this;
  }
  constexpr unordered_multiset& operator=(unordered_multiset&& x) noexcept(
      allocator_traits<Allocator>::is_always_equal::value && is_nothrow_move_assignable_v<Hash> &&
      is_nothrow_move_assignable_v<Pred>) {
    this->move_assign(x);
    return *this;
  }
  constexpr unordered_multiset& operator=(initializer_list<value_type> il) {
    this->assign_il(il);
    return *this;
  }

  constexpr void swap(unordered_multiset& x) noexcept(allocator_traits<Allocator>::is_always_equal::value &&
                                                      is_nothrow_swappable_v<Hash> && is_nothrow_swappable_v<Pred>) {
    this->swap_impl(x);
  }

  template <class H2, class P2>
  constexpr void merge(unordered_multiset<Key, H2, P2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class H2, class P2>
  constexpr void merge(unordered_multiset<Key, H2, P2, Allocator>&& source) {
    this->merge_from(source);
  }
  template <class H2, class P2>
  constexpr void merge(unordered_set<Key, H2, P2, Allocator>& source) {
    this->merge_from(source);
  }
  template <class H2, class P2>
  constexpr void merge(unordered_set<Key, H2, P2, Allocator>&& source) {
    this->merge_from(source);
  }
};

// ---- deduction guides ([unord.set.overview], [unord.multiset.overview]) ----
template <class InputIterator, class Hash = hash<ycxx::detail::iter_value_type<InputIterator>>,
          class Pred = equal_to<ycxx::detail::iter_value_type<InputIterator>>,
          class Allocator = allocator<ycxx::detail::iter_value_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::unord_hash_arg<Hash> &&
           ycxx::detail::unord_pred_arg<Pred> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_set(InputIterator, InputIterator, typename ycxx::detail::alloc_info<Allocator>::size_type = 0,
              Hash = Hash(), Pred = Pred(), Allocator = Allocator())
    -> unordered_set<ycxx::detail::iter_value_type<InputIterator>, Hash, Pred, Allocator>;
template <ranges::input_range R, class Hash = hash<ranges::range_value_t<R>>,
          class Pred = equal_to<ranges::range_value_t<R>>, class Allocator = allocator<ranges::range_value_t<R>>>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::unord_pred_arg<Pred> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
unordered_set(from_range_t, R&&, typename ycxx::detail::alloc_info<Allocator>::size_type = 0, Hash = Hash(),
              Pred = Pred(), Allocator = Allocator()) -> unordered_set<ranges::range_value_t<R>, Hash, Pred, Allocator>;
template <class T, class Hash = hash<T>, class Pred = equal_to<T>, class Allocator = allocator<T>>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::unord_pred_arg<Pred> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
unordered_set(initializer_list<T>, typename ycxx::detail::alloc_info<Allocator>::size_type = 0, Hash = Hash(),
              Pred = Pred(), Allocator = Allocator()) -> unordered_set<T, Hash, Pred, Allocator>;
template <class InputIterator, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_set(InputIterator, InputIterator, typename ycxx::detail::alloc_info<Allocator>::size_type, Allocator)
    -> unordered_set<ycxx::detail::iter_value_type<InputIterator>, hash<ycxx::detail::iter_value_type<InputIterator>>,
                     equal_to<ycxx::detail::iter_value_type<InputIterator>>, Allocator>;
template <class InputIterator, class Hash, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::unord_hash_arg<Hash> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
unordered_set(InputIterator, InputIterator, typename ycxx::detail::alloc_info<Allocator>::size_type, Hash, Allocator)
    -> unordered_set<ycxx::detail::iter_value_type<InputIterator>, Hash,
                     equal_to<ycxx::detail::iter_value_type<InputIterator>>, Allocator>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
unordered_set(from_range_t, R&&, typename ycxx::detail::alloc_info<Allocator>::size_type, Allocator)
    -> unordered_set<ranges::range_value_t<R>, hash<ranges::range_value_t<R>>, equal_to<ranges::range_value_t<R>>,
                     Allocator>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
unordered_set(from_range_t, R&&, Allocator)
    -> unordered_set<ranges::range_value_t<R>, hash<ranges::range_value_t<R>>, equal_to<ranges::range_value_t<R>>,
                     Allocator>;
template <ranges::input_range R, class Hash, class Allocator>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_set(from_range_t, R&&, typename ycxx::detail::alloc_info<Allocator>::size_type, Hash, Allocator)
    -> unordered_set<ranges::range_value_t<R>, Hash, equal_to<ranges::range_value_t<R>>, Allocator>;
template <class T, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
unordered_set(initializer_list<T>, typename ycxx::detail::alloc_info<Allocator>::size_type, Allocator)
    -> unordered_set<T, hash<T>, equal_to<T>, Allocator>;
template <class T, class Hash, class Allocator>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_set(initializer_list<T>, typename ycxx::detail::alloc_info<Allocator>::size_type, Hash, Allocator)
    -> unordered_set<T, Hash, equal_to<T>, Allocator>;

template <class InputIterator, class Hash = hash<ycxx::detail::iter_value_type<InputIterator>>,
          class Pred = equal_to<ycxx::detail::iter_value_type<InputIterator>>,
          class Allocator = allocator<ycxx::detail::iter_value_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::unord_hash_arg<Hash> &&
           ycxx::detail::unord_pred_arg<Pred> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multiset(InputIterator, InputIterator, typename ycxx::detail::alloc_info<Allocator>::size_type = 0,
                   Hash = Hash(), Pred = Pred(), Allocator = Allocator())
    -> unordered_multiset<ycxx::detail::iter_value_type<InputIterator>, Hash, Pred, Allocator>;
template <ranges::input_range R, class Hash = hash<ranges::range_value_t<R>>,
          class Pred = equal_to<ranges::range_value_t<R>>, class Allocator = allocator<ranges::range_value_t<R>>>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::unord_pred_arg<Pred> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multiset(from_range_t, R&&, typename ycxx::detail::alloc_info<Allocator>::size_type = 0, Hash = Hash(),
                   Pred = Pred(), Allocator = Allocator())
    -> unordered_multiset<ranges::range_value_t<R>, Hash, Pred, Allocator>;
template <class T, class Hash = hash<T>, class Pred = equal_to<T>, class Allocator = allocator<T>>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::unord_pred_arg<Pred> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multiset(initializer_list<T>, typename ycxx::detail::alloc_info<Allocator>::size_type = 0, Hash = Hash(),
                   Pred = Pred(), Allocator = Allocator()) -> unordered_multiset<T, Hash, Pred, Allocator>;
template <class InputIterator, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multiset(InputIterator, InputIterator, typename ycxx::detail::alloc_info<Allocator>::size_type, Allocator)
    -> unordered_multiset<ycxx::detail::iter_value_type<InputIterator>,
                          hash<ycxx::detail::iter_value_type<InputIterator>>,
                          equal_to<ycxx::detail::iter_value_type<InputIterator>>, Allocator>;
template <class InputIterator, class Hash, class Allocator>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::unord_hash_arg<Hash> &&
           ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multiset(InputIterator, InputIterator, typename ycxx::detail::alloc_info<Allocator>::size_type, Hash,
                   Allocator)
    -> unordered_multiset<ycxx::detail::iter_value_type<InputIterator>, Hash,
                          equal_to<ycxx::detail::iter_value_type<InputIterator>>, Allocator>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multiset(from_range_t, R&&, typename ycxx::detail::alloc_info<Allocator>::size_type, Allocator)
    -> unordered_multiset<ranges::range_value_t<R>, hash<ranges::range_value_t<R>>,
                          equal_to<ranges::range_value_t<R>>, Allocator>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multiset(from_range_t, R&&, Allocator)
    -> unordered_multiset<ranges::range_value_t<R>, hash<ranges::range_value_t<R>>,
                          equal_to<ranges::range_value_t<R>>, Allocator>;
template <ranges::input_range R, class Hash, class Allocator>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multiset(from_range_t, R&&, typename ycxx::detail::alloc_info<Allocator>::size_type, Hash, Allocator)
    -> unordered_multiset<ranges::range_value_t<R>, Hash, equal_to<ranges::range_value_t<R>>, Allocator>;
template <class T, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multiset(initializer_list<T>, typename ycxx::detail::alloc_info<Allocator>::size_type, Allocator)
    -> unordered_multiset<T, hash<T>, equal_to<T>, Allocator>;
template <class T, class Hash, class Allocator>
  requires ycxx::detail::unord_hash_arg<Hash> && ycxx::detail::qualifies_as_allocator<Allocator>
unordered_multiset(initializer_list<T>, typename ycxx::detail::alloc_info<Allocator>::size_type, Hash, Allocator)
    -> unordered_multiset<T, Hash, equal_to<T>, Allocator>;

// ---- comparisons, swap ----
template <class Key, class Hash, class Pred, class Alloc>
constexpr bool operator==(const unordered_set<Key, Hash, Pred, Alloc>& a, const unordered_set<Key, Hash, Pred, Alloc>& b) {
  return ycxx::detail::hash_table_access::equal(a, b);
}
template <class Key, class Hash, class Pred, class Alloc>
constexpr bool operator==(const unordered_multiset<Key, Hash, Pred, Alloc>& a,
                          const unordered_multiset<Key, Hash, Pred, Alloc>& b) {
  return ycxx::detail::hash_table_access::equal(a, b);
}
template <class Key, class Hash, class Pred, class Alloc>
constexpr void swap(unordered_set<Key, Hash, Pred, Alloc>& x,
                    unordered_set<Key, Hash, Pred, Alloc>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}
template <class Key, class Hash, class Pred, class Alloc>
constexpr void swap(unordered_multiset<Key, Hash, Pred, Alloc>& x,
                    unordered_multiset<Key, Hash, Pred, Alloc>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

// ---- [unord.set.erasure], [unord.multiset.erasure] ----
template <class K, class H, class P, class A, class Predicate>
constexpr typename unordered_set<K, H, P, A>::size_type erase_if(unordered_set<K, H, P, A>& c, Predicate pred) {
  return ycxx::detail::hash_table_access::erase_if(c, pred);
}
template <class K, class H, class P, class A, class Predicate>
constexpr typename unordered_multiset<K, H, P, A>::size_type erase_if(unordered_multiset<K, H, P, A>& c,
                                                                      Predicate pred) {
  return ycxx::detail::hash_table_access::erase_if(c, pred);
}

namespace pmr {
template <class Key, class Hash = hash<Key>, class Pred = equal_to<Key>>
using unordered_set = std::unordered_set<Key, Hash, Pred, polymorphic_allocator<Key>>;
template <class Key, class Hash = hash<Key>, class Pred = equal_to<Key>>
using unordered_multiset = std::unordered_multiset<Key, Hash, Pred, polymorphic_allocator<Key>>;
} // namespace pmr

} // namespace std
