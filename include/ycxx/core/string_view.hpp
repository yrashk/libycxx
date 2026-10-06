// libycxx core: <string_view> ([string.view]). The iterator is a plain const charT*.
#pragma once

#include <ycxx/core/char_traits.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/iosfwd.hpp>
#include <ycxx/core/iterator_adaptors.hpp>
#include <ycxx/core/limits.hpp>
#include <ycxx/core/range_access.hpp>
#include <ycxx/core/ranges_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class __charT, class __traits = char_traits<__charT>>
class basic_string_view;

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {
// [string.view.cons]/12.5: d.operator ::std::basic_string_view<charT, traits>() is not valid.
template <class _Dp, class __charT, class __traits>
concept __has_string_view_conversion =
    requires(_Dp& d) { d.operator ::std::basic_string_view<__charT, __traits>(); };

// [string.view.comparison]: traits::comparison_category if valid, otherwise weak_ordering.
template <class __traits>
struct __sv_comparison_category {
  using type = std::weak_ordering;
};
template <class __traits>
  requires requires { typename __traits::comparison_category; }
struct __sv_comparison_category<__traits> {
  using type = typename __traits::comparison_category;
};
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class __charT, class __traits>
class basic_string_view {
  static_assert(is_same_v<typename __traits::char_type, __charT>,
                "std::basic_string_view: traits::char_type must be charT");
  static_assert(!is_array_v<__charT> && is_trivially_copyable_v<__charT> && is_trivially_default_constructible_v<__charT> &&
                    is_standard_layout_v<__charT>,
                "std::basic_string_view: charT must be a char-like type");

public:
  using traits_type = __traits;
  using value_type = __charT;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using reference = value_type&;
  using const_reference = const value_type&;
  using const_iterator = const __charT*;
  using iterator = const_iterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = const_reverse_iterator;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  static constexpr size_type npos = size_type(-1);

  // ---- [string.view.cons] ----
  constexpr basic_string_view() noexcept : __data_(nullptr), __size_(0) {}
  constexpr basic_string_view(const basic_string_view&) noexcept = default;
  constexpr basic_string_view& operator=(const basic_string_view&) noexcept = default;
  constexpr basic_string_view(const __charT* str) noexcept(noexcept(__traits::length(str)))
      : __data_(str), __size_(__traits::length(str)) {}
  basic_string_view(nullptr_t) = delete;
  constexpr basic_string_view(const __charT* str, size_type __len) : __data_(str), __size_(__len) {}
  template <class _It, class _End>
    requires contiguous_iterator<_It> && sized_sentinel_for<_End, _It> && is_same_v<iter_value_t<_It>, __charT> &&
             (!is_convertible_v<_End, size_type>)
  constexpr basic_string_view(_It begin, _End end) noexcept(noexcept(end - begin))
      : __data_(std::to_address(begin)), __size_(static_cast<size_type>(end - begin)) {}
  template <class _Rp>
    requires(!is_same_v<remove_cvref_t<_Rp>, basic_string_view>) && ranges::contiguous_range<_Rp> &&
            ranges::sized_range<_Rp> && is_same_v<ranges::range_value_t<_Rp>, __charT> &&
            (!is_convertible_v<_Rp, const __charT*>) &&
            (!__ycxx::__detail::__has_string_view_conversion<remove_cvref_t<_Rp>, __charT, __traits>)
  constexpr explicit basic_string_view(_Rp&& r) : __data_(ranges::data(r)), __size_(static_cast<size_type>(ranges::size(r))) {}

  // ---- [string.view.iterators] ----
  constexpr const_iterator begin() const noexcept { return __data_; }
  constexpr const_iterator end() const noexcept { return __data_ + __size_; }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [string.view.capacity] ----
  constexpr size_type size() const noexcept { return __size_; }
  constexpr size_type length() const noexcept { return __size_; }
  constexpr size_type max_size() const noexcept {
    // Pointer differences over the viewed range must be representable.
    constexpr size_type __by_ptrdiff = static_cast<size_type>(numeric_limits<ptrdiff_t>::max()) / sizeof(__charT);
    return __by_ptrdiff;
  }
  [[nodiscard]] constexpr bool empty() const noexcept { return __size_ == 0; }

  // ---- [string.view.access] ----
  constexpr const_reference operator[](size_type __pos) const {
    __ycxx::__detail::__precondition(__pos < __size_, "std::basic_string_view::operator[]: index out of range");
    return __data_[__pos];
  }
  constexpr const_reference at(size_type __pos) const {
    if (__pos >= __size_)
      __ycxx::__detail::__throw_out_of_range("std::basic_string_view::at: index out of range");
    return __data_[__pos];
  }
  constexpr const_reference front() const {
    __ycxx::__detail::__precondition(__size_ != 0, "std::basic_string_view::front: empty");
    return __data_[0];
  }
  constexpr const_reference back() const {
    __ycxx::__detail::__precondition(__size_ != 0, "std::basic_string_view::back: empty");
    return __data_[__size_ - 1];
  }
  constexpr const __charT* data() const noexcept { return __data_; }

  // ---- [string.view.modifiers] ----
  constexpr void remove_prefix(size_type n) {
    __ycxx::__detail::__precondition(n <= __size_, "std::basic_string_view::remove_prefix: n > size()");
    __data_ += n;
    __size_ -= n;
  }
  constexpr void remove_suffix(size_type n) {
    __ycxx::__detail::__precondition(n <= __size_, "std::basic_string_view::remove_suffix: n > size()");
    __size_ -= n;
  }
  constexpr void swap(basic_string_view& s) noexcept {
    basic_string_view t = *this;
    *this = s;
    s = t;
  }

  // ---- [string.view.ops] ----
  constexpr size_type copy(__charT* s, size_type n, size_type __pos = 0) const {
    __check_pos(__pos, "std::basic_string_view::copy: pos > size()");
    size_type __rlen = __clamp_len(__pos, n);
    __traits::copy(s, __data_ + __pos, __rlen);
    return __rlen;
  }
  constexpr basic_string_view substr(size_type __pos = 0, size_type n = npos) const {
    __check_pos(__pos, "std::basic_string_view::substr: pos > size()");
    return basic_string_view(__data_ + __pos, __clamp_len(__pos, n));
  }
  constexpr basic_string_view subview(size_type __pos = 0, size_type n = npos) const {
    __check_pos(__pos, "std::basic_string_view::subview: pos > size()");
    return basic_string_view(__data_ + __pos, __clamp_len(__pos, n));
  }

  constexpr int compare(basic_string_view s) const noexcept {
    size_type __rlen = __size_ < s.__size_ ? __size_ : s.__size_;
    if (int r = __rlen == 0 ? 0 : __traits::compare(__data_, s.__data_, __rlen); r != 0)
      return r;
    return __size_ < s.__size_ ? -1 : (__size_ > s.__size_ ? 1 : 0);
  }
  constexpr int compare(size_type __pos1, size_type __n1, basic_string_view s) const {
    return substr(__pos1, __n1).compare(s);
  }
  constexpr int compare(size_type __pos1, size_type __n1, basic_string_view s, size_type __pos2, size_type __n2) const {
    return substr(__pos1, __n1).compare(s.substr(__pos2, __n2));
  }
  constexpr int compare(const __charT* s) const { return compare(basic_string_view(s)); }
  constexpr int compare(size_type __pos1, size_type __n1, const __charT* s) const {
    return substr(__pos1, __n1).compare(basic_string_view(s));
  }
  constexpr int compare(size_type __pos1, size_type __n1, const __charT* s, size_type __n2) const {
    return substr(__pos1, __n1).compare(basic_string_view(s, __n2));
  }

  constexpr bool starts_with(basic_string_view __x) const noexcept {
    return __size_ >= __x.__size_ && basic_string_view(__data_, __x.__size_).compare(__x) == 0;
  }
  constexpr bool starts_with(__charT __x) const noexcept { return !empty() && __traits::eq(__data_[0], __x); }
  constexpr bool starts_with(const __charT* __x) const { return starts_with(basic_string_view(__x)); }
  constexpr bool ends_with(basic_string_view __x) const noexcept {
    return __size_ >= __x.__size_ && basic_string_view(__data_ + (__size_ - __x.__size_), __x.__size_).compare(__x) == 0;
  }
  constexpr bool ends_with(__charT __x) const noexcept { return !empty() && __traits::eq(__data_[__size_ - 1], __x); }
  constexpr bool ends_with(const __charT* __x) const { return ends_with(basic_string_view(__x)); }
  constexpr bool contains(basic_string_view __x) const noexcept { return find(__x) != npos; }
  constexpr bool contains(__charT __x) const noexcept { return find(__x) != npos; }
  constexpr bool contains(const __charT* __x) const { return find(__x) != npos; }

  // ---- [string.view.find] ----
  constexpr size_type find(basic_string_view s, size_type __pos = 0) const noexcept {
    if (s.__size_ > __size_ || __pos > __size_ - s.__size_)
      return npos;
    if (s.__size_ == 0)
      return __pos;
    // Scan for the first character with traits::find, then compare the rest.
    const __charT* const last = __data_ + (__size_ - s.__size_) + 1;
    for (const __charT* p = __data_ + __pos; p < last;) {
      p = __traits::find(p, static_cast<size_type>(last - p), s.__data_[0]);
      if (!p)
        return npos;
      if (__traits::compare(p + 1, s.__data_ + 1, s.__size_ - 1) == 0)
        return static_cast<size_type>(p - __data_);
      ++p;
    }
    return npos;
  }
  constexpr size_type find(__charT c, size_type __pos = 0) const noexcept {
    if (__pos >= __size_)
      return npos;
    const __charT* p = __traits::find(__data_ + __pos, __size_ - __pos, c);
    return p ? static_cast<size_type>(p - __data_) : npos;
  }
  constexpr size_type find(const __charT* s, size_type __pos, size_type n) const {
    return find(basic_string_view(s, n), __pos);
  }
  constexpr size_type find(const __charT* s, size_type __pos = 0) const { return find(basic_string_view(s), __pos); }

  constexpr size_type rfind(basic_string_view s, size_type __pos = npos) const noexcept {
    if (s.__size_ > __size_)
      return npos;
    size_type __xpos = __size_ - s.__size_;
    if (__pos < __xpos)
      __xpos = __pos;
    for (;; --__xpos) {
      if (s.__size_ == 0 || __traits::compare(__data_ + __xpos, s.__data_, s.__size_) == 0)
        return __xpos;
      if (__xpos == 0)
        return npos;
    }
  }
  constexpr size_type rfind(__charT c, size_type __pos = npos) const noexcept {
    return rfind(basic_string_view(__builtin_addressof(c), 1), __pos);
  }
  constexpr size_type rfind(const __charT* s, size_type __pos, size_type n) const {
    return rfind(basic_string_view(s, n), __pos);
  }
  constexpr size_type rfind(const __charT* s, size_type __pos = npos) const { return rfind(basic_string_view(s), __pos); }

  constexpr size_type find_first_of(basic_string_view s, size_type __pos = 0) const noexcept {
    return __scan_forward(s, __pos, true);
  }
  constexpr size_type find_first_of(__charT c, size_type __pos = 0) const noexcept { return find(c, __pos); }
  constexpr size_type find_first_of(const __charT* s, size_type __pos, size_type n) const {
    return find_first_of(basic_string_view(s, n), __pos);
  }
  constexpr size_type find_first_of(const __charT* s, size_type __pos = 0) const {
    return find_first_of(basic_string_view(s), __pos);
  }

  constexpr size_type find_last_of(basic_string_view s, size_type __pos = npos) const noexcept {
    return __scan_backward(s, __pos, true);
  }
  constexpr size_type find_last_of(__charT c, size_type __pos = npos) const noexcept { return rfind(c, __pos); }
  constexpr size_type find_last_of(const __charT* s, size_type __pos, size_type n) const {
    return find_last_of(basic_string_view(s, n), __pos);
  }
  constexpr size_type find_last_of(const __charT* s, size_type __pos = npos) const {
    return find_last_of(basic_string_view(s), __pos);
  }

  constexpr size_type find_first_not_of(basic_string_view s, size_type __pos = 0) const noexcept {
    return __scan_forward(s, __pos, false);
  }
  constexpr size_type find_first_not_of(__charT c, size_type __pos = 0) const noexcept {
    return find_first_not_of(basic_string_view(__builtin_addressof(c), 1), __pos);
  }
  constexpr size_type find_first_not_of(const __charT* s, size_type __pos, size_type n) const {
    return find_first_not_of(basic_string_view(s, n), __pos);
  }
  constexpr size_type find_first_not_of(const __charT* s, size_type __pos = 0) const {
    return find_first_not_of(basic_string_view(s), __pos);
  }

  constexpr size_type find_last_not_of(basic_string_view s, size_type __pos = npos) const noexcept {
    return __scan_backward(s, __pos, false);
  }
  constexpr size_type find_last_not_of(__charT c, size_type __pos = npos) const noexcept {
    return find_last_not_of(basic_string_view(__builtin_addressof(c), 1), __pos);
  }
  constexpr size_type find_last_not_of(const __charT* s, size_type __pos, size_type n) const {
    return find_last_not_of(basic_string_view(s, n), __pos);
  }
  constexpr size_type find_last_not_of(const __charT* s, size_type __pos = npos) const {
    return find_last_not_of(basic_string_view(s), __pos);
  }

private:
  // Membership test for the find_*_of family. For byte-sized characters with the standard
  // traits (whose eq is ==), a set of more than a few characters is turned into a 256-bit table,
  // so the scan is O(size() + s.size()) instead of O(size() * s.size()).
  struct __char_set {
    const basic_string_view& s;
    unsigned long long bits[4] = {};
    bool __use_table = false;
    constexpr explicit __char_set(const basic_string_view& set) : s(set) {
      if constexpr (sizeof(__charT) == 1 && is_same_v<__traits, char_traits<__charT>>) {
        if (set.__size_ > 8) {
          __use_table = true;
          for (__charT c : set) {
            const unsigned __u = static_cast<unsigned char>(c);
            bits[__u / 64] |= 1ull << (__u % 64);
          }
        }
      }
    }
    constexpr bool contains(__charT c) const noexcept {
      if constexpr (sizeof(__charT) == 1) {
        if (__use_table) {
          const unsigned __u = static_cast<unsigned char>(c);
          return (bits[__u / 64] >> (__u % 64)) & 1;
        }
      }
      return __traits::find(s.__data_, s.__size_, c) != nullptr;
    }
  };
  constexpr size_type __scan_forward(basic_string_view set, size_type __pos, bool __want) const noexcept {
    const __char_set __cs(set);
    for (; __pos < __size_; ++__pos)
      if (__cs.contains(__data_[__pos]) == __want)
        return __pos;
    return npos;
  }
  constexpr size_type __scan_backward(basic_string_view set, size_type __pos, bool __want) const noexcept {
    if (__size_ == 0)
      return npos;
    const __char_set __cs(set);
    for (size_type i = __pos < __size_ ? __pos : __size_ - 1;; --i) {
      if (__cs.contains(__data_[i]) == __want)
        return i;
      if (i == 0)
        return npos;
    }
  }

  constexpr void __check_pos(size_type __pos, const char* what) const {
    if (__pos > __size_)
      __ycxx::__detail::__throw_out_of_range(what);
  }
  constexpr size_type __clamp_len(size_type __pos, size_type n) const noexcept {
    return n < __size_ - __pos ? n : __size_ - __pos;
  }

  const __charT* __data_;
  size_type __size_;
};

// ---- [string.view.deduct] ----
template <class _It, class _End>
  requires contiguous_iterator<_It> && sized_sentinel_for<_End, _It>
basic_string_view(_It, _End) -> basic_string_view<iter_value_t<_It>>;
template <class _Rp>
  requires ranges::contiguous_range<_Rp>
basic_string_view(_Rp&&) -> basic_string_view<ranges::range_value_t<_Rp>>;

template <class __charT, class __traits>
constexpr bool ranges::enable_view<basic_string_view<__charT, __traits>> = true;
template <class __charT, class __traits>
constexpr bool ranges::enable_borrowed_range<basic_string_view<__charT, __traits>> = true;

// ---- [string.view.comparison] ----
template <class __charT, class __traits>
constexpr bool operator==(basic_string_view<__charT, __traits> __lhs,
                          type_identity_t<basic_string_view<__charT, __traits>> __rhs) noexcept {
  return __lhs.size() == __rhs.size() && __lhs.compare(__rhs) == 0;
}
template <class __charT, class __traits>
constexpr typename __ycxx::__detail::__sv_comparison_category<__traits>::type operator<=>(
    basic_string_view<__charT, __traits> __lhs, type_identity_t<basic_string_view<__charT, __traits>> __rhs) noexcept {
  using _Rp = typename __ycxx::__detail::__sv_comparison_category<__traits>::type;
  static_assert(is_same_v<_Rp, partial_ordering> || is_same_v<_Rp, weak_ordering> || is_same_v<_Rp, strong_ordering>,
                "std::basic_string_view: traits::comparison_category must be a comparison category type");
  return static_cast<_Rp>(__lhs.compare(__rhs) <=> 0);
}

using string_view = basic_string_view<char>;
using u8string_view = basic_string_view<char8_t>;
using u16string_view = basic_string_view<char16_t>;
using u32string_view = basic_string_view<char32_t>;
using wstring_view = basic_string_view<wchar_t>;

// ---- [string.view.hash] ----
// Hashes the character sequence, as the basic_string specializations will.
template <>
struct hash<string_view> {
  [[nodiscard]] size_t operator()(string_view s) const noexcept {
    return static_cast<size_t>(__ycxx::__detail::__hash_chars(s.data(), s.size()));
  }
};
template <>
struct hash<u8string_view> {
  [[nodiscard]] size_t operator()(u8string_view s) const noexcept {
    return static_cast<size_t>(__ycxx::__detail::__hash_chars(s.data(), s.size()));
  }
};
template <>
struct hash<u16string_view> {
  [[nodiscard]] size_t operator()(u16string_view s) const noexcept {
    return static_cast<size_t>(__ycxx::__detail::__hash_chars(s.data(), s.size()));
  }
};
template <>
struct hash<u32string_view> {
  [[nodiscard]] size_t operator()(u32string_view s) const noexcept {
    return static_cast<size_t>(__ycxx::__detail::__hash_chars(s.data(), s.size()));
  }
};
template <>
struct hash<wstring_view> {
  [[nodiscard]] size_t operator()(wstring_view s) const noexcept {
    return static_cast<size_t>(__ycxx::__detail::__hash_chars(s.data(), s.size()));
  }
};

// ---- [string.view.literals] ----
inline namespace literals {
inline namespace string_view_literals {
constexpr string_view operator""sv(const char* str, size_t __len) noexcept { return string_view{str, __len}; }
constexpr u8string_view operator""sv(const char8_t* str, size_t __len) noexcept { return u8string_view{str, __len}; }
constexpr u16string_view operator""sv(const char16_t* str, size_t __len) noexcept { return u16string_view{str, __len}; }
constexpr u32string_view operator""sv(const char32_t* str, size_t __len) noexcept { return u32string_view{str, __len}; }
constexpr wstring_view operator""sv(const wchar_t* str, size_t __len) noexcept { return wstring_view{str, __len}; }
} // namespace string_view_literals
} // namespace literals

// [string.view.io]: declared here, defined with basic_ostream (ycxx/hosted/ostream.hpp).
template <class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __os, basic_string_view<__charT, __traits> str);

} // namespace std
