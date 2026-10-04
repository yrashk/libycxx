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

namespace std {

inline constexpr size_t dynamic_extent = numeric_limits<size_t>::max();

template <class ElementType, size_t Extent = dynamic_extent>
class span;

} // namespace std

namespace ycxx::detail {

// constexpr-wrapper-like and integral-constant-like ([expos.only.entity], [span.syn])
template <class T>
concept constexpr_wrapper_like =
    std::convertible_to<T, decltype(T::value)> && std::equality_comparable_with<T, decltype(T::value)> &&
    std::bool_constant<T() == T::value>::value &&
    std::bool_constant<static_cast<decltype(T::value)>(T()) == T::value>::value;
template <class T>
concept integral_constant_like = std::is_integral_v<std::remove_cvref_t<decltype(T::value)>> &&
                                 !std::is_same_v<bool, std::remove_cvref_t<decltype(T::value)>> &&
                                 constexpr_wrapper_like<T>;
// maybe-static-ext
template <class T>
inline constexpr std::size_t maybe_static_ext = std::dynamic_extent;
template <integral_constant_like T>
inline constexpr std::size_t maybe_static_ext<T> = {T::value};

template <class T>
inline constexpr bool is_span = false;
template <class T, std::size_t E>
inline constexpr bool is_span<std::span<T, E>> = true;
template <class T>
inline constexpr bool is_std_array = false;
template <class T, std::size_t N>
inline constexpr bool is_std_array<std::array<T, N>> = true;

// "Only qualification conversions": is_convertible_v<From(*)[], To(*)[]>.
template <class From, class To>
concept span_compatible = std::is_convertible_v<From (*)[], To (*)[]>;

// Size storage: nothing for a static extent.
template <std::size_t Extent>
struct span_extent {
  constexpr span_extent(std::size_t) noexcept {}
  static constexpr std::size_t get() noexcept { return Extent; }
};
template <>
struct span_extent<std::dynamic_extent> {
  std::size_t n;
  constexpr span_extent(std::size_t s) noexcept : n(s) {}
  constexpr std::size_t get() const noexcept { return n; }
};

} // namespace ycxx::detail

namespace std {

template <class ElementType, size_t Extent>
class span {
  static_assert(is_object_v<ElementType> && !is_abstract_v<ElementType>,
                "std::span: ElementType must be a complete object type that is not abstract");

public:
  using element_type = ElementType;
  using value_type = remove_cv_t<ElementType>;
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
  static constexpr size_type extent = Extent;

  // ---- [span.cons] ----
  constexpr span() noexcept
    requires(Extent == dynamic_extent || Extent == 0)
      : data_(nullptr), size_(0) {}

  template <class It>
    requires contiguous_iterator<It> &&
             ycxx::detail::span_compatible<remove_reference_t<iter_reference_t<It>>, element_type>
  // "Throws: Nothing" ([span.cons]/7): noexcept is a permitted strengthening.
  constexpr explicit(extent != dynamic_extent) span(It first, size_type count) noexcept
      : data_(std::to_address(first)), size_(count) {
    if constexpr (extent != dynamic_extent)
      ycxx::detail::precondition(count == extent, "std::span: count != extent");
  }

  template <class It, class End>
    requires ycxx::detail::span_compatible<remove_reference_t<iter_reference_t<It>>, element_type> &&
             contiguous_iterator<It> && sized_sentinel_for<End, It> && (!is_convertible_v<End, size_t>)
  constexpr explicit(extent != dynamic_extent) span(It first, End last) noexcept(noexcept(last - first))
      : span(checked{}, std::to_address(first), static_cast<size_type>(last - first)) {}

  template <size_t N>
    requires(extent == dynamic_extent || N == extent)
  constexpr span(type_identity_t<element_type> (&arr)[N]) noexcept : data_(arr), size_(N) {}
  template <class T, size_t N>
    requires(extent == dynamic_extent || N == extent) && ycxx::detail::span_compatible<T, element_type>
  constexpr span(array<T, N>& arr) noexcept : data_(arr.data()), size_(N) {}
  template <class T, size_t N>
    requires(extent == dynamic_extent || N == extent) && ycxx::detail::span_compatible<const T, element_type>
  constexpr span(const array<T, N>& arr) noexcept : data_(arr.data()), size_(N) {}

  template <class R>
    requires ranges::contiguous_range<R> && ranges::sized_range<R> &&
             (ranges::borrowed_range<R> || is_const_v<element_type>) &&
             (!ycxx::detail::is_span<remove_cvref_t<R>>) && (!ycxx::detail::is_std_array<remove_cvref_t<R>>) &&
             (!is_array_v<remove_cvref_t<R>>) &&
             ycxx::detail::span_compatible<remove_reference_t<ranges::range_reference_t<R>>, element_type>
  constexpr explicit(extent != dynamic_extent) span(R&& r)
      : span(checked{}, ranges::data(r), static_cast<size_type>(ranges::size(r))) {}

  constexpr span(const span& other) noexcept = default;

  template <class OtherElementType, size_t OtherExtent>
    requires(extent == dynamic_extent || OtherExtent == dynamic_extent || extent == OtherExtent) &&
            ycxx::detail::span_compatible<OtherElementType, element_type>
  constexpr explicit(extent != dynamic_extent && OtherExtent == dynamic_extent)
      span(const span<OtherElementType, OtherExtent>& s) noexcept
      : data_(s.data()), size_(s.size()) {
    if constexpr (extent != dynamic_extent)
      ycxx::detail::precondition(s.size() == extent, "std::span: size != extent");
  }

  constexpr span& operator=(const span& other) noexcept = default;

  // ---- [span.sub] ----
  template <size_t Count>
  constexpr span<element_type, Count> first() const {
    static_assert(Count <= Extent, "std::span::first: Count > Extent");
    ycxx::detail::precondition(Count <= size(), "std::span::first: Count > size()");
    return span<element_type, Count>(data(), Count);
  }
  template <size_t Count>
  constexpr span<element_type, Count> last() const {
    static_assert(Count <= Extent, "std::span::last: Count > Extent");
    ycxx::detail::precondition(Count <= size(), "std::span::last: Count > size()");
    return span<element_type, Count>(data() + (size() - Count), Count);
  }
  template <size_t Offset, size_t Count = dynamic_extent>
  constexpr auto subspan() const {
    static_assert(Offset <= Extent && (Count == dynamic_extent || Count <= Extent - Offset),
                  "std::span::subspan: Offset/Count out of range for Extent");
    ycxx::detail::precondition(Offset <= size() && (Count == dynamic_extent || Count <= size() - Offset),
                               "std::span::subspan: Offset/Count out of range");
    constexpr size_t E = Count != dynamic_extent ? Count : (Extent != dynamic_extent ? Extent - Offset : dynamic_extent);
    return span<element_type, E>(data() + Offset, Count != dynamic_extent ? Count : size() - Offset);
  }
  constexpr span<element_type, dynamic_extent> first(size_type count) const {
    ycxx::detail::precondition(count <= size(), "std::span::first: count > size()");
    return {data(), count};
  }
  constexpr span<element_type, dynamic_extent> last(size_type count) const {
    ycxx::detail::precondition(count <= size(), "std::span::last: count > size()");
    return {data() + (size() - count), count};
  }
  constexpr span<element_type, dynamic_extent> subspan(size_type offset, size_type count = dynamic_extent) const {
    ycxx::detail::precondition(offset <= size() && (count == dynamic_extent || count <= size() - offset),
                               "std::span::subspan: offset/count out of range");
    return {data() + offset, count == dynamic_extent ? size() - offset : count};
  }

  // ---- [span.obs] ----
  constexpr size_type size() const noexcept { return size_.get(); }
  constexpr size_type size_bytes() const noexcept { return size() * sizeof(element_type); }
  [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

  // ---- [span.elem] ----
  constexpr reference operator[](size_type idx) const {
    ycxx::detail::precondition(idx < size(), "std::span::operator[]: index out of range");
    return data_[idx];
  }
  constexpr reference at(size_type idx) const {
    if (idx >= size())
      ycxx::detail::throw_out_of_range("std::span::at: index out of range");
    return data_[idx];
  }
  constexpr reference front() const {
    ycxx::detail::precondition(!empty(), "std::span::front: empty span");
    return *data_;
  }
  constexpr reference back() const {
    ycxx::detail::precondition(!empty(), "std::span::back: empty span");
    return data_[size() - 1];
  }
  constexpr pointer data() const noexcept { return data_; }

  // ---- [span.iterators] ----
  constexpr iterator begin() const noexcept { return data_; }
  constexpr iterator end() const noexcept { return data_ + size(); }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
  constexpr reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

private:
  // Every size-taking constructor ends here, so the size expression is evaluated exactly once
  // ([span.cons]) and the hardened extent check sees that one value.
  struct checked {};
  template <class P>
  constexpr span(checked, P* p, size_type n) noexcept : data_(p), size_(n) {
    if constexpr (extent != dynamic_extent)
      ycxx::detail::precondition(n == extent, "std::span: size != extent");
  }

  pointer data_;
  [[no_unique_address]] ycxx::detail::span_extent<Extent> size_;
};

// ---- [span.deduct] ----
template <class It, class EndOrSize>
  requires contiguous_iterator<It>
span(It, EndOrSize) -> span<remove_reference_t<iter_reference_t<It>>, ycxx::detail::maybe_static_ext<EndOrSize>>;
template <class T, size_t N>
span(T (&)[N]) -> span<T, N>;
template <class T, size_t N>
span(array<T, N>&) -> span<T, N>;
template <class T, size_t N>
span(const array<T, N>&) -> span<const T, N>;
template <class R>
  requires ranges::contiguous_range<R>
span(R&&) -> span<remove_reference_t<ranges::range_reference_t<R>>>;

template <class ElementType, size_t Extent>
constexpr bool ranges::enable_view<span<ElementType, Extent>> = true;
template <class ElementType, size_t Extent>
constexpr bool ranges::enable_borrowed_range<span<ElementType, Extent>> = true;

// ---- [span.objectrep] ----
template <class ElementType, size_t Extent>
  requires(!is_volatile_v<ElementType>)
span<const byte, Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent> as_bytes(
    span<ElementType, Extent> s) noexcept {
  using R = span<const byte, Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>;
  return R{reinterpret_cast<const byte*>(s.data()), s.size_bytes()};
}
template <class ElementType, size_t Extent>
  requires(!is_const_v<ElementType>) && (!is_volatile_v<ElementType>)
span<byte, Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent> as_writable_bytes(
    span<ElementType, Extent> s) noexcept {
  using R = span<byte, Extent == dynamic_extent ? dynamic_extent : sizeof(ElementType) * Extent>;
  return R{reinterpret_cast<byte*>(s.data()), s.size_bytes()};
}

} // namespace std
