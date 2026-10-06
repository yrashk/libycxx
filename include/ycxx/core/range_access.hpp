// libycxx core: range access CPOs ([range.access]) and the basic range concepts they need.
// Available through both <iterator> and <ranges>.
#pragma once

#include <ycxx/core/iterator_core.hpp>
#include <ycxx/core/memory_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
template <class _Tp>
constexpr bool enable_borrowed_range = false;
template <class _Tp>
constexpr bool disable_sized_range = false;
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// Helpers of the range-access CPOs. They live here, not in the CPOs' namespaces, and are always
// called qualified: a CPO object's namespace is an associated namespace of its type, so it must
// declare nothing ADL could find, and an unqualified call could reach a user's decay_copy.
// The CPOs themselves write the draft's auto(x), not decay_copy(x): a prvalue x is not moved, so
// their noexcept does not count a move ([range.access.begin]/2.3).
template <class _Tp>
constexpr std::decay_t<_Tp> __decay_copy(_Tp&& t) noexcept(std::is_nothrow_convertible_v<_Tp, std::decay_t<_Tp>>) {
  return static_cast<_Tp&&>(t);
}
template <class _Tp>
constexpr auto __to_unsigned_like(_Tp t) noexcept {
  return static_cast<std::make_unsigned_t<_Tp>>(t);
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__range_access {

template <class _Tp>
concept __class_or_enum = std::is_class_v<std::remove_cvref_t<_Tp>> || std::is_union_v<std::remove_cvref_t<_Tp>> ||
                        std::is_enum_v<std::remove_cvref_t<_Tp>>;

// An rvalue may only be accessed if the range is borrowed.
template <class _Tp>
concept __maybe_borrowed = std::is_lvalue_reference_v<_Tp> || std::ranges::enable_borrowed_range<std::remove_cvref_t<_Tp>>;

template <class _Tp>
concept __complete_array_elem = requires { sizeof(std::remove_all_extents_t<std::remove_reference_t<_Tp>>); };


// ---- begin ----
namespace __begin_ns {
void begin() = delete; // hides outer declarations: the call below uses argument-dependent lookup only

template <class _Tp>
concept __member = requires(_Tp& t) {
  { auto(t.begin()) } -> std::input_or_output_iterator;
};
template <class _Tp>
concept __adl = __class_or_enum<_Tp> && requires(_Tp& t) {
  { auto(begin(t)) } -> std::input_or_output_iterator;
};

struct __fn {
  template <class _Tp>
  static consteval bool nothrow() {
    if constexpr (std::is_array_v<std::remove_reference_t<_Tp>>)
      return true;
    else if constexpr (__member<_Tp>)
      return noexcept(auto(std::declval<_Tp&>().begin()));
    else
      return noexcept(auto(begin(std::declval<_Tp&>())));
  }
  template <class _Tp>
    requires __maybe_borrowed<_Tp> && (std::is_array_v<std::remove_reference_t<_Tp>> || __member<_Tp> || __adl<_Tp>)
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const noexcept(nothrow<_Tp>()) {
    if constexpr (std::is_array_v<std::remove_reference_t<_Tp>>) {
      static_assert(__complete_array_elem<_Tp>, "ranges::begin: array of incomplete type");
      return t + 0;
    } else if constexpr (__member<_Tp>) {
      return t.begin(); // decay-copy: the auto return type decays, and a prvalue is not moved
    } else {
      return begin(t);
    }
  }
};
} // namespace begin_ns

}} // namespace __ycxx::__detail::__range_access

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline namespace cpo {
inline constexpr __ycxx::__detail::__range_access::__begin_ns::__fn begin{};
}
template <class _Tp>
using iterator_t = decltype(ranges::begin(std::declval<_Tp&>()));
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__range_access {

// ---- end ----
namespace __end_ns {
void end() = delete; // hides outer declarations: the call below uses argument-dependent lookup only

template <class _Tp>
concept __member = requires(_Tp& t) {
  typename std::ranges::iterator_t<_Tp>;
  { auto(t.end()) } -> std::sentinel_for<std::ranges::iterator_t<_Tp>>;
};
template <class _Tp>
concept __adl = __class_or_enum<_Tp> && requires(_Tp& t) {
  typename std::ranges::iterator_t<_Tp>;
  { auto(end(t)) } -> std::sentinel_for<std::ranges::iterator_t<_Tp>>;
};

struct __fn {
  template <class _Tp>
  static consteval bool nothrow() {
    if constexpr (std::is_bounded_array_v<std::remove_reference_t<_Tp>>)
      return true;
    else if constexpr (__member<_Tp>)
      return noexcept(auto(std::declval<_Tp&>().end()));
    else
      return noexcept(auto(end(std::declval<_Tp&>())));
  }
  template <class _Tp>
    requires __maybe_borrowed<_Tp> && (std::is_bounded_array_v<std::remove_reference_t<_Tp>> || __member<_Tp> || __adl<_Tp>)
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const noexcept(nothrow<_Tp>()) {
    if constexpr (std::is_bounded_array_v<std::remove_reference_t<_Tp>>) {
      static_assert(__complete_array_elem<_Tp>, "ranges::end: array of incomplete type");
      return t + std::extent_v<std::remove_reference_t<_Tp>>;
    } else if constexpr (__member<_Tp>) {
      return t.end();
    } else {
      return end(t);
    }
  }
};
} // namespace end_ns

}} // namespace __ycxx::__detail::__range_access

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline namespace cpo {
inline constexpr __ycxx::__detail::__range_access::__end_ns::__fn end{};
}

// [range.range]
template <class _Tp>
concept range = requires(_Tp& t) {
  ranges::begin(t);
  ranges::end(t);
};
template <class _Tp>
concept borrowed_range = range<_Tp> && (is_lvalue_reference_v<_Tp> || enable_borrowed_range<remove_cvref_t<_Tp>>);
template <range _Rp>
using sentinel_t = decltype(ranges::end(declval<_Rp&>()));
template <range _Rp>
using range_difference_t = iter_difference_t<iterator_t<_Rp>>;
template <range _Rp>
using range_value_t = iter_value_t<iterator_t<_Rp>>;
template <range _Rp>
using range_reference_t = iter_reference_t<iterator_t<_Rp>>;
template <range _Rp>
using range_rvalue_reference_t = iter_rvalue_reference_t<iterator_t<_Rp>>;
template <range _Rp>
using range_common_reference_t = iter_common_reference_t<iterator_t<_Rp>>;

}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__range_access {

template <class _Tp>
concept __integer_like_ = __integer_like<_Tp>;


// ---- size ----
namespace __size_ns {
void size() = delete; // hides outer declarations: the call below uses argument-dependent lookup only

template <class _Tp>
concept __member = !std::ranges::disable_sized_range<std::remove_cvref_t<_Tp>> && requires(_Tp& t) {
  { auto(t.size()) } -> __integer_like_;
};
template <class _Tp>
concept __adl = !std::ranges::disable_sized_range<std::remove_cvref_t<_Tp>> && __class_or_enum<_Tp> && requires(_Tp& t) {
  { auto(size(t)) } -> __integer_like_;
};
template <class _Tp>
concept __difference = requires(_Tp& t) {
  { std::ranges::begin(t) } -> std::forward_iterator;
  { std::ranges::end(t) } -> std::sized_sentinel_for<decltype(std::ranges::begin(t))>;
};

struct __fn {
  template <class _Tp>
  static consteval bool nothrow() {
    if constexpr (std::is_bounded_array_v<std::remove_reference_t<_Tp>>)
      return true;
    else if constexpr (__member<_Tp>)
      return noexcept(auto(std::declval<_Tp&>().size()));
    else if constexpr (__adl<_Tp>)
      return noexcept(auto(size(std::declval<_Tp&>())));
    else
      return noexcept(std::ranges::end(std::declval<_Tp&>()) - std::ranges::begin(std::declval<_Tp&>()));
  }
  template <class _Tp>
    requires(!std::is_unbounded_array_v<std::remove_reference_t<_Tp>>) &&
            (std::is_bounded_array_v<std::remove_reference_t<_Tp>> || __member<_Tp> || __adl<_Tp> || __difference<_Tp>)
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const noexcept(nothrow<_Tp>()) {
    if constexpr (std::is_bounded_array_v<std::remove_reference_t<_Tp>>)
      return ::__ycxx::__detail::__decay_copy(std::extent_v<std::remove_reference_t<_Tp>>);
    else if constexpr (__member<_Tp>)
      return t.size();
    else if constexpr (__adl<_Tp>)
      return size(t);
    else
      return ::__ycxx::__detail::__to_unsigned_like(std::ranges::end(t) - std::ranges::begin(t));
  }
};
} // namespace size_ns

}} // namespace __ycxx::__detail::__range_access

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline namespace cpo {
inline constexpr __ycxx::__detail::__range_access::__size_ns::__fn size{};
}
}} // namespace std::ranges

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__range_access {

// ---- ssize ----
namespace __ssize_ns {
struct __fn {
  template <class _Tp>
    requires requires(_Tp&& t) { std::ranges::size(t); }
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const noexcept(noexcept(std::ranges::size(t))) {
    using size_type = decltype(std::ranges::size(t));
    using __signed_type = std::make_signed_t<size_type>;
    using result = std::conditional_t<(sizeof(std::ptrdiff_t) > sizeof(__signed_type)), std::ptrdiff_t, __signed_type>;
    return static_cast<result>(std::ranges::size(t));
  }
};
} // namespace ssize_ns

// ---- empty ----
namespace __empty_ns {
template <class _Tp>
concept __member = requires(_Tp& t) { bool(t.empty()); };
template <class _Tp>
concept __via_size = requires(_Tp& t) { std::ranges::size(t) == 0; };
template <class _Tp>
concept __via_iter = requires(_Tp& t) {
  { std::ranges::begin(t) } -> std::forward_iterator;
  bool(std::ranges::begin(t) == std::ranges::end(t));
};
struct __fn {
  template <class _Tp>
  static consteval bool nothrow() {
    if constexpr (__member<_Tp>)
      return noexcept(bool(std::declval<_Tp&>().empty()));
    else if constexpr (__via_size<_Tp>)
      return noexcept(std::ranges::size(std::declval<_Tp&>()) == 0);
    else
      return noexcept(bool(std::ranges::begin(std::declval<_Tp&>()) == std::ranges::end(std::declval<_Tp&>())));
  }
  template <class _Tp>
    requires(!std::is_unbounded_array_v<std::remove_reference_t<_Tp>>) && (__member<_Tp> || __via_size<_Tp> || __via_iter<_Tp>)
  [[nodiscard]] constexpr bool operator()(_Tp&& t) const noexcept(nothrow<_Tp>()) {
    if constexpr (__member<_Tp>)
      return bool(t.empty());
    else if constexpr (__via_size<_Tp>)
      return std::ranges::size(t) == 0;
    else
      return bool(std::ranges::begin(t) == std::ranges::end(t));
  }
};
} // namespace empty_ns

// ---- data ----
namespace __data_ns {
template <class _Tp>
concept __pointer_to_object = std::is_pointer_v<_Tp> && std::is_object_v<std::remove_pointer_t<_Tp>>;
template <class _Tp>
concept __member = requires(_Tp& t) {
  { auto(t.data()) } -> __pointer_to_object;
};
template <class _Tp>
concept __via_begin = requires(_Tp& t) {
  { std::ranges::begin(t) } -> std::contiguous_iterator;
};
struct __fn {
  template <class _Tp>
  static consteval bool nothrow() {
    if constexpr (__member<_Tp>)
      return noexcept(auto(std::declval<_Tp&>().data()));
    else
      return noexcept(std::ranges::begin(std::declval<_Tp&>()));
  }
  template <class _Tp>
    requires __maybe_borrowed<_Tp> && (__member<_Tp> || __via_begin<_Tp>)
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const noexcept(nothrow<_Tp>()) {
    if constexpr (__member<_Tp>)
      return t.data();
    else
      return std::to_address(std::ranges::begin(t));
  }
};
} // namespace data_ns

// ---- reserve_hint (C++26) ----
namespace __reserve_hint_ns {
void reserve_hint() = delete; // hides outer declarations: the call below uses argument-dependent lookup only
template <class _Tp>
concept __member = requires(_Tp& t) {
  { auto(t.reserve_hint()) } -> __integer_like_;
};
template <class _Tp>
concept __adl = __class_or_enum<_Tp> && requires(_Tp& t) {
  { auto(reserve_hint(t)) } -> __integer_like_;
};
struct __fn {
  template <class _Tp>
  static consteval bool nothrow() {
    if constexpr (requires(_Tp& t) { std::ranges::size(t); })
      return noexcept(std::ranges::size(std::declval<_Tp&>()));
    else if constexpr (__member<_Tp>)
      return noexcept(auto(std::declval<_Tp&>().reserve_hint()));
    else
      return noexcept(auto(reserve_hint(std::declval<_Tp&>())));
  }
  template <class _Tp>
    requires requires(_Tp& t) { std::ranges::size(t); } || __member<_Tp> || __adl<_Tp>
  [[nodiscard]] constexpr auto operator()(_Tp&& t) const noexcept(nothrow<_Tp>()) {
    if constexpr (requires { std::ranges::size(t); })
      return std::ranges::size(t);
    else if constexpr (__member<_Tp>)
      return t.reserve_hint();
    else
      return reserve_hint(t);
  }
};
} // namespace reserve_hint_ns

}} // namespace __ycxx::__detail::__range_access

namespace [[__gnu__::__visibility__("hidden")]] std { namespace ranges {
inline namespace cpo {
inline constexpr __ycxx::__detail::__range_access::__ssize_ns::__fn ssize{};
inline constexpr __ycxx::__detail::__range_access::__empty_ns::__fn empty{};
inline constexpr __ycxx::__detail::__range_access::__data_ns::__fn data{};
inline constexpr __ycxx::__detail::__range_access::__reserve_hint_ns::__fn reserve_hint{};
} // namespace cpo

template <class _Tp>
concept sized_range = range<_Tp> && requires(_Tp& t) { ranges::size(t); };
template <class _Tp>
concept approximately_sized_range = range<_Tp> && requires(_Tp& t) { ranges::reserve_hint(t); };
template <sized_range _Rp>
using range_size_t = decltype(ranges::size(declval<_Rp&>()));

template <class _Rp, class _Tp>
concept output_range = range<_Rp> && output_iterator<iterator_t<_Rp>, _Tp>;
template <class _Tp>
concept input_range = range<_Tp> && input_iterator<iterator_t<_Tp>>;
template <class _Tp>
concept forward_range = input_range<_Tp> && forward_iterator<iterator_t<_Tp>>;
template <class _Tp>
concept bidirectional_range = forward_range<_Tp> && bidirectional_iterator<iterator_t<_Tp>>;
template <class _Tp>
concept random_access_range = bidirectional_range<_Tp> && random_access_iterator<iterator_t<_Tp>>;
template <class _Tp>
concept contiguous_range = random_access_range<_Tp> && contiguous_iterator<iterator_t<_Tp>> && requires(_Tp& t) {
  { ranges::data(t) } -> same_as<add_pointer_t<range_reference_t<_Tp>>>;
};
template <class _Tp>
concept common_range = range<_Tp> && same_as<iterator_t<_Tp>, sentinel_t<_Tp>>;

}} // namespace std::ranges

// ---------------------------------------------------------------------------------------------
// [iterator.range] std::begin & co.
// ---------------------------------------------------------------------------------------------
namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Cp>
constexpr auto begin(_Cp& c) noexcept(noexcept(c.begin())) -> decltype(c.begin()) {
  return c.begin();
}
template <class _Cp>
constexpr auto begin(const _Cp& c) noexcept(noexcept(c.begin())) -> decltype(c.begin()) {
  return c.begin();
}
template <class _Cp>
constexpr auto end(_Cp& c) noexcept(noexcept(c.end())) -> decltype(c.end()) {
  return c.end();
}
template <class _Cp>
constexpr auto end(const _Cp& c) noexcept(noexcept(c.end())) -> decltype(c.end()) {
  return c.end();
}
template <class _Tp, size_t _Np>
constexpr _Tp* begin(_Tp (&a)[_Np]) noexcept {
  return a;
}
template <class _Tp, size_t _Np>
constexpr _Tp* end(_Tp (&a)[_Np]) noexcept {
  return a + _Np;
}
template <class _Cp>
constexpr auto cbegin(const _Cp& c) noexcept(noexcept(std::begin(c))) -> decltype(std::begin(c)) {
  return std::begin(c);
}
template <class _Cp>
constexpr auto cend(const _Cp& c) noexcept(noexcept(std::end(c))) -> decltype(std::end(c)) {
  return std::end(c);
}
template <class _Cp>
constexpr auto size(const _Cp& c) noexcept(noexcept(c.size())) -> decltype(c.size()) {
  return c.size();
}
template <class _Tp, size_t _Np>
constexpr size_t size(const _Tp (&)[_Np]) noexcept {
  return _Np;
}
template <class _Cp>
constexpr auto ssize(const _Cp& c) noexcept(noexcept(c.size())) -> common_type_t<ptrdiff_t, make_signed_t<decltype(c.size())>> {
  return static_cast<common_type_t<ptrdiff_t, make_signed_t<decltype(c.size())>>>(c.size());
}
template <class _Tp, ptrdiff_t _Np>
constexpr ptrdiff_t ssize(const _Tp (&)[_Np]) noexcept {
  return _Np;
}
template <class _Cp>
[[nodiscard]] constexpr auto empty(const _Cp& c) noexcept(noexcept(c.empty())) -> decltype(c.empty()) {
  return c.empty();
}
template <class _Tp, size_t _Np>
[[nodiscard]] constexpr bool empty(const _Tp (&)[_Np]) noexcept {
  return false;
}
template <class _Cp>
constexpr auto data(_Cp& c) noexcept(noexcept(c.data())) -> decltype(c.data()) {
  return c.data();
}
template <class _Cp>
constexpr auto data(const _Cp& c) noexcept(noexcept(c.data())) -> decltype(c.data()) {
  return c.data();
}
template <class _Tp, size_t _Np>
constexpr _Tp* data(_Tp (&a)[_Np]) noexcept {
  return a;
}
// initializer_list overloads of empty/data are provided generically through il.empty()/il.data().

} // namespace std
