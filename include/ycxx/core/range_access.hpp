// libycxx core: range access CPOs ([range.access]) and the basic range concepts they need.
// Available through both <iterator> and <ranges>.
#pragma once

#include <ycxx/core/iterator_core.hpp>
#include <ycxx/core/memory_base.hpp>

namespace std::ranges {
template <class T>
constexpr bool enable_borrowed_range = false;
template <class T>
constexpr bool disable_sized_range = false;
} // namespace std::ranges

namespace ycxx::detail::range_access {

template <class T>
concept class_or_enum = std::is_class_v<std::remove_cvref_t<T>> || std::is_enum_v<std::remove_cvref_t<T>>;

// An rvalue may only be accessed if the range is borrowed.
template <class T>
concept maybe_borrowed = std::is_lvalue_reference_v<T> || std::ranges::enable_borrowed_range<std::remove_cvref_t<T>>;

template <class T>
concept complete_array_elem = requires { sizeof(std::remove_all_extents_t<std::remove_reference_t<T>>); };

template <class T>
constexpr std::decay_t<T> decay_copy(T&& t) noexcept(std::is_nothrow_convertible_v<T, std::decay_t<T>>) {
  return static_cast<T&&>(t);
}

// ---- begin ----
namespace begin_ns {
void begin(auto&) = delete;
void begin(const auto&) = delete;

template <class T>
concept member = requires(T& t) {
  { decay_copy(t.begin()) } -> std::input_or_output_iterator;
};
template <class T>
concept adl = class_or_enum<T> && requires(T& t) {
  { decay_copy(begin(t)) } -> std::input_or_output_iterator;
};

struct fn {
  template <class T>
    requires maybe_borrowed<T> && (std::is_array_v<std::remove_reference_t<T>> || member<T> || adl<T>)
  [[nodiscard]] constexpr auto operator()(T&& t) const noexcept {
    if constexpr (std::is_array_v<std::remove_reference_t<T>>) {
      static_assert(complete_array_elem<T>, "ranges::begin: array of incomplete type");
      return t + 0;
    } else if constexpr (member<T>) {
      return decay_copy(t.begin());
    } else {
      return decay_copy(begin(t));
    }
  }
};
} // namespace begin_ns

} // namespace ycxx::detail::range_access

namespace std::ranges {
inline namespace cpo {
inline constexpr ycxx::detail::range_access::begin_ns::fn begin{};
}
template <class T>
using iterator_t = decltype(ranges::begin(std::declval<T&>()));
} // namespace std::ranges

namespace ycxx::detail::range_access {

// ---- end ----
namespace end_ns {
void end(auto&) = delete;
void end(const auto&) = delete;

template <class T>
concept member = requires(T& t) {
  typename std::ranges::iterator_t<T>;
  { decay_copy(t.end()) } -> std::sentinel_for<std::ranges::iterator_t<T>>;
};
template <class T>
concept adl = class_or_enum<T> && requires(T& t) {
  typename std::ranges::iterator_t<T>;
  { decay_copy(end(t)) } -> std::sentinel_for<std::ranges::iterator_t<T>>;
};

struct fn {
  template <class T>
    requires maybe_borrowed<T> && (std::is_bounded_array_v<std::remove_reference_t<T>> || member<T> || adl<T>)
  [[nodiscard]] constexpr auto operator()(T&& t) const noexcept {
    if constexpr (std::is_bounded_array_v<std::remove_reference_t<T>>) {
      static_assert(complete_array_elem<T>, "ranges::end: array of incomplete type");
      return t + std::extent_v<std::remove_reference_t<T>>;
    } else if constexpr (member<T>) {
      return decay_copy(t.end());
    } else {
      return decay_copy(end(t));
    }
  }
};
} // namespace end_ns

} // namespace ycxx::detail::range_access

namespace std::ranges {
inline namespace cpo {
inline constexpr ycxx::detail::range_access::end_ns::fn end{};
}

// [range.range]
template <class T>
concept range = requires(T& t) {
  ranges::begin(t);
  ranges::end(t);
};
template <class T>
concept borrowed_range = range<T> && (is_lvalue_reference_v<T> || enable_borrowed_range<remove_cvref_t<T>>);
template <range R>
using sentinel_t = decltype(ranges::end(declval<R&>()));
template <range R>
using range_difference_t = iter_difference_t<iterator_t<R>>;
template <range R>
using range_value_t = iter_value_t<iterator_t<R>>;
template <range R>
using range_reference_t = iter_reference_t<iterator_t<R>>;
template <range R>
using range_rvalue_reference_t = iter_rvalue_reference_t<iterator_t<R>>;
template <range R>
using range_common_reference_t = iter_common_reference_t<iterator_t<R>>;

} // namespace std::ranges

namespace ycxx::detail::range_access {

template <class T>
concept integer_like_ = integer_like<T>;

template <class T>
constexpr auto to_unsigned_like(T t) noexcept {
  return static_cast<std::make_unsigned_t<T>>(t);
}

// ---- size ----
namespace size_ns {
void size(auto&) = delete;
void size(const auto&) = delete;

template <class T>
concept member = !std::ranges::disable_sized_range<std::remove_cvref_t<T>> && requires(T& t) {
  { decay_copy(t.size()) } -> integer_like_;
};
template <class T>
concept adl = !std::ranges::disable_sized_range<std::remove_cvref_t<T>> && class_or_enum<T> && requires(T& t) {
  { decay_copy(size(t)) } -> integer_like_;
};
template <class T>
concept difference = requires(T& t) {
  { std::ranges::begin(t) } -> std::forward_iterator;
  { std::ranges::end(t) } -> std::sized_sentinel_for<decltype(std::ranges::begin(t))>;
};

struct fn {
  template <class T>
    requires std::is_bounded_array_v<std::remove_reference_t<T>> || member<T> || adl<T> || difference<T>
  [[nodiscard]] constexpr auto operator()(T&& t) const noexcept {
    if constexpr (std::is_bounded_array_v<std::remove_reference_t<T>>)
      return decay_copy(std::extent_v<std::remove_reference_t<T>>);
    else if constexpr (member<T>)
      return decay_copy(t.size());
    else if constexpr (adl<T>)
      return decay_copy(size(t));
    else
      return to_unsigned_like(std::ranges::end(t) - std::ranges::begin(t));
  }
};
} // namespace size_ns

} // namespace ycxx::detail::range_access

namespace std::ranges {
inline namespace cpo {
inline constexpr ycxx::detail::range_access::size_ns::fn size{};
}
} // namespace std::ranges

namespace ycxx::detail::range_access {

// ---- ssize ----
struct ssize_fn {
  template <class T>
    requires requires(T&& t) { std::ranges::size(t); }
  [[nodiscard]] constexpr auto operator()(T&& t) const noexcept {
    using size_type = decltype(std::ranges::size(t));
    using signed_type = std::make_signed_t<size_type>;
    using result = std::conditional_t<(sizeof(std::ptrdiff_t) > sizeof(signed_type)), std::ptrdiff_t, signed_type>;
    return static_cast<result>(std::ranges::size(t));
  }
};

// ---- empty ----
namespace empty_ns {
template <class T>
concept member = requires(T& t) { bool(t.empty()); };
template <class T>
concept via_size = requires(T& t) { std::ranges::size(t) == 0; };
template <class T>
concept via_iter = requires(T& t) {
  { std::ranges::begin(t) } -> std::forward_iterator;
  bool(std::ranges::begin(t) == std::ranges::end(t));
};
struct fn {
  template <class T>
    requires member<T> || via_size<T> || via_iter<T>
  [[nodiscard]] constexpr bool operator()(T&& t) const noexcept {
    if constexpr (member<T>)
      return bool(t.empty());
    else if constexpr (via_size<T>)
      return std::ranges::size(t) == 0;
    else
      return bool(std::ranges::begin(t) == std::ranges::end(t));
  }
};
} // namespace empty_ns

// ---- data ----
namespace data_ns {
template <class T>
concept pointer_to_object = std::is_pointer_v<T> && std::is_object_v<std::remove_pointer_t<T>>;
template <class T>
concept member = requires(T& t) {
  { decay_copy(t.data()) } -> pointer_to_object;
};
template <class T>
concept via_begin = requires(T& t) {
  { std::ranges::begin(t) } -> std::contiguous_iterator;
};
struct fn {
  template <class T>
    requires maybe_borrowed<T> && (member<T> || via_begin<T>)
  [[nodiscard]] constexpr auto operator()(T&& t) const noexcept {
    if constexpr (member<T>)
      return decay_copy(t.data());
    else
      return std::to_address(std::ranges::begin(t));
  }
};
} // namespace data_ns

// ---- reserve_hint (C++26) ----
namespace reserve_hint_ns {
void reserve_hint(auto&) = delete;
void reserve_hint(const auto&) = delete;
template <class T>
concept member = requires(T& t) {
  { decay_copy(t.reserve_hint()) } -> integer_like_;
};
template <class T>
concept adl = class_or_enum<T> && requires(T& t) {
  { decay_copy(reserve_hint(t)) } -> integer_like_;
};
struct fn {
  template <class T>
    requires requires(T& t) { std::ranges::size(t); } || member<T> || adl<T>
  [[nodiscard]] constexpr auto operator()(T&& t) const noexcept {
    if constexpr (requires { std::ranges::size(t); })
      return std::ranges::size(t);
    else if constexpr (member<T>)
      return decay_copy(t.reserve_hint());
    else
      return decay_copy(reserve_hint(t));
  }
};
} // namespace reserve_hint_ns

} // namespace ycxx::detail::range_access

namespace std::ranges {
inline namespace cpo {
inline constexpr ycxx::detail::range_access::ssize_fn ssize{};
inline constexpr ycxx::detail::range_access::empty_ns::fn empty{};
inline constexpr ycxx::detail::range_access::data_ns::fn data{};
inline constexpr ycxx::detail::range_access::reserve_hint_ns::fn reserve_hint{};
} // namespace cpo

template <class T>
concept sized_range = range<T> && requires(T& t) { ranges::size(t); };
template <class T>
concept approximately_sized_range = range<T> && requires(T& t) { ranges::reserve_hint(t); };
template <sized_range R>
using range_size_t = decltype(ranges::size(declval<R&>()));

template <class R, class T>
concept output_range = range<R> && output_iterator<iterator_t<R>, T>;
template <class T>
concept input_range = range<T> && input_iterator<iterator_t<T>>;
template <class T>
concept forward_range = input_range<T> && forward_iterator<iterator_t<T>>;
template <class T>
concept bidirectional_range = forward_range<T> && bidirectional_iterator<iterator_t<T>>;
template <class T>
concept random_access_range = bidirectional_range<T> && random_access_iterator<iterator_t<T>>;
template <class T>
concept contiguous_range = random_access_range<T> && contiguous_iterator<iterator_t<T>> && requires(T& t) {
  { ranges::data(t) } -> same_as<add_pointer_t<range_reference_t<T>>>;
};
template <class T>
concept common_range = range<T> && same_as<iterator_t<T>, sentinel_t<T>>;

} // namespace std::ranges

// ---------------------------------------------------------------------------------------------
// [iterator.range] std::begin & co.
// ---------------------------------------------------------------------------------------------
namespace std {

template <class C>
constexpr auto begin(C& c) -> decltype(c.begin()) {
  return c.begin();
}
template <class C>
constexpr auto begin(const C& c) -> decltype(c.begin()) {
  return c.begin();
}
template <class C>
constexpr auto end(C& c) -> decltype(c.end()) {
  return c.end();
}
template <class C>
constexpr auto end(const C& c) -> decltype(c.end()) {
  return c.end();
}
template <class T, size_t N>
constexpr T* begin(T (&a)[N]) noexcept {
  return a;
}
template <class T, size_t N>
constexpr T* end(T (&a)[N]) noexcept {
  return a + N;
}
template <class C>
constexpr auto cbegin(const C& c) noexcept(noexcept(std::begin(c))) -> decltype(std::begin(c)) {
  return std::begin(c);
}
template <class C>
constexpr auto cend(const C& c) noexcept(noexcept(std::end(c))) -> decltype(std::end(c)) {
  return std::end(c);
}
template <class C>
constexpr auto size(const C& c) -> decltype(c.size()) {
  return c.size();
}
template <class T, size_t N>
constexpr size_t size(const T (&)[N]) noexcept {
  return N;
}
template <class C>
constexpr auto ssize(const C& c) -> common_type_t<ptrdiff_t, make_signed_t<decltype(c.size())>> {
  return static_cast<common_type_t<ptrdiff_t, make_signed_t<decltype(c.size())>>>(c.size());
}
template <class T, ptrdiff_t N>
constexpr ptrdiff_t ssize(const T (&)[N]) noexcept {
  return N;
}
template <class C>
[[nodiscard]] constexpr auto empty(const C& c) -> decltype(c.empty()) {
  return c.empty();
}
template <class T, size_t N>
[[nodiscard]] constexpr bool empty(const T (&)[N]) noexcept {
  return false;
}
template <class C>
constexpr auto data(C& c) -> decltype(c.data()) {
  return c.data();
}
template <class C>
constexpr auto data(const C& c) -> decltype(c.data()) {
  return c.data();
}
template <class T, size_t N>
constexpr T* data(T (&a)[N]) noexcept {
  return a;
}
// initializer_list overloads of empty/data are provided generically through il.empty()/il.data().

} // namespace std
