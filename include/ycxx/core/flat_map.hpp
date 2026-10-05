// libycxx core: flat_map and flat_multimap ([flat.map], [flat.multimap]), their erasure,
// deduction guides and uses_allocator specializations.
//
// The keys and the mapped values live in two sequence containers (c_.keys, c_.values) kept in
// step; flat_map_base holds them and the members both adaptors share. The iterators pair an
// iterator into each container; their reference is pair<const Key&, T&>, a proxy. Every
// operation that changes the containers is covered by a flat_guard (flat_support.hpp), which
// restores the invariants of [flat.map.overview]/5 by clearing both containers if the operation
// exits via an exception. Bulk insertions append, then sort and merge through an index
// permutation (flat_support.hpp).
#pragma once

#include <initializer_list>
#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/flat_support.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/optional.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/tuple.hpp>
#include <ycxx/core/vector.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

template <class KC, class MC, bool Const>
class flat_map_iter {
  using KIt = typename KC::const_iterator;
  using MIt = std::conditional_t<Const, typename MC::const_iterator, typename MC::iterator>;
  using K = typename KC::value_type;
  using T = typename MC::value_type;

  KIt k_{};
  MIt m_{};

  template <class, class, bool>
  friend class flat_map_iter;
  template <class, class, class, class, class, bool>
  friend class flat_map_base;

  constexpr flat_map_iter(KIt k, MIt m) noexcept : k_(k), m_(m) {}

public:
  using iterator_concept = std::random_access_iterator_tag;
  // The reference is a proxy, but the Cpp17 algorithms (std::prev, std::distance, ...) should
  // still treat these iterators as random access.
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::pair<K, T>;
  using difference_type = std::ptrdiff_t;
  using reference = std::pair<const K&, std::conditional_t<Const, const T&, T&>>;

  // The result of operator->: holds the proxy reference. Also the iterator's pointer type, so
  // that reverse_iterator::operator-> works.
  struct arrow {
    reference r;
    constexpr const reference* operator->() const noexcept { return __builtin_addressof(r); }
  };
  using pointer = arrow;

  constexpr flat_map_iter() = default;
  template <bool C2>
    requires(Const && !C2)
  constexpr flat_map_iter(const flat_map_iter<KC, MC, C2>& o) noexcept : k_(o.k_), m_(o.m_) {}

  constexpr reference operator*() const noexcept { return reference(*k_, *m_); }
  constexpr arrow operator->() const noexcept { return arrow{**this}; }
  constexpr reference operator[](difference_type n) const noexcept { return *(*this + n); }

  constexpr flat_map_iter& operator++() noexcept {
    ++k_;
    ++m_;
    return *this;
  }
  constexpr flat_map_iter operator++(int) noexcept {
    flat_map_iter t = *this;
    ++*this;
    return t;
  }
  constexpr flat_map_iter& operator--() noexcept {
    --k_;
    --m_;
    return *this;
  }
  constexpr flat_map_iter operator--(int) noexcept {
    flat_map_iter t = *this;
    --*this;
    return t;
  }
  constexpr flat_map_iter& operator+=(difference_type n) noexcept {
    k_ += static_cast<typename KC::difference_type>(n);
    m_ += static_cast<typename MC::difference_type>(n);
    return *this;
  }
  constexpr flat_map_iter& operator-=(difference_type n) noexcept {
    k_ -= static_cast<typename KC::difference_type>(n);
    m_ -= static_cast<typename MC::difference_type>(n);
    return *this;
  }
  friend constexpr flat_map_iter operator+(flat_map_iter i, difference_type n) noexcept { return i += n; }
  friend constexpr flat_map_iter operator+(difference_type n, flat_map_iter i) noexcept { return i += n; }
  friend constexpr flat_map_iter operator-(flat_map_iter i, difference_type n) noexcept { return i -= n; }
  friend constexpr difference_type operator-(const flat_map_iter& a, const flat_map_iter& b) noexcept {
    return static_cast<difference_type>(a.k_ - b.k_);
  }
  friend constexpr bool operator==(const flat_map_iter& a, const flat_map_iter& b) noexcept { return a.k_ == b.k_; }
  friend constexpr std::strong_ordering operator<=>(const flat_map_iter& a, const flat_map_iter& b) noexcept {
    return static_cast<difference_type>(a.k_ - b.k_) <=> 0;
  }
};

// The members flat_map (Multi false) and flat_multimap share.
template <class Key, class T, class Compare, class KC, class MC, bool Multi>
class flat_map_base {
  static_assert(std::is_same_v<Key, typename KC::value_type>,
                "flat_map: Key must be KeyContainer::value_type ([flat.map.overview]/8)");
  static_assert(std::is_same_v<T, typename MC::value_type>,
                "flat_map: T must be MappedContainer::value_type ([flat.map.overview]/8)");

public:
  // ---- types ----
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<key_type, mapped_type>;
  using key_compare = Compare;
  using reference = std::pair<const key_type&, mapped_type&>;
  using const_reference = std::pair<const key_type&, const mapped_type&>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using iterator = flat_map_iter<KC, MC, false>;
  using const_iterator = flat_map_iter<KC, MC, true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using key_container_type = KC;
  using mapped_container_type = MC;

  class value_compare {
    friend class flat_map_base;
    key_compare comp;
    constexpr value_compare(key_compare c) : comp(c) {}

  public:
    constexpr bool operator()(const_reference x, const_reference y) const { return comp(x.first, y.first); }
  };

  struct containers {
    key_container_type keys;
    mapped_container_type values;
  };

protected:
  containers c_;
  [[no_unique_address]] key_compare compare_;

  // ---- helpers ----
  template <class A, class B>
  constexpr bool lt(const A& a, const B& b) const {
    return static_cast<bool>(static_cast<const key_compare&>(compare_)(a, b));
  }
  constexpr auto less_fn() const noexcept {
    return [this](const auto& a, const auto& b) -> bool { return lt(a, b); };
  }
  constexpr const key_type& key_at(size_type i) const { return ::ycxx::detail::row_at(c_.keys, i); }
  constexpr iterator it_at(size_type i) noexcept {
    return iterator(c_.keys.cbegin() + static_cast<typename KC::difference_type>(i),
                    c_.values.begin() + static_cast<typename MC::difference_type>(i));
  }
  constexpr const_iterator it_at(size_type i) const noexcept {
    return const_iterator(c_.keys.cbegin() + static_cast<typename KC::difference_type>(i),
                          c_.values.cbegin() + static_cast<typename MC::difference_type>(i));
  }
  constexpr size_type index_of(const_iterator it) const noexcept {
    return static_cast<size_type>(it.k_ - c_.keys.cbegin());
  }
  constexpr auto guard() noexcept { return ::ycxx::detail::flat_guard(c_.keys, c_.values); }

  // The first index whose key is not less than x, the first whose key is greater than x.
  template <class K>
  constexpr size_type lower_index(const K& x) const {
    return lower_index(x, 0, size());
  }
  template <class K>
  constexpr size_type lower_index(const K& x, size_type lo, size_type hi) const {
    size_type n = hi - lo;
    while (n > 0) {
      const size_type half = n / 2;
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
  constexpr size_type upper_index(const K& x) const {
    return upper_index(x, 0, size());
  }
  template <class K>
  constexpr size_type upper_index(const K& x, size_type lo, size_type hi) const {
    size_type n = hi - lo;
    while (n > 0) {
      const size_type half = n / 2;
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
  constexpr size_type find_index(const K& x) const {
    const size_type i = lower_index(x);
    if (i == size() || lt(x, key_at(i)))
      return size();
    return i;
  }
  // Unique keys: the index of the element with key x, or where x belongs (found false).
  template <class K>
  constexpr std::pair<size_type, bool> unique_pos(const K& x) const {
    const size_type i = lower_index(x);
    return {i, i != size() && !lt(x, key_at(i))};
  }
  // Unique keys with a hint: constant comparisons when x belongs just before hint.
  template <class K>
  constexpr std::pair<size_type, bool> unique_pos_hint(const_iterator hint, const K& x) const {
    const size_type h = index_of(hint);
    if ((h == 0 || lt(key_at(h - 1), x)) && (h == size() || lt(x, key_at(h))))
      return {h, false};
    return unique_pos(x);
  }
  // Equivalent keys with a hint: as close as possible to the position just before hint.
  template <class K>
  constexpr size_type multi_pos_hint(const_iterator hint, const K& x) const {
    const size_type h = index_of(hint);
    if (h > 0 && lt(x, key_at(h - 1)))
      return upper_index(x, 0, h - 1);
    if (h < size() && lt(key_at(h), x))
      return lower_index(x, h + 1, size());
    return h;
  }

  // Inserts the row (key, mapped value built from margs) at index i.
  template <class K, class... MArgs>
  constexpr iterator insert_row(size_type i, K&& k, MArgs&&... margs) {
    auto g = guard();
    c_.keys.emplace(c_.keys.cbegin() + static_cast<typename KC::difference_type>(i), static_cast<K&&>(k));
    c_.values.emplace(c_.values.cbegin() + static_cast<typename MC::difference_type>(i),
                      static_cast<MArgs&&>(margs)...);
    g.release();
    return it_at(i);
  }
  constexpr std::pair<iterator, bool> insert_unique(value_type&& t) {
    const auto p = unique_pos(t.first);
    if (p.second)
      return {it_at(p.first), false};
    return {insert_row(p.first, static_cast<key_type&&>(t.first), static_cast<mapped_type&&>(t.second)), true};
  }
  constexpr iterator insert_unique_hint(const_iterator hint, value_type&& t) {
    const auto p = unique_pos_hint(hint, t.first);
    if (p.second)
      return it_at(p.first);
    return insert_row(p.first, static_cast<key_type&&>(t.first), static_cast<mapped_type&&>(t.second));
  }
  constexpr iterator insert_multi(value_type&& t) {
    const size_type i = upper_index(t.first);
    return insert_row(i, static_cast<key_type&&>(t.first), static_cast<mapped_type&&>(t.second));
  }
  constexpr iterator insert_multi_hint(const_iterator hint, value_type&& t) {
    const size_type i = multi_pos_hint(hint, t.first);
    return insert_row(i, static_cast<key_type&&>(t.first), static_cast<mapped_type&&>(t.second));
  }

  // Sorts the rows from `from` on into the sorted rows before them; with unique keys, then keeps
  // the first of each run of equivalent keys. g restores the invariants on an exception.
  constexpr void sort_from(size_type from) {
    auto less = less_fn();
    ::ycxx::detail::sort_rows(less, from, c_.keys, c_.values);
    if constexpr (!Multi)
      ::ycxx::detail::unique_rows(less, c_.keys, c_.values);
  }
  // [flat.map.cons]/1: the containers given to a constructor.
  constexpr void sort_all() {
    ::ycxx::detail::precondition(c_.keys.size() == c_.values.size(),
                                 "flat_map: key and mapped containers of different sizes");
    auto g = guard();
    sort_from(0);
    g.release();
  }
  // [flat.map.modifiers]/6, /11: appends each element, then sorts the new rows in.
  template <class It, class Sent>
  constexpr void insert_elems(It first, Sent last) {
    const size_type old = size();
    auto g = guard();
    for (; first != last; ++first) {
      value_type value = *first;
      c_.keys.insert(c_.keys.end(), static_cast<key_type&&>(value.first));
      c_.values.insert(c_.values.end(), static_cast<mapped_type&&>(value.second));
    }
    sort_from(old);
    g.release();
  }
  template <class R>
  constexpr void insert_range_elems(R&& rg) {
    insert_elems(std::ranges::begin(rg), std::ranges::end(rg));
  }

  // Moves the containers out of o, which is emptied, even if a move throws.
  static constexpr containers take(flat_map_base& o) {
    auto g = o.guard();
    return containers{static_cast<KC&&>(o.c_.keys), static_cast<MC&&>(o.c_.values)};
  }
  template <class A>
  static constexpr containers take(flat_map_base& o, const A& a) {
    auto g = o.guard();
    return containers{std::make_obj_using_allocator<KC>(a, static_cast<KC&&>(o.c_.keys)),
                      std::make_obj_using_allocator<MC>(a, static_cast<MC&&>(o.c_.values))};
  }

  // ---- construction and assignment ----
  constexpr explicit flat_map_base(const key_compare& comp) : c_(), compare_(comp) {}
  template <class A>
  constexpr flat_map_base(const key_compare& comp, const A& a)
      : c_{std::make_obj_using_allocator<KC>(a), std::make_obj_using_allocator<MC>(a)}, compare_(comp) {}
  constexpr flat_map_base(KC&& k, MC&& m, const key_compare& comp)
      : c_{static_cast<KC&&>(k), static_cast<MC&&>(m)}, compare_(comp) {}
  template <class A>
  constexpr flat_map_base(const KC& k, const MC& m, const key_compare& comp, const A& a)
      : c_{std::make_obj_using_allocator<KC>(a, k), std::make_obj_using_allocator<MC>(a, m)}, compare_(comp) {}
  constexpr flat_map_base(const flat_map_base&) = default;
  constexpr flat_map_base(flat_map_base&& o) : c_(take(o)), compare_(o.compare_) {}
  template <class A>
  constexpr flat_map_base(const flat_map_base& o, const A& a)
      : c_{std::make_obj_using_allocator<KC>(a, o.c_.keys), std::make_obj_using_allocator<MC>(a, o.c_.values)},
        compare_(o.compare_) {}
  template <class A>
  constexpr flat_map_base(flat_map_base&& o, const A& a) : c_(take(o, a)), compare_(o.compare_) {}

  constexpr void copy_assign(const flat_map_base& o) {
    if (this == __builtin_addressof(o))
      return;
    auto g = guard();
    c_.keys = o.c_.keys;
    c_.values = o.c_.values;
    compare_ = o.compare_;
    g.release();
  }
  constexpr void move_assign(flat_map_base& o) {
    if (this == __builtin_addressof(o))
      return;
    auto go = o.guard(); // o is emptied in any case
    auto g = guard();
    c_.keys = static_cast<KC&&>(o.c_.keys);
    c_.values = static_cast<MC&&>(o.c_.values);
    compare_ = static_cast<key_compare&&>(o.compare_);
    g.release();
  }
  constexpr void swap_impl(flat_map_base& y) {
    auto g = guard();
    auto gy = y.guard();
    ::ycxx::detail::swap_adl::do_swap(compare_, y.compare_);
    ::ycxx::detail::swap_adl::do_swap(c_.keys, y.c_.keys);
    ::ycxx::detail::swap_adl::do_swap(c_.values, y.c_.values);
    gy.release();
    g.release();
  }

public:
  // ---- iterators ----
  constexpr iterator begin() noexcept { return iterator(c_.keys.cbegin(), c_.values.begin()); }
  constexpr const_iterator begin() const noexcept { return const_iterator(c_.keys.cbegin(), c_.values.cbegin()); }
  constexpr iterator end() noexcept { return iterator(c_.keys.cend(), c_.values.end()); }
  constexpr const_iterator end() const noexcept { return const_iterator(c_.keys.cend(), c_.values.cend()); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [flat.map.capacity] ----
  [[nodiscard]] constexpr bool empty() const noexcept { return c_.keys.empty(); }
  constexpr size_type size() const noexcept { return c_.keys.size(); }
  constexpr size_type max_size() const noexcept {
    const auto k = static_cast<size_type>(c_.keys.max_size());
    const auto m = static_cast<size_type>(c_.values.max_size());
    return k < m ? k : m;
  }

  // ---- modifiers ----
  constexpr containers extract() && {
    auto g = guard();
    return containers{static_cast<KC&&>(c_.keys), static_cast<MC&&>(c_.values)};
  }
  constexpr void replace(key_container_type&& key_cont, mapped_container_type&& mapped_cont) {
    ::ycxx::detail::precondition(key_cont.size() == mapped_cont.size(),
                                 "flat_map::replace: key and mapped containers of different sizes");
    auto g = guard();
    c_.keys = static_cast<KC&&>(key_cont);
    c_.values = static_cast<MC&&>(mapped_cont);
    g.release();
  }

  constexpr iterator erase(iterator position) { return erase(const_iterator(position)); }
  constexpr iterator erase(const_iterator position) {
    const size_type i = index_of(position);
    ::ycxx::detail::precondition(i < size(), "flat_map::erase: end() iterator");
    auto g = guard();
    c_.keys.erase(c_.keys.cbegin() + static_cast<typename KC::difference_type>(i));
    c_.values.erase(c_.values.cbegin() + static_cast<typename MC::difference_type>(i));
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
    const size_type i = index_of(first), j = index_of(last);
    if (i != j) {
      auto g = guard();
      c_.keys.erase(c_.keys.cbegin() + static_cast<typename KC::difference_type>(i),
                    c_.keys.cbegin() + static_cast<typename KC::difference_type>(j));
      c_.values.erase(c_.values.cbegin() + static_cast<typename MC::difference_type>(i),
                      c_.values.cbegin() + static_cast<typename MC::difference_type>(j));
      g.release();
    }
    return it_at(i);
  }
  constexpr void clear() noexcept {
    c_.keys.clear();
    c_.values.clear();
  }

  // ---- observers ----
  constexpr key_compare key_comp() const { return compare_; }
  constexpr value_compare value_comp() const { return value_compare(compare_); }
  constexpr const key_container_type& keys() const noexcept { return c_.keys; }
  constexpr const mapped_container_type& values() const noexcept { return c_.values; }

  // ---- map operations ----
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
      return upper_index(x) - lower_index(x);
    else
      return find_index(x) == size() ? 0 : 1;
  }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr size_type count(const K& x) const {
    return upper_index(x) - lower_index(x);
  }
  constexpr bool contains(const key_type& x) const { return find_index(x) != size(); }
  template <class K>
    requires ::ycxx::detail::transparent_compare<Compare>
  constexpr bool contains(const K& x) const {
    return find_index(x) != size();
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
    const size_type i = lower_index(x), j = upper_index(x);
    erase(it_at(i), it_at(j));
    return j - i;
  }
};

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {

template <class Key, class T, class Compare = less<Key>, class KeyContainer = vector<Key>,
          class MappedContainer = vector<T>>
class flat_map
    : public ycxx::adl_free::flat_map_base<Key, T, Compare, KeyContainer, MappedContainer, false> {
  using base = ycxx::adl_free::flat_map_base<Key, T, Compare, KeyContainer, MappedContainer, false>;

public:
  // ---- types ----
  using typename base::const_iterator;
  using typename base::const_reference;
  using typename base::const_reverse_iterator;
  using typename base::containers;
  using typename base::difference_type;
  using typename base::iterator;
  using typename base::key_compare;
  using typename base::key_container_type;
  using typename base::key_type;
  using typename base::mapped_container_type;
  using typename base::mapped_type;
  using typename base::reference;
  using typename base::reverse_iterator;
  using typename base::size_type;
  using typename base::value_compare;
  using typename base::value_type;

  // ---- [flat.map.cons] ----
  constexpr flat_map() : flat_map(key_compare()) {}
  constexpr flat_map(const flat_map&) = default;
  constexpr flat_map(flat_map&& x) : base(static_cast<base&&>(x)) {}
  constexpr flat_map& operator=(const flat_map& x) {
    this->copy_assign(x);
    return *this;
  }
  constexpr flat_map& operator=(flat_map&& x) {
    this->move_assign(x);
    return *this;
  }
  constexpr explicit flat_map(const key_compare& comp) : base(comp) {}
  constexpr flat_map(key_container_type key_cont, mapped_container_type mapped_cont,
                     const key_compare& comp = key_compare())
      : base(static_cast<key_container_type&&>(key_cont), static_cast<mapped_container_type&&>(mapped_cont), comp) {
    this->sort_all();
  }
  constexpr flat_map(sorted_unique_t, key_container_type key_cont, mapped_container_type mapped_cont,
                     const key_compare& comp = key_compare())
      : base(static_cast<key_container_type&&>(key_cont), static_cast<mapped_container_type&&>(mapped_cont), comp) {
    ycxx::detail::precondition(this->c_.keys.size() == this->c_.values.size(),
                               "flat_map: key and mapped containers of different sizes");
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr flat_map(InputIterator first, InputIterator last, const key_compare& comp = key_compare()) : base(comp) {
    this->insert_elems(first, last);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr flat_map(sorted_unique_t, InputIterator first, InputIterator last, const key_compare& comp = key_compare())
      : base(comp) {
    this->insert_elems(first, last);
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr flat_map(Tag, R&& rg) : flat_map(from_range, static_cast<R&&>(rg), key_compare()) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr flat_map(Tag, R&& rg, const key_compare& comp) : base(comp) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  constexpr flat_map(initializer_list<value_type> il, const key_compare& comp = key_compare())
      : flat_map(il.begin(), il.end(), comp) {}
  constexpr flat_map(sorted_unique_t, initializer_list<value_type> il, const key_compare& comp = key_compare())
      : flat_map(sorted_unique, il.begin(), il.end(), comp) {}

  // ---- [flat.map.cons.alloc] ----
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr explicit flat_map(const Alloc& a) : base(key_compare(), a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(const key_compare& comp, const Alloc& a) : base(comp, a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(const key_container_type& key_cont, const mapped_container_type& mapped_cont, const Alloc& a)
      : base(key_cont, mapped_cont, key_compare(), a) {
    this->sort_all();
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(const key_container_type& key_cont, const mapped_container_type& mapped_cont,
                     const key_compare& comp, const Alloc& a)
      : base(key_cont, mapped_cont, comp, a) {
    this->sort_all();
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(sorted_unique_t, const key_container_type& key_cont, const mapped_container_type& mapped_cont,
                     const Alloc& a)
      : base(key_cont, mapped_cont, key_compare(), a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(sorted_unique_t, const key_container_type& key_cont, const mapped_container_type& mapped_cont,
                     const key_compare& comp, const Alloc& a)
      : base(key_cont, mapped_cont, comp, a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(const flat_map& x, const Alloc& a) : base(x, a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(flat_map&& x, const Alloc& a) : base(static_cast<base&&>(x), a) {}
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(InputIterator first, InputIterator last, const Alloc& a) : base(key_compare(), a) {
    this->insert_elems(first, last);
  }
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(InputIterator first, InputIterator last, const key_compare& comp, const Alloc& a)
      : base(comp, a) {
    this->insert_elems(first, last);
  }
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(sorted_unique_t, InputIterator first, InputIterator last, const Alloc& a)
      : base(key_compare(), a) {
    this->insert_elems(first, last);
  }
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(sorted_unique_t, InputIterator first, InputIterator last, const key_compare& comp,
                     const Alloc& a)
      : base(comp, a) {
    this->insert_elems(first, last);
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R, class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(Tag, R&& rg, const Alloc& a) : base(key_compare(), a) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R, class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(Tag, R&& rg, const key_compare& comp, const Alloc& a) : base(comp, a) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(initializer_list<value_type> il, const Alloc& a) : base(key_compare(), a) {
    this->insert_elems(il.begin(), il.end());
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(initializer_list<value_type> il, const key_compare& comp, const Alloc& a) : base(comp, a) {
    this->insert_elems(il.begin(), il.end());
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(sorted_unique_t, initializer_list<value_type> il, const Alloc& a) : base(key_compare(), a) {
    this->insert_elems(il.begin(), il.end());
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_map(sorted_unique_t, initializer_list<value_type> il, const key_compare& comp, const Alloc& a)
      : base(comp, a) {
    this->insert_elems(il.begin(), il.end());
  }

  constexpr flat_map& operator=(initializer_list<value_type> il) {
    this->clear();
    this->insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- [flat.map.access] ----
  // The constraints of the try_emplace call each operator[] is equivalent to are checked up front.
  constexpr mapped_type& operator[](const key_type& x)
    requires is_constructible_v<mapped_type>
  {
    return try_emplace(x).first->second;
  }
  constexpr mapped_type& operator[](key_type&& x)
    requires is_constructible_v<mapped_type>
  {
    return try_emplace(static_cast<key_type&&>(x)).first->second;
  }
  template <class K>
    requires ycxx::detail::transparent_non_iter<Compare, K, iterator, const_iterator> &&
             is_constructible_v<key_type, K> && is_constructible_v<mapped_type>
  constexpr mapped_type& operator[](K&& x) {
    return try_emplace(static_cast<K&&>(x)).first->second;
  }
  constexpr mapped_type& at(const key_type& x) { return at_impl(x); }
  constexpr const mapped_type& at(const key_type& x) const { return const_cast<flat_map*>(this)->at_impl(x); }
  template <class K>
    requires ycxx::detail::transparent_compare<Compare>
  constexpr mapped_type& at(const K& x) {
    return at_impl(x);
  }
  template <class K>
    requires ycxx::detail::transparent_compare<Compare>
  constexpr const mapped_type& at(const K& x) const {
    return const_cast<flat_map*>(this)->at_impl(x);
  }
  constexpr optional<mapped_type&> lookup(const key_type& x) { return lookup_impl<mapped_type&>(x); }
  constexpr optional<const mapped_type&> lookup(const key_type& x) const {
    return const_cast<flat_map*>(this)->template lookup_impl<const mapped_type&>(x);
  }
  template <class K>
    requires ycxx::detail::transparent_compare<Compare>
  constexpr optional<mapped_type&> lookup(const K& x) {
    return lookup_impl<mapped_type&>(x);
  }
  template <class K>
    requires ycxx::detail::transparent_compare<Compare>
  constexpr optional<const mapped_type&> lookup(const K& x) const {
    return const_cast<flat_map*>(this)->template lookup_impl<const mapped_type&>(x);
  }

  // ---- [flat.map.modifiers] ----
  template <class... Args>
    requires is_constructible_v<pair<key_type, mapped_type>, Args...>
  constexpr pair<iterator, bool> emplace(Args&&... args) {
    return this->insert_unique(value_type(static_cast<Args&&>(args)...));
  }
  template <class... Args>
    requires is_constructible_v<pair<key_type, mapped_type>, Args...>
  constexpr iterator emplace_hint(const_iterator position, Args&&... args) {
    return this->insert_unique_hint(position, value_type(static_cast<Args&&>(args)...));
  }
  constexpr pair<iterator, bool> insert(const value_type& x) { return emplace(x); }
  constexpr pair<iterator, bool> insert(value_type&& x) { return emplace(static_cast<value_type&&>(x)); }
  constexpr iterator insert(const_iterator position, const value_type& x) { return emplace_hint(position, x); }
  constexpr iterator insert(const_iterator position, value_type&& x) {
    return emplace_hint(position, static_cast<value_type&&>(x));
  }
  template <class P>
    requires is_constructible_v<pair<key_type, mapped_type>, P>
  constexpr pair<iterator, bool> insert(P&& x) {
    return emplace(static_cast<P&&>(x));
  }
  template <class P>
    requires is_constructible_v<pair<key_type, mapped_type>, P>
  constexpr iterator insert(const_iterator position, P&& x) {
    return emplace_hint(position, static_cast<P&&>(x));
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

  template <class... Args>
    requires is_constructible_v<mapped_type, Args...>
  constexpr pair<iterator, bool> try_emplace(const key_type& k, Args&&... args) {
    return try_emplace_impl(this->unique_pos(k), k, static_cast<Args&&>(args)...);
  }
  template <class... Args>
    requires is_constructible_v<mapped_type, Args...>
  constexpr pair<iterator, bool> try_emplace(key_type&& k, Args&&... args) {
    return try_emplace_impl(this->unique_pos(k), static_cast<key_type&&>(k), static_cast<Args&&>(args)...);
  }
  template <class K, class... Args>
    requires ycxx::detail::transparent_non_iter<Compare, K, iterator, const_iterator> &&
             is_constructible_v<key_type, K> && is_constructible_v<mapped_type, Args...>
  constexpr pair<iterator, bool> try_emplace(K&& k, Args&&... args) {
    return try_emplace_impl(this->unique_pos(k), static_cast<K&&>(k), static_cast<Args&&>(args)...);
  }
  template <class... Args>
    requires is_constructible_v<mapped_type, Args...>
  constexpr iterator try_emplace(const_iterator hint, const key_type& k, Args&&... args) {
    return try_emplace_impl(this->unique_pos_hint(hint, k), k, static_cast<Args&&>(args)...).first;
  }
  template <class... Args>
    requires is_constructible_v<mapped_type, Args...>
  constexpr iterator try_emplace(const_iterator hint, key_type&& k, Args&&... args) {
    return try_emplace_impl(this->unique_pos_hint(hint, k), static_cast<key_type&&>(k), static_cast<Args&&>(args)...)
        .first;
  }
  template <class K, class... Args>
    requires ycxx::detail::transparent_compare<Compare> && is_constructible_v<key_type, K> &&
             is_constructible_v<mapped_type, Args...>
  constexpr iterator try_emplace(const_iterator hint, K&& k, Args&&... args) {
    return try_emplace_impl(this->unique_pos_hint(hint, k), static_cast<K&&>(k), static_cast<Args&&>(args)...).first;
  }
  template <class M>
    requires is_assignable_v<mapped_type&, M> && is_constructible_v<mapped_type, M>
  constexpr pair<iterator, bool> insert_or_assign(const key_type& k, M&& obj) {
    return insert_or_assign_impl(this->unique_pos(k), k, static_cast<M&&>(obj));
  }
  template <class M>
    requires is_assignable_v<mapped_type&, M> && is_constructible_v<mapped_type, M>
  constexpr pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj) {
    return insert_or_assign_impl(this->unique_pos(k), static_cast<key_type&&>(k), static_cast<M&&>(obj));
  }
  template <class K, class M>
    requires ycxx::detail::transparent_compare<Compare> && is_constructible_v<key_type, K> &&
             is_assignable_v<mapped_type&, M> && is_constructible_v<mapped_type, M>
  constexpr pair<iterator, bool> insert_or_assign(K&& k, M&& obj) {
    return insert_or_assign_impl(this->unique_pos(k), static_cast<K&&>(k), static_cast<M&&>(obj));
  }
  template <class M>
    requires is_assignable_v<mapped_type&, M> && is_constructible_v<mapped_type, M>
  constexpr iterator insert_or_assign(const_iterator hint, const key_type& k, M&& obj) {
    return insert_or_assign_impl(this->unique_pos_hint(hint, k), k, static_cast<M&&>(obj)).first;
  }
  template <class M>
    requires is_assignable_v<mapped_type&, M> && is_constructible_v<mapped_type, M>
  constexpr iterator insert_or_assign(const_iterator hint, key_type&& k, M&& obj) {
    return insert_or_assign_impl(this->unique_pos_hint(hint, k), static_cast<key_type&&>(k), static_cast<M&&>(obj))
        .first;
  }
  template <class K, class M>
    requires ycxx::detail::transparent_compare<Compare> && is_constructible_v<key_type, K> &&
             is_assignable_v<mapped_type&, M> && is_constructible_v<mapped_type, M>
  constexpr iterator insert_or_assign(const_iterator hint, K&& k, M&& obj) {
    return insert_or_assign_impl(this->unique_pos_hint(hint, k), static_cast<K&&>(k), static_cast<M&&>(obj)).first;
  }

  constexpr void swap(flat_map& y) noexcept(is_nothrow_swappable_v<key_container_type> &&
                                            is_nothrow_swappable_v<mapped_container_type> &&
                                            is_nothrow_swappable_v<key_compare>) {
    this->swap_impl(y);
  }

  friend constexpr bool operator==(const flat_map& x, const flat_map& y) {
    return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
  }
  // A template, so that the return type is formed only when the operator is used.
  template <class V = value_type>
  friend constexpr ycxx::detail::synth_three_way_result<V> operator<=>(const flat_map& x, const flat_map& y) {
    return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(),
                                                  ycxx::detail::synth_three_way);
  }
  friend constexpr void swap(flat_map& x, flat_map& y) noexcept(noexcept(x.swap(y))) { x.swap(y); }

private:
  template <class K>
  constexpr mapped_type& at_impl(const K& x) {
    const size_type i = this->find_index(x);
    if (i == this->size())
      ycxx::detail::throw_out_of_range("std::flat_map::at: key not found");
    return ycxx::detail::row_at(this->c_.values, i);
  }
  template <class R, class K>
  constexpr optional<R> lookup_impl(const K& x) {
    const size_type i = this->find_index(x);
    if (i == this->size())
      return nullopt;
    return optional<R>(ycxx::detail::row_at(this->c_.values, i));
  }
  template <class K, class... Args>
  constexpr pair<iterator, bool> try_emplace_impl(pair<size_type, bool> p, K&& k, Args&&... args) {
    if (p.second)
      return {this->it_at(p.first), false};
    return {this->insert_row(p.first, static_cast<K&&>(k), static_cast<Args&&>(args)...), true};
  }
  template <class K, class M>
  constexpr pair<iterator, bool> insert_or_assign_impl(pair<size_type, bool> p, K&& k, M&& obj) {
    if (p.second) {
      ycxx::detail::row_at(this->c_.values, p.first) = static_cast<M&&>(obj);
      return {this->it_at(p.first), false};
    }
    return {this->insert_row(p.first, static_cast<K&&>(k), static_cast<M&&>(obj)), true};
  }
};

template <class Key, class T, class Compare = less<Key>, class KeyContainer = vector<Key>,
          class MappedContainer = vector<T>>
class flat_multimap
    : public ycxx::adl_free::flat_map_base<Key, T, Compare, KeyContainer, MappedContainer, true> {
  using base = ycxx::adl_free::flat_map_base<Key, T, Compare, KeyContainer, MappedContainer, true>;

public:
  // ---- types ----
  using typename base::const_iterator;
  using typename base::const_reference;
  using typename base::const_reverse_iterator;
  using typename base::containers;
  using typename base::difference_type;
  using typename base::iterator;
  using typename base::key_compare;
  using typename base::key_container_type;
  using typename base::key_type;
  using typename base::mapped_container_type;
  using typename base::mapped_type;
  using typename base::reference;
  using typename base::reverse_iterator;
  using typename base::size_type;
  using typename base::value_compare;
  using typename base::value_type;

  // ---- [flat.multimap.cons] ----
  constexpr flat_multimap() : flat_multimap(key_compare()) {}
  constexpr flat_multimap(const flat_multimap&) = default;
  constexpr flat_multimap(flat_multimap&& x) : base(static_cast<base&&>(x)) {}
  constexpr flat_multimap& operator=(const flat_multimap& x) {
    this->copy_assign(x);
    return *this;
  }
  constexpr flat_multimap& operator=(flat_multimap&& x) {
    this->move_assign(x);
    return *this;
  }
  constexpr explicit flat_multimap(const key_compare& comp) : base(comp) {}
  constexpr flat_multimap(key_container_type key_cont, mapped_container_type mapped_cont,
                          const key_compare& comp = key_compare())
      : base(static_cast<key_container_type&&>(key_cont), static_cast<mapped_container_type&&>(mapped_cont), comp) {
    this->sort_all();
  }
  constexpr flat_multimap(sorted_equivalent_t, key_container_type key_cont, mapped_container_type mapped_cont,
                          const key_compare& comp = key_compare())
      : base(static_cast<key_container_type&&>(key_cont), static_cast<mapped_container_type&&>(mapped_cont), comp) {
    ycxx::detail::precondition(this->c_.keys.size() == this->c_.values.size(),
                               "flat_multimap: key and mapped containers of different sizes");
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr flat_multimap(InputIterator first, InputIterator last, const key_compare& comp = key_compare())
      : base(comp) {
    this->insert_elems(first, last);
  }
  template <class InputIterator>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator>
  constexpr flat_multimap(sorted_equivalent_t, InputIterator first, InputIterator last,
                          const key_compare& comp = key_compare())
      : base(comp) {
    this->insert_elems(first, last);
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr flat_multimap(Tag, R&& rg) : flat_multimap(from_range, static_cast<R&&>(rg), key_compare()) {}
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R>
  constexpr flat_multimap(Tag, R&& rg, const key_compare& comp) : base(comp) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  constexpr flat_multimap(initializer_list<value_type> il, const key_compare& comp = key_compare())
      : flat_multimap(il.begin(), il.end(), comp) {}
  constexpr flat_multimap(sorted_equivalent_t, initializer_list<value_type> il,
                          const key_compare& comp = key_compare())
      : flat_multimap(sorted_equivalent, il.begin(), il.end(), comp) {}

  // ---- [flat.multimap.cons.alloc] ----
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr explicit flat_multimap(const Alloc& a) : base(key_compare(), a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(const key_compare& comp, const Alloc& a) : base(comp, a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(const key_container_type& key_cont, const mapped_container_type& mapped_cont,
                          const Alloc& a)
      : base(key_cont, mapped_cont, key_compare(), a) {
    this->sort_all();
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(const key_container_type& key_cont, const mapped_container_type& mapped_cont,
                          const key_compare& comp, const Alloc& a)
      : base(key_cont, mapped_cont, comp, a) {
    this->sort_all();
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(sorted_equivalent_t, const key_container_type& key_cont,
                          const mapped_container_type& mapped_cont, const Alloc& a)
      : base(key_cont, mapped_cont, key_compare(), a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(sorted_equivalent_t, const key_container_type& key_cont,
                          const mapped_container_type& mapped_cont, const key_compare& comp, const Alloc& a)
      : base(key_cont, mapped_cont, comp, a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(const flat_multimap& x, const Alloc& a) : base(x, a) {}
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(flat_multimap&& x, const Alloc& a) : base(static_cast<base&&>(x), a) {}
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(InputIterator first, InputIterator last, const Alloc& a) : base(key_compare(), a) {
    this->insert_elems(first, last);
  }
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(InputIterator first, InputIterator last, const key_compare& comp, const Alloc& a)
      : base(comp, a) {
    this->insert_elems(first, last);
  }
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(sorted_equivalent_t, InputIterator first, InputIterator last, const Alloc& a)
      : base(key_compare(), a) {
    this->insert_elems(first, last);
  }
  template <class InputIterator, class Alloc>
    requires ycxx::detail::qualifies_as_input_iterator<InputIterator> &&
             ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(sorted_equivalent_t, InputIterator first, InputIterator last, const key_compare& comp,
                          const Alloc& a)
      : base(comp, a) {
    this->insert_elems(first, last);
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R, class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(Tag, R&& rg, const Alloc& a) : base(key_compare(), a) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  template <ycxx::detail::from_range_tag Tag, ycxx::detail::container_compatible_range<value_type> R, class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(Tag, R&& rg, const key_compare& comp, const Alloc& a) : base(comp, a) {
    this->insert_range_elems(static_cast<R&&>(rg));
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(initializer_list<value_type> il, const Alloc& a) : base(key_compare(), a) {
    this->insert_elems(il.begin(), il.end());
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(initializer_list<value_type> il, const key_compare& comp, const Alloc& a)
      : base(comp, a) {
    this->insert_elems(il.begin(), il.end());
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(sorted_equivalent_t, initializer_list<value_type> il, const Alloc& a)
      : base(key_compare(), a) {
    this->insert_elems(il.begin(), il.end());
  }
  template <class Alloc>
    requires ycxx::detail::flat_alloc_for<Alloc, KeyContainer, MappedContainer>
  constexpr flat_multimap(sorted_equivalent_t, initializer_list<value_type> il, const key_compare& comp,
                          const Alloc& a)
      : base(comp, a) {
    this->insert_elems(il.begin(), il.end());
  }

  constexpr flat_multimap& operator=(initializer_list<value_type> il) {
    this->clear();
    this->insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- modifiers ----
  template <class... Args>
    requires is_constructible_v<pair<key_type, mapped_type>, Args...>
  constexpr iterator emplace(Args&&... args) {
    return this->insert_multi(value_type(static_cast<Args&&>(args)...));
  }
  template <class... Args>
    requires is_constructible_v<pair<key_type, mapped_type>, Args...>
  constexpr iterator emplace_hint(const_iterator position, Args&&... args) {
    return this->insert_multi_hint(position, value_type(static_cast<Args&&>(args)...));
  }
  constexpr iterator insert(const value_type& x) { return emplace(x); }
  constexpr iterator insert(value_type&& x) { return emplace(static_cast<value_type&&>(x)); }
  constexpr iterator insert(const_iterator position, const value_type& x) { return emplace_hint(position, x); }
  constexpr iterator insert(const_iterator position, value_type&& x) {
    return emplace_hint(position, static_cast<value_type&&>(x));
  }
  template <class P>
    requires is_constructible_v<pair<key_type, mapped_type>, P>
  constexpr iterator insert(P&& x) {
    return emplace(static_cast<P&&>(x));
  }
  template <class P>
    requires is_constructible_v<pair<key_type, mapped_type>, P>
  constexpr iterator insert(const_iterator position, P&& x) {
    return emplace_hint(position, static_cast<P&&>(x));
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

  constexpr void swap(flat_multimap& y) noexcept(is_nothrow_swappable_v<key_container_type> &&
                                                 is_nothrow_swappable_v<mapped_container_type> &&
                                                 is_nothrow_swappable_v<key_compare>) {
    this->swap_impl(y);
  }

  friend constexpr bool operator==(const flat_multimap& x, const flat_multimap& y) {
    return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin());
  }
  // A template, so that the return type is formed only when the operator is used.
  template <class V = value_type>
  friend constexpr ycxx::detail::synth_three_way_result<V> operator<=>(const flat_multimap& x, const flat_multimap& y) {
    return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(),
                                                  ycxx::detail::synth_three_way);
  }
  friend constexpr void swap(flat_multimap& x, flat_multimap& y) noexcept(noexcept(x.swap(y))) { x.swap(y); }
};

// ---- deduction guides ([container.adaptors.general]/6) ----
template <class KeyContainer, class MappedContainer, class Compare = less<typename KeyContainer::value_type>>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_container_arg<MappedContainer> &&
           ycxx::detail::flat_compare_for<Compare, KeyContainer>
flat_map(KeyContainer, MappedContainer, Compare = Compare())
    -> flat_map<typename KeyContainer::value_type, typename MappedContainer::value_type, Compare, KeyContainer,
                MappedContainer>;
template <class KeyContainer, class MappedContainer, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_container_arg<MappedContainer> &&
           uses_allocator_v<KeyContainer, Allocator> && uses_allocator_v<MappedContainer, Allocator>
flat_map(KeyContainer, MappedContainer, Allocator)
    -> flat_map<typename KeyContainer::value_type, typename MappedContainer::value_type,
                less<typename KeyContainer::value_type>, KeyContainer, MappedContainer>;
template <class KeyContainer, class MappedContainer, class Compare, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_container_arg<MappedContainer> &&
           ycxx::detail::flat_compare_for<Compare, KeyContainer> && uses_allocator_v<KeyContainer, Allocator> &&
           uses_allocator_v<MappedContainer, Allocator>
flat_map(KeyContainer, MappedContainer, Compare, Allocator)
    -> flat_map<typename KeyContainer::value_type, typename MappedContainer::value_type, Compare, KeyContainer,
                MappedContainer>;
template <class KeyContainer, class MappedContainer, class Compare = less<typename KeyContainer::value_type>>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_container_arg<MappedContainer> &&
           ycxx::detail::flat_compare_for<Compare, KeyContainer>
flat_map(sorted_unique_t, KeyContainer, MappedContainer, Compare = Compare())
    -> flat_map<typename KeyContainer::value_type, typename MappedContainer::value_type, Compare, KeyContainer,
                MappedContainer>;
template <class KeyContainer, class MappedContainer, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_container_arg<MappedContainer> &&
           uses_allocator_v<KeyContainer, Allocator> && uses_allocator_v<MappedContainer, Allocator>
flat_map(sorted_unique_t, KeyContainer, MappedContainer, Allocator)
    -> flat_map<typename KeyContainer::value_type, typename MappedContainer::value_type,
                less<typename KeyContainer::value_type>, KeyContainer, MappedContainer>;
template <class KeyContainer, class MappedContainer, class Compare, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_container_arg<MappedContainer> &&
           ycxx::detail::flat_compare_for<Compare, KeyContainer> && uses_allocator_v<KeyContainer, Allocator> &&
           uses_allocator_v<MappedContainer, Allocator>
flat_map(sorted_unique_t, KeyContainer, MappedContainer, Compare, Allocator)
    -> flat_map<typename KeyContainer::value_type, typename MappedContainer::value_type, Compare, KeyContainer,
                MappedContainer>;
template <class InputIterator, class Compare = less<ycxx::detail::iter_key_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::deducible_compare<Compare>
flat_map(InputIterator, InputIterator, Compare = Compare())
    -> flat_map<ycxx::detail::iter_key_type<InputIterator>, ycxx::detail::iter_mapped_type<InputIterator>, Compare>;
template <class InputIterator, class Compare = less<ycxx::detail::iter_key_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::deducible_compare<Compare>
flat_map(sorted_unique_t, InputIterator, InputIterator, Compare = Compare())
    -> flat_map<ycxx::detail::iter_key_type<InputIterator>, ycxx::detail::iter_mapped_type<InputIterator>, Compare>;
template <ranges::input_range R, class Compare = less<ycxx::detail::range_key_type<R>>,
          class Allocator = allocator<byte>>
  requires ycxx::detail::deducible_compare<Compare> && ycxx::detail::qualifies_as_allocator<Allocator>
flat_map(from_range_t, R&&, Compare = Compare(), Allocator = Allocator())
    -> flat_map<ycxx::detail::range_key_type<R>, ycxx::detail::range_mapped_type<R>, Compare,
                vector<ycxx::detail::range_key_type<R>,
                       ycxx::detail::rebound_alloc<Allocator, ycxx::detail::range_key_type<R>>>,
                vector<ycxx::detail::range_mapped_type<R>,
                       ycxx::detail::rebound_alloc<Allocator, ycxx::detail::range_mapped_type<R>>>>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
flat_map(from_range_t, R&&, Allocator)
    -> flat_map<ycxx::detail::range_key_type<R>, ycxx::detail::range_mapped_type<R>,
                less<ycxx::detail::range_key_type<R>>,
                vector<ycxx::detail::range_key_type<R>,
                       ycxx::detail::rebound_alloc<Allocator, ycxx::detail::range_key_type<R>>>,
                vector<ycxx::detail::range_mapped_type<R>,
                       ycxx::detail::rebound_alloc<Allocator, ycxx::detail::range_mapped_type<R>>>>;
template <class Key, class T, class Compare = less<Key>>
  requires ycxx::detail::deducible_compare<Compare>
flat_map(initializer_list<pair<Key, T>>, Compare = Compare()) -> flat_map<Key, T, Compare>;
template <class Key, class T, class Compare = less<Key>>
  requires ycxx::detail::deducible_compare<Compare>
flat_map(sorted_unique_t, initializer_list<pair<Key, T>>, Compare = Compare()) -> flat_map<Key, T, Compare>;

template <class KeyContainer, class MappedContainer, class Compare = less<typename KeyContainer::value_type>>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_container_arg<MappedContainer> &&
           ycxx::detail::flat_compare_for<Compare, KeyContainer>
flat_multimap(KeyContainer, MappedContainer, Compare = Compare())
    -> flat_multimap<typename KeyContainer::value_type, typename MappedContainer::value_type, Compare, KeyContainer,
                     MappedContainer>;
template <class KeyContainer, class MappedContainer, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_container_arg<MappedContainer> &&
           uses_allocator_v<KeyContainer, Allocator> && uses_allocator_v<MappedContainer, Allocator>
flat_multimap(KeyContainer, MappedContainer, Allocator)
    -> flat_multimap<typename KeyContainer::value_type, typename MappedContainer::value_type,
                     less<typename KeyContainer::value_type>, KeyContainer, MappedContainer>;
template <class KeyContainer, class MappedContainer, class Compare, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_container_arg<MappedContainer> &&
           ycxx::detail::flat_compare_for<Compare, KeyContainer> && uses_allocator_v<KeyContainer, Allocator> &&
           uses_allocator_v<MappedContainer, Allocator>
flat_multimap(KeyContainer, MappedContainer, Compare, Allocator)
    -> flat_multimap<typename KeyContainer::value_type, typename MappedContainer::value_type, Compare, KeyContainer,
                     MappedContainer>;
template <class KeyContainer, class MappedContainer, class Compare = less<typename KeyContainer::value_type>>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_container_arg<MappedContainer> &&
           ycxx::detail::flat_compare_for<Compare, KeyContainer>
flat_multimap(sorted_equivalent_t, KeyContainer, MappedContainer, Compare = Compare())
    -> flat_multimap<typename KeyContainer::value_type, typename MappedContainer::value_type, Compare, KeyContainer,
                     MappedContainer>;
template <class KeyContainer, class MappedContainer, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_container_arg<MappedContainer> &&
           uses_allocator_v<KeyContainer, Allocator> && uses_allocator_v<MappedContainer, Allocator>
flat_multimap(sorted_equivalent_t, KeyContainer, MappedContainer, Allocator)
    -> flat_multimap<typename KeyContainer::value_type, typename MappedContainer::value_type,
                     less<typename KeyContainer::value_type>, KeyContainer, MappedContainer>;
template <class KeyContainer, class MappedContainer, class Compare, class Allocator>
  requires ycxx::detail::flat_container_arg<KeyContainer> && ycxx::detail::flat_container_arg<MappedContainer> &&
           ycxx::detail::flat_compare_for<Compare, KeyContainer> && uses_allocator_v<KeyContainer, Allocator> &&
           uses_allocator_v<MappedContainer, Allocator>
flat_multimap(sorted_equivalent_t, KeyContainer, MappedContainer, Compare, Allocator)
    -> flat_multimap<typename KeyContainer::value_type, typename MappedContainer::value_type, Compare, KeyContainer,
                     MappedContainer>;
template <class InputIterator, class Compare = less<ycxx::detail::iter_key_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::deducible_compare<Compare>
flat_multimap(InputIterator, InputIterator, Compare = Compare())
    -> flat_multimap<ycxx::detail::iter_key_type<InputIterator>, ycxx::detail::iter_mapped_type<InputIterator>,
                     Compare>;
template <class InputIterator, class Compare = less<ycxx::detail::iter_key_type<InputIterator>>>
  requires ycxx::detail::qualifies_as_input_iterator<InputIterator> && ycxx::detail::deducible_compare<Compare>
flat_multimap(sorted_equivalent_t, InputIterator, InputIterator, Compare = Compare())
    -> flat_multimap<ycxx::detail::iter_key_type<InputIterator>, ycxx::detail::iter_mapped_type<InputIterator>,
                     Compare>;
template <ranges::input_range R, class Compare = less<ycxx::detail::range_key_type<R>>,
          class Allocator = allocator<byte>>
  requires ycxx::detail::deducible_compare<Compare> && ycxx::detail::qualifies_as_allocator<Allocator>
flat_multimap(from_range_t, R&&, Compare = Compare(), Allocator = Allocator())
    -> flat_multimap<ycxx::detail::range_key_type<R>, ycxx::detail::range_mapped_type<R>, Compare,
                     vector<ycxx::detail::range_key_type<R>,
                            ycxx::detail::rebound_alloc<Allocator, ycxx::detail::range_key_type<R>>>,
                     vector<ycxx::detail::range_mapped_type<R>,
                            ycxx::detail::rebound_alloc<Allocator, ycxx::detail::range_mapped_type<R>>>>;
template <ranges::input_range R, class Allocator>
  requires ycxx::detail::qualifies_as_allocator<Allocator>
flat_multimap(from_range_t, R&&, Allocator)
    -> flat_multimap<ycxx::detail::range_key_type<R>, ycxx::detail::range_mapped_type<R>,
                     less<ycxx::detail::range_key_type<R>>,
                     vector<ycxx::detail::range_key_type<R>,
                            ycxx::detail::rebound_alloc<Allocator, ycxx::detail::range_key_type<R>>>,
                     vector<ycxx::detail::range_mapped_type<R>,
                            ycxx::detail::rebound_alloc<Allocator, ycxx::detail::range_mapped_type<R>>>>;
template <class Key, class T, class Compare = less<Key>>
  requires ycxx::detail::deducible_compare<Compare>
flat_multimap(initializer_list<pair<Key, T>>, Compare = Compare()) -> flat_multimap<Key, T, Compare>;
template <class Key, class T, class Compare = less<Key>>
  requires ycxx::detail::deducible_compare<Compare>
flat_multimap(sorted_equivalent_t, initializer_list<pair<Key, T>>, Compare = Compare())
    -> flat_multimap<Key, T, Compare>;

// ---- uses_allocator ----
template <class Key, class T, class Compare, class KeyContainer, class MappedContainer, class Allocator>
struct uses_allocator<flat_map<Key, T, Compare, KeyContainer, MappedContainer>, Allocator>
    : bool_constant<uses_allocator_v<KeyContainer, Allocator> && uses_allocator_v<MappedContainer, Allocator>> {};
template <class Key, class T, class Compare, class KeyContainer, class MappedContainer, class Allocator>
struct uses_allocator<flat_multimap<Key, T, Compare, KeyContainer, MappedContainer>, Allocator>
    : bool_constant<uses_allocator_v<KeyContainer, Allocator> && uses_allocator_v<MappedContainer, Allocator>> {};

// ---- [flat.map.erasure], [flat.multimap.erasure] ----
template <class Key, class T, class Compare, class KeyContainer, class MappedContainer, class Predicate>
constexpr typename flat_map<Key, T, Compare, KeyContainer, MappedContainer>::size_type
erase_if(flat_map<Key, T, Compare, KeyContainer, MappedContainer>& c, Predicate pred) {
  // The containers are taken out (c is empty if pred throws) and put back.
  auto cs = static_cast<flat_map<Key, T, Compare, KeyContainer, MappedContainer>&&>(c).extract();
  auto test = [&](size_t i) -> bool {
    return static_cast<bool>(pred(pair<const Key&, const T&>(ycxx::detail::row_at(cs.keys, i),
                                                             ycxx::detail::row_at(cs.values, i))));
  };
  const auto n = ycxx::detail::erase_rows_if(test, cs.keys, cs.values);
  c.replace(static_cast<KeyContainer&&>(cs.keys), static_cast<MappedContainer&&>(cs.values));
  return n;
}
template <class Key, class T, class Compare, class KeyContainer, class MappedContainer, class Predicate>
constexpr typename flat_multimap<Key, T, Compare, KeyContainer, MappedContainer>::size_type
erase_if(flat_multimap<Key, T, Compare, KeyContainer, MappedContainer>& c, Predicate pred) {
  auto cs = static_cast<flat_multimap<Key, T, Compare, KeyContainer, MappedContainer>&&>(c).extract();
  auto test = [&](size_t i) -> bool {
    return static_cast<bool>(pred(pair<const Key&, const T&>(ycxx::detail::row_at(cs.keys, i),
                                                             ycxx::detail::row_at(cs.values, i))));
  };
  const auto n = ycxx::detail::erase_rows_if(test, cs.keys, cs.values);
  c.replace(static_cast<KeyContainer&&>(cs.keys), static_cast<MappedContainer&&>(cs.values));
  return n;
}

} // namespace std
