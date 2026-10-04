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

namespace std {

template <class charT, class traits = char_traits<charT>>
class basic_string_view;

} // namespace std

namespace ycxx::detail {
// [string.view.cons]/12.5: d.operator ::std::basic_string_view<charT, traits>() is not valid.
template <class D, class charT, class traits>
concept has_string_view_conversion =
    requires(D& d) { d.operator ::std::basic_string_view<charT, traits>(); };

// [string.view.comparison]: traits::comparison_category if valid, otherwise weak_ordering.
template <class traits>
struct sv_comparison_category {
  using type = std::weak_ordering;
};
template <class traits>
  requires requires { typename traits::comparison_category; }
struct sv_comparison_category<traits> {
  using type = typename traits::comparison_category;
};
} // namespace ycxx::detail

namespace std {

template <class charT, class traits>
class basic_string_view {
  static_assert(is_same_v<typename traits::char_type, charT>,
                "std::basic_string_view: traits::char_type must be charT");
  static_assert(!is_array_v<charT> && is_trivially_copyable_v<charT> && is_trivially_default_constructible_v<charT> &&
                    is_standard_layout_v<charT>,
                "std::basic_string_view: charT must be a char-like type");

public:
  using traits_type = traits;
  using value_type = charT;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using reference = value_type&;
  using const_reference = const value_type&;
  using const_iterator = const charT*;
  using iterator = const_iterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = const_reverse_iterator;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  static constexpr size_type npos = size_type(-1);

  // ---- [string.view.cons] ----
  constexpr basic_string_view() noexcept : data_(nullptr), size_(0) {}
  constexpr basic_string_view(const basic_string_view&) noexcept = default;
  constexpr basic_string_view& operator=(const basic_string_view&) noexcept = default;
  constexpr basic_string_view(const charT* str) noexcept : data_(str), size_(traits::length(str)) {} // noexcept: [res.on.exception.handling]/5
  basic_string_view(nullptr_t) = delete;
  constexpr basic_string_view(const charT* str, size_type len) : data_(str), size_(len) {}
  template <class It, class End>
    requires contiguous_iterator<It> && sized_sentinel_for<End, It> && is_same_v<iter_value_t<It>, charT> &&
             (!is_convertible_v<End, size_type>)
  constexpr basic_string_view(It begin, End end) noexcept(noexcept(end - begin))
      : data_(std::to_address(begin)), size_(static_cast<size_type>(end - begin)) {}
  template <class R>
    requires(!is_same_v<remove_cvref_t<R>, basic_string_view>) && ranges::contiguous_range<R> &&
            ranges::sized_range<R> && is_same_v<ranges::range_value_t<R>, charT> &&
            (!is_convertible_v<R, const charT*>) &&
            (!ycxx::detail::has_string_view_conversion<remove_cvref_t<R>, charT, traits>)
  constexpr explicit basic_string_view(R&& r) : data_(ranges::data(r)), size_(static_cast<size_type>(ranges::size(r))) {}

  // ---- [string.view.iterators] ----
  constexpr const_iterator begin() const noexcept { return data_; }
  constexpr const_iterator end() const noexcept { return data_ + size_; }
  constexpr const_iterator cbegin() const noexcept { return begin(); }
  constexpr const_iterator cend() const noexcept { return end(); }
  constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  // ---- [string.view.capacity] ----
  constexpr size_type size() const noexcept { return size_; }
  constexpr size_type length() const noexcept { return size_; }
  constexpr size_type max_size() const noexcept {
    // Pointer differences over the viewed range must be representable.
    constexpr size_type by_ptrdiff = static_cast<size_type>(numeric_limits<ptrdiff_t>::max()) / sizeof(charT);
    return by_ptrdiff;
  }
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

  // ---- [string.view.access] ----
  constexpr const_reference operator[](size_type pos) const {
    ycxx::detail::precondition(pos < size_, "std::basic_string_view::operator[]: index out of range");
    return data_[pos];
  }
  constexpr const_reference at(size_type pos) const {
    if (pos >= size_)
      ycxx::detail::throw_out_of_range("std::basic_string_view::at: index out of range");
    return data_[pos];
  }
  constexpr const_reference front() const {
    ycxx::detail::precondition(size_ != 0, "std::basic_string_view::front: empty");
    return data_[0];
  }
  constexpr const_reference back() const {
    ycxx::detail::precondition(size_ != 0, "std::basic_string_view::back: empty");
    return data_[size_ - 1];
  }
  constexpr const charT* data() const noexcept { return data_; }

  // ---- [string.view.modifiers] ----
  constexpr void remove_prefix(size_type n) {
    ycxx::detail::precondition(n <= size_, "std::basic_string_view::remove_prefix: n > size()");
    data_ += n;
    size_ -= n;
  }
  constexpr void remove_suffix(size_type n) {
    ycxx::detail::precondition(n <= size_, "std::basic_string_view::remove_suffix: n > size()");
    size_ -= n;
  }
  constexpr void swap(basic_string_view& s) noexcept {
    basic_string_view t = *this;
    *this = s;
    s = t;
  }

  // ---- [string.view.ops] ----
  constexpr size_type copy(charT* s, size_type n, size_type pos = 0) const {
    check_pos(pos, "std::basic_string_view::copy: pos > size()");
    size_type rlen = clamp_len(pos, n);
    traits::copy(s, data_ + pos, rlen);
    return rlen;
  }
  constexpr basic_string_view substr(size_type pos = 0, size_type n = npos) const {
    check_pos(pos, "std::basic_string_view::substr: pos > size()");
    return basic_string_view(data_ + pos, clamp_len(pos, n));
  }
  constexpr basic_string_view subview(size_type pos = 0, size_type n = npos) const {
    check_pos(pos, "std::basic_string_view::subview: pos > size()");
    return basic_string_view(data_ + pos, clamp_len(pos, n));
  }

  constexpr int compare(basic_string_view s) const noexcept {
    size_type rlen = size_ < s.size_ ? size_ : s.size_;
    if (int r = rlen == 0 ? 0 : traits::compare(data_, s.data_, rlen); r != 0)
      return r;
    return size_ < s.size_ ? -1 : (size_ > s.size_ ? 1 : 0);
  }
  constexpr int compare(size_type pos1, size_type n1, basic_string_view s) const {
    return substr(pos1, n1).compare(s);
  }
  constexpr int compare(size_type pos1, size_type n1, basic_string_view s, size_type pos2, size_type n2) const {
    return substr(pos1, n1).compare(s.substr(pos2, n2));
  }
  constexpr int compare(const charT* s) const { return compare(basic_string_view(s)); }
  constexpr int compare(size_type pos1, size_type n1, const charT* s) const {
    return substr(pos1, n1).compare(basic_string_view(s));
  }
  constexpr int compare(size_type pos1, size_type n1, const charT* s, size_type n2) const {
    return substr(pos1, n1).compare(basic_string_view(s, n2));
  }

  constexpr bool starts_with(basic_string_view x) const noexcept {
    return size_ >= x.size_ && basic_string_view(data_, x.size_).compare(x) == 0;
  }
  constexpr bool starts_with(charT x) const noexcept { return !empty() && traits::eq(data_[0], x); }
  constexpr bool starts_with(const charT* x) const { return starts_with(basic_string_view(x)); }
  constexpr bool ends_with(basic_string_view x) const noexcept {
    return size_ >= x.size_ && basic_string_view(data_ + (size_ - x.size_), x.size_).compare(x) == 0;
  }
  constexpr bool ends_with(charT x) const noexcept { return !empty() && traits::eq(data_[size_ - 1], x); }
  constexpr bool ends_with(const charT* x) const { return ends_with(basic_string_view(x)); }
  constexpr bool contains(basic_string_view x) const noexcept { return find(x) != npos; }
  constexpr bool contains(charT x) const noexcept { return find(x) != npos; }
  constexpr bool contains(const charT* x) const { return find(x) != npos; }

  // ---- [string.view.find] ----
  constexpr size_type find(basic_string_view s, size_type pos = 0) const noexcept {
    if (s.size_ > size_ || pos > size_ - s.size_)
      return npos;
    if (s.size_ == 0)
      return pos;
    // Scan for the first character with traits::find, then compare the rest.
    const charT* const last = data_ + (size_ - s.size_) + 1;
    for (const charT* p = data_ + pos; p < last;) {
      p = traits::find(p, static_cast<size_type>(last - p), s.data_[0]);
      if (!p)
        return npos;
      if (traits::compare(p + 1, s.data_ + 1, s.size_ - 1) == 0)
        return static_cast<size_type>(p - data_);
      ++p;
    }
    return npos;
  }
  constexpr size_type find(charT c, size_type pos = 0) const noexcept {
    if (pos >= size_)
      return npos;
    const charT* p = traits::find(data_ + pos, size_ - pos, c);
    return p ? static_cast<size_type>(p - data_) : npos;
  }
  constexpr size_type find(const charT* s, size_type pos, size_type n) const {
    return find(basic_string_view(s, n), pos);
  }
  constexpr size_type find(const charT* s, size_type pos = 0) const { return find(basic_string_view(s), pos); }

  constexpr size_type rfind(basic_string_view s, size_type pos = npos) const noexcept {
    if (s.size_ > size_)
      return npos;
    size_type xpos = size_ - s.size_;
    if (pos < xpos)
      xpos = pos;
    for (;; --xpos) {
      if (s.size_ == 0 || traits::compare(data_ + xpos, s.data_, s.size_) == 0)
        return xpos;
      if (xpos == 0)
        return npos;
    }
  }
  constexpr size_type rfind(charT c, size_type pos = npos) const noexcept {
    return rfind(basic_string_view(__builtin_addressof(c), 1), pos);
  }
  constexpr size_type rfind(const charT* s, size_type pos, size_type n) const {
    return rfind(basic_string_view(s, n), pos);
  }
  constexpr size_type rfind(const charT* s, size_type pos = npos) const { return rfind(basic_string_view(s), pos); }

  constexpr size_type find_first_of(basic_string_view s, size_type pos = 0) const noexcept {
    return scan_forward(s, pos, true);
  }
  constexpr size_type find_first_of(charT c, size_type pos = 0) const noexcept { return find(c, pos); }
  constexpr size_type find_first_of(const charT* s, size_type pos, size_type n) const {
    return find_first_of(basic_string_view(s, n), pos);
  }
  constexpr size_type find_first_of(const charT* s, size_type pos = 0) const {
    return find_first_of(basic_string_view(s), pos);
  }

  constexpr size_type find_last_of(basic_string_view s, size_type pos = npos) const noexcept {
    return scan_backward(s, pos, true);
  }
  constexpr size_type find_last_of(charT c, size_type pos = npos) const noexcept { return rfind(c, pos); }
  constexpr size_type find_last_of(const charT* s, size_type pos, size_type n) const {
    return find_last_of(basic_string_view(s, n), pos);
  }
  constexpr size_type find_last_of(const charT* s, size_type pos = npos) const {
    return find_last_of(basic_string_view(s), pos);
  }

  constexpr size_type find_first_not_of(basic_string_view s, size_type pos = 0) const noexcept {
    return scan_forward(s, pos, false);
  }
  constexpr size_type find_first_not_of(charT c, size_type pos = 0) const noexcept {
    return find_first_not_of(basic_string_view(__builtin_addressof(c), 1), pos);
  }
  constexpr size_type find_first_not_of(const charT* s, size_type pos, size_type n) const {
    return find_first_not_of(basic_string_view(s, n), pos);
  }
  constexpr size_type find_first_not_of(const charT* s, size_type pos = 0) const {
    return find_first_not_of(basic_string_view(s), pos);
  }

  constexpr size_type find_last_not_of(basic_string_view s, size_type pos = npos) const noexcept {
    return scan_backward(s, pos, false);
  }
  constexpr size_type find_last_not_of(charT c, size_type pos = npos) const noexcept {
    return find_last_not_of(basic_string_view(__builtin_addressof(c), 1), pos);
  }
  constexpr size_type find_last_not_of(const charT* s, size_type pos, size_type n) const {
    return find_last_not_of(basic_string_view(s, n), pos);
  }
  constexpr size_type find_last_not_of(const charT* s, size_type pos = npos) const {
    return find_last_not_of(basic_string_view(s), pos);
  }

private:
  // Membership test for the find_*_of family. For byte-sized characters with the standard
  // traits (whose eq is ==), a set of more than a few characters is turned into a 256-bit table,
  // so the scan is O(size() + s.size()) instead of O(size() * s.size()).
  struct char_set {
    const basic_string_view& s;
    unsigned long long bits[4] = {};
    bool use_table = false;
    constexpr explicit char_set(const basic_string_view& set) : s(set) {
      if constexpr (sizeof(charT) == 1 && is_same_v<traits, char_traits<charT>>) {
        if (set.size_ > 8) {
          use_table = true;
          for (charT c : set) {
            const unsigned u = static_cast<unsigned char>(c);
            bits[u / 64] |= 1ull << (u % 64);
          }
        }
      }
    }
    constexpr bool contains(charT c) const noexcept {
      if constexpr (sizeof(charT) == 1) {
        if (use_table) {
          const unsigned u = static_cast<unsigned char>(c);
          return (bits[u / 64] >> (u % 64)) & 1;
        }
      }
      return traits::find(s.data_, s.size_, c) != nullptr;
    }
  };
  constexpr size_type scan_forward(basic_string_view set, size_type pos, bool want) const noexcept {
    const char_set cs(set);
    for (; pos < size_; ++pos)
      if (cs.contains(data_[pos]) == want)
        return pos;
    return npos;
  }
  constexpr size_type scan_backward(basic_string_view set, size_type pos, bool want) const noexcept {
    if (size_ == 0)
      return npos;
    const char_set cs(set);
    for (size_type i = pos < size_ ? pos : size_ - 1;; --i) {
      if (cs.contains(data_[i]) == want)
        return i;
      if (i == 0)
        return npos;
    }
  }

  constexpr void check_pos(size_type pos, const char* what) const {
    if (pos > size_)
      ycxx::detail::throw_out_of_range(what);
  }
  constexpr size_type clamp_len(size_type pos, size_type n) const noexcept {
    return n < size_ - pos ? n : size_ - pos;
  }

  const charT* data_;
  size_type size_;
};

// ---- [string.view.deduct] ----
template <class It, class End>
  requires contiguous_iterator<It> && sized_sentinel_for<End, It>
basic_string_view(It, End) -> basic_string_view<iter_value_t<It>>;
template <class R>
  requires ranges::contiguous_range<R>
basic_string_view(R&&) -> basic_string_view<ranges::range_value_t<R>>;

template <class charT, class traits>
constexpr bool ranges::enable_view<basic_string_view<charT, traits>> = true;
template <class charT, class traits>
constexpr bool ranges::enable_borrowed_range<basic_string_view<charT, traits>> = true;

// ---- [string.view.comparison] ----
template <class charT, class traits>
constexpr bool operator==(basic_string_view<charT, traits> lhs,
                          type_identity_t<basic_string_view<charT, traits>> rhs) noexcept {
  return lhs.size() == rhs.size() && lhs.compare(rhs) == 0;
}
template <class charT, class traits>
constexpr typename ycxx::detail::sv_comparison_category<traits>::type operator<=>(
    basic_string_view<charT, traits> lhs, type_identity_t<basic_string_view<charT, traits>> rhs) noexcept {
  using R = typename ycxx::detail::sv_comparison_category<traits>::type;
  static_assert(is_same_v<R, partial_ordering> || is_same_v<R, weak_ordering> || is_same_v<R, strong_ordering>,
                "std::basic_string_view: traits::comparison_category must be a comparison category type");
  return static_cast<R>(lhs.compare(rhs) <=> 0);
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
    return static_cast<size_t>(ycxx::detail::hash_chars(s.data(), s.size()));
  }
};
template <>
struct hash<u8string_view> {
  [[nodiscard]] size_t operator()(u8string_view s) const noexcept {
    return static_cast<size_t>(ycxx::detail::hash_chars(s.data(), s.size()));
  }
};
template <>
struct hash<u16string_view> {
  [[nodiscard]] size_t operator()(u16string_view s) const noexcept {
    return static_cast<size_t>(ycxx::detail::hash_chars(s.data(), s.size()));
  }
};
template <>
struct hash<u32string_view> {
  [[nodiscard]] size_t operator()(u32string_view s) const noexcept {
    return static_cast<size_t>(ycxx::detail::hash_chars(s.data(), s.size()));
  }
};
template <>
struct hash<wstring_view> {
  [[nodiscard]] size_t operator()(wstring_view s) const noexcept {
    return static_cast<size_t>(ycxx::detail::hash_chars(s.data(), s.size()));
  }
};

// ---- [string.view.literals] ----
inline namespace literals {
inline namespace string_view_literals {
constexpr string_view operator""sv(const char* str, size_t len) noexcept { return string_view{str, len}; }
constexpr u8string_view operator""sv(const char8_t* str, size_t len) noexcept { return u8string_view{str, len}; }
constexpr u16string_view operator""sv(const char16_t* str, size_t len) noexcept { return u16string_view{str, len}; }
constexpr u32string_view operator""sv(const char32_t* str, size_t len) noexcept { return u32string_view{str, len}; }
constexpr wstring_view operator""sv(const wchar_t* str, size_t len) noexcept { return wstring_view{str, len}; }
} // namespace string_view_literals
} // namespace literals

// [string.view.io]: declared here, defined with basic_ostream (ycxx/hosted/ostream.hpp).
template <class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, basic_string_view<charT, traits> str);

} // namespace std
