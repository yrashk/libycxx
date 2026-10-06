// libycxx core: <array>
#pragma once

#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/tuple_like.hpp>
#include <ycxx/core/utility_base.hpp>
#include <ycxx/core/error.hpp>
#include <initializer_list>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// Storage for array<T, 0>: no elements, but alignof(T) and data() support.
template <class _Tp>
struct alignas(_Tp) __empty_array_storage {};
// array<const T, 0> is not assignable, like array<const T, N> for N > 0.
template <class _Tp>
struct alignas(_Tp) __empty_array_storage<const _Tp> {
  __empty_array_storage& operator=(const __empty_array_storage&) = delete;
};
template <class _Tp, std::size_t _Np>
struct __array_storage {
  using type = _Tp[_Np];
};
template <class _Tp>
struct __array_storage<_Tp, 0> {
  using type = __empty_array_storage<_Tp>;
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp, size_t _Np>
struct array {
  using value_type = _Tp;
  using pointer = _Tp*;
  using const_pointer = const _Tp*;
  using reference = _Tp&;
  using const_reference = const _Tp&;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using iterator = _Tp*;
  using const_iterator = const _Tp*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // Public because array is an aggregate ([array.overview]); the reserved name keeps it out of
  // the way of user macros.
  typename __ycxx::__detail::__array_storage<_Tp, _Np>::type __elems;

  constexpr void fill(const _Tp& __u) {
    for (size_t i = 0; i < _Np; ++i)
      data()[i] = __u;
  }
  constexpr void swap(array& a) noexcept(_Np == 0 || is_nothrow_swappable_v<_Tp>) {
    // Not redundant: for N == 0, T need not be swappable, and the loop body must not be instantiated.
    if constexpr (_Np != 0)
      for (size_t i = 0; i < _Np; ++i)
        __ycxx::__detail::__swap_adl::__do_swap(data()[i], a.data()[i]);
  }

  constexpr pointer data() noexcept {
    if constexpr (_Np == 0)
      return nullptr;
    else
      return __elems;
  }
  constexpr const_pointer data() const noexcept {
    if constexpr (_Np == 0)
      return nullptr;
    else
      return __elems;
  }

  constexpr iterator begin() noexcept { return data(); }
  constexpr const_iterator begin() const noexcept { return data(); }
  constexpr iterator end() noexcept { return data() + _Np; }
  constexpr const_iterator end() const noexcept { return data() + _Np; }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  [[nodiscard]] constexpr bool empty() const noexcept { return _Np == 0; }
  constexpr size_type size() const noexcept { return _Np; }
  constexpr size_type max_size() const noexcept { return _Np; }

  constexpr reference operator[](size_type n) {
    __ycxx::__detail::__precondition(n < _Np, "std::array::operator[]: index out of range");
    return data()[n];
  }
  constexpr const_reference operator[](size_type n) const {
    __ycxx::__detail::__precondition(n < _Np, "std::array::operator[]: index out of range");
    return data()[n];
  }
  constexpr reference at(size_type n) {
    if (n >= _Np)
      __ycxx::__detail::__throw_out_of_range("std::array::at: index out of range");
    return data()[n];
  }
  constexpr const_reference at(size_type n) const {
    if (n >= _Np)
      __ycxx::__detail::__throw_out_of_range("std::array::at: index out of range");
    return data()[n];
  }
  constexpr reference front() {
    __ycxx::__detail::__precondition(_Np != 0, "std::array::front: empty array");
    return data()[0];
  }
  constexpr const_reference front() const {
    __ycxx::__detail::__precondition(_Np != 0, "std::array::front: empty array");
    return data()[0];
  }
  constexpr reference back() {
    __ycxx::__detail::__precondition(_Np != 0, "std::array::back: empty array");
    return data()[_Np - 1];
  }
  constexpr const_reference back() const {
    __ycxx::__detail::__precondition(_Np != 0, "std::array::back: empty array");
    return data()[_Np - 1];
  }
};

template <class _Tp, class... _Up>
  requires(is_same_v<_Tp, _Up> && ...)
array(_Tp, _Up...) -> array<_Tp, 1 + sizeof...(_Up)>;

template <class _Tp, size_t _Np>
constexpr bool operator==(const array<_Tp, _Np>& __x, const array<_Tp, _Np>& y) {
  for (size_t i = 0; i < _Np; ++i)
    if (!(__x[i] == y[i]))
      return false;
  return true;
}
template <class _Tp, size_t _Np>
constexpr __ycxx::__detail::__synth_three_way_result<_Tp> operator<=>(const array<_Tp, _Np>& __x, const array<_Tp, _Np>& y) {
  for (size_t i = 0; i < _Np; ++i)
    if (auto c = __ycxx::__detail::__synth_three_way(__x[i], y[i]); c != 0)
      return c;
  return strong_ordering::equal;
}

template <class _Tp, size_t _Np>
  requires(_Np == 0 || is_swappable_v<_Tp>)
constexpr void swap(array<_Tp, _Np>& __x, array<_Tp, _Np>& y) noexcept(noexcept(__x.swap(y))) {
  __x.swap(y);
}

// [array.creation]
template <class _Tp, size_t _Np>
constexpr array<remove_cv_t<_Tp>, _Np> to_array(_Tp (&a)[_Np]) {
  static_assert(!is_array_v<_Tp>, "std::to_array: multidimensional arrays are not supported");
  static_assert(is_constructible_v<remove_cv_t<_Tp>, _Tp&>, "std::to_array: T must be copy constructible");
  return [&]<size_t... _Ip>(index_sequence<_Ip...>) { return array<remove_cv_t<_Tp>, _Np>{{a[_Ip]...}}; }(
      make_index_sequence<_Np>{});
}
template <class _Tp, size_t _Np>
constexpr array<remove_cv_t<_Tp>, _Np> to_array(_Tp (&&a)[_Np]) {
  static_assert(!is_array_v<_Tp>, "std::to_array: multidimensional arrays are not supported");
  static_assert(is_constructible_v<remove_cv_t<_Tp>, _Tp>, "std::to_array: T must be move constructible");
  return [&]<size_t... _Ip>(index_sequence<_Ip...>) {
    return array<remove_cv_t<_Tp>, _Np>{{static_cast<_Tp&&>(a[_Ip])...}};
  }(make_index_sequence<_Np>{});
}

// [array.tuple]
template <class _Tp, size_t _Np>
struct tuple_size<array<_Tp, _Np>> : integral_constant<size_t, _Np> {};
template <size_t _Ip, class _Tp, size_t _Np>
struct tuple_element<_Ip, array<_Tp, _Np>> {
  static_assert(_Ip < _Np, "tuple_element index out of range for std::array");
  using type = _Tp;
};
template <size_t _Ip, class _Tp, size_t _Np>
constexpr _Tp& get(array<_Tp, _Np>& a) noexcept {
  static_assert(_Ip < _Np, "std::get: index out of range for std::array");
  return a.__elems[_Ip];
}
template <size_t _Ip, class _Tp, size_t _Np>
constexpr _Tp&& get(array<_Tp, _Np>&& a) noexcept {
  static_assert(_Ip < _Np, "std::get: index out of range for std::array");
  return static_cast<_Tp&&>(a.__elems[_Ip]);
}
template <size_t _Ip, class _Tp, size_t _Np>
constexpr const _Tp& get(const array<_Tp, _Np>& a) noexcept {
  static_assert(_Ip < _Np, "std::get: index out of range for std::array");
  return a.__elems[_Ip];
}
template <size_t _Ip, class _Tp, size_t _Np>
constexpr const _Tp&& get(const array<_Tp, _Np>&& a) noexcept {
  static_assert(_Ip < _Np, "std::get: index out of range for std::array");
  return static_cast<const _Tp&&>(a.__elems[_Ip]);
}

} // namespace std
