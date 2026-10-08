// libycxx core: inplace_vector ([inplace.vector]) and its erasure functions.
//
// Representation: the elements live in an anonymous union `{ _Tp __d_[_Np]; ... }` (so no element
// is constructed until it is inserted), followed by the size in the smallest unsigned type
// that can hold N. For N == 0 the storage is an empty class. All of it lives in the base
// __ycxx::__adl_free::__iv_storage, whose special members are defaulted (trivial) exactly when
// [inplace.vector.overview]/5 asks for a trivial special member of inplace_vector, which then
// declares none of its own.
//
// Constant evaluation: beginning the lifetime of one element of an array member of a union
// (P3074, C++26) works on GCC 16 but not on Clang 23 (probed in-language below). So:
//  - for a trivially destructible, default-constructible T the whole array is
//    value-initialized when an inplace_vector is created during constant evaluation; the
//    elements are then reconstructed in place. This also lets a constant-initialized
//    inplace_vector of such a type hold elements.
//  - on a compiler without P3074, an inplace_vector of a non-trivially-destructible T keeps
//    its elements in storage from std::allocator during constant evaluation (a second union
//    member holds the pointer); they never outlive that evaluation.
//  - other element types (trivially destructible but not default-constructible) cannot be
//    used in constant evaluation on such a compiler.
#pragma once

#include <initializer_list>
#include <ycxx/core/container_base.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/memory_base.hpp>
#include <ycxx/core/optional.hpp>
#include <ycxx/core/sequence_support.hpp>
#include <ycxx/core/swap.hpp>
#include <ycxx/core/utility_base.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// Probe: can construct_at begin the lifetime of one element of an array that is a union
// member with no active member, during constant evaluation (P3074)?
namespace __iv_probe {
struct __elem {
  int __v;
  constexpr __elem(int __x) : __v(__x) {}
  constexpr ~__elem() {}
};
template <class _Tp>
union __holder {
  _Tp d[2];
  constexpr __holder() {}
  constexpr ~__holder() {}
};
template <class _Tp>
constexpr bool run() {
  __holder<_Tp> h;
  std::construct_at(h.d + 0, 1);
  const bool r = h.d[0].__v == 1;
  std::destroy_at(h.d + 0);
  return r;
}
template <class _Tp>
concept __works = requires { typename std::bool_constant<run<_Tp>()>; };
} // namespace iv_probe
inline constexpr bool __union_array_lifetime = __iv_probe::__works<__iv_probe::__elem>;

template <std::size_t _Np>
using __iv_size_t = std::conditional_t<
    (_Np <= 0xffu), unsigned char,
    std::conditional_t<(_Np <= 0xffffu), unsigned short, std::conditional_t<(_Np <= 0xffffffffu), unsigned, std::size_t>>>;

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __adl_free {

template <class _Tp, std::size_t _Np, bool = (_Np == 0)>
struct __iv_storage;

// N == 0: empty and trivial ([inplace.vector.overview]/5).
template <class _Tp, std::size_t _Np>
struct __iv_storage<_Tp, _Np, true> {
  static constexpr _Tp* __iv_data() noexcept { return nullptr; }
  static constexpr std::size_t __iv_size() noexcept { return 0; }
  static constexpr void __iv_set_size(std::size_t) noexcept {}
  static constexpr void __iv_prepare() noexcept {}
};

template <class _Tp, std::size_t _Np>
struct __iv_storage<_Tp, _Np, false> {
  using size_type = __ycxx::__detail::__iv_size_t<_Np>;
  // See the header comment.
  static constexpr bool __prefill = std::is_trivially_destructible_v<_Tp> && std::is_default_constructible_v<_Tp>;
  static constexpr bool __heap = !__ycxx::__detail::__union_array_lifetime && !std::is_trivially_destructible_v<_Tp>;

  union {
    _Tp __d_[_Np];
    std::conditional_t<__heap, _Tp*, unsigned char> __cx_;
  };
  size_type __n_ = 0;

  constexpr _Tp* __iv_data() noexcept {
    if constexpr (__heap) {
      if consteval {
        return __cx_;
      }
    }
    return __d_;
  }
  constexpr const _Tp* __iv_data() const noexcept {
    if constexpr (__heap) {
      if consteval {
        return __cx_;
      }
    }
    return __d_;
  }
  constexpr std::size_t __iv_size() const noexcept { return __n_; }
  constexpr void __iv_set_size(std::size_t n) noexcept { __n_ = static_cast<size_type>(n); }
  // Called before elements are constructed.
  constexpr void __iv_prepare() {
    if constexpr (__heap) {
      if consteval {
        if (!__cx_)
          __cx_ = std::allocator<_Tp>().allocate(_Np);
      }
    }
  }
  constexpr void __iv_destroy_from(std::size_t k) noexcept {
    _Tp* const p = __iv_data();
    for (std::size_t i = k; i != __n_; ++i)
      std::destroy_at(p + i);
    __n_ = static_cast<size_type>(k);
  }
  template <class _Src>
  constexpr void __iv_construct_from(_Src* __q, std::size_t n) {
    __iv_prepare();
    _Tp* const p = __iv_data();
    for (std::size_t i = __n_; i != n; ++i) {
      if constexpr (std::is_const_v<_Src>)
        std::construct_at(p + i, __q[i]);
      else
        std::construct_at(p + i, static_cast<_Tp&&>(__q[i]));
      ++__n_;
    }
  }

  constexpr __iv_storage() noexcept {
    if consteval {
      if constexpr (__heap)
        __cx_ = nullptr;
      else if constexpr (__prefill)
        std::construct_at(__builtin_addressof(__d_));
    }
  }

  constexpr __iv_storage(const __iv_storage&)
    requires std::is_trivially_copy_constructible_v<_Tp>
  = default;
  // (The copy operations' noexcept is a strengthening; the draft specifies none.)
  constexpr __iv_storage(const __iv_storage& __o) noexcept(std::is_nothrow_copy_constructible_v<_Tp>) : __iv_storage() {
    __iv_construct_from(__o.__iv_data(), __o.__n_);
  }

  constexpr __iv_storage(__iv_storage&&)
    requires std::is_trivially_move_constructible_v<_Tp>
  = default;
  constexpr __iv_storage(__iv_storage&& __o) noexcept(std::is_nothrow_move_constructible_v<_Tp>) : __iv_storage() {
    __iv_construct_from(__o.__iv_data(), __o.__n_);
  }

  constexpr __iv_storage& operator=(const __iv_storage&)
    requires std::is_trivially_destructible_v<_Tp> && std::is_trivially_copy_constructible_v<_Tp> &&
                 std::is_trivially_copy_assignable_v<_Tp>
  = default;
  constexpr __iv_storage& operator=(const __iv_storage& __o) noexcept(std::is_nothrow_copy_constructible_v<_Tp> &&
                                                                std::is_nothrow_copy_assignable_v<_Tp>) {
    if (this != __builtin_addressof(__o)) {
      const _Tp* const __q = __o.__iv_data();
      _Tp* const p = __iv_data();
      const std::size_t common = __n_ < __o.__n_ ? __n_ : __o.__n_;
      for (std::size_t i = 0; i != common; ++i)
        p[i] = __q[i];
      if (__o.__n_ > __n_)
        __iv_construct_from(__q, __o.__n_);
      else
        __iv_destroy_from(__o.__n_);
    }
    return *this;
  }

  constexpr __iv_storage& operator=(__iv_storage&&)
    requires std::is_trivially_destructible_v<_Tp> && std::is_trivially_move_constructible_v<_Tp> &&
                 std::is_trivially_move_assignable_v<_Tp>
  = default;
  constexpr __iv_storage& operator=(__iv_storage&& __o) noexcept(std::is_nothrow_move_assignable_v<_Tp> &&
                                                           std::is_nothrow_move_constructible_v<_Tp>) {
    if (this != __builtin_addressof(__o)) {
      _Tp* const __q = __o.__iv_data();
      _Tp* const p = __iv_data();
      const std::size_t common = __n_ < __o.__n_ ? __n_ : __o.__n_;
      for (std::size_t i = 0; i != common; ++i)
        p[i] = static_cast<_Tp&&>(__q[i]);
      if (__o.__n_ > __n_)
        __iv_construct_from(__q, __o.__n_);
      else
        __iv_destroy_from(__o.__n_);
    }
    return *this;
  }

  constexpr ~__iv_storage()
    requires std::is_trivially_destructible_v<_Tp>
  = default;
  constexpr ~__iv_storage() {
    __iv_destroy_from(0);
    if constexpr (__heap) {
      if consteval {
        if (__cx_)
          std::allocator<_Tp>().deallocate(__cx_, _Np);
      }
    }
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp, size_t _Np>
class inplace_vector : __ycxx::__adl_free::__iv_storage<_Tp, _Np> {
  using base = __ycxx::__adl_free::__iv_storage<_Tp, _Np>;

public:
  // ---- types ----
  using value_type = _Tp;
  using pointer = _Tp*;
  using const_pointer = const _Tp*;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using iterator = __ycxx::__adl_free::__contiguous_iter<_Tp, inplace_vector, difference_type>;
  using const_iterator = __ycxx::__adl_free::__contiguous_iter<const _Tp, inplace_vector, difference_type>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
  static constexpr void __check_room(size_type __have, size_type add) {
    if (add > _Np - __have)
      __ycxx::__detail::__throw_bad_alloc();
  }
  // Appends one element; size() < N.
  template <class... _Args>
  constexpr _Tp& __construct_back(_Args&&... __args) {
    if constexpr (_Np == 0) {
      __ycxx::__detail::__throw_bad_alloc(); // unreachable: callers check the room first
    } else {
      this->__iv_prepare();
      _Tp* const p = this->__iv_data() + this->__iv_size();
      std::construct_at(p, static_cast<_Args&&>(__args)...);
      this->__iv_set_size(this->__iv_size() + 1);
      return *p;
    }
  }
  // Removes the elements appended since `__old` unless released (an exception unwinds past it).
  struct __append_guard {
    inplace_vector& __v;
    size_type __old;
    constexpr ~__append_guard() {
      if (__old != size_type(-1))
        __v.__truncate(__old);
    }
  };
  constexpr void __truncate(size_type n) noexcept {
    if constexpr (_Np != 0)
      this->__iv_destroy_from(n);
  }

  // Appends the n elements of [first, last), or nothing when they do not fit.
  template <class _It, class _Sent>
  constexpr void __append_counted(_It first, _Sent last, size_type n) {
    __check_room(size(), n);
    (void)last;
    __append_guard __g{*this, size()};
    for (size_type i = 0; i != n; ++i) {
      __construct_back(*first);
      ++first;
    }
    __g.__old = size_type(-1);
  }
  // Appends the elements of a single-pass sequence; bad_alloc (with no effects) when they do
  // not fit.
  template <class _It, class _Sent>
  constexpr void __append_input(_It first, _Sent last) {
    __append_guard __g{*this, size()};
    for (; first != last; ++first) {
      __check_room(size(), 1);
      __construct_back(*first);
    }
    __g.__old = size_type(-1);
  }
  // Rotates the elements appended since index `__old` to index off.
  constexpr iterator __move_into_place(size_type __off, size_type __old) {
    _Tp* const p = data();
    __ycxx::__detail::__rotate_elements<__ycxx::__detail::__plain_temp<_Tp>>(p + __off, p + __old, p + size());
    return begin() + static_cast<difference_type>(__off);
  }
  template <class _It, class _Sent>
  constexpr void __assign_counted(_It first, _Sent last, size_type n) {
    __check_room(0, n);
    _Tp* const p = data();
    const size_type __sz = size();
    const size_type common = n < __sz ? n : __sz;
    for (size_type i = 0; i != common; ++i) {
      p[i] = *first;
      ++first;
    }
    if (n <= __sz)
      __truncate(n);
    else
      __append_counted(static_cast<_It&&>(first), static_cast<_Sent&&>(last), n - __sz);
  }
  template <class _It, class _Sent>
  constexpr void __assign_input(_It first, _Sent last) {
    _Tp* const p = data();
    size_type i = 0;
    for (; i != size() && first != last; ++first) {
      p[i] = *first;
      ++i;
    }
    if (i != size())
      __truncate(i);
    else
      __append_input(static_cast<_It&&>(first), static_cast<_Sent&&>(last));
  }
  template <class _Rp>
  static constexpr bool __counted_range = ranges::forward_range<_Rp> || ranges::sized_range<_Rp>;
  // [inplace.vector.cons]/9: Mandates: ranges::size(rg) <= N when it is a constant expression.
  template <class _Rp>
  static constexpr void __check_constant_size(_Rp& __rg) {
    if constexpr (requires { typename integral_constant<size_t, static_cast<size_t>(ranges::size(__rg))>; })
      static_assert(static_cast<size_t>(ranges::size(__rg)) <= _Np,
                    "std::inplace_vector(from_range_t, R&&): the range has more than N elements");
  }
  template <class _Rp>
  constexpr void __append_range_impl(_Rp& __rg) {
    if constexpr (__counted_range<_Rp>) {
      const auto n = ranges::distance(__rg);
      __append_counted(ranges::begin(__rg), ranges::end(__rg), static_cast<size_type>(n));
    } else {
      __append_input(ranges::begin(__rg), ranges::end(__rg));
    }
  }

public:
  // ---- [inplace.vector.cons] ----
  constexpr inplace_vector() noexcept = default;
  constexpr explicit inplace_vector(size_type n) {
    __check_room(0, n);
    for (size_type i = 0; i != n; ++i)
      __construct_back();
  }
  constexpr inplace_vector(size_type n, const _Tp& value) {
    __check_room(0, n);
    for (size_type i = 0; i != n; ++i)
      __construct_back(value);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr inplace_vector(_InputIterator first, _InputIterator last) {
    if constexpr (__ycxx::__detail::__multipass_iterator<_InputIterator>) {
      const auto n = __ycxx::__detail::__iter_pair_distance(first, last);
      __append_counted(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last),
                     static_cast<size_type>(n));
    } else {
      __append_input(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
    }
  }
  template <__ycxx::__detail::__from_range_tag _Tag, __ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr inplace_vector(_Tag, _Rp&& __rg) {
    __check_constant_size(__rg);
    __append_range_impl(__rg);
  }
  constexpr inplace_vector(initializer_list<_Tp> il) { __append_counted(il.begin(), il.end(), il.size()); }
  // Copy and move construction and assignment and the destructor come from the base.

  constexpr inplace_vector& operator=(initializer_list<_Tp> il) {
    __assign_counted(il.begin(), il.end(), il.size());
    return *this;
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr void assign(_InputIterator first, _InputIterator last) {
    if constexpr (__ycxx::__detail::__multipass_iterator<_InputIterator>) {
      const auto n = __ycxx::__detail::__iter_pair_distance(first, last);
      __assign_counted(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last),
                     static_cast<size_type>(n));
    } else {
      __assign_input(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
    }
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr void assign_range(_Rp&& __rg) {
    static_assert(assignable_from<_Tp&, ranges::range_reference_t<_Rp>>,
                  "std::inplace_vector::assign_range: T& must be assignable from the range's reference type");
    if constexpr (__counted_range<_Rp>) {
      const auto n = ranges::distance(__rg);
      __assign_counted(ranges::begin(__rg), ranges::end(__rg), static_cast<size_type>(n));
    } else {
      __assign_input(ranges::begin(__rg), ranges::end(__rg));
    }
  }
  constexpr void assign(size_type n, const _Tp& __u) {
    __check_room(0, n);
    _Tp* const p = data();
    const size_type __sz = size();
    const size_type common = n < __sz ? n : __sz;
    for (size_type i = 0; i != common; ++i)
      p[i] = __u;
    if (n <= __sz) {
      __truncate(n);
    } else {
      // u may be an element; it is not touched by the appends.
      __append_guard __g{*this, __sz};
      for (size_type i = __sz; i != n; ++i)
        __construct_back(__u);
      __g.__old = size_type(-1);
    }
  }
  constexpr void assign(initializer_list<_Tp> il) { __assign_counted(il.begin(), il.end(), il.size()); }

  // ---- iterators ----
  constexpr iterator begin() noexcept { return iterator(data()); }
  constexpr const_iterator begin() const noexcept { return const_iterator(data()); }
  constexpr iterator end() noexcept { return iterator(data() + size()); }
  constexpr const_iterator end() const noexcept { return const_iterator(data() + size()); }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [inplace.vector.capacity] ----
  [[nodiscard]] constexpr bool empty() const noexcept { return this->__iv_size() == 0; }
  constexpr size_type size() const noexcept { return this->__iv_size(); }
  static constexpr size_type max_size() noexcept { return _Np; }
  static constexpr size_type capacity() noexcept { return _Np; }
  constexpr void resize(size_type __sz) {
    const size_type cur = size();
    if (__sz <= cur) {
      __truncate(__sz);
      return;
    }
    __check_room(0, __sz);
    __append_guard __g{*this, cur};
    for (size_type i = cur; i != __sz; ++i)
      __construct_back();
    __g.__old = size_type(-1);
  }
  constexpr void resize(size_type __sz, const _Tp& c) {
    const size_type cur = size();
    if (__sz <= cur) {
      __truncate(__sz);
      return;
    }
    __check_room(0, __sz);
    __append_guard __g{*this, cur};
    for (size_type i = cur; i != __sz; ++i)
      __construct_back(c);
    __g.__old = size_type(-1);
  }
  static constexpr void reserve(size_type n) {
    if (n > _Np)
      __ycxx::__detail::__throw_bad_alloc();
  }
  static constexpr void shrink_to_fit() noexcept {}

  // ---- element access ----
  constexpr reference operator[](size_type n) {
    __ycxx::__detail::__precondition(n < size(), "std::inplace_vector::operator[]: index out of range");
    return data()[n];
  }
  constexpr const_reference operator[](size_type n) const {
    __ycxx::__detail::__precondition(n < size(), "std::inplace_vector::operator[]: index out of range");
    return data()[n];
  }
  constexpr reference at(size_type n) {
    if (n >= size())
      __ycxx::__detail::__throw_out_of_range("std::inplace_vector::at: index out of range");
    return data()[n];
  }
  constexpr const_reference at(size_type n) const {
    if (n >= size())
      __ycxx::__detail::__throw_out_of_range("std::inplace_vector::at: index out of range");
    return data()[n];
  }
  constexpr reference front() {
    __ycxx::__detail::__precondition(size() != 0, "std::inplace_vector::front: empty inplace_vector");
    return data()[0];
  }
  constexpr const_reference front() const {
    __ycxx::__detail::__precondition(size() != 0, "std::inplace_vector::front: empty inplace_vector");
    return data()[0];
  }
  constexpr reference back() {
    __ycxx::__detail::__precondition(size() != 0, "std::inplace_vector::back: empty inplace_vector");
    return data()[size() - 1];
  }
  constexpr const_reference back() const {
    __ycxx::__detail::__precondition(size() != 0, "std::inplace_vector::back: empty inplace_vector");
    return data()[size() - 1];
  }

  // ---- [inplace.vector.data] ----
  constexpr _Tp* data() noexcept { return this->__iv_data(); }
  constexpr const _Tp* data() const noexcept { return this->__iv_data(); }

  // ---- [inplace.vector.modifiers] ----
  template <class... _Args>
  constexpr reference emplace_back(_Args&&... __args) {
    __check_room(size(), 1);
    return __construct_back(static_cast<_Args&&>(__args)...);
  }
  constexpr reference push_back(const _Tp& __x) { return emplace_back(__x); }
  constexpr reference push_back(_Tp&& __x) { return emplace_back(static_cast<_Tp&&>(__x)); }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr void append_range(_Rp&& __rg) {
    __append_range_impl(__rg);
  }
  constexpr void pop_back() {
    __ycxx::__detail::__precondition(size() != 0, "std::inplace_vector::pop_back: empty inplace_vector");
    __truncate(size() - 1);
  }

  template <class... _Args>
  constexpr optional<reference> try_emplace_back(_Args&&... __args) {
    if (size() == _Np)
      return nullopt;
    return optional<reference>(in_place, __construct_back(static_cast<_Args&&>(__args)...));
  }
  constexpr optional<reference> try_push_back(const _Tp& __x) { return try_emplace_back(__x); }
  constexpr optional<reference> try_push_back(_Tp&& __x) { return try_emplace_back(static_cast<_Tp&&>(__x)); }
  template <class... _Args>
  constexpr reference unchecked_emplace_back(_Args&&... __args) {
    __ycxx::__detail::__precondition(size() < _Np, "std::inplace_vector::unchecked_emplace_back: inplace_vector is full");
    return __construct_back(static_cast<_Args&&>(__args)...);
  }
  constexpr reference unchecked_push_back(const _Tp& __x) { return unchecked_emplace_back(__x); }
  constexpr reference unchecked_push_back(_Tp&& __x) { return unchecked_emplace_back(static_cast<_Tp&&>(__x)); }

  // Insertions construct the new elements at the end (so arguments that refer to elements stay
  // intact and an exception leaves begin() + [0, size()) as it was), then rotate them into place.
  template <class... _Args>
  constexpr iterator emplace(const_iterator position, _Args&&... __args) {
    const size_type __off = static_cast<size_type>(position - cbegin());
    __check_room(size(), 1);
    const size_type __old = size();
    __construct_back(static_cast<_Args&&>(__args)...);
    return __move_into_place(__off, __old);
  }
  constexpr iterator insert(const_iterator position, const _Tp& __x) { return emplace(position, __x); }
  constexpr iterator insert(const_iterator position, _Tp&& __x) { return emplace(position, static_cast<_Tp&&>(__x)); }
  constexpr iterator insert(const_iterator position, size_type n, const _Tp& __x) {
    const size_type __off = static_cast<size_type>(position - cbegin());
    __check_room(size(), n);
    const size_type __old = size();
    __append_guard __g{*this, __old};
    for (size_type i = 0; i != n; ++i)
      __construct_back(__x);
    __g.__old = size_type(-1);
    return __move_into_place(__off, __old);
  }
  template <class _InputIterator>
    requires __ycxx::__detail::__qualifies_as_input_iterator<_InputIterator>
  constexpr iterator insert(const_iterator position, _InputIterator first, _InputIterator last) {
    const size_type __off = static_cast<size_type>(position - cbegin());
    const size_type __old = size();
    if constexpr (__ycxx::__detail::__multipass_iterator<_InputIterator>) {
      const auto n = __ycxx::__detail::__iter_pair_distance(first, last);
      __append_counted(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last),
                     static_cast<size_type>(n));
    } else {
      __append_input(static_cast<_InputIterator&&>(first), static_cast<_InputIterator&&>(last));
    }
    return __move_into_place(__off, __old);
  }
  template <__ycxx::__detail::__container_compatible_range<_Tp> _Rp>
  constexpr iterator insert_range(const_iterator position, _Rp&& __rg) {
    const size_type __off = static_cast<size_type>(position - cbegin());
    const size_type __old = size();
    __append_range_impl(__rg);
    return __move_into_place(__off, __old);
  }
  constexpr iterator insert(const_iterator position, initializer_list<_Tp> il) {
    const size_type __off = static_cast<size_type>(position - cbegin());
    const size_type __old = size();
    __append_counted(il.begin(), il.end(), il.size());
    return __move_into_place(__off, __old);
  }

  constexpr iterator erase(const_iterator position) {
    __ycxx::__detail::__precondition(position != cend(), "std::inplace_vector::erase: iterator not dereferenceable");
    return erase(position, position + 1);
  }
  constexpr iterator erase(const_iterator first, const_iterator last) {
    const size_type p = static_cast<size_type>(first - cbegin());
    const size_type __q = static_cast<size_type>(last - cbegin());
    if (p != __q) {
      _Tp* const d = data();
      const size_type __sz = size();
      for (size_type i = __q; i != __sz; ++i)
        d[p + (i - __q)] = static_cast<_Tp&&>(d[i]);
      __truncate(__sz - (__q - p));
    }
    return begin() + static_cast<difference_type>(p);
  }
  constexpr void swap(inplace_vector& __x) noexcept(_Np == 0 ||
                                                  (is_nothrow_swappable_v<_Tp> && is_nothrow_move_constructible_v<_Tp>)) {
    if constexpr (_Np != 0) {
      if (this == __builtin_addressof(__x))
        return;
      inplace_vector& __lng = size() < __x.size() ? __x : *this;
      inplace_vector& __shrt = size() < __x.size() ? *this : __x;
      _Tp* const a = __shrt.data();
      _Tp* const b = __lng.data();
      const size_type m = __shrt.size();
      for (size_type i = 0; i != m; ++i)
        __ycxx::__detail::__swap_adl::__do_swap(a[i], b[i]);
      for (size_type i = m; i != __lng.size(); ++i)
        __shrt.__construct_back(static_cast<_Tp&&>(b[i]));
      __lng.__truncate(m);
    }
  }
  constexpr void clear() noexcept { __truncate(0); }

  friend constexpr bool operator==(const inplace_vector& __x, const inplace_vector& y) {
    const size_type n = __x.size();
    if (n != y.size())
      return false;
    const _Tp* const a = __x.data();
    const _Tp* const b = y.data();
    for (size_type i = 0; i != n; ++i)
      if (!static_cast<bool>(a[i] == b[i]))
        return false;
    return true;
  }
  friend constexpr auto operator<=>(const inplace_vector& __x, const inplace_vector& y)
    requires requires(const _Tp t) { __ycxx::__detail::__synth_three_way(t, t); }
  {
    using _Rp = __ycxx::__detail::__synth_three_way_result<_Tp>;
    const size_type __nx = __x.size();
    const size_type __ny = y.size();
    const size_type n = __nx < __ny ? __nx : __ny;
    const _Tp* const a = __x.data();
    const _Tp* const b = y.data();
    for (size_type i = 0; i != n; ++i)
      if (auto c = __ycxx::__detail::__synth_three_way(a[i], b[i]); c != 0)
        return static_cast<_Rp>(c);
    return static_cast<_Rp>(__nx <=> __ny);
  }
  friend constexpr void swap(inplace_vector& __x,
                             inplace_vector& y) noexcept(_Np == 0 || (is_nothrow_swappable_v<_Tp> &&
                                                                    is_nothrow_move_constructible_v<_Tp>)) {
    __x.swap(y);
  }
};

// ---- [inplace.vector.erasure] ----
template <class _Tp, size_t _Np, class _Predicate>
constexpr typename inplace_vector<_Tp, _Np>::size_type erase_if(inplace_vector<_Tp, _Np>& c, _Predicate pred) {
  _Tp* const base = c.data();
  _Tp* first = base;
  _Tp* const last = base + c.size();
  while (first != last && !static_cast<bool>(pred(*first)))
    ++first;
  _Tp* out = first;
  if (first != last) {
    for (++first; first != last; ++first)
      if (!static_cast<bool>(pred(*first))) {
        *out = static_cast<_Tp&&>(*first);
        ++out;
      }
  }
  const auto r = static_cast<typename inplace_vector<_Tp, _Np>::size_type>(last - out);
  c.erase(c.begin() + (out - base), c.end());
  return r;
}
template <class _Tp, size_t _Np, class _Up = _Tp>
constexpr typename inplace_vector<_Tp, _Np>::size_type erase(inplace_vector<_Tp, _Np>& c, const _Up& value) {
  return std::erase_if(c, [&value](const _Tp& e) { return static_cast<bool>(e == value); });
}

}} // namespace std
