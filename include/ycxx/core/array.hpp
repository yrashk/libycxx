// libycxx core: <array>
#pragma once

#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/tuple_like.hpp>
#include <ycxx/core/utility_base.hpp>
#include <ycxx/core/error.hpp>
#include <initializer_list>

namespace ycxx::detail {
// Storage for array<T, 0>: no elements, but alignof(T) and data() support.
template <class T>
struct alignas(T) empty_array_storage {};
// array<const T, 0> is not assignable, like array<const T, N> for N > 0.
template <class T>
struct alignas(T) empty_array_storage<const T> {
  empty_array_storage& operator=(const empty_array_storage&) = delete;
};
template <class T, std::size_t N>
struct array_storage {
  using type = T[N];
};
template <class T>
struct array_storage<T, 0> {
  using type = empty_array_storage<T>;
};
} // namespace ycxx::detail

namespace std {

template <class T, size_t N>
struct array {
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using iterator = T*;
  using const_iterator = const T*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // Public because array is an aggregate ([array.overview]); the reserved name keeps it out of
  // the way of user macros.
  typename ycxx::detail::array_storage<T, N>::type __elems;

  constexpr void fill(const T& u) {
    for (size_t i = 0; i < N; ++i)
      data()[i] = u;
  }
  constexpr void swap(array& a) noexcept(N == 0 || is_nothrow_swappable_v<T>) {
    // Not redundant: for N == 0, T need not be swappable, and the loop body must not be instantiated.
    if constexpr (N != 0)
      for (size_t i = 0; i < N; ++i)
        ycxx::detail::swap_adl::do_swap(data()[i], a.data()[i]);
  }

  constexpr pointer data() noexcept {
    if constexpr (N == 0)
      return nullptr;
    else
      return __elems;
  }
  constexpr const_pointer data() const noexcept {
    if constexpr (N == 0)
      return nullptr;
    else
      return __elems;
  }

  constexpr iterator begin() noexcept { return data(); }
  constexpr const_iterator begin() const noexcept { return data(); }
  constexpr iterator end() noexcept { return data() + N; }
  constexpr const_iterator end() const noexcept { return data() + N; }
  constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  [[nodiscard]] constexpr bool empty() const noexcept { return N == 0; }
  constexpr size_type size() const noexcept { return N; }
  constexpr size_type max_size() const noexcept { return N; }

  constexpr reference operator[](size_type n) {
    ycxx::detail::precondition(n < N, "std::array::operator[]: index out of range");
    return data()[n];
  }
  constexpr const_reference operator[](size_type n) const {
    ycxx::detail::precondition(n < N, "std::array::operator[]: index out of range");
    return data()[n];
  }
  constexpr reference at(size_type n) {
    if (n >= N)
      ycxx::detail::throw_out_of_range("std::array::at: index out of range");
    return data()[n];
  }
  constexpr const_reference at(size_type n) const {
    if (n >= N)
      ycxx::detail::throw_out_of_range("std::array::at: index out of range");
    return data()[n];
  }
  constexpr reference front() {
    ycxx::detail::precondition(N != 0, "std::array::front: empty array");
    return data()[0];
  }
  constexpr const_reference front() const {
    ycxx::detail::precondition(N != 0, "std::array::front: empty array");
    return data()[0];
  }
  constexpr reference back() {
    ycxx::detail::precondition(N != 0, "std::array::back: empty array");
    return data()[N - 1];
  }
  constexpr const_reference back() const {
    ycxx::detail::precondition(N != 0, "std::array::back: empty array");
    return data()[N - 1];
  }
};

template <class T, class... U>
  requires(is_same_v<T, U> && ...)
array(T, U...) -> array<T, 1 + sizeof...(U)>;

template <class T, size_t N>
constexpr bool operator==(const array<T, N>& x, const array<T, N>& y) {
  for (size_t i = 0; i < N; ++i)
    if (!(x[i] == y[i]))
      return false;
  return true;
}
template <class T, size_t N>
constexpr ycxx::detail::synth_three_way_result<T> operator<=>(const array<T, N>& x, const array<T, N>& y) {
  for (size_t i = 0; i < N; ++i)
    if (auto c = ycxx::detail::synth_three_way(x[i], y[i]); c != 0)
      return c;
  return strong_ordering::equal;
}

template <class T, size_t N>
  requires(N == 0 || is_swappable_v<T>)
constexpr void swap(array<T, N>& x, array<T, N>& y) noexcept(noexcept(x.swap(y))) {
  x.swap(y);
}

// [array.creation]
template <class T, size_t N>
constexpr array<remove_cv_t<T>, N> to_array(T (&a)[N]) {
  static_assert(!is_array_v<T>, "std::to_array: multidimensional arrays are not supported");
  static_assert(is_constructible_v<remove_cv_t<T>, T&>, "std::to_array: T must be copy constructible");
  return [&]<size_t... I>(index_sequence<I...>) { return array<remove_cv_t<T>, N>{{a[I]...}}; }(
      make_index_sequence<N>{});
}
template <class T, size_t N>
constexpr array<remove_cv_t<T>, N> to_array(T (&&a)[N]) {
  static_assert(!is_array_v<T>, "std::to_array: multidimensional arrays are not supported");
  static_assert(is_constructible_v<remove_cv_t<T>, T>, "std::to_array: T must be move constructible");
  return [&]<size_t... I>(index_sequence<I...>) {
    return array<remove_cv_t<T>, N>{{static_cast<T&&>(a[I])...}};
  }(make_index_sequence<N>{});
}

// [array.tuple]
template <class T, size_t N>
struct tuple_size<array<T, N>> : integral_constant<size_t, N> {};
template <size_t I, class T, size_t N>
struct tuple_element<I, array<T, N>> {
  static_assert(I < N, "tuple_element index out of range for std::array");
  using type = T;
};
template <size_t I, class T, size_t N>
constexpr T& get(array<T, N>& a) noexcept {
  static_assert(I < N, "std::get: index out of range for std::array");
  return a.__elems[I];
}
template <size_t I, class T, size_t N>
constexpr T&& get(array<T, N>&& a) noexcept {
  static_assert(I < N, "std::get: index out of range for std::array");
  return static_cast<T&&>(a.__elems[I]);
}
template <size_t I, class T, size_t N>
constexpr const T& get(const array<T, N>& a) noexcept {
  static_assert(I < N, "std::get: index out of range for std::array");
  return a.__elems[I];
}
template <size_t I, class T, size_t N>
constexpr const T&& get(const array<T, N>&& a) noexcept {
  static_assert(I < N, "std::get: index out of range for std::array");
  return static_cast<const T&&>(a.__elems[I]);
}

} // namespace std
