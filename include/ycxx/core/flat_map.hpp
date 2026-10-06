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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

template <class _KC, class _MC, bool _Const>
class __flat_map_iter {
  using _KIt = typename _KC::const_iterator;
  using _MIt = std::conditional_t<_Const, typename _MC::const_iterator, typename _MC::iterator>;
  using _Kp = typename _KC::value_type;
  using _Tp = typename _MC::value_type;

  _KIt __k_{};
  _MIt __m_{};

  template <class, class, bool>
  friend class __flat_map_iter;
  template <class, class, class, class, class, bool>
  friend class __flat_map_base;

  constexpr __flat_map_iter(_KIt k, _MIt m) noexcept : __k_(k), __m_(m) {}

public:
  using iterator_concept = std::random_access_iterator_tag;
  // The reference is a proxy, but the Cpp17 algorithms (std::prev, std::distance, ...) should
  // still treat these iterators as random access.
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::pair<_Kp, _Tp>;
  using difference_type = std::ptrdiff_t;
  using reference = std::pair<const _Kp&, std::conditional_t<_Const, const _Tp&, _Tp&>>;

  // The result of operator->: holds the proxy reference. Also the iterator's pointer type, so
  // that reverse_iterator::operator-> works.
  struct __arrow {
    reference r;
    constexpr const reference* operator->() const noexcept { return __builtin_addressof(r); }
  };
  using pointer = __arrow;

  constexpr __flat_map_iter() = default;
  template <bool _C2>
    requires(_Const && !_C2)
  constexpr __flat_map_iter(const __flat_map_iter<_KC, _MC, _C2>& __o) noexcept : __k_(__o.__k_), __m_(__o.__m_) {}

  constexpr reference operator*() const noexcept { return reference(*__k_, *__m_); }
  constexpr __arrow operator->() const noexcept { return __arrow{**this}; }
  constexpr reference operator[](difference_type n) const noexcept { return *(*this + n); }

  constexpr __flat_map_iter& operator++() noexcept {
    ++__k_;
    ++__m_;
    return *this;
  }
  constexpr __flat_map_iter operator++(int) noexcept {
    __flat_map_iter t = *this;
    ++*this;
    return t;
  }
  constexpr __flat_map_iter& operator--() noexcept {
    --__k_;
    --__m_;
    return *this;
  }
  constexpr __flat_map_iter operator--(int) noexcept {
    __flat_map_iter t = *this;
    --*this;
    return t;
  }
  constexpr __flat_map_iter& operator+=(difference_type n) noexcept {
    __k_ += static_cast<typename _KC::difference_type>(n);
    __m_ += static_cast<typename _MC::difference_type>(n);
    return *this;
  }
  constexpr __flat_map_iter& operator-=(difference_type n) noexcept {
    __k_ -= static_cast<typename _KC::difference_type>(n);
    __m_ -= static_cast<typename _MC::difference_type>(n);
    return *this;
  }
  friend constexpr __flat_map_iter operator+(__flat_map_iter i, difference_type n) noexcept { return i += n; }
  friend constexpr __flat_map_iter operator+(difference_type n, __flat_map_iter i) noexcept { return i += n; }
  friend constexpr __flat_map_iter operator-(__flat_map_iter i, difference_type n) noexcept { return i -= n; }
  friend constexpr difference_type operator-(const __flat_map_iter& a, const __flat_map_iter& b) noexcept {
    return static_cast<difference_type>(a.__k_ - b.__k_);
  }
  friend constexpr bool operator==(const __flat_map_iter& a, const __flat_map_iter& b) noexcept { return a.__k_ == b.__k_; }
  friend constexpr std::strong_ordering operator<=>(const __flat_map_iter& a, const __flat_map_iter& b) noexcept {
    return static_cast<difference_type>(a.__k_ - b.__k_) <=> 0;
  }
};

// The members flat_map (Multi false) and flat_multimap share.
template <class _Key, class _Tp, class _Compare, class _KC, class _MC, bool _Multi>
class __flat_map_base {
  static_assert(std::is_same_v<_Key, typename _KC::value_type>,
                "flat_map: Key must be KeyContainer::value_type ([flat.map.overview]/8)");
  static_assert(std::is_same_v<_Tp, typename _MC::value_type>,
                "flat_map: T must be MappedContainer::value_type ([flat.map.overview]/8)");

public:
  // ---- types ----
  using key_type = _Key;
  using mapped_type = _Tp;
  using value_type = std::pair<key_type, mapped_type>;
  using key_compare = _Compare;
  using reference = std::pair<const key_type&, mapped_type&>;
  using const_reference = std::pair<const key_type&, const mapped_type&>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using iterator = __flat_map_iter<_KC, _MC, false>;
  using const_iterator = __flat_map_iter<_KC, _MC, true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using __key_container_type = _KC;
  using __mapped_container_type = _MC;

  class value_compare {
    friend class __flat_map_base;
    key_compare comp;
    constexpr value_compare(key_compare c) : comp(c) {}

  public:
    constexpr bool operator()(const_reference __x, const_reference y) const { return comp(__x.first, y.first); }
  };

  struct containers {
    __key_container_type keys;
    __mapped_container_type values;
  };

protected:
  containers __c_;
  [[no_unique_address]] key_compare __compare_;

  // ---- helpers ----
  template <class _Ap, class _Bp>
  constexpr bool __lt(const _Ap& a, const _Bp& b) const {
    return static_cast<bool>(static_cast<const key_compare&>(__compare_)(a, b));
  }
  constexpr auto __less_fn() const noexcept {
    return [this](const auto& a, const auto& b) -> bool { return __lt(a, b); };
  }
  constexpr const key_type& __key_at(size_type i) const { return ::__ycxx::__detail::__row_at(__c_.keys, i); }
  constexpr iterator __it_at(size_type i) noexcept {
    return iterator(__c_.keys.cbegin() + static_cast<typename _KC::difference_type>(i),
                    __c_.values.begin() + static_cast<typename _MC::difference_type>(i));
  }
  constexpr const_iterator __it_at(size_type i) const noexcept {
    return const_iterator(__c_.keys.cbegin() + static_cast<typename _KC::difference_type>(i),
                          __c_.values.cbegin() + static_cast<typename _MC::difference_type>(i));
  }
  constexpr size_type __index_of(const_iterator __it) const noexcept {
    return static_cast<size_type>(__it.__k_ - __c_.keys.cbegin());
  }
  constexpr auto __guard() noexcept { return ::__ycxx::__detail::__flat_guard(__c_.keys, __c_.values); }

  // The first index whose key is not less than x, the first whose key is greater than x.
  template <class _Kp>
  constexpr size_type __lower_index(const _Kp& __x) const {
    return __lower_index(__x, 0, size());
  }
  template <class _Kp>
  constexpr size_type __lower_index(const _Kp& __x, size_type __lo, size_type __hi) const {
    size_type n = __hi - __lo;
    while (n > 0) {
      const size_type __half = n / 2;
      if (__lt(__key_at(__lo + __half), __x)) {
        __lo += __half + 1;
        n -= __half + 1;
      } else {
        n = __half;
      }
    }
    return __lo;
  }
  template <class _Kp>
  constexpr size_type __upper_index(const _Kp& __x) const {
    return __upper_index(__x, 0, size());
  }
  template <class _Kp>
  constexpr size_type __upper_index(const _Kp& __x, size_type __lo, size_type __hi) const {
    size_type n = __hi - __lo;
    while (n > 0) {
      const size_type __half = n / 2;
      if (!__lt(__x, __key_at(__lo + __half))) {
        __lo += __half + 1;
        n -= __half + 1;
      } else {
        n = __half;
      }
    }
    return __lo;
  }
  template <class _Kp>
  constexpr size_type __find_index(const _Kp& __x) const {
    const size_type i = __lower_index(__x);
    if (i == size() || __lt(__x, __key_at(i)))
      return size();
    return i;
  }
  // Unique keys: the index of the element with key x, or where x belongs (found false).
  template <class _Kp>
  constexpr std::pair<size_type, bool> __unique_pos(const _Kp& __x) const {
    const size_type i = __lower_index(__x);
    return {i, i != size() && !__lt(__x, __key_at(i))};
  }
  // Unique keys with a hint: constant comparisons when x belongs just before hint.
  template <class _Kp>
  constexpr std::pair<size_type, bool> __unique_pos_hint(const_iterator __hint, const _Kp& __x) const {
    const size_type h = __index_of(__hint);
    if ((h == 0 || __lt(__key_at(h - 1), __x)) && (h == size() || __lt(__x, __key_at(h))))
      return {h, false};
    return __unique_pos(__x);
  }
  // Equivalent keys with a hint: as close as possible to the position just before hint.
  template <class _Kp>
  constexpr size_type __multi_pos_hint(const_iterator __hint, const _Kp& __x) const {
    const size_type h = __index_of(__hint);
    if (h > 0 && __lt(__x, __key_at(h - 1)))
      return __upper_index(__x, 0, h - 1);
    if (h < size() && __lt(__key_at(h), __x))
      return __lower_index(__x, h + 1, size());
    return h;
  }

  // Inserts the row (key, mapped value built from margs) at index i.
  template <class _Kp, class... _MArgs>
  constexpr iterator __insert_row(size_type i, _Kp&& k, _MArgs&&... __margs) {
    auto __g = __guard();
    __c_.keys.emplace(__c_.keys.cbegin() + static_cast<typename _KC::difference_type>(i), static_cast<_Kp&&>(k));
    __c_.values.emplace(__c_.values.cbegin() + static_cast<typename _MC::difference_type>(i),
                      static_cast<_MArgs&&>(__margs)...);
    __g.release();
    return __it_at(i);
  }
  constexpr std::pair<iterator, bool> __insert_unique(value_type&& t) {
    const auto p = __unique_pos(t.first);
    if (p.second)
      return {__it_at(p.first), false};
    return {__insert_row(p.first, static_cast<key_type&&>(t.first), static_cast<mapped_type&&>(t.second)), true};
  }
  constexpr iterator __insert_unique_hint(const_iterator __hint, value_type&& t) {
    const auto p = __unique_pos_hint(__hint, t.first);
    if (p.second)
      return __it_at(p.first);
    return __insert_row(p.first, static_cast<key_type&&>(t.first), static_cast<mapped_type&&>(t.second));
  }
  constexpr iterator __insert_multi(value_type&& t) {
    const size_type i = __upper_index(t.first);
    return __insert_row(i, static_cast<key_type&&>(t.first), static_cast<mapped_type&&>(t.second));
  }
  constexpr iterator __insert_multi_hint(const_iterator __hint, value_type&& t) {
    const size_type i = __multi_pos_hint(__hint, t.first);
    return __insert_row(i, static_cast<key_type&&>(t.first), static_cast<mapped_type&&>(t.second));
  }

  // Sorts the rows from `from` on into the sorted rows before them; with unique keys, then keeps
  // the first of each run of equivalent keys. g restores the invariants on an exception.
  constexpr void __sort_from(size_type from) {
    auto less = __less_fn();
    ::__ycxx::__detail::__sort_rows(less, from, __c_.keys, __c_.values);
    if constexpr (!_Multi)
      ::__ycxx::__detail::__unique_rows(less, __c_.keys, __c_.values);
  }
  // [flat.map.cons]/1: the containers given to a constructor.
  constexpr void __sort_all() {
    ::__ycxx::__detail::__precondition(__c_.keys.size() == __c_.values.size(),
                                 "flat_map: key and mapped containers of different sizes");
    auto __g = __guard();
    __sort_from(0);
    __g.release();
  }
  // [flat.map.modifiers]/6, /11: appends each element, then sorts the new rows in.
  template <class _It, class _Sent>
  constexpr void __insert_elems(_It first, _Sent last) {
    const size_type __old = size();
    auto __g = __guard();
    for (; first != last; ++first) {
      value_type value = *first;
      __c_.keys.insert(__c_.keys.end(), static_cast<key_type&&>(value.first));
      __c_.values.insert(__c_.values.end(), static_cast<mapped_type&&>(value.second));
    }
    __sort_from(__old);
    __g.release();
  }
  template <class _Rp>
  constexpr void __insert_range_elems(_Rp&& __rg) {
    __insert_elems(std::ranges::begin(__rg), std::ranges::end(__rg));
  }

  // Moves the containers out of o, which is emptied, even if a move throws.
  static constexpr containers take(__flat_map_base& __o) {
    auto __g = __o.__guard();
    return containers{static_cast<_KC&&>(__o.__c_.keys), static_cast<_MC&&>(__o.__c_.values)};
  }
  template <class _Ap>
  static constexpr containers take(__flat_map_base& __o, const _Ap& a) {
    auto __g = __o.__guard();
    return containers{std::make_obj_using_allocator<_KC>(a, static_cast<_KC&&>(__o.__c_.keys)),
                      std::make_obj_using_allocator<_MC>(a, static_cast<_MC&&>(__o.__c_.values))};
  }

  // ---- construction and assignment ----
  constexpr explicit __flat_map_base(const key_compare& comp) : __c_(), __compare_(comp) {}
  template <class _Ap>
  constexpr __flat_map_base(const key_compare& comp, const _Ap& a)
      : __c_{std::make_obj_using_allocator<_KC>(a), std::make_obj_using_allocator<_MC>(a)}, __compare_(comp) {}
  constexpr __flat_map_base(_KC&& k, _MC&& m, const key_compare& comp)
      : __c_{static_cast<_KC&&>(k), static_cast<_MC&&>(m)}, __compare_(comp) {}
  template <class _Ap>
  constexpr __flat_map_base(const _KC& k, const _MC& m, const key_compare& comp, const _Ap& a)
      : __c_{std::make_obj_using_allocator<_KC>(a, k), std::make_obj_using_allocator<_MC>(a, m)}, __compare_(comp) {}
  constexpr __flat_map_base(const __flat_map_base&) = default;
  constexpr __flat_map_base(__flat_map_base&& __o) : __c_(take(__o)), __compare_(__o.__compare_) {}
  template <class _Ap>
  constexpr __flat_map_base(const __flat_map_base& __o, const _Ap& a)
      : __c_{std::make_obj_using_allocator<_KC>(a, __o.__c_.keys), std::make_obj_using_allocator<_MC>(a, __o.__c_.values)},
        __compare_(__o.__compare_) {}
  template <class _Ap>
  constexpr __flat_map_base(__flat_map_base&& __o, const _Ap& a) : __c_(take(__o, a)), __compare_(__o.__compare_) {}

  constexpr void __copy_assign(const __flat_map_base& __o) {
    if (this == __builtin_addressof(__o))
      return;
    auto __g = __guard();
    __c_.keys = __o.__c_.keys;
    __c_.values = __o.__c_.values;
    __compare_ = __o.__compare_;
    __g.release();
  }
  constexpr void __move_assign(__flat_map_base& __o) {
    if (this == __builtin_addressof(__o))
      return;
    auto __go = __o.__guard(); // o is emptied in any case
    auto __g = __guard();
    __c_.keys = static_cast<_KC&&>(__o.__c_.keys);
    __c_.values = static_cast<_MC&&>(__o.__c_.values);
    __compare_ = static_cast<key_compare&&>(__o.__compare_);
    __g.release();
  }
  constexpr void __swap_impl(__flat_map_base& y) {
    auto __g = __guard();
    auto __gy = y.__guard();
    ::__ycxx::__detail::__swap_adl::__do_swap(__compare_, y.__compare_);
    ::__ycxx::__detail::__swap_adl::__do_swap(__c_.keys, y.__c_.keys);
    ::__ycxx::__detail::__swap_adl::__do_swap(__c_.values, y.__c_.values);
    __gy.release();
    __g.release();
  }

public:
  // ---- iterators ----
  constexpr iterator begin() noexcept { return iterator(__c_.keys.cbegin(), __c_.values.begin()); }
  constexpr const_iterator begin() const noexcept { return const_iterator(__c_.keys.cbegin(), __c_.values.cbegin()); }
  constexpr iterator end() noexcept { return iterator(__c_.keys.cend(), __c_.values.end()); }
  constexpr const_iterator end() const noexcept { return const_iterator(__c_.keys.cend(), __c_.values.cend()); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [flat.map.capacity] ----
  [[nodiscard]] constexpr bool empty() const noexcept { return __c_.keys.empty(); }
  constexpr size_type size() const noexcept { return __c_.keys.size(); }
  constexpr size_type max_size() const noexcept {
    const auto k = static_cast<size_type>(__c_.keys.max_size());
    const auto m = static_cast<size_type>(__c_.values.max_size());
    return k < m ? k : m;
  }

  // ---- modifiers ----
  constexpr containers extract() && {
    auto __g = __guard();
    return containers{static_cast<_KC&&>(__c_.keys), static_cast<_MC&&>(__c_.values)};
  }
  constexpr void replace(__key_container_type&& __key_cont, __mapped_container_type&& __mapped_cont) {
    ::__ycxx::__detail::__precondition(__key_cont.size() == __mapped_cont.size(),
                                 "flat_map::replace: key and mapped containers of different sizes");
    auto __g = __guard();
    __c_.keys = static_cast<_KC&&>(__key_cont);
    __c_.values = static_cast<_MC&&>(__mapped_cont);
    __g.release();
  }

  constexpr iterator erase(iterator position) { return erase(const_iterator(position)); }
  constexpr iterator erase(const_iterator position) {
    const size_type i = __index_of(position);
    ::__ycxx::__detail::__precondition(i < size(), "flat_map::erase: end() iterator");
    auto __g = __guard();
    __c_.keys.erase(__c_.keys.cbegin() + static_cast<typename _KC::difference_type>(i));
    __c_.values.erase(__c_.values.cbegin() + static_cast<typename _MC::difference_type>(i));
    __g.release();
    return __it_at(i);
  }
  constexpr size_type erase(const key_type& __x) { return __erase_key(__x); }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_non_iter<_Compare, _Kp, iterator, const_iterator>
  constexpr size_type erase(_Kp&& __x) {
    return __erase_key(__x);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    const size_type i = __index_of(first), __j = __index_of(last);
    if (i != __j) {
      auto __g = __guard();
      __c_.keys.erase(__c_.keys.cbegin() + static_cast<typename _KC::difference_type>(i),
                    __c_.keys.cbegin() + static_cast<typename _KC::difference_type>(__j));
      __c_.values.erase(__c_.values.cbegin() + static_cast<typename _MC::difference_type>(i),
                      __c_.values.cbegin() + static_cast<typename _MC::difference_type>(__j));
      __g.release();
    }
    return __it_at(i);
  }
  constexpr void clear() noexcept {
    __c_.keys.clear();
    __c_.values.clear();
  }

  // ---- observers ----
  constexpr key_compare key_comp() const { return __compare_; }
  constexpr value_compare value_comp() const { return value_compare(__compare_); }
  constexpr const __key_container_type& keys() const noexcept { return __c_.keys; }
  constexpr const __mapped_container_type& values() const noexcept { return __c_.values; }

  // ---- map operations ----
  constexpr iterator find(const key_type& __x) { return __it_at(__find_index(__x)); }
  constexpr const_iterator find(const key_type& __x) const { return __it_at(__find_index(__x)); }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr iterator find(const _Kp& __x) {
    return __it_at(__find_index(__x));
  }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr const_iterator find(const _Kp& __x) const {
    return __it_at(__find_index(__x));
  }
  constexpr size_type count(const key_type& __x) const {
    if constexpr (_Multi)
      return __upper_index(__x) - __lower_index(__x);
    else
      return __find_index(__x) == size() ? 0 : 1;
  }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr size_type count(const _Kp& __x) const {
    return __upper_index(__x) - __lower_index(__x);
  }
  constexpr bool contains(const key_type& __x) const { return __find_index(__x) != size(); }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr bool contains(const _Kp& __x) const {
    return __find_index(__x) != size();
  }
  constexpr iterator lower_bound(const key_type& __x) { return __it_at(__lower_index(__x)); }
  constexpr const_iterator lower_bound(const key_type& __x) const { return __it_at(__lower_index(__x)); }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr iterator lower_bound(const _Kp& __x) {
    return __it_at(__lower_index(__x));
  }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr const_iterator lower_bound(const _Kp& __x) const {
    return __it_at(__lower_index(__x));
  }
  constexpr iterator upper_bound(const key_type& __x) { return __it_at(__upper_index(__x)); }
  constexpr const_iterator upper_bound(const key_type& __x) const { return __it_at(__upper_index(__x)); }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr iterator upper_bound(const _Kp& __x) {
    return __it_at(__upper_index(__x));
  }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr const_iterator upper_bound(const _Kp& __x) const {
    return __it_at(__upper_index(__x));
  }
  constexpr std::pair<iterator, iterator> equal_range(const key_type& __x) {
    return {__it_at(__lower_index(__x)), __it_at(__upper_index(__x))};
  }
  constexpr std::pair<const_iterator, const_iterator> equal_range(const key_type& __x) const {
    return {__it_at(__lower_index(__x)), __it_at(__upper_index(__x))};
  }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr std::pair<iterator, iterator> equal_range(const _Kp& __x) {
    return {__it_at(__lower_index(__x)), __it_at(__upper_index(__x))};
  }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr std::pair<const_iterator, const_iterator> equal_range(const _Kp& __x) const {
    return {__it_at(__lower_index(__x)), __it_at(__upper_index(__x))};
  }

private:
  template <class _Kp>
  constexpr size_type __erase_key(const _Kp& __x) {
    const size_type i = __lower_index(__x), __j = __upper_index(__x);
    erase(__it_at(i), __it_at(__j));
    return __j - i;
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Key, class _Tp, class _Compare = less<_Key>, class _KeyContainer = vector<_Key>,
          class _MappedContainer = vector<_Tp>>
class flat_map
    : public __ycxx::__adl_free::__flat_map_base<_Key, _Tp, _Compare, _KeyContainer, _MappedContainer, false> {
  using base = __ycxx::__adl_free::__flat_map_base<_Key, _Tp, _Compare, _KeyContainer, _MappedContainer, false>;

public:
  // ---- types ----
  using typename base::const_iterator;
  using typename base::const_reference;
  using typename base::const_reverse_iterator;
  using typename base::containers;
  using typename base::difference_type;
  using typename base::iterator;
  using typename base::key_compare;
  using typename base::__key_container_type;
  using typename base::key_type;
  using typename base::__mapped_container_type;
  using typename base::mapped_type;
  using typename base::reference;
  using typename base::reverse_iterator;
  using typename base::size_type;
  using typename base::value_compare;
  using typename base::value_type;

  // ---- [flat.map.cons] ----
  constexpr flat_map() : flat_map(key_compare()) {}
  constexpr flat_map(const flat_map&) = default;
  constexpr flat_map(flat_map&& __x) : base(static_cast<base&&>(__x)) {}
  constexpr flat_map& operator=(const flat_map& __x) {
    this->__copy_assign(__x);
    return *this;
  }
  constexpr flat_map& operator=(flat_map&& __x) {
    this->__move_assign(__x);
    return *this;
  }
  constexpr explicit flat_map(const key_compare& comp) : base(comp) {}
  constexpr flat_map(__key_container_type __key_cont, __mapped_container_type __mapped_cont,
                     const key_compare& comp = key_compare())
      : base(static_cast<__key_container_type&&>(__key_cont), static_cast<__mapped_container_type&&>(__mapped_cont), comp) {
    this->__sort_all();
  }
  constexpr flat_map(sorted_unique_t, __key_container_type __key_cont, __mapped_container_type __mapped_cont,
                     const key_compare& comp = key_compare())
      : base(static_cast<__key_container_type&&>(__key_cont), static_cast<__mapped_container_type&&>(__mapped_cont), comp) {
    __ycxx::__detail::__precondition(this->__c_.keys.size() == this->__c_.values.size(),
                               "flat_map: key and mapped containers of different sizes");
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr flat_map(_InputIterator first, _InputIterator last, const key_compare& comp = key_compare()) : base(comp) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr flat_map(sorted_unique_t, _InputIterator first, _InputIterator last, const key_compare& comp = key_compare())
      : base(comp) {
    this->__insert_elems(first, last);
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr flat_map(_Tag, _Rp&& __rg) : flat_map(from_range, static_cast<_Rp&&>(__rg), key_compare()) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr flat_map(_Tag, _Rp&& __rg, const key_compare& comp) : base(comp) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  constexpr flat_map(initializer_list<value_type> il, const key_compare& comp = key_compare())
      : flat_map(il.begin(), il.end(), comp) {}
  constexpr flat_map(sorted_unique_t, initializer_list<value_type> il, const key_compare& comp = key_compare())
      : flat_map(sorted_unique, il.begin(), il.end(), comp) {}

  // ---- [flat.map.cons.alloc] ----
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr explicit flat_map(const _Alloc& a) : base(key_compare(), a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(const key_compare& comp, const _Alloc& a) : base(comp, a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(const __key_container_type& __key_cont, const __mapped_container_type& __mapped_cont, const _Alloc& a)
      : base(__key_cont, __mapped_cont, key_compare(), a) {
    this->__sort_all();
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(const __key_container_type& __key_cont, const __mapped_container_type& __mapped_cont,
                     const key_compare& comp, const _Alloc& a)
      : base(__key_cont, __mapped_cont, comp, a) {
    this->__sort_all();
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(sorted_unique_t, const __key_container_type& __key_cont, const __mapped_container_type& __mapped_cont,
                     const _Alloc& a)
      : base(__key_cont, __mapped_cont, key_compare(), a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(sorted_unique_t, const __key_container_type& __key_cont, const __mapped_container_type& __mapped_cont,
                     const key_compare& comp, const _Alloc& a)
      : base(__key_cont, __mapped_cont, comp, a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(const flat_map& __x, const _Alloc& a) : base(__x, a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(flat_map&& __x, const _Alloc& a) : base(static_cast<base&&>(__x), a) {}
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(_InputIterator first, _InputIterator last, const _Alloc& a) : base(key_compare(), a) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(_InputIterator first, _InputIterator last, const key_compare& comp, const _Alloc& a)
      : base(comp, a) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(sorted_unique_t, _InputIterator first, _InputIterator last, const _Alloc& a)
      : base(key_compare(), a) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(sorted_unique_t, _InputIterator first, _InputIterator last, const key_compare& comp,
                     const _Alloc& a)
      : base(comp, a) {
    this->__insert_elems(first, last);
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp, class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(_Tag, _Rp&& __rg, const _Alloc& a) : base(key_compare(), a) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp, class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(_Tag, _Rp&& __rg, const key_compare& comp, const _Alloc& a) : base(comp, a) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(initializer_list<value_type> il, const _Alloc& a) : base(key_compare(), a) {
    this->__insert_elems(il.begin(), il.end());
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(initializer_list<value_type> il, const key_compare& comp, const _Alloc& a) : base(comp, a) {
    this->__insert_elems(il.begin(), il.end());
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(sorted_unique_t, initializer_list<value_type> il, const _Alloc& a) : base(key_compare(), a) {
    this->__insert_elems(il.begin(), il.end());
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_map(sorted_unique_t, initializer_list<value_type> il, const key_compare& comp, const _Alloc& a)
      : base(comp, a) {
    this->__insert_elems(il.begin(), il.end());
  }

  constexpr flat_map& operator=(initializer_list<value_type> il) {
    this->clear();
    this->__insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- [flat.map.access] ----
  // The constraints of the try_emplace call each operator[] is equivalent to are checked up front.
  constexpr mapped_type& operator[](const key_type& __x)
    requires is_constructible_v<mapped_type>
  {
    return try_emplace(__x).first->second;
  }
  constexpr mapped_type& operator[](key_type&& __x)
    requires is_constructible_v<mapped_type>
  {
    return try_emplace(static_cast<key_type&&>(__x)).first->second;
  }
  template <class _Kp>
    requires __ycxx::__detail::__transparent_non_iter<_Compare, _Kp, iterator, const_iterator> &&
             is_constructible_v<key_type, _Kp> && is_constructible_v<mapped_type>
  constexpr mapped_type& operator[](_Kp&& __x) {
    return try_emplace(static_cast<_Kp&&>(__x)).first->second;
  }
  constexpr mapped_type& at(const key_type& __x) { return __at_impl(__x); }
  constexpr const mapped_type& at(const key_type& __x) const { return const_cast<flat_map*>(this)->__at_impl(__x); }
  template <class _Kp>
    requires __ycxx::__detail::__transparent_compare<_Compare>
  constexpr mapped_type& at(const _Kp& __x) {
    return __at_impl(__x);
  }
  template <class _Kp>
    requires __ycxx::__detail::__transparent_compare<_Compare>
  constexpr const mapped_type& at(const _Kp& __x) const {
    return const_cast<flat_map*>(this)->__at_impl(__x);
  }
  constexpr optional<mapped_type&> lookup(const key_type& __x) { return __lookup_impl<mapped_type&>(__x); }
  constexpr optional<const mapped_type&> lookup(const key_type& __x) const {
    return const_cast<flat_map*>(this)->template __lookup_impl<const mapped_type&>(__x);
  }
  template <class _Kp>
    requires __ycxx::__detail::__transparent_compare<_Compare>
  constexpr optional<mapped_type&> lookup(const _Kp& __x) {
    return __lookup_impl<mapped_type&>(__x);
  }
  template <class _Kp>
    requires __ycxx::__detail::__transparent_compare<_Compare>
  constexpr optional<const mapped_type&> lookup(const _Kp& __x) const {
    return const_cast<flat_map*>(this)->template __lookup_impl<const mapped_type&>(__x);
  }

  // ---- [flat.map.modifiers] ----
  template <class... _Args>
    requires is_constructible_v<pair<key_type, mapped_type>, _Args...>
  constexpr pair<iterator, bool> emplace(_Args&&... __args) {
    return this->__insert_unique(value_type(static_cast<_Args&&>(__args)...));
  }
  template <class... _Args>
    requires is_constructible_v<pair<key_type, mapped_type>, _Args...>
  constexpr iterator emplace_hint(const_iterator position, _Args&&... __args) {
    return this->__insert_unique_hint(position, value_type(static_cast<_Args&&>(__args)...));
  }
  constexpr pair<iterator, bool> insert(const value_type& __x) { return emplace(__x); }
  constexpr pair<iterator, bool> insert(value_type&& __x) { return emplace(static_cast<value_type&&>(__x)); }
  constexpr iterator insert(const_iterator position, const value_type& __x) { return emplace_hint(position, __x); }
  constexpr iterator insert(const_iterator position, value_type&& __x) {
    return emplace_hint(position, static_cast<value_type&&>(__x));
  }
  template <class _Pp>
    requires is_constructible_v<pair<key_type, mapped_type>, _Pp>
  constexpr pair<iterator, bool> insert(_Pp&& __x) {
    return emplace(static_cast<_Pp&&>(__x));
  }
  template <class _Pp>
    requires is_constructible_v<pair<key_type, mapped_type>, _Pp>
  constexpr iterator insert(const_iterator position, _Pp&& __x) {
    return emplace_hint(position, static_cast<_Pp&&>(__x));
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr void insert(_InputIterator first, _InputIterator last) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr void insert(sorted_unique_t, _InputIterator first, _InputIterator last) {
    this->__insert_elems(first, last);
  }
  template <__ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr void insert_range(_Rp&& __rg) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  template <__ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr void insert_range(sorted_unique_t, _Rp&& __rg) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  constexpr void insert(initializer_list<value_type> il) { insert(il.begin(), il.end()); }
  constexpr void insert(sorted_unique_t, initializer_list<value_type> il) {
    insert(sorted_unique, il.begin(), il.end());
  }

  template <class... _Args>
    requires is_constructible_v<mapped_type, _Args...>
  constexpr pair<iterator, bool> try_emplace(const key_type& k, _Args&&... __args) {
    return __try_emplace_impl(this->__unique_pos(k), k, static_cast<_Args&&>(__args)...);
  }
  template <class... _Args>
    requires is_constructible_v<mapped_type, _Args...>
  constexpr pair<iterator, bool> try_emplace(key_type&& k, _Args&&... __args) {
    return __try_emplace_impl(this->__unique_pos(k), static_cast<key_type&&>(k), static_cast<_Args&&>(__args)...);
  }
  template <class _Kp, class... _Args>
    requires __ycxx::__detail::__transparent_non_iter<_Compare, _Kp, iterator, const_iterator> &&
             is_constructible_v<key_type, _Kp> && is_constructible_v<mapped_type, _Args...>
  constexpr pair<iterator, bool> try_emplace(_Kp&& k, _Args&&... __args) {
    return __try_emplace_impl(this->__unique_pos(k), static_cast<_Kp&&>(k), static_cast<_Args&&>(__args)...);
  }
  template <class... _Args>
    requires is_constructible_v<mapped_type, _Args...>
  constexpr iterator try_emplace(const_iterator __hint, const key_type& k, _Args&&... __args) {
    return __try_emplace_impl(this->__unique_pos_hint(__hint, k), k, static_cast<_Args&&>(__args)...).first;
  }
  template <class... _Args>
    requires is_constructible_v<mapped_type, _Args...>
  constexpr iterator try_emplace(const_iterator __hint, key_type&& k, _Args&&... __args) {
    return __try_emplace_impl(this->__unique_pos_hint(__hint, k), static_cast<key_type&&>(k), static_cast<_Args&&>(__args)...)
        .first;
  }
  template <class _Kp, class... _Args>
    requires __ycxx::__detail::__transparent_compare<_Compare> && is_constructible_v<key_type, _Kp> &&
             is_constructible_v<mapped_type, _Args...>
  constexpr iterator try_emplace(const_iterator __hint, _Kp&& k, _Args&&... __args) {
    return __try_emplace_impl(this->__unique_pos_hint(__hint, k), static_cast<_Kp&&>(k), static_cast<_Args&&>(__args)...).first;
  }
  template <class _Mp>
    requires is_assignable_v<mapped_type&, _Mp> && is_constructible_v<mapped_type, _Mp>
  constexpr pair<iterator, bool> insert_or_assign(const key_type& k, _Mp&& __obj) {
    return __insert_or_assign_impl(this->__unique_pos(k), k, static_cast<_Mp&&>(__obj));
  }
  template <class _Mp>
    requires is_assignable_v<mapped_type&, _Mp> && is_constructible_v<mapped_type, _Mp>
  constexpr pair<iterator, bool> insert_or_assign(key_type&& k, _Mp&& __obj) {
    return __insert_or_assign_impl(this->__unique_pos(k), static_cast<key_type&&>(k), static_cast<_Mp&&>(__obj));
  }
  template <class _Kp, class _Mp>
    requires __ycxx::__detail::__transparent_compare<_Compare> && is_constructible_v<key_type, _Kp> &&
             is_assignable_v<mapped_type&, _Mp> && is_constructible_v<mapped_type, _Mp>
  constexpr pair<iterator, bool> insert_or_assign(_Kp&& k, _Mp&& __obj) {
    return __insert_or_assign_impl(this->__unique_pos(k), static_cast<_Kp&&>(k), static_cast<_Mp&&>(__obj));
  }
  template <class _Mp>
    requires is_assignable_v<mapped_type&, _Mp> && is_constructible_v<mapped_type, _Mp>
  constexpr iterator insert_or_assign(const_iterator __hint, const key_type& k, _Mp&& __obj) {
    return __insert_or_assign_impl(this->__unique_pos_hint(__hint, k), k, static_cast<_Mp&&>(__obj)).first;
  }
  template <class _Mp>
    requires is_assignable_v<mapped_type&, _Mp> && is_constructible_v<mapped_type, _Mp>
  constexpr iterator insert_or_assign(const_iterator __hint, key_type&& k, _Mp&& __obj) {
    return __insert_or_assign_impl(this->__unique_pos_hint(__hint, k), static_cast<key_type&&>(k), static_cast<_Mp&&>(__obj))
        .first;
  }
  template <class _Kp, class _Mp>
    requires __ycxx::__detail::__transparent_compare<_Compare> && is_constructible_v<key_type, _Kp> &&
             is_assignable_v<mapped_type&, _Mp> && is_constructible_v<mapped_type, _Mp>
  constexpr iterator insert_or_assign(const_iterator __hint, _Kp&& k, _Mp&& __obj) {
    return __insert_or_assign_impl(this->__unique_pos_hint(__hint, k), static_cast<_Kp&&>(k), static_cast<_Mp&&>(__obj)).first;
  }

  constexpr void swap(flat_map& y) noexcept(is_nothrow_swappable_v<__key_container_type> &&
                                            is_nothrow_swappable_v<__mapped_container_type> &&
                                            is_nothrow_swappable_v<key_compare>) {
    this->__swap_impl(y);
  }

  friend constexpr bool operator==(const flat_map& __x, const flat_map& y) {
    return __x.size() == y.size() && std::equal(__x.begin(), __x.end(), y.begin());
  }
  // A template, so that the return type is formed only when the operator is used.
  template <class _Vp = value_type>
  friend constexpr __ycxx::__detail::__synth_three_way_result<_Vp> operator<=>(const flat_map& __x, const flat_map& y) {
    return std::lexicographical_compare_three_way(__x.begin(), __x.end(), y.begin(), y.end(),
                                                  __ycxx::__detail::__synth_three_way);
  }
  friend constexpr void swap(flat_map& __x, flat_map& y) noexcept(noexcept(__x.swap(y))) { __x.swap(y); }

private:
  template <class _Kp>
  constexpr mapped_type& __at_impl(const _Kp& __x) {
    const size_type i = this->__find_index(__x);
    if (i == this->size())
      __ycxx::__detail::__throw_out_of_range("std::flat_map::at: key not found");
    return __ycxx::__detail::__row_at(this->__c_.values, i);
  }
  template <class _Rp, class _Kp>
  constexpr optional<_Rp> __lookup_impl(const _Kp& __x) {
    const size_type i = this->__find_index(__x);
    if (i == this->size())
      return nullopt;
    return optional<_Rp>(__ycxx::__detail::__row_at(this->__c_.values, i));
  }
  template <class _Kp, class... _Args>
  constexpr pair<iterator, bool> __try_emplace_impl(pair<size_type, bool> p, _Kp&& k, _Args&&... __args) {
    if (p.second)
      return {this->__it_at(p.first), false};
    return {this->__insert_row(p.first, static_cast<_Kp&&>(k), static_cast<_Args&&>(__args)...), true};
  }
  template <class _Kp, class _Mp>
  constexpr pair<iterator, bool> __insert_or_assign_impl(pair<size_type, bool> p, _Kp&& k, _Mp&& __obj) {
    if (p.second) {
      __ycxx::__detail::__row_at(this->__c_.values, p.first) = static_cast<_Mp&&>(__obj);
      return {this->__it_at(p.first), false};
    }
    return {this->__insert_row(p.first, static_cast<_Kp&&>(k), static_cast<_Mp&&>(__obj)), true};
  }
};

template <class _Key, class _Tp, class _Compare = less<_Key>, class _KeyContainer = vector<_Key>,
          class _MappedContainer = vector<_Tp>>
class flat_multimap
    : public __ycxx::__adl_free::__flat_map_base<_Key, _Tp, _Compare, _KeyContainer, _MappedContainer, true> {
  using base = __ycxx::__adl_free::__flat_map_base<_Key, _Tp, _Compare, _KeyContainer, _MappedContainer, true>;

public:
  // ---- types ----
  using typename base::const_iterator;
  using typename base::const_reference;
  using typename base::const_reverse_iterator;
  using typename base::containers;
  using typename base::difference_type;
  using typename base::iterator;
  using typename base::key_compare;
  using typename base::__key_container_type;
  using typename base::key_type;
  using typename base::__mapped_container_type;
  using typename base::mapped_type;
  using typename base::reference;
  using typename base::reverse_iterator;
  using typename base::size_type;
  using typename base::value_compare;
  using typename base::value_type;

  // ---- [flat.multimap.cons] ----
  constexpr flat_multimap() : flat_multimap(key_compare()) {}
  constexpr flat_multimap(const flat_multimap&) = default;
  constexpr flat_multimap(flat_multimap&& __x) : base(static_cast<base&&>(__x)) {}
  constexpr flat_multimap& operator=(const flat_multimap& __x) {
    this->__copy_assign(__x);
    return *this;
  }
  constexpr flat_multimap& operator=(flat_multimap&& __x) {
    this->__move_assign(__x);
    return *this;
  }
  constexpr explicit flat_multimap(const key_compare& comp) : base(comp) {}
  constexpr flat_multimap(__key_container_type __key_cont, __mapped_container_type __mapped_cont,
                          const key_compare& comp = key_compare())
      : base(static_cast<__key_container_type&&>(__key_cont), static_cast<__mapped_container_type&&>(__mapped_cont), comp) {
    this->__sort_all();
  }
  constexpr flat_multimap(sorted_equivalent_t, __key_container_type __key_cont, __mapped_container_type __mapped_cont,
                          const key_compare& comp = key_compare())
      : base(static_cast<__key_container_type&&>(__key_cont), static_cast<__mapped_container_type&&>(__mapped_cont), comp) {
    __ycxx::__detail::__precondition(this->__c_.keys.size() == this->__c_.values.size(),
                               "flat_multimap: key and mapped containers of different sizes");
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr flat_multimap(_InputIterator first, _InputIterator last, const key_compare& comp = key_compare())
      : base(comp) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr flat_multimap(sorted_equivalent_t, _InputIterator first, _InputIterator last,
                          const key_compare& comp = key_compare())
      : base(comp) {
    this->__insert_elems(first, last);
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr flat_multimap(_Tag, _Rp&& __rg) : flat_multimap(from_range, static_cast<_Rp&&>(__rg), key_compare()) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr flat_multimap(_Tag, _Rp&& __rg, const key_compare& comp) : base(comp) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  constexpr flat_multimap(initializer_list<value_type> il, const key_compare& comp = key_compare())
      : flat_multimap(il.begin(), il.end(), comp) {}
  constexpr flat_multimap(sorted_equivalent_t, initializer_list<value_type> il,
                          const key_compare& comp = key_compare())
      : flat_multimap(sorted_equivalent, il.begin(), il.end(), comp) {}

  // ---- [flat.multimap.cons.alloc] ----
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr explicit flat_multimap(const _Alloc& a) : base(key_compare(), a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(const key_compare& comp, const _Alloc& a) : base(comp, a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(const __key_container_type& __key_cont, const __mapped_container_type& __mapped_cont,
                          const _Alloc& a)
      : base(__key_cont, __mapped_cont, key_compare(), a) {
    this->__sort_all();
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(const __key_container_type& __key_cont, const __mapped_container_type& __mapped_cont,
                          const key_compare& comp, const _Alloc& a)
      : base(__key_cont, __mapped_cont, comp, a) {
    this->__sort_all();
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(sorted_equivalent_t, const __key_container_type& __key_cont,
                          const __mapped_container_type& __mapped_cont, const _Alloc& a)
      : base(__key_cont, __mapped_cont, key_compare(), a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(sorted_equivalent_t, const __key_container_type& __key_cont,
                          const __mapped_container_type& __mapped_cont, const key_compare& comp, const _Alloc& a)
      : base(__key_cont, __mapped_cont, comp, a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(const flat_multimap& __x, const _Alloc& a) : base(__x, a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(flat_multimap&& __x, const _Alloc& a) : base(static_cast<base&&>(__x), a) {}
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(_InputIterator first, _InputIterator last, const _Alloc& a) : base(key_compare(), a) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(_InputIterator first, _InputIterator last, const key_compare& comp, const _Alloc& a)
      : base(comp, a) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(sorted_equivalent_t, _InputIterator first, _InputIterator last, const _Alloc& a)
      : base(key_compare(), a) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(sorted_equivalent_t, _InputIterator first, _InputIterator last, const key_compare& comp,
                          const _Alloc& a)
      : base(comp, a) {
    this->__insert_elems(first, last);
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp, class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(_Tag, _Rp&& __rg, const _Alloc& a) : base(key_compare(), a) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp, class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(_Tag, _Rp&& __rg, const key_compare& comp, const _Alloc& a) : base(comp, a) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(initializer_list<value_type> il, const _Alloc& a) : base(key_compare(), a) {
    this->__insert_elems(il.begin(), il.end());
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(initializer_list<value_type> il, const key_compare& comp, const _Alloc& a)
      : base(comp, a) {
    this->__insert_elems(il.begin(), il.end());
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(sorted_equivalent_t, initializer_list<value_type> il, const _Alloc& a)
      : base(key_compare(), a) {
    this->__insert_elems(il.begin(), il.end());
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer, _MappedContainer>
  constexpr flat_multimap(sorted_equivalent_t, initializer_list<value_type> il, const key_compare& comp,
                          const _Alloc& a)
      : base(comp, a) {
    this->__insert_elems(il.begin(), il.end());
  }

  constexpr flat_multimap& operator=(initializer_list<value_type> il) {
    this->clear();
    this->__insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- modifiers ----
  template <class... _Args>
    requires is_constructible_v<pair<key_type, mapped_type>, _Args...>
  constexpr iterator emplace(_Args&&... __args) {
    return this->__insert_multi(value_type(static_cast<_Args&&>(__args)...));
  }
  template <class... _Args>
    requires is_constructible_v<pair<key_type, mapped_type>, _Args...>
  constexpr iterator emplace_hint(const_iterator position, _Args&&... __args) {
    return this->__insert_multi_hint(position, value_type(static_cast<_Args&&>(__args)...));
  }
  constexpr iterator insert(const value_type& __x) { return emplace(__x); }
  constexpr iterator insert(value_type&& __x) { return emplace(static_cast<value_type&&>(__x)); }
  constexpr iterator insert(const_iterator position, const value_type& __x) { return emplace_hint(position, __x); }
  constexpr iterator insert(const_iterator position, value_type&& __x) {
    return emplace_hint(position, static_cast<value_type&&>(__x));
  }
  template <class _Pp>
    requires is_constructible_v<pair<key_type, mapped_type>, _Pp>
  constexpr iterator insert(_Pp&& __x) {
    return emplace(static_cast<_Pp&&>(__x));
  }
  template <class _Pp>
    requires is_constructible_v<pair<key_type, mapped_type>, _Pp>
  constexpr iterator insert(const_iterator position, _Pp&& __x) {
    return emplace_hint(position, static_cast<_Pp&&>(__x));
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr void insert(_InputIterator first, _InputIterator last) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr void insert(sorted_equivalent_t, _InputIterator first, _InputIterator last) {
    this->__insert_elems(first, last);
  }
  template <__ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr void insert_range(_Rp&& __rg) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  template <__ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr void insert_range(sorted_equivalent_t, _Rp&& __rg) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  constexpr void insert(initializer_list<value_type> il) { insert(il.begin(), il.end()); }
  constexpr void insert(sorted_equivalent_t, initializer_list<value_type> il) {
    insert(sorted_equivalent, il.begin(), il.end());
  }

  constexpr void swap(flat_multimap& y) noexcept(is_nothrow_swappable_v<__key_container_type> &&
                                                 is_nothrow_swappable_v<__mapped_container_type> &&
                                                 is_nothrow_swappable_v<key_compare>) {
    this->__swap_impl(y);
  }

  friend constexpr bool operator==(const flat_multimap& __x, const flat_multimap& y) {
    return __x.size() == y.size() && std::equal(__x.begin(), __x.end(), y.begin());
  }
  // A template, so that the return type is formed only when the operator is used.
  template <class _Vp = value_type>
  friend constexpr __ycxx::__detail::__synth_three_way_result<_Vp> operator<=>(const flat_multimap& __x, const flat_multimap& y) {
    return std::lexicographical_compare_three_way(__x.begin(), __x.end(), y.begin(), y.end(),
                                                  __ycxx::__detail::__synth_three_way);
  }
  friend constexpr void swap(flat_multimap& __x, flat_multimap& y) noexcept(noexcept(__x.swap(y))) { __x.swap(y); }
};

// ---- deduction guides ([container.adaptors.general]/6) ----
template <class _KeyContainer, class _MappedContainer, class _Compare = less<typename _KeyContainer::value_type>>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_container_arg<_MappedContainer> &&
           __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer>
flat_map(_KeyContainer, _MappedContainer, _Compare = _Compare())
    -> flat_map<typename _KeyContainer::value_type, typename _MappedContainer::value_type, _Compare, _KeyContainer,
                _MappedContainer>;
template <class _KeyContainer, class _MappedContainer, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_container_arg<_MappedContainer> &&
           uses_allocator_v<_KeyContainer, _Allocator> && uses_allocator_v<_MappedContainer, _Allocator>
flat_map(_KeyContainer, _MappedContainer, _Allocator)
    -> flat_map<typename _KeyContainer::value_type, typename _MappedContainer::value_type,
                less<typename _KeyContainer::value_type>, _KeyContainer, _MappedContainer>;
template <class _KeyContainer, class _MappedContainer, class _Compare, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_container_arg<_MappedContainer> &&
           __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer> && uses_allocator_v<_KeyContainer, _Allocator> &&
           uses_allocator_v<_MappedContainer, _Allocator>
flat_map(_KeyContainer, _MappedContainer, _Compare, _Allocator)
    -> flat_map<typename _KeyContainer::value_type, typename _MappedContainer::value_type, _Compare, _KeyContainer,
                _MappedContainer>;
template <class _KeyContainer, class _MappedContainer, class _Compare = less<typename _KeyContainer::value_type>>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_container_arg<_MappedContainer> &&
           __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer>
flat_map(sorted_unique_t, _KeyContainer, _MappedContainer, _Compare = _Compare())
    -> flat_map<typename _KeyContainer::value_type, typename _MappedContainer::value_type, _Compare, _KeyContainer,
                _MappedContainer>;
template <class _KeyContainer, class _MappedContainer, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_container_arg<_MappedContainer> &&
           uses_allocator_v<_KeyContainer, _Allocator> && uses_allocator_v<_MappedContainer, _Allocator>
flat_map(sorted_unique_t, _KeyContainer, _MappedContainer, _Allocator)
    -> flat_map<typename _KeyContainer::value_type, typename _MappedContainer::value_type,
                less<typename _KeyContainer::value_type>, _KeyContainer, _MappedContainer>;
template <class _KeyContainer, class _MappedContainer, class _Compare, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_container_arg<_MappedContainer> &&
           __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer> && uses_allocator_v<_KeyContainer, _Allocator> &&
           uses_allocator_v<_MappedContainer, _Allocator>
flat_map(sorted_unique_t, _KeyContainer, _MappedContainer, _Compare, _Allocator)
    -> flat_map<typename _KeyContainer::value_type, typename _MappedContainer::value_type, _Compare, _KeyContainer,
                _MappedContainer>;
template <class _InputIterator, class _Compare = less<__ycxx::__detail::__iter_key_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__deducible_compare<_Compare>
flat_map(_InputIterator, _InputIterator, _Compare = _Compare())
    -> flat_map<__ycxx::__detail::__iter_key_type<_InputIterator>, __ycxx::__detail::__iter_mapped_type<_InputIterator>, _Compare>;
template <class _InputIterator, class _Compare = less<__ycxx::__detail::__iter_key_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__deducible_compare<_Compare>
flat_map(sorted_unique_t, _InputIterator, _InputIterator, _Compare = _Compare())
    -> flat_map<__ycxx::__detail::__iter_key_type<_InputIterator>, __ycxx::__detail::__iter_mapped_type<_InputIterator>, _Compare>;
template <ranges::input_range _Rp, class _Compare = less<__ycxx::__detail::__range_key_type<_Rp>>,
          class _Allocator = allocator<byte>>
  requires __ycxx::__detail::__deducible_compare<_Compare> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
flat_map(from_range_t, _Rp&&, _Compare = _Compare(), _Allocator = _Allocator())
    -> flat_map<__ycxx::__detail::__range_key_type<_Rp>, __ycxx::__detail::__range_mapped_type<_Rp>, _Compare,
                vector<__ycxx::__detail::__range_key_type<_Rp>,
                       __ycxx::__detail::__rebound_alloc<_Allocator, __ycxx::__detail::__range_key_type<_Rp>>>,
                vector<__ycxx::__detail::__range_mapped_type<_Rp>,
                       __ycxx::__detail::__rebound_alloc<_Allocator, __ycxx::__detail::__range_mapped_type<_Rp>>>>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
flat_map(from_range_t, _Rp&&, _Allocator)
    -> flat_map<__ycxx::__detail::__range_key_type<_Rp>, __ycxx::__detail::__range_mapped_type<_Rp>,
                less<__ycxx::__detail::__range_key_type<_Rp>>,
                vector<__ycxx::__detail::__range_key_type<_Rp>,
                       __ycxx::__detail::__rebound_alloc<_Allocator, __ycxx::__detail::__range_key_type<_Rp>>>,
                vector<__ycxx::__detail::__range_mapped_type<_Rp>,
                       __ycxx::__detail::__rebound_alloc<_Allocator, __ycxx::__detail::__range_mapped_type<_Rp>>>>;
template <class _Key, class _Tp, class _Compare = less<_Key>>
  requires __ycxx::__detail::__deducible_compare<_Compare>
flat_map(initializer_list<pair<_Key, _Tp>>, _Compare = _Compare()) -> flat_map<_Key, _Tp, _Compare>;
template <class _Key, class _Tp, class _Compare = less<_Key>>
  requires __ycxx::__detail::__deducible_compare<_Compare>
flat_map(sorted_unique_t, initializer_list<pair<_Key, _Tp>>, _Compare = _Compare()) -> flat_map<_Key, _Tp, _Compare>;

template <class _KeyContainer, class _MappedContainer, class _Compare = less<typename _KeyContainer::value_type>>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_container_arg<_MappedContainer> &&
           __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer>
flat_multimap(_KeyContainer, _MappedContainer, _Compare = _Compare())
    -> flat_multimap<typename _KeyContainer::value_type, typename _MappedContainer::value_type, _Compare, _KeyContainer,
                     _MappedContainer>;
template <class _KeyContainer, class _MappedContainer, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_container_arg<_MappedContainer> &&
           uses_allocator_v<_KeyContainer, _Allocator> && uses_allocator_v<_MappedContainer, _Allocator>
flat_multimap(_KeyContainer, _MappedContainer, _Allocator)
    -> flat_multimap<typename _KeyContainer::value_type, typename _MappedContainer::value_type,
                     less<typename _KeyContainer::value_type>, _KeyContainer, _MappedContainer>;
template <class _KeyContainer, class _MappedContainer, class _Compare, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_container_arg<_MappedContainer> &&
           __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer> && uses_allocator_v<_KeyContainer, _Allocator> &&
           uses_allocator_v<_MappedContainer, _Allocator>
flat_multimap(_KeyContainer, _MappedContainer, _Compare, _Allocator)
    -> flat_multimap<typename _KeyContainer::value_type, typename _MappedContainer::value_type, _Compare, _KeyContainer,
                     _MappedContainer>;
template <class _KeyContainer, class _MappedContainer, class _Compare = less<typename _KeyContainer::value_type>>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_container_arg<_MappedContainer> &&
           __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer>
flat_multimap(sorted_equivalent_t, _KeyContainer, _MappedContainer, _Compare = _Compare())
    -> flat_multimap<typename _KeyContainer::value_type, typename _MappedContainer::value_type, _Compare, _KeyContainer,
                     _MappedContainer>;
template <class _KeyContainer, class _MappedContainer, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_container_arg<_MappedContainer> &&
           uses_allocator_v<_KeyContainer, _Allocator> && uses_allocator_v<_MappedContainer, _Allocator>
flat_multimap(sorted_equivalent_t, _KeyContainer, _MappedContainer, _Allocator)
    -> flat_multimap<typename _KeyContainer::value_type, typename _MappedContainer::value_type,
                     less<typename _KeyContainer::value_type>, _KeyContainer, _MappedContainer>;
template <class _KeyContainer, class _MappedContainer, class _Compare, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_container_arg<_MappedContainer> &&
           __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer> && uses_allocator_v<_KeyContainer, _Allocator> &&
           uses_allocator_v<_MappedContainer, _Allocator>
flat_multimap(sorted_equivalent_t, _KeyContainer, _MappedContainer, _Compare, _Allocator)
    -> flat_multimap<typename _KeyContainer::value_type, typename _MappedContainer::value_type, _Compare, _KeyContainer,
                     _MappedContainer>;
template <class _InputIterator, class _Compare = less<__ycxx::__detail::__iter_key_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__deducible_compare<_Compare>
flat_multimap(_InputIterator, _InputIterator, _Compare = _Compare())
    -> flat_multimap<__ycxx::__detail::__iter_key_type<_InputIterator>, __ycxx::__detail::__iter_mapped_type<_InputIterator>,
                     _Compare>;
template <class _InputIterator, class _Compare = less<__ycxx::__detail::__iter_key_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__deducible_compare<_Compare>
flat_multimap(sorted_equivalent_t, _InputIterator, _InputIterator, _Compare = _Compare())
    -> flat_multimap<__ycxx::__detail::__iter_key_type<_InputIterator>, __ycxx::__detail::__iter_mapped_type<_InputIterator>,
                     _Compare>;
template <ranges::input_range _Rp, class _Compare = less<__ycxx::__detail::__range_key_type<_Rp>>,
          class _Allocator = allocator<byte>>
  requires __ycxx::__detail::__deducible_compare<_Compare> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
flat_multimap(from_range_t, _Rp&&, _Compare = _Compare(), _Allocator = _Allocator())
    -> flat_multimap<__ycxx::__detail::__range_key_type<_Rp>, __ycxx::__detail::__range_mapped_type<_Rp>, _Compare,
                     vector<__ycxx::__detail::__range_key_type<_Rp>,
                            __ycxx::__detail::__rebound_alloc<_Allocator, __ycxx::__detail::__range_key_type<_Rp>>>,
                     vector<__ycxx::__detail::__range_mapped_type<_Rp>,
                            __ycxx::__detail::__rebound_alloc<_Allocator, __ycxx::__detail::__range_mapped_type<_Rp>>>>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
flat_multimap(from_range_t, _Rp&&, _Allocator)
    -> flat_multimap<__ycxx::__detail::__range_key_type<_Rp>, __ycxx::__detail::__range_mapped_type<_Rp>,
                     less<__ycxx::__detail::__range_key_type<_Rp>>,
                     vector<__ycxx::__detail::__range_key_type<_Rp>,
                            __ycxx::__detail::__rebound_alloc<_Allocator, __ycxx::__detail::__range_key_type<_Rp>>>,
                     vector<__ycxx::__detail::__range_mapped_type<_Rp>,
                            __ycxx::__detail::__rebound_alloc<_Allocator, __ycxx::__detail::__range_mapped_type<_Rp>>>>;
template <class _Key, class _Tp, class _Compare = less<_Key>>
  requires __ycxx::__detail::__deducible_compare<_Compare>
flat_multimap(initializer_list<pair<_Key, _Tp>>, _Compare = _Compare()) -> flat_multimap<_Key, _Tp, _Compare>;
template <class _Key, class _Tp, class _Compare = less<_Key>>
  requires __ycxx::__detail::__deducible_compare<_Compare>
flat_multimap(sorted_equivalent_t, initializer_list<pair<_Key, _Tp>>, _Compare = _Compare())
    -> flat_multimap<_Key, _Tp, _Compare>;

// ---- uses_allocator ----
template <class _Key, class _Tp, class _Compare, class _KeyContainer, class _MappedContainer, class _Allocator>
struct uses_allocator<flat_map<_Key, _Tp, _Compare, _KeyContainer, _MappedContainer>, _Allocator>
    : bool_constant<uses_allocator_v<_KeyContainer, _Allocator> && uses_allocator_v<_MappedContainer, _Allocator>> {};
template <class _Key, class _Tp, class _Compare, class _KeyContainer, class _MappedContainer, class _Allocator>
struct uses_allocator<flat_multimap<_Key, _Tp, _Compare, _KeyContainer, _MappedContainer>, _Allocator>
    : bool_constant<uses_allocator_v<_KeyContainer, _Allocator> && uses_allocator_v<_MappedContainer, _Allocator>> {};

// ---- [flat.map.erasure], [flat.multimap.erasure] ----
template <class _Key, class _Tp, class _Compare, class _KeyContainer, class _MappedContainer, class _Predicate>
constexpr typename flat_map<_Key, _Tp, _Compare, _KeyContainer, _MappedContainer>::size_type
erase_if(flat_map<_Key, _Tp, _Compare, _KeyContainer, _MappedContainer>& c, _Predicate pred) {
  // The containers are taken out (c is empty if pred throws) and put back.
  auto __cs = static_cast<flat_map<_Key, _Tp, _Compare, _KeyContainer, _MappedContainer>&&>(c).extract();
  auto test = [&](size_t i) -> bool {
    return static_cast<bool>(pred(pair<const _Key&, const _Tp&>(__ycxx::__detail::__row_at(__cs.keys, i),
                                                             __ycxx::__detail::__row_at(__cs.values, i))));
  };
  const auto n = __ycxx::__detail::__erase_rows_if(test, __cs.keys, __cs.values);
  c.replace(static_cast<_KeyContainer&&>(__cs.keys), static_cast<_MappedContainer&&>(__cs.values));
  return n;
}
template <class _Key, class _Tp, class _Compare, class _KeyContainer, class _MappedContainer, class _Predicate>
constexpr typename flat_multimap<_Key, _Tp, _Compare, _KeyContainer, _MappedContainer>::size_type
erase_if(flat_multimap<_Key, _Tp, _Compare, _KeyContainer, _MappedContainer>& c, _Predicate pred) {
  auto __cs = static_cast<flat_multimap<_Key, _Tp, _Compare, _KeyContainer, _MappedContainer>&&>(c).extract();
  auto test = [&](size_t i) -> bool {
    return static_cast<bool>(pred(pair<const _Key&, const _Tp&>(__ycxx::__detail::__row_at(__cs.keys, i),
                                                             __ycxx::__detail::__row_at(__cs.values, i))));
  };
  const auto n = __ycxx::__detail::__erase_rows_if(test, __cs.keys, __cs.values);
  c.replace(static_cast<_KeyContainer&&>(__cs.keys), static_cast<_MappedContainer&&>(__cs.values));
  return n;
}

} // namespace std
