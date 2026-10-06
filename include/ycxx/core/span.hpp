// libycxx core: <span> ([views.span], [span.objectrep]).
//
// The iterator is a plain pointer (it models contiguous_iterator and meets every container
// iterator requirement). A span with a static extent stores only the pointer.
#pragma once

#include <ycxx/core/array.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/range_access.hpp>
#include <ycxx/core/ranges_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

inline constexpr size_t dynamic_extent = numeric_limits<size_t>::max();

template <class _ElementType, size_t _Extent = dynamic_extent>
class span;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// constexpr-wrapper-like and integral-constant-like ([expos.only.entity], [span.syn])
template <class _Tp>
concept __constexpr_wrapper_like =
    std::convertible_to<_Tp, decltype(_Tp::value)> && std::equality_comparable_with<_Tp, decltype(_Tp::value)> &&
    std::bool_constant<_Tp() == _Tp::value>::value &&
    std::bool_constant<static_cast<decltype(_Tp::value)>(_Tp()) == _Tp::value>::value;
template <class _Tp>
concept __integral_constant_like = std::is_integral_v<std::remove_cvref_t<decltype(_Tp::value)>> &&
                                 !std::is_same_v<bool, std::remove_cvref_t<decltype(_Tp::value)>> &&
                                 __constexpr_wrapper_like<_Tp>;
// maybe-static-ext
template <class _Tp>
inline constexpr std::size_t __maybe_static_ext = std::dynamic_extent;
template <__integral_constant_like _Tp>
inline constexpr std::size_t __maybe_static_ext<_Tp> = {_Tp::value};

template <class _Tp>
inline constexpr bool __is_span = false;
template <class _Tp, std::size_t _Ep>
inline constexpr bool __is_span<std::span<_Tp, _Ep>> = true;
template <class _Tp>
inline constexpr bool __is_std_array = false;
template <class _Tp, std::size_t _Np>
inline constexpr bool __is_std_array<std::array<_Tp, _Np>> = true;

// "Only qualification conversions": is_convertible_v<From(*)[], To(*)[]>.
template <class _From, class _To>
concept __span_compatible = std::is_convertible_v<_From (*)[], _To (*)[]>;

// Size storage: nothing for a static extent.
template <std::size_t _Extent>
struct __span_extent {
  constexpr __span_extent(std::size_t) noexcept {}
  static constexpr std::size_t get() noexcept { return _Extent; }
};
template <>
struct __span_extent<std::dynamic_extent> {
  std::size_t n;
  constexpr __span_extent(std::size_t s) noexcept : n(s) {}
  constexpr std::size_t get() const noexcept { return n; }
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _ElementType, size_t _Extent>
class span {
  static_assert(is_object_v<_ElementType> && !is_abstract_v<_ElementType>,
                "std::span: ElementType must be a complete object type that is not abstract");

public:
  using element_type = _ElementType;
  using value_type = remove_cv_t<_ElementType>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using pointer = element_type*;
  using const_pointer = const element_type*;
  using reference = element_type&;
  using const_reference = const element_type&;
  using iterator = pointer;
  using const_iterator = std::const_iterator<iterator>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::const_iterator<reverse_iterator>;
  static constexpr size_type extent = _Extent;

  // ---- [span.cons] ----
  constexpr span() noexcept
    requires(_Extent == dynamic_extent || _Extent == 0)
      : __data_(nullptr), __size_(0) {}

  template <class _It>
    requires contiguous_iterator<_It> &&
             __ycxx::__detail::__span_compatible<remove_reference_t<iter_reference_t<_It>>, element_type>
  // "Throws: Nothing" ([span.cons]/7): noexcept is a permitted strengthening.
  constexpr explicit(extent != dynamic_extent) span(_It first, size_type count) noexcept
      : __data_(std::to_address(first)), __size_(count) {
    if constexpr (extent != dynamic_extent)
      __ycxx::__detail::__precondition(count == extent, "std::span: count != extent");
  }

  template <class _It, class _End>
    requires __ycxx::__detail::__span_compatible<remove_reference_t<iter_reference_t<_It>>, element_type> &&
             contiguous_iterator<_It> && sized_sentinel_for<_End, _It> && (!is_convertible_v<_End, size_t>)
  constexpr explicit(extent != dynamic_extent) span(_It first, _End last) noexcept(noexcept(last - first))
      : span(__checked{}, std::to_address(first), static_cast<size_type>(last - first)) {}

  template <size_t _Np>
    requires(extent == dynamic_extent || _Np == extent)
  constexpr span(type_identity_t<element_type> (&__arr)[_Np]) noexcept : __data_(__arr), __size_(_Np) {}
  template <class _Tp, size_t _Np>
    requires(extent == dynamic_extent || _Np == extent) && __ycxx::__detail::__span_compatible<_Tp, element_type>
  constexpr span(array<_Tp, _Np>& __arr) noexcept : __data_(__arr.data()), __size_(_Np) {}
  template <class _Tp, size_t _Np>
    requires(extent == dynamic_extent || _Np == extent) && __ycxx::__detail::__span_compatible<const _Tp, element_type>
  constexpr span(const array<_Tp, _Np>& __arr) noexcept : __data_(__arr.data()), __size_(_Np) {}

  template <class _Rp>
    requires ranges::contiguous_range<_Rp> && ranges::sized_range<_Rp> &&
             (ranges::borrowed_range<_Rp> || is_const_v<element_type>) &&
             (!__ycxx::__detail::__is_span<remove_cvref_t<_Rp>>) && (!__ycxx::__detail::__is_std_array<remove_cvref_t<_Rp>>) &&
             (!is_array_v<remove_cvref_t<_Rp>>) &&
             __ycxx::__detail::__span_compatible<remove_reference_t<ranges::range_reference_t<_Rp>>, element_type>
  constexpr explicit(extent != dynamic_extent) span(_Rp&& r)
      : span(__checked{}, ranges::data(r), static_cast<size_type>(ranges::size(r))) {}

  constexpr span(const span& other) noexcept = default;

  template <class _OtherElementType, size_t _OtherExtent>
    requires(extent == dynamic_extent || _OtherExtent == dynamic_extent || extent == _OtherExtent) &&
            __ycxx::__detail::__span_compatible<_OtherElementType, element_type>
  constexpr explicit(extent != dynamic_extent && _OtherExtent == dynamic_extent)
      span(const span<_OtherElementType, _OtherExtent>& s) noexcept
      : __data_(s.data()), __size_(s.size()) {
    if constexpr (extent != dynamic_extent)
      __ycxx::__detail::__precondition(s.size() == extent, "std::span: size != extent");
  }

  constexpr span& operator=(const span& other) noexcept = default;

  // ---- [span.sub] ----
  template <size_t _Count>
  constexpr span<element_type, _Count> first() const {
    static_assert(_Count <= _Extent, "std::span::first: Count > Extent");
    __ycxx::__detail::__precondition(_Count <= size(), "std::span::first: Count > size()");
    return span<element_type, _Count>(data(), _Count);
  }
  template <size_t _Count>
  constexpr span<element_type, _Count> last() const {
    static_assert(_Count <= _Extent, "std::span::last: Count > Extent");
    __ycxx::__detail::__precondition(_Count <= size(), "std::span::last: Count > size()");
    return span<element_type, _Count>(data() + (size() - _Count), _Count);
  }
  template <size_t _Offset, size_t _Count = dynamic_extent>
  constexpr auto subspan() const {
    static_assert(_Offset <= _Extent && (_Count == dynamic_extent || _Count <= _Extent - _Offset),
                  "std::span::subspan: Offset/Count out of range for Extent");
    __ycxx::__detail::__precondition(_Offset <= size() && (_Count == dynamic_extent || _Count <= size() - _Offset),
                               "std::span::subspan: Offset/Count out of range");
    constexpr size_t _Ep = _Count != dynamic_extent ? _Count : (_Extent != dynamic_extent ? _Extent - _Offset : dynamic_extent);
    return span<element_type, _Ep>(data() + _Offset, _Count != dynamic_extent ? _Count : size() - _Offset);
  }
  constexpr span<element_type, dynamic_extent> first(size_type count) const {
    __ycxx::__detail::__precondition(count <= size(), "std::span::first: count > size()");
    return {data(), count};
  }
  constexpr span<element_type, dynamic_extent> last(size_type count) const {
    __ycxx::__detail::__precondition(count <= size(), "std::span::last: count > size()");
    return {data() + (size() - count), count};
  }
  constexpr span<element_type, dynamic_extent> subspan(size_type offset, size_type count = dynamic_extent) const {
    __ycxx::__detail::__precondition(offset <= size() && (count == dynamic_extent || count <= size() - offset),
                               "std::span::subspan: offset/count out of range");
    return {data() + offset, count == dynamic_extent ? size() - offset : count};
  }

  // ---- [span.obs] ----
  constexpr size_type size() const noexcept { return __size_.get(); }
  constexpr size_type size_bytes() const noexcept { return size() * sizeof(element_type); }
  [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

  // ---- [span.elem] ----
  constexpr reference operator[](size_type __idx) const {
    __ycxx::__detail::__precondition(__idx < size(), "std::span::operator[]: index out of range");
    return __data_[__idx];
  }
  constexpr reference at(size_type __idx) const {
    if (__idx >= size())
      __ycxx::__detail::__throw_out_of_range("std::span::at: index out of range");
    return __data_[__idx];
  }
  constexpr reference front() const {
    __ycxx::__detail::__precondition(!empty(), "std::span::front: empty span");
    return *__data_;
  }
  constexpr reference back() const {
    __ycxx::__detail::__precondition(!empty(), "std::span::back: empty span");
    return __data_[size() - 1];
  }
  constexpr pointer data() const noexcept { return __data_; }

  // ---- [span.iterators] ----
  constexpr iterator begin() const noexcept { return __data_; }
  constexpr iterator end() const noexcept { return __data_ + size(); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
  constexpr reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

private:
  // The (It, End) and range constructors end here, so `last - first` / ranges::size(r) is
  // evaluated exactly once ([span.cons]) and the hardened extent check sees that one value.
  struct __checked {};
  template <class _Pp>
  constexpr span(__checked, _Pp* p, size_type n) noexcept : __data_(p), __size_(n) {
    if constexpr (extent != dynamic_extent)
      __ycxx::__detail::__precondition(n == extent, "std::span: size != extent");
  }

  pointer __data_;
  [[no_unique_address]] __ycxx::__detail::__span_extent<_Extent> __size_;
};

// ---- [span.deduct] ----
template <class _It, class _EndOrSize>
  requires contiguous_iterator<_It>
span(_It, _EndOrSize) -> span<remove_reference_t<iter_reference_t<_It>>, __ycxx::__detail::__maybe_static_ext<_EndOrSize>>;
template <class _Tp, size_t _Np>
span(_Tp (&)[_Np]) -> span<_Tp, _Np>;
template <class _Tp, size_t _Np>
span(array<_Tp, _Np>&) -> span<_Tp, _Np>;
template <class _Tp, size_t _Np>
span(const array<_Tp, _Np>&) -> span<const _Tp, _Np>;
template <class _Rp>
  requires ranges::contiguous_range<_Rp>
span(_Rp&&) -> span<remove_reference_t<ranges::range_reference_t<_Rp>>>;

template <class _ElementType, size_t _Extent>
constexpr bool ranges::enable_view<span<_ElementType, _Extent>> = true;
template <class _ElementType, size_t _Extent>
constexpr bool ranges::enable_borrowed_range<span<_ElementType, _Extent>> = true;

// ---- [span.objectrep] ----
template <class _ElementType, size_t _Extent>
  requires(!is_volatile_v<_ElementType>)
span<const byte, _Extent == dynamic_extent ? dynamic_extent : sizeof(_ElementType) * _Extent> as_bytes(
    span<_ElementType, _Extent> s) noexcept {
  using _Rp = span<const byte, _Extent == dynamic_extent ? dynamic_extent : sizeof(_ElementType) * _Extent>;
  return _Rp{reinterpret_cast<const byte*>(s.data()), s.size_bytes()};
}
template <class _ElementType, size_t _Extent>
  requires(!is_const_v<_ElementType>) && (!is_volatile_v<_ElementType>)
span<byte, _Extent == dynamic_extent ? dynamic_extent : sizeof(_ElementType) * _Extent> as_writable_bytes(
    span<_ElementType, _Extent> s) noexcept {
  using _Rp = span<byte, _Extent == dynamic_extent ? dynamic_extent : sizeof(_ElementType) * _Extent>;
  return _Rp{reinterpret_cast<byte*>(s.data()), s.size_bytes()};
}

} // namespace std
