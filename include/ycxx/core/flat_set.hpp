// libycxx core: flat_set and flat_multiset ([flat.set], [flat.multiset]), their erasure,
// deduction guides and uses_allocator specializations.
//
// The keys live in one sequence container c_; flat_set_base holds it and the members both
// adaptors share. iterator and const_iterator are both the container's const_iterator. As for
// flat_map, every modification is covered by a flat_guard (flat_support.hpp) that restores the
// sorted invariant ([flat.set.overview]/5-6) if it exits via an exception.
#pragma once

#include <initializer_list>
#include <ycxx/core/algo_base.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/flat_support.hpp>
#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/tuple.hpp>
#include <ycxx/core/vector.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// The members flat_set (Multi false) and flat_multiset share.
template <class _Key, class _Compare, class _KC, bool _Multi>
class __flat_set_base {
  static_assert(std::is_same_v<_Key, typename _KC::value_type>,
                "flat_set: Key must be KeyContainer::value_type ([flat.set.overview]/8)");

public:
  // ---- types ----
  using key_type = _Key;
  using value_type = _Key;
  using key_compare = _Compare;
  using value_compare = _Compare;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = typename _KC::size_type;
  using difference_type = typename _KC::difference_type;
  using iterator = typename _KC::const_iterator;
  using const_iterator = typename _KC::const_iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using container_type = _KC;

protected:
  container_type __c_;
  [[no_unique_address]] key_compare __compare_;

  // ---- helpers ----
  template <class _Ap, class _Bp>
  constexpr bool lt(const _Ap& a, const _Bp& b) const {
    return static_cast<bool>(static_cast<const key_compare&>(__compare_)(a, b));
  }
  constexpr auto __less_fn() const noexcept {
    return [this](const auto& a, const auto& b) -> bool { return lt(a, b); };
  }
  constexpr const key_type& __key_at(std::size_t i) const { return ::__ycxx::__detail::__row_at(__c_, i); }
  constexpr const_iterator __it_at(std::size_t i) const noexcept {
    return __c_.cbegin() + static_cast<difference_type>(i);
  }
  constexpr std::size_t __index_of(const_iterator __it) const noexcept {
    return static_cast<std::size_t>(__it - __c_.cbegin());
  }
  constexpr auto __guard() noexcept { return ::__ycxx::__detail::__flat_guard(__c_); }

  template <class _Kp>
  constexpr std::size_t __lower_index(const _Kp& __x, std::size_t __lo, std::size_t __hi) const {
    std::size_t n = __hi - __lo;
    while (n > 0) {
      const std::size_t __half = n / 2;
      if (lt(__key_at(__lo + __half), __x)) {
        __lo += __half + 1;
        n -= __half + 1;
      } else {
        n = __half;
      }
    }
    return __lo;
  }
  template <class _Kp>
  constexpr std::size_t __lower_index(const _Kp& __x) const {
    return __lower_index(__x, 0, __c_.size());
  }
  template <class _Kp>
  constexpr std::size_t __upper_index(const _Kp& __x, std::size_t __lo, std::size_t __hi) const {
    std::size_t n = __hi - __lo;
    while (n > 0) {
      const std::size_t __half = n / 2;
      if (!lt(__x, __key_at(__lo + __half))) {
        __lo += __half + 1;
        n -= __half + 1;
      } else {
        n = __half;
      }
    }
    return __lo;
  }
  template <class _Kp>
  constexpr std::size_t __upper_index(const _Kp& __x) const {
    return __upper_index(__x, 0, __c_.size());
  }
  template <class _Kp>
  constexpr std::size_t __find_index(const _Kp& __x) const {
    const std::size_t i = __lower_index(__x);
    if (i == __c_.size() || lt(__x, __key_at(i)))
      return __c_.size();
    return i;
  }
  template <class _Kp>
  constexpr std::pair<std::size_t, bool> __unique_pos(const _Kp& __x) const {
    const std::size_t i = __lower_index(__x);
    return {i, i != __c_.size() && !lt(__x, __key_at(i))};
  }
  template <class _Kp>
  constexpr std::pair<std::size_t, bool> __unique_pos_hint(const_iterator __hint, const _Kp& __x) const {
    const std::size_t h = __index_of(__hint);
    if ((h == 0 || lt(__key_at(h - 1), __x)) && (h == __c_.size() || lt(__x, __key_at(h))))
      return {h, false};
    return __unique_pos(__x);
  }
  template <class _Kp>
  constexpr std::size_t __multi_pos_hint(const_iterator __hint, const _Kp& __x) const {
    const std::size_t h = __index_of(__hint);
    if (h > 0 && lt(__x, __key_at(h - 1)))
      return __upper_index(__x, 0, h - 1);
    if (h < __c_.size() && lt(__key_at(h), __x))
      return __lower_index(__x, h + 1, __c_.size());
    return h;
  }

  template <class... _Args>
  constexpr iterator __insert_at(std::size_t i, _Args&&... __args) {
    auto __g = __guard();
    __c_.emplace(__it_at(i), static_cast<_Args&&>(__args)...);
    __g.release();
    return __it_at(i);
  }
  constexpr std::pair<iterator, bool> __insert_unique(value_type&& t) {
    const auto p = __unique_pos(t);
    if (p.second)
      return {__it_at(p.first), false};
    return {__insert_at(p.first, static_cast<value_type&&>(t)), true};
  }
  constexpr iterator __insert_unique_hint(const_iterator __hint, value_type&& t) {
    const auto p = __unique_pos_hint(__hint, t);
    if (p.second)
      return __it_at(p.first);
    return __insert_at(p.first, static_cast<value_type&&>(t));
  }
  constexpr iterator __insert_multi(value_type&& t) {
    return __insert_at(__upper_index(t), static_cast<value_type&&>(t));
  }
  constexpr iterator __insert_multi_hint(const_iterator __hint, value_type&& t) {
    return __insert_at(__multi_pos_hint(__hint, t), static_cast<value_type&&>(t));
  }

  constexpr void __sort_from(std::size_t from) {
    auto less = __less_fn();
    ::__ycxx::__detail::__sort_rows(less, from, __c_);
    if constexpr (!_Multi)
      ::__ycxx::__detail::__unique_rows(less, __c_);
  }
  constexpr void __sort_all() {
    auto __g = __guard();
    __sort_from(0);
    __g.release();
  }
  // [flat.set.modifiers]/5: c.insert(c.end(), first, last), then sorts the new keys in.
  template <class _It>
  constexpr void __insert_elems(_It first, _It last) {
    const std::size_t __old = __c_.size();
    auto __g = __guard();
    __c_.insert(__c_.end(), first, last);
    __sort_from(__old);
    __g.release();
  }
  // [flat.set.modifiers]/10: each element is inserted at the end, then the new keys sorted in.
  template <class _Rp>
  constexpr void __insert_range_elems(_Rp&& __rg) {
    const std::size_t __old = __c_.size();
    auto __g = __guard();
    for (auto&& e : __rg)
      __c_.insert(__c_.end(), value_type(static_cast<decltype(e)&&>(e)));
    __sort_from(__old);
    __g.release();
  }

  static constexpr container_type take(__flat_set_base& __o) {
    auto __g = __o.__guard();
    return static_cast<container_type&&>(__o.__c_);
  }
  template <class _Ap>
  static constexpr container_type take(__flat_set_base& __o, const _Ap& a) {
    auto __g = __o.__guard();
    return std::make_obj_using_allocator<container_type>(a, static_cast<container_type&&>(__o.__c_));
  }

  // ---- construction and assignment ----
  constexpr explicit __flat_set_base(const key_compare& comp) : __c_(), __compare_(comp) {}
  template <class _Ap>
  constexpr __flat_set_base(const key_compare& comp, const _Ap& a)
      : __c_(std::make_obj_using_allocator<container_type>(a)), __compare_(comp) {}
  constexpr __flat_set_base(container_type&& c, const key_compare& comp)
      : __c_(static_cast<container_type&&>(c)), __compare_(comp) {}
  template <class _Ap>
  constexpr __flat_set_base(const container_type& c, const key_compare& comp, const _Ap& a)
      : __c_(std::make_obj_using_allocator<container_type>(a, c)), __compare_(comp) {}
  template <class _It>
  constexpr __flat_set_base(_It first, _It last, const key_compare& comp) : __c_(first, last), __compare_(comp) {}
  template <class _It, class _Ap>
  constexpr __flat_set_base(_It first, _It last, const key_compare& comp, const _Ap& a)
      : __c_(std::make_obj_using_allocator<container_type>(a, first, last)), __compare_(comp) {}
  constexpr __flat_set_base(const __flat_set_base&) = default;
  constexpr __flat_set_base(__flat_set_base&& __o) : __c_(take(__o)), __compare_(__o.__compare_) {}
  template <class _Ap>
  constexpr __flat_set_base(const __flat_set_base& __o, const _Ap& a)
      : __c_(std::make_obj_using_allocator<container_type>(a, __o.__c_)), __compare_(__o.__compare_) {}
  template <class _Ap>
  constexpr __flat_set_base(__flat_set_base&& __o, const _Ap& a) : __c_(take(__o, a)), __compare_(__o.__compare_) {}

  constexpr void __copy_assign(const __flat_set_base& __o) {
    if (this == __builtin_addressof(__o))
      return;
    auto __g = __guard();
    __c_ = __o.__c_;
    __compare_ = __o.__compare_;
    __g.release();
  }
  constexpr void __move_assign(__flat_set_base& __o) {
    if (this == __builtin_addressof(__o))
      return;
    auto __go = __o.__guard(); // o is emptied in any case
    auto __g = __guard();
    __c_ = static_cast<container_type&&>(__o.__c_);
    __compare_ = static_cast<key_compare&&>(__o.__compare_);
    __g.release();
  }
  constexpr void __swap_impl(__flat_set_base& y) {
    auto __g = __guard();
    auto __gy = y.__guard();
    ::__ycxx::__detail::__swap_adl::__do_swap(__compare_, y.__compare_);
    ::__ycxx::__detail::__swap_adl::__do_swap(__c_, y.__c_);
    __gy.release();
    __g.release();
  }

public:
  // ---- iterators ----
  constexpr iterator begin() noexcept { return __c_.cbegin(); }
  constexpr const_iterator begin() const noexcept { return __c_.cbegin(); }
  constexpr iterator end() noexcept { return __c_.cend(); }
  constexpr const_iterator end() const noexcept { return __c_.cend(); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- capacity ----
  [[nodiscard]] constexpr bool empty() const noexcept { return __c_.empty(); }
  constexpr size_type size() const noexcept { return __c_.size(); }
  constexpr size_type max_size() const noexcept { return __c_.max_size(); }

  // ---- modifiers ----
  constexpr container_type extract() && {
    auto __g = __guard();
    return static_cast<container_type&&>(__c_);
  }
  constexpr void replace(container_type&& __cont) {
    auto __g = __guard();
    __c_ = static_cast<container_type&&>(__cont);
    __g.release();
  }
  constexpr iterator erase(const_iterator position) {
    const std::size_t i = __index_of(position);
    ::__ycxx::__detail::__precondition(i < __c_.size(), "flat_set::erase: end() iterator");
    auto __g = __guard();
    __c_.erase(position);
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
    const std::size_t i = __index_of(first);
    if (first != last) {
      auto __g = __guard();
      __c_.erase(first, last);
      __g.release();
    }
    return __it_at(i);
  }
  constexpr void clear() noexcept { __c_.clear(); }

  // ---- observers ----
  constexpr key_compare key_comp() const { return __compare_; }
  constexpr value_compare value_comp() const { return __compare_; }

  // ---- set operations ----
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
      return static_cast<size_type>(__upper_index(__x) - __lower_index(__x));
    else
      return __find_index(__x) == __c_.size() ? 0 : 1;
  }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr size_type count(const _Kp& __x) const {
    return static_cast<size_type>(__upper_index(__x) - __lower_index(__x));
  }
  constexpr bool contains(const key_type& __x) const { return __find_index(__x) != __c_.size(); }
  template <class _Kp>
    requires ::__ycxx::__detail::__transparent_compare<_Compare>
  constexpr bool contains(const _Kp& __x) const {
    return __find_index(__x) != __c_.size();
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
    const std::size_t i = __lower_index(__x), __j = __upper_index(__x);
    erase(__it_at(i), __it_at(__j));
    return static_cast<size_type>(__j - i);
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Key, class _Compare = less<_Key>, class _KeyContainer = vector<_Key>>
class flat_set : public __ycxx::__adl_free::__flat_set_base<_Key, _Compare, _KeyContainer, false> {
  using base = __ycxx::__adl_free::__flat_set_base<_Key, _Compare, _KeyContainer, false>;

public:
  // ---- types ----
  using typename base::const_iterator;
  using typename base::const_reference;
  using typename base::const_reverse_iterator;
  using typename base::container_type;
  using typename base::difference_type;
  using typename base::iterator;
  using typename base::key_compare;
  using typename base::key_type;
  using typename base::reference;
  using typename base::reverse_iterator;
  using typename base::size_type;
  using typename base::value_compare;
  using typename base::value_type;

  // ---- [flat.set.cons] ----
  constexpr flat_set() : flat_set(key_compare()) {}
  constexpr flat_set(const flat_set&) = default;
  constexpr flat_set(flat_set&& __x) : base(static_cast<base&&>(__x)) {}
  constexpr flat_set& operator=(const flat_set& __x) {
    this->__copy_assign(__x);
    return *this;
  }
  constexpr flat_set& operator=(flat_set&& __x) {
    this->__move_assign(__x);
    return *this;
  }
  constexpr explicit flat_set(const key_compare& comp) : base(comp) {}
  constexpr explicit flat_set(container_type __cont, const key_compare& comp = key_compare())
      : base(static_cast<container_type&&>(__cont), comp) {
    this->__sort_all();
  }
  constexpr flat_set(sorted_unique_t, container_type __cont, const key_compare& comp = key_compare())
      : base(static_cast<container_type&&>(__cont), comp) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr flat_set(_InputIterator first, _InputIterator last, const key_compare& comp = key_compare()) : base(comp) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr flat_set(sorted_unique_t, _InputIterator first, _InputIterator last, const key_compare& comp = key_compare())
      : base(first, last, comp) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr flat_set(_Tag, _Rp&& __rg) : flat_set(from_range, static_cast<_Rp&&>(__rg), key_compare()) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr flat_set(_Tag, _Rp&& __rg, const key_compare& comp) : base(comp) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  constexpr flat_set(initializer_list<value_type> il, const key_compare& comp = key_compare())
      : flat_set(il.begin(), il.end(), comp) {}
  constexpr flat_set(sorted_unique_t, initializer_list<value_type> il, const key_compare& comp = key_compare())
      : flat_set(sorted_unique, il.begin(), il.end(), comp) {}

  // ---- [flat.set.cons.alloc] ----
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr explicit flat_set(const _Alloc& a) : base(key_compare(), a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(const key_compare& comp, const _Alloc& a) : base(comp, a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(const container_type& __cont, const _Alloc& a) : base(__cont, key_compare(), a) {
    this->__sort_all();
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(const container_type& __cont, const key_compare& comp, const _Alloc& a) : base(__cont, comp, a) {
    this->__sort_all();
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(sorted_unique_t, const container_type& __cont, const _Alloc& a) : base(__cont, key_compare(), a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(sorted_unique_t, const container_type& __cont, const key_compare& comp, const _Alloc& a)
      : base(__cont, comp, a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(const flat_set& __x, const _Alloc& a) : base(__x, a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(flat_set&& __x, const _Alloc& a) : base(static_cast<base&&>(__x), a) {}
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(_InputIterator first, _InputIterator last, const _Alloc& a) : base(key_compare(), a) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(_InputIterator first, _InputIterator last, const key_compare& comp, const _Alloc& a)
      : base(comp, a) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(sorted_unique_t, _InputIterator first, _InputIterator last, const _Alloc& a)
      : base(first, last, key_compare(), a) {}
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(sorted_unique_t, _InputIterator first, _InputIterator last, const key_compare& comp,
                     const _Alloc& a)
      : base(first, last, comp, a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp, class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(_Tag, _Rp&& __rg, const _Alloc& a) : base(key_compare(), a) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp, class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(_Tag, _Rp&& __rg, const key_compare& comp, const _Alloc& a) : base(comp, a) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(initializer_list<value_type> il, const _Alloc& a) : base(key_compare(), a) {
    this->__insert_elems(il.begin(), il.end());
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(initializer_list<value_type> il, const key_compare& comp, const _Alloc& a) : base(comp, a) {
    this->__insert_elems(il.begin(), il.end());
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(sorted_unique_t, initializer_list<value_type> il, const _Alloc& a)
      : base(il.begin(), il.end(), key_compare(), a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_set(sorted_unique_t, initializer_list<value_type> il, const key_compare& comp, const _Alloc& a)
      : base(il.begin(), il.end(), comp, a) {}

  constexpr flat_set& operator=(initializer_list<value_type> il) {
    this->clear();
    this->__insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- [flat.set.modifiers] ----
  template <class... _Args>
    requires is_constructible_v<value_type, _Args...>
  constexpr pair<iterator, bool> emplace(_Args&&... __args) {
    return this->__insert_unique(value_type(static_cast<_Args&&>(__args)...));
  }
  template <class... _Args>
    requires is_constructible_v<value_type, _Args...>
  constexpr iterator emplace_hint(const_iterator position, _Args&&... __args) {
    return this->__insert_unique_hint(position, value_type(static_cast<_Args&&>(__args)...));
  }
  constexpr pair<iterator, bool> insert(const value_type& __x) { return emplace(__x); }
  constexpr pair<iterator, bool> insert(value_type&& __x) { return emplace(static_cast<value_type&&>(__x)); }
  template <class _Kp>
    requires __ycxx::__detail::__transparent_compare<_Compare> && is_constructible_v<value_type, _Kp>
  constexpr pair<iterator, bool> insert(_Kp&& __x) {
    const auto p = this->__unique_pos(__x);
    if (p.second)
      return {this->__it_at(p.first), false};
    return {this->__insert_at(p.first, static_cast<_Kp&&>(__x)), true};
  }
  constexpr iterator insert(const_iterator position, const value_type& __x) { return emplace_hint(position, __x); }
  constexpr iterator insert(const_iterator position, value_type&& __x) {
    return emplace_hint(position, static_cast<value_type&&>(__x));
  }
  template <class _Kp>
    requires __ycxx::__detail::__transparent_compare<_Compare> && is_constructible_v<value_type, _Kp>
  constexpr iterator insert(const_iterator __hint, _Kp&& __x) {
    const auto p = this->__unique_pos_hint(__hint, __x);
    if (p.second)
      return this->__it_at(p.first);
    return this->__insert_at(p.first, static_cast<_Kp&&>(__x));
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

  constexpr void swap(flat_set& y) noexcept(is_nothrow_swappable_v<container_type> &&
                                            is_nothrow_swappable_v<key_compare>) {
    this->__swap_impl(y);
  }

  friend constexpr bool operator==(const flat_set& __x, const flat_set& y) {
    return __x.size() == y.size() && std::equal(__x.begin(), __x.end(), y.begin());
  }
  // A template, so that the return type is formed only when the operator is used.
  template <class _Vp = value_type>
  friend constexpr __ycxx::__detail::__synth_three_way_result<_Vp> operator<=>(const flat_set& __x, const flat_set& y) {
    return std::lexicographical_compare_three_way(__x.begin(), __x.end(), y.begin(), y.end(),
                                                  __ycxx::__detail::__synth_three_way);
  }
  friend constexpr void swap(flat_set& __x, flat_set& y) noexcept(noexcept(__x.swap(y))) { __x.swap(y); }
};

template <class _Key, class _Compare = less<_Key>, class _KeyContainer = vector<_Key>>
class flat_multiset : public __ycxx::__adl_free::__flat_set_base<_Key, _Compare, _KeyContainer, true> {
  using base = __ycxx::__adl_free::__flat_set_base<_Key, _Compare, _KeyContainer, true>;

public:
  // ---- types ----
  using typename base::const_iterator;
  using typename base::const_reference;
  using typename base::const_reverse_iterator;
  using typename base::container_type;
  using typename base::difference_type;
  using typename base::iterator;
  using typename base::key_compare;
  using typename base::key_type;
  using typename base::reference;
  using typename base::reverse_iterator;
  using typename base::size_type;
  using typename base::value_compare;
  using typename base::value_type;

  // ---- [flat.multiset.cons] ----
  constexpr flat_multiset() : flat_multiset(key_compare()) {}
  constexpr flat_multiset(const flat_multiset&) = default;
  constexpr flat_multiset(flat_multiset&& __x) : base(static_cast<base&&>(__x)) {}
  constexpr flat_multiset& operator=(const flat_multiset& __x) {
    this->__copy_assign(__x);
    return *this;
  }
  constexpr flat_multiset& operator=(flat_multiset&& __x) {
    this->__move_assign(__x);
    return *this;
  }
  constexpr explicit flat_multiset(const key_compare& comp) : base(comp) {}
  constexpr explicit flat_multiset(container_type __cont, const key_compare& comp = key_compare())
      : base(static_cast<container_type&&>(__cont), comp) {
    this->__sort_all();
  }
  constexpr flat_multiset(sorted_equivalent_t, container_type __cont, const key_compare& comp = key_compare())
      : base(static_cast<container_type&&>(__cont), comp) {}
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr flat_multiset(_InputIterator first, _InputIterator last, const key_compare& comp = key_compare())
      : base(comp) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr flat_multiset(sorted_equivalent_t, _InputIterator first, _InputIterator last,
                          const key_compare& comp = key_compare())
      : base(first, last, comp) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr flat_multiset(_Tag, _Rp&& __rg) : flat_multiset(from_range, static_cast<_Rp&&>(__rg), key_compare()) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp>
  constexpr flat_multiset(_Tag, _Rp&& __rg, const key_compare& comp) : base(comp) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  constexpr flat_multiset(initializer_list<value_type> il, const key_compare& comp = key_compare())
      : flat_multiset(il.begin(), il.end(), comp) {}
  constexpr flat_multiset(sorted_equivalent_t, initializer_list<value_type> il,
                          const key_compare& comp = key_compare())
      : flat_multiset(sorted_equivalent, il.begin(), il.end(), comp) {}

  // ---- [flat.multiset.cons.alloc] ----
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr explicit flat_multiset(const _Alloc& a) : base(key_compare(), a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(const key_compare& comp, const _Alloc& a) : base(comp, a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(const container_type& __cont, const _Alloc& a) : base(__cont, key_compare(), a) {
    this->__sort_all();
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(const container_type& __cont, const key_compare& comp, const _Alloc& a)
      : base(__cont, comp, a) {
    this->__sort_all();
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(sorted_equivalent_t, const container_type& __cont, const _Alloc& a)
      : base(__cont, key_compare(), a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(sorted_equivalent_t, const container_type& __cont, const key_compare& comp, const _Alloc& a)
      : base(__cont, comp, a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(const flat_multiset& __x, const _Alloc& a) : base(__x, a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(flat_multiset&& __x, const _Alloc& a) : base(static_cast<base&&>(__x), a) {}
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(_InputIterator first, _InputIterator last, const _Alloc& a) : base(key_compare(), a) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(_InputIterator first, _InputIterator last, const key_compare& comp, const _Alloc& a)
      : base(comp, a) {
    this->__insert_elems(first, last);
  }
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(sorted_equivalent_t, _InputIterator first, _InputIterator last, const _Alloc& a)
      : base(first, last, key_compare(), a) {}
  template <class _InputIterator, class _Alloc>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> &&
             __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(sorted_equivalent_t, _InputIterator first, _InputIterator last, const key_compare& comp,
                          const _Alloc& a)
      : base(first, last, comp, a) {}
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp, class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(_Tag, _Rp&& __rg, const _Alloc& a) : base(key_compare(), a) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<value_type> _Rp, class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(_Tag, _Rp&& __rg, const key_compare& comp, const _Alloc& a) : base(comp, a) {
    this->__insert_range_elems(static_cast<_Rp&&>(__rg));
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(initializer_list<value_type> il, const _Alloc& a) : base(key_compare(), a) {
    this->__insert_elems(il.begin(), il.end());
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(initializer_list<value_type> il, const key_compare& comp, const _Alloc& a)
      : base(comp, a) {
    this->__insert_elems(il.begin(), il.end());
  }
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(sorted_equivalent_t, initializer_list<value_type> il, const _Alloc& a)
      : base(il.begin(), il.end(), key_compare(), a) {}
  template <class _Alloc>
    requires __ycxx::__detail::__flat_alloc_for<_Alloc, _KeyContainer>
  constexpr flat_multiset(sorted_equivalent_t, initializer_list<value_type> il, const key_compare& comp,
                          const _Alloc& a)
      : base(il.begin(), il.end(), comp, a) {}

  constexpr flat_multiset& operator=(initializer_list<value_type> il) {
    this->clear();
    this->__insert_elems(il.begin(), il.end());
    return *this;
  }

  // ---- [flat.multiset.modifiers] ----
  template <class... _Args>
    requires is_constructible_v<value_type, _Args...>
  constexpr iterator emplace(_Args&&... __args) {
    return this->__insert_multi(value_type(static_cast<_Args&&>(__args)...));
  }
  template <class... _Args>
    requires is_constructible_v<value_type, _Args...>
  constexpr iterator emplace_hint(const_iterator position, _Args&&... __args) {
    return this->__insert_multi_hint(position, value_type(static_cast<_Args&&>(__args)...));
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

  constexpr void swap(flat_multiset& y) noexcept(is_nothrow_swappable_v<container_type> &&
                                                 is_nothrow_swappable_v<key_compare>) {
    this->__swap_impl(y);
  }

  friend constexpr bool operator==(const flat_multiset& __x, const flat_multiset& y) {
    return __x.size() == y.size() && std::equal(__x.begin(), __x.end(), y.begin());
  }
  // A template, so that the return type is formed only when the operator is used.
  template <class _Vp = value_type>
  friend constexpr __ycxx::__detail::__synth_three_way_result<_Vp> operator<=>(const flat_multiset& __x, const flat_multiset& y) {
    return std::lexicographical_compare_three_way(__x.begin(), __x.end(), y.begin(), y.end(),
                                                  __ycxx::__detail::__synth_three_way);
  }
  friend constexpr void swap(flat_multiset& __x, flat_multiset& y) noexcept(noexcept(__x.swap(y))) { __x.swap(y); }
};

// ---- deduction guides ([container.adaptors.general]/6) ----
template <class _KeyContainer, class _Compare = less<typename _KeyContainer::value_type>>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer>
flat_set(_KeyContainer, _Compare = _Compare())
    -> flat_set<typename _KeyContainer::value_type, _Compare, _KeyContainer>;
template <class _KeyContainer, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && uses_allocator_v<_KeyContainer, _Allocator>
flat_set(_KeyContainer, _Allocator)
    -> flat_set<typename _KeyContainer::value_type, less<typename _KeyContainer::value_type>, _KeyContainer>;
template <class _KeyContainer, class _Compare, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer> &&
           uses_allocator_v<_KeyContainer, _Allocator>
flat_set(_KeyContainer, _Compare, _Allocator) -> flat_set<typename _KeyContainer::value_type, _Compare, _KeyContainer>;
template <class _KeyContainer, class _Compare = less<typename _KeyContainer::value_type>>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer>
flat_set(sorted_unique_t, _KeyContainer, _Compare = _Compare())
    -> flat_set<typename _KeyContainer::value_type, _Compare, _KeyContainer>;
template <class _KeyContainer, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && uses_allocator_v<_KeyContainer, _Allocator>
flat_set(sorted_unique_t, _KeyContainer, _Allocator)
    -> flat_set<typename _KeyContainer::value_type, less<typename _KeyContainer::value_type>, _KeyContainer>;
template <class _KeyContainer, class _Compare, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer> &&
           uses_allocator_v<_KeyContainer, _Allocator>
flat_set(sorted_unique_t, _KeyContainer, _Compare, _Allocator)
    -> flat_set<typename _KeyContainer::value_type, _Compare, _KeyContainer>;
template <class _InputIterator, class _Compare = less<__ycxx::__detail::__iter_value_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__deducible_compare<_Compare>
flat_set(_InputIterator, _InputIterator, _Compare = _Compare())
    -> flat_set<__ycxx::__detail::__iter_value_type<_InputIterator>, _Compare>;
template <class _InputIterator, class _Compare = less<__ycxx::__detail::__iter_value_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__deducible_compare<_Compare>
flat_set(sorted_unique_t, _InputIterator, _InputIterator, _Compare = _Compare())
    -> flat_set<__ycxx::__detail::__iter_value_type<_InputIterator>, _Compare>;
template <ranges::input_range _Rp, class _Compare = less<ranges::range_value_t<_Rp>>,
          class _Allocator = allocator<ranges::range_value_t<_Rp>>>
  requires __ycxx::__detail::__deducible_compare<_Compare> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
flat_set(from_range_t, _Rp&&, _Compare = _Compare(), _Allocator = _Allocator())
    -> flat_set<ranges::range_value_t<_Rp>, _Compare,
                vector<ranges::range_value_t<_Rp>,
                       __ycxx::__detail::__rebound_alloc<_Allocator, ranges::range_value_t<_Rp>>>>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
flat_set(from_range_t, _Rp&&, _Allocator)
    -> flat_set<ranges::range_value_t<_Rp>, less<ranges::range_value_t<_Rp>>,
                vector<ranges::range_value_t<_Rp>,
                       __ycxx::__detail::__rebound_alloc<_Allocator, ranges::range_value_t<_Rp>>>>;
template <class _Key, class _Compare = less<_Key>>
  requires __ycxx::__detail::__deducible_compare<_Compare>
flat_set(initializer_list<_Key>, _Compare = _Compare()) -> flat_set<_Key, _Compare>;
template <class _Key, class _Compare = less<_Key>>
  requires __ycxx::__detail::__deducible_compare<_Compare>
flat_set(sorted_unique_t, initializer_list<_Key>, _Compare = _Compare()) -> flat_set<_Key, _Compare>;

template <class _KeyContainer, class _Compare = less<typename _KeyContainer::value_type>>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer>
flat_multiset(_KeyContainer, _Compare = _Compare())
    -> flat_multiset<typename _KeyContainer::value_type, _Compare, _KeyContainer>;
template <class _KeyContainer, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && uses_allocator_v<_KeyContainer, _Allocator>
flat_multiset(_KeyContainer, _Allocator)
    -> flat_multiset<typename _KeyContainer::value_type, less<typename _KeyContainer::value_type>, _KeyContainer>;
template <class _KeyContainer, class _Compare, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer> &&
           uses_allocator_v<_KeyContainer, _Allocator>
flat_multiset(_KeyContainer, _Compare, _Allocator)
    -> flat_multiset<typename _KeyContainer::value_type, _Compare, _KeyContainer>;
template <class _KeyContainer, class _Compare = less<typename _KeyContainer::value_type>>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer>
flat_multiset(sorted_equivalent_t, _KeyContainer, _Compare = _Compare())
    -> flat_multiset<typename _KeyContainer::value_type, _Compare, _KeyContainer>;
template <class _KeyContainer, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && uses_allocator_v<_KeyContainer, _Allocator>
flat_multiset(sorted_equivalent_t, _KeyContainer, _Allocator)
    -> flat_multiset<typename _KeyContainer::value_type, less<typename _KeyContainer::value_type>, _KeyContainer>;
template <class _KeyContainer, class _Compare, class _Allocator>
  requires __ycxx::__detail::__flat_container_arg<_KeyContainer> && __ycxx::__detail::__flat_compare_for<_Compare, _KeyContainer> &&
           uses_allocator_v<_KeyContainer, _Allocator>
flat_multiset(sorted_equivalent_t, _KeyContainer, _Compare, _Allocator)
    -> flat_multiset<typename _KeyContainer::value_type, _Compare, _KeyContainer>;
template <class _InputIterator, class _Compare = less<__ycxx::__detail::__iter_value_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__deducible_compare<_Compare>
flat_multiset(_InputIterator, _InputIterator, _Compare = _Compare())
    -> flat_multiset<__ycxx::__detail::__iter_value_type<_InputIterator>, _Compare>;
template <class _InputIterator, class _Compare = less<__ycxx::__detail::__iter_value_type<_InputIterator>>>
  requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator> && __ycxx::__detail::__deducible_compare<_Compare>
flat_multiset(sorted_equivalent_t, _InputIterator, _InputIterator, _Compare = _Compare())
    -> flat_multiset<__ycxx::__detail::__iter_value_type<_InputIterator>, _Compare>;
template <ranges::input_range _Rp, class _Compare = less<ranges::range_value_t<_Rp>>,
          class _Allocator = allocator<ranges::range_value_t<_Rp>>>
  requires __ycxx::__detail::__deducible_compare<_Compare> && __ycxx::__detail::__qualifies_as_allocator<_Allocator>
flat_multiset(from_range_t, _Rp&&, _Compare = _Compare(), _Allocator = _Allocator())
    -> flat_multiset<ranges::range_value_t<_Rp>, _Compare,
                     vector<ranges::range_value_t<_Rp>,
                            __ycxx::__detail::__rebound_alloc<_Allocator, ranges::range_value_t<_Rp>>>>;
template <ranges::input_range _Rp, class _Allocator>
  requires __ycxx::__detail::__qualifies_as_allocator<_Allocator>
flat_multiset(from_range_t, _Rp&&, _Allocator)
    -> flat_multiset<ranges::range_value_t<_Rp>, less<ranges::range_value_t<_Rp>>,
                     vector<ranges::range_value_t<_Rp>,
                            __ycxx::__detail::__rebound_alloc<_Allocator, ranges::range_value_t<_Rp>>>>;
template <class _Key, class _Compare = less<_Key>>
  requires __ycxx::__detail::__deducible_compare<_Compare>
flat_multiset(initializer_list<_Key>, _Compare = _Compare()) -> flat_multiset<_Key, _Compare>;
template <class _Key, class _Compare = less<_Key>>
  requires __ycxx::__detail::__deducible_compare<_Compare>
flat_multiset(sorted_equivalent_t, initializer_list<_Key>, _Compare = _Compare()) -> flat_multiset<_Key, _Compare>;

// ---- uses_allocator ----
template <class _Key, class _Compare, class _KeyContainer, class _Allocator>
struct uses_allocator<flat_set<_Key, _Compare, _KeyContainer>, _Allocator>
    : bool_constant<uses_allocator_v<_KeyContainer, _Allocator>> {};
template <class _Key, class _Compare, class _KeyContainer, class _Allocator>
struct uses_allocator<flat_multiset<_Key, _Compare, _KeyContainer>, _Allocator>
    : bool_constant<uses_allocator_v<_KeyContainer, _Allocator>> {};

// ---- [flat.set.erasure], [flat.multiset.erasure] ----
template <class _Key, class _Compare, class _KeyContainer, class _Predicate>
constexpr typename flat_set<_Key, _Compare, _KeyContainer>::size_type erase_if(flat_set<_Key, _Compare, _KeyContainer>& c,
                                                                            _Predicate pred) {
  // The container is taken out (c is empty if pred throws) and put back.
  _KeyContainer __cont = static_cast<flat_set<_Key, _Compare, _KeyContainer>&&>(c).extract();
  auto test = [&](size_t i) -> bool {
    return static_cast<bool>(pred(static_cast<const _Key&>(__ycxx::__detail::__row_at(__cont, i))));
  };
  const auto n = __ycxx::__detail::__erase_rows_if(test, __cont);
  c.replace(static_cast<_KeyContainer&&>(__cont));
  return static_cast<typename flat_set<_Key, _Compare, _KeyContainer>::size_type>(n);
}
template <class _Key, class _Compare, class _KeyContainer, class _Predicate>
constexpr typename flat_multiset<_Key, _Compare, _KeyContainer>::size_type
erase_if(flat_multiset<_Key, _Compare, _KeyContainer>& c, _Predicate pred) {
  _KeyContainer __cont = static_cast<flat_multiset<_Key, _Compare, _KeyContainer>&&>(c).extract();
  auto test = [&](size_t i) -> bool {
    return static_cast<bool>(pred(static_cast<const _Key&>(__ycxx::__detail::__row_at(__cont, i))));
  };
  const auto n = __ycxx::__detail::__erase_rows_if(test, __cont);
  c.replace(static_cast<_KeyContainer&&>(__cont));
  return static_cast<typename flat_multiset<_Key, _Compare, _KeyContainer>::size_type>(n);
}

} // namespace std
