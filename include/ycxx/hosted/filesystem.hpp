// libycxx hosted: the file system library ([filesystems]) for POSIX (Linux, macOS).
//
// path::value_type is char and the native ordinary encoding is taken to be UTF-8; wchar_t is
// UTF-32 (UTF-16 where wchar_t has 16 bits). Paths in the other encoded character types are
// converted directly between the Unicode encodings (an ill-formed sequence becomes U+FFFD); a
// path built with a locale converts through that locale's codecvt<wchar_t, char, mbstate_t>
// ([fs.path.construct]/6). There are no root-names: a leading "//" is a root-directory. The
// native format is the generic one; the generic observers write each directory-separator (a run
// of slashes) as a single slash.
//
// Everything that touches the file system is in the hosted runtime (src/hosted/filesystem.cpp):
// the operations with an error_code& argument, directory iteration and directory_entry's
// refresh and observers. The forms that report errors by throwing are inline here: each calls
// the error_code form and throws filesystem_error through ycxx::detail::raise_with, so under
// -fno-exceptions they reach ycxx_error_handler instead. The lexical members of path that do
// not depend on a template argument are out of line as well.
//
// Not provided: formatter<filesystem::path, charT> ([fs.path.fmtr]) — <format> is not merged
// yet; display_string() and generic_display_string() return what it would produce for char
// (the native string, since no transcoding is needed).
#pragma once

#include <ycxx/core/basic_string.hpp>
#include <ycxx/core/compare.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/hash.hpp>
#include <ycxx/core/iterator_core.hpp>
#include <ycxx/core/iterator_ops.hpp>
#include <ycxx/core/range_access.hpp>
#include <ycxx/core/ranges_base.hpp>
#include <ycxx/core/ranges_subrange.hpp> // ranges::view, for the enable_view specializations
#include <ycxx/core/shared_ptr.hpp>
#include <ycxx/core/string_view.hpp>
#include <ycxx/core/system_error.hpp>
#include <ycxx/hosted/file_clock.hpp>
#include <ycxx/hosted/iomanip.hpp>

namespace std::filesystem {
class path;
class directory_entry;
} // namespace std::filesystem

namespace ycxx::detail {

// [fs.req]/1: the encoded character types.
template <class C>
concept fs_echar = std::is_same_v<C, char> || std::is_same_v<C, wchar_t> || std::is_same_v<C, char8_t> ||
                   std::is_same_v<C, char16_t> || std::is_same_v<C, char32_t>;

// ---- Unicode transcoding ([fs.path.type.cvt]) ----
// The encoding family of a code unit type: 1 UTF-8 (char is the native ordinary encoding, taken
// to be UTF-8), 2 UTF-16, 4 UTF-32.
template <class C>
inline constexpr int utf_width = std::is_same_v<C, char16_t> ? 2 : (std::is_same_v<C, char32_t> || sizeof(C) == 4) ? 4 : sizeof(C) == 2 ? 2 : 1;

// Decodes one code point of code unit type C from [p, e) (p != e; e may be a sentinel) and
// advances p past it. An ill-formed or truncated sequence yields U+FFFD and consumes its maximal
// subpart (at least one code unit). A code unit is read once and only before p moves past it,
// so this works on single-pass input iterators.
template <class C, class I, class E>
constexpr char32_t utf_decode(I& p, const E& e) {
  constexpr int w = utf_width<C>;
  if constexpr (w == 4) {
    char32_t c = static_cast<char32_t>(*p);
    ++p;
    return (c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF)) ? char32_t(0xFFFD) : c;
  } else if constexpr (w == 2) {
    char32_t c = static_cast<char16_t>(*p);
    ++p;
    if (c < 0xD800 || c > 0xDFFF)
      return c;
    if (c <= 0xDBFF && !(p == e)) {
      char32_t d = static_cast<char16_t>(*p);
      if (d >= 0xDC00 && d <= 0xDFFF) {
        ++p;
        return 0x10000 + ((c - 0xD800) << 10) + (d - 0xDC00);
      }
    }
    return 0xFFFD;
  } else {
    unsigned c = static_cast<unsigned char>(*p);
    ++p;
    if (c < 0x80)
      return c;
    int n;
    char32_t cp;
    unsigned lo = 0x80, hi = 0xBF; // the valid range of the second byte
    if (c >= 0xC2 && c <= 0xDF) {
      n = 1;
      cp = c & 0x1F;
    } else if (c >= 0xE0 && c <= 0xEF) {
      n = 2;
      cp = c & 0x0F;
      if (c == 0xE0)
        lo = 0xA0;
      else if (c == 0xED)
        hi = 0x9F;
    } else if (c >= 0xF0 && c <= 0xF4) {
      n = 3;
      cp = c & 0x07;
      if (c == 0xF0)
        lo = 0x90;
      else if (c == 0xF4)
        hi = 0x8F;
    } else {
      return 0xFFFD;
    }
    for (int i = 0; i < n; ++i) {
      if (p == e)
        return 0xFFFD;
      unsigned b = static_cast<unsigned char>(*p);
      if (i == 0 ? (b < lo || b > hi) : (b & 0xC0) != 0x80)
        return 0xFFFD;
      cp = (cp << 6) | (b & 0x3F);
      ++p;
    }
    return cp;
  }
}

// Appends the encoding of code point c in the code units of out's character type.
template <class S>
constexpr void utf_encode(S& out, char32_t c) {
  using C = typename S::value_type;
  constexpr int w = utf_width<C>;
  if constexpr (w == 4) {
    out.push_back(static_cast<C>(c));
  } else if constexpr (w == 2) {
    if (c < 0x10000) {
      out.push_back(static_cast<C>(c));
    } else {
      c -= 0x10000;
      out.push_back(static_cast<C>(0xD800 + (c >> 10)));
      out.push_back(static_cast<C>(0xDC00 + (c & 0x3FF)));
    }
  } else if (c < 0x80) {
    out.push_back(static_cast<C>(c));
  } else if (c < 0x800) {
    out.push_back(static_cast<C>(0xC0 | (c >> 6)));
    out.push_back(static_cast<C>(0x80 | (c & 0x3F)));
  } else if (c < 0x10000) {
    out.push_back(static_cast<C>(0xE0 | (c >> 12)));
    out.push_back(static_cast<C>(0x80 | ((c >> 6) & 0x3F)));
    out.push_back(static_cast<C>(0x80 | (c & 0x3F)));
  } else {
    out.push_back(static_cast<C>(0xF0 | (c >> 18)));
    out.push_back(static_cast<C>(0x80 | ((c >> 12) & 0x3F)));
    out.push_back(static_cast<C>(0x80 | ((c >> 6) & 0x3F)));
    out.push_back(static_cast<C>(0x80 | (c & 0x3F)));
  }
}

// The end of a null-terminated sequence ([fs.path.req]/1.3-1.4): the first iterator whose
// element equals the value type's value-initialized value.
struct fs_ntcts_end {
  template <class I>
  friend constexpr bool operator==(const I& i, fs_ntcts_end) {
    return *i == std::remove_cvref_t<decltype(*i)>();
  }
};

// Appends [p, e) of code unit type C to out, converted to out's encoding. Code units of the
// same encoding are copied unchanged ([fs.path.type.cvt]/3: an argument already in the value
// type is not modified). Nothing is allocated beyond out's growth.
template <class C, class S, class I, class E>
constexpr void utf_append(S& out, I p, E e) {
  using O = typename S::value_type;
  if constexpr (utf_width<O> == utf_width<C>) {
    for (; !(p == e); ++p)
      out.push_back(static_cast<O>(*p));
  } else {
    while (!(p == e))
      ::ycxx::detail::utf_encode(out, ::ycxx::detail::utf_decode<C>(p, e));
  }
}

// ---- [fs.path.req]: Source arguments ----
template <class T>
struct fs_string_source {
  static constexpr bool value = false;
};
template <class C, class Tr, class A>
struct fs_string_source<std::basic_string<C, Tr, A>> {
  static constexpr bool value = fs_echar<C>;
};
template <class C, class Tr>
struct fs_string_source<std::basic_string_view<C, Tr>> {
  static constexpr bool value = fs_echar<C>;
};

template <class S>
concept fs_ntcts_source = requires { typename std::iterator_traits<std::decay_t<S>>::value_type; } &&
                          fs_echar<std::remove_cv_t<typename std::iterator_traits<std::decay_t<S>>::value_type>>;

// [fs.path.req]/2: a Source is a basic_string, a basic_string_view, or an iterator (a character
// array after decay) over a null-terminated sequence of an encoded character type; never path.
template <class S>
concept fs_source = !std::is_same_v<std::remove_cvref_t<S>, std::filesystem::path> &&
                    (fs_string_source<std::remove_cvref_t<S>>::value || fs_ntcts_source<S>);

// [fs.req]/3: an InputIterator whose value type is an encoded character type.
template <class I>
concept fs_char_iterator = requires { typename std::iterator_traits<I>::value_type; } &&
                           fs_echar<std::remove_cv_t<typename std::iterator_traits<I>::value_type>>;

template <class S, bool = fs_string_source<std::remove_cvref_t<S>>::value>
struct fs_source_char {
  using type = std::remove_cv_t<typename std::iterator_traits<std::decay_t<S>>::value_type>;
};
template <class S>
struct fs_source_char<S, true> {
  using type = typename std::remove_cvref_t<S>::value_type;
};
template <class S>
using fs_source_char_t = typename fs_source_char<S>::type;

// Appends the native (char, UTF-8) form of [first, last) to out.
template <class I>
void fs_append_range(std::string& out, I first, I last) {
  using C = std::remove_cv_t<typename std::iterator_traits<I>::value_type>;
  if constexpr (std::is_same_v<C, char> && std::is_pointer_v<I>)
    out.append(first, static_cast<std::size_t>(last - first));
  else
    ::ycxx::detail::utf_append<C>(out, static_cast<I&&>(first), static_cast<I&&>(last));
}

// Appends the native form of the effective range of a Source ([fs.path.req]/1) to out.
template <class S>
void fs_append_source(std::string& out, const S& s) {
  using C = fs_source_char_t<S>;
  if constexpr (fs_string_source<S>::value) {
    if constexpr (std::is_same_v<C, char>)
      out.append(s.data(), s.size());
    else
      ::ycxx::detail::utf_append<C>(out, s.data(), s.data() + s.size());
  } else {
    auto it = s; // a character array decays to a pointer
    if constexpr (std::is_same_v<C, char> && std::is_pointer_v<decltype(it)>)
      out.append(it);
    else
      ::ycxx::detail::utf_append<C>(out, static_cast<decltype(it)&&>(it), fs_ntcts_end());
  }
}

template <class S>
std::string fs_native_source(const S& s) {
  std::string out;
  ::ycxx::detail::fs_append_source(out, s);
  return out;
}
template <class I>
std::string fs_native_range(I first, I last) {
  std::string out;
  ::ycxx::detail::fs_append_range(out, static_cast<I&&>(first), static_cast<I&&>(last));
  return out;
}

// [fs.path.construct]/6: chars converted to wide characters by the locale's
// codecvt<wchar_t, char, mbstate_t>, then to the native encoding.
std::string fs_native_through_locale(const char* first, const char* last, const std::locale& loc);

// The native pathname converted to basic_string<EcharT, traits, Allocator>, allocated by a.
// With `generic`, in the generic format ([fs.path.generic.obs]/1): each directory-separator (a
// run of slashes) is a single slash.
template <class EcharT, class traits, class Allocator>
std::basic_string<EcharT, traits, Allocator> fs_convert_out(const std::string& s, const Allocator& a,
                                                            bool generic = false) {
  std::basic_string<EcharT, traits, Allocator> out(a);
  const char* p = s.data();
  const char* const e = p + s.size();
  if constexpr (utf_width<EcharT> == 1)
    out.reserve(s.size());
  while (p != e) {
    const char* q = p;
    while (q != e && *q != '/')
      ++q;
    ::ycxx::detail::utf_append<char>(out, p, q);
    if (q == e)
      break;
    out.push_back(static_cast<EcharT>('/'));
    p = q + 1;
    if (generic)
      while (p != e && *p == '/')
        ++p;
  }
  return out;
}

} // namespace ycxx::detail

namespace std::filesystem {

// [fs.class.path]
class path {
public:
  using value_type = char;
  using string_type = basic_string<value_type>;
  static constexpr value_type preferred_separator = '/';

  // [fs.enum.path.format]: on POSIX both formats are the same, so the format is ignored.
  enum format { native_format, generic_format, auto_format };

  class iterator;
  using const_iterator = iterator;

  // [fs.path.construct]
  path() noexcept {}
  path(const path& p) = default;
  path(path&& p) noexcept = default;
  path(string_type&& source, format = auto_format) : s_(static_cast<string_type&&>(source)) {}
  template <class Source>
    requires ycxx::detail::fs_source<Source>
  path(const Source& source, format = auto_format) {
    ycxx::detail::fs_append_source(s_, source);
  }
  template <class InputIterator>
    requires ycxx::detail::fs_char_iterator<InputIterator>
  path(InputIterator first, InputIterator last, format = auto_format) {
    ycxx::detail::fs_append_range(s_, first, last);
  }
  template <class Source>
    requires ycxx::detail::fs_source<Source>
  path(const Source& source, const locale& loc, format = auto_format) {
    static_assert(is_same_v<ycxx::detail::fs_source_char_t<Source>, char>,
                  "[fs.path.construct]/5: the value type of Source must be char");
    string_type tmp = ycxx::detail::fs_native_source(source);
    s_ = ycxx::detail::fs_native_through_locale(tmp.data(), tmp.data() + tmp.size(), loc);
  }
  template <class InputIterator>
    requires ycxx::detail::fs_char_iterator<InputIterator>
  path(InputIterator first, InputIterator last, const locale& loc, format = auto_format) {
    static_assert(is_same_v<remove_cv_t<typename iterator_traits<InputIterator>::value_type>, char>,
                  "[fs.path.construct]/5: the value type of InputIterator must be char");
    string_type tmp = ycxx::detail::fs_native_range(first, last);
    s_ = ycxx::detail::fs_native_through_locale(tmp.data(), tmp.data() + tmp.size(), loc);
  }
  ~path() = default;

  // [fs.path.assign]
  path& operator=(const path& p) = default;
  path& operator=(path&& p) noexcept {
    if (this != __builtin_addressof(p))
      s_ = static_cast<string_type&&>(p.s_);
    return *this;
  }
  path& operator=(string_type&& source) { return assign(static_cast<string_type&&>(source)); }
  path& assign(string_type&& source) {
    s_ = static_cast<string_type&&>(source);
    return *this;
  }
  template <class Source>
    requires ycxx::detail::fs_source<Source>
  path& operator=(const Source& source) {
    return assign(source);
  }
  template <class Source>
    requires ycxx::detail::fs_source<Source>
  path& assign(const Source& source) {
    s_.clear(); // keeps the capacity: no allocation when the pathname fits
    ycxx::detail::fs_append_source(s_, source);
    return *this;
  }
  template <class InputIterator>
    requires ycxx::detail::fs_char_iterator<InputIterator>
  path& assign(InputIterator first, InputIterator last) {
    s_.clear();
    ycxx::detail::fs_append_range(s_, first, last);
    return *this;
  }

  // [fs.path.append]
  path& operator/=(const path& p);
  template <class Source>
    requires ycxx::detail::fs_source<Source>
  path& operator/=(const Source& source) {
    return operator/=(path(source));
  }
  template <class Source>
    requires ycxx::detail::fs_source<Source>
  path& append(const Source& source) {
    return operator/=(path(source));
  }
  template <class InputIterator>
    requires ycxx::detail::fs_char_iterator<InputIterator>
  path& append(InputIterator first, InputIterator last) {
    return operator/=(path(first, last));
  }

  // [fs.path.concat]
  path& operator+=(const path& x) {
    s_.append(x.s_);
    return *this;
  }
  path& operator+=(const string_type& x) {
    s_.append(x);
    return *this;
  }
  path& operator+=(basic_string_view<value_type> x) {
    s_.append(x);
    return *this;
  }
  path& operator+=(const value_type* x) {
    s_.append(x);
    return *this;
  }
  path& operator+=(value_type x) {
    s_.push_back(x);
    return *this;
  }
  template <class Source>
    requires ycxx::detail::fs_source<Source>
  path& operator+=(const Source& x) {
    return concat(x);
  }
  template <class EcharT>
    requires ycxx::detail::fs_echar<EcharT>
  path& operator+=(EcharT x) {
    return *this += basic_string_view<EcharT>(__builtin_addressof(x), 1);
  }
  template <class Source>
    requires ycxx::detail::fs_source<Source>
  path& concat(const Source& x) {
    ycxx::detail::fs_append_source(s_, x);
    return *this;
  }
  template <class InputIterator>
    requires ycxx::detail::fs_char_iterator<InputIterator>
  path& concat(InputIterator first, InputIterator last) {
    ycxx::detail::fs_append_range(s_, first, last);
    return *this;
  }

  // [fs.path.modifiers]
  void clear() noexcept { s_.clear(); }
  path& make_preferred() { return *this; } // '/' is the only separator
  path& remove_filename();
  path& replace_filename(const path& replacement);
  path& replace_extension(const path& replacement = path());
  void swap(path& rhs) noexcept { s_.swap(rhs.s_); }

  // [fs.path.nonmember]
  friend bool operator==(const path& lhs, const path& rhs) noexcept { return lhs.compare(rhs) == 0; }
  friend strong_ordering operator<=>(const path& lhs, const path& rhs) noexcept { return lhs.compare(rhs) <=> 0; }
  friend path operator/(const path& lhs, const path& rhs) {
    path r(lhs);
    r /= rhs;
    return r;
  }

  // [fs.path.native.obs]
  const string_type& native() const noexcept { return s_; }
  const value_type* c_str() const noexcept { return s_.c_str(); }
  operator string_type() const { return s_; }
  template <class EcharT, class traits = char_traits<EcharT>, class Allocator = allocator<EcharT>>
    requires ycxx::detail::fs_echar<EcharT>
  basic_string<EcharT, traits, Allocator> string(const Allocator& a = Allocator()) const {
    return ycxx::detail::fs_convert_out<EcharT, traits, Allocator>(s_, a);
  }
  std::string string() const { return s_; } // [depr.fs.path.obs]
  std::string display_string() const { return s_; }
  std::string native_encoded_string() const { return s_; }
  std::wstring wstring() const { return string<wchar_t>(); }
  std::u8string u8string() const { return string<char8_t>(); }
  std::u16string u16string() const { return string<char16_t>(); }
  std::u32string u32string() const { return string<char32_t>(); }

  // [fs.path.generic.obs]: the native format with each run of slashes written as one.
  template <class EcharT, class traits = char_traits<EcharT>, class Allocator = allocator<EcharT>>
    requires ycxx::detail::fs_echar<EcharT>
  basic_string<EcharT, traits, Allocator> generic_string(const Allocator& a = Allocator()) const {
    return ycxx::detail::fs_convert_out<EcharT, traits, Allocator>(s_, a, true);
  }
  std::string generic_string() const { return generic_string<char>(); } // [depr.fs.path.obs]
  std::string generic_display_string() const { return generic_string<char>(); }
  std::string generic_native_encoded_string() const { return generic_string<char>(); }
  std::wstring generic_wstring() const { return generic_string<wchar_t>(); }
  std::u8string generic_u8string() const { return generic_string<char8_t>(); }
  std::u16string generic_u16string() const { return generic_string<char16_t>(); }
  std::u32string generic_u32string() const { return generic_string<char32_t>(); }

  // [fs.path.compare]
  int compare(const path& p) const noexcept;
  int compare(const string_type& s) const { return compare(basic_string_view<value_type>(s)); }
  int compare(basic_string_view<value_type> s) const;
  int compare(const value_type* s) const { return compare(basic_string_view<value_type>(s)); }

  // [fs.path.decompose]
  path root_name() const;
  path root_directory() const;
  path root_path() const;
  path relative_path() const;
  path parent_path() const;
  path filename() const;
  path stem() const;
  path extension() const;

  // [fs.path.query]
  [[nodiscard]] bool empty() const noexcept { return s_.empty(); }
  bool has_root_name() const { return false; }
  bool has_root_directory() const { return !s_.empty() && s_[0] == '/'; }
  bool has_root_path() const { return has_root_directory(); }
  bool has_relative_path() const;
  bool has_parent_path() const;
  bool has_filename() const;
  bool has_stem() const;
  bool has_extension() const;
  bool is_absolute() const { return has_root_directory(); }
  bool is_relative() const { return !is_absolute(); }

  // [fs.path.gen]
  path lexically_normal() const;
  path lexically_relative(const path& base) const;
  path lexically_proximate(const path& base) const;

  // [fs.path.itr]
  iterator begin() const;
  iterator end() const;

  // [fs.path.io]
  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const path& p) {
    os << std::quoted(p.string<charT, traits>());
    return os;
  }
  template <class charT, class traits>
  friend basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, path& p) {
    basic_string<charT, traits> tmp;
    is >> std::quoted(tmp);
    p = tmp;
    return is;
  }

private:
  string_type s_;
};

// [fs.path.itr]: a bidirectional iterator over the elements; the element it designates is held
// in the iterator (a stashing iterator, as [fs.path.itr]/2 permits). pos_ is the offset of the
// element in the pathname: 0 for the root-directory, the first character of a filename, the
// offset of the last separator for the empty element after a trailing separator, and the
// pathname's size for the end iterator.
class path::iterator {
public:
  using iterator_category = bidirectional_iterator_tag;
  using value_type = path;
  using difference_type = ptrdiff_t;
  using pointer = const path*;
  using reference = const path&;

  iterator() = default;
  reference operator*() const { return elem_; }
  pointer operator->() const { return __builtin_addressof(elem_); }
  iterator& operator++();
  iterator operator++(int) {
    iterator t(*this);
    ++*this;
    return t;
  }
  iterator& operator--();
  iterator operator--(int) {
    iterator t(*this);
    --*this;
    return t;
  }
  friend bool operator==(const iterator& a, const iterator& b) noexcept { return a.p_ == b.p_ && a.pos_ == b.pos_; }

private:
  friend class path;
  void load();

  const path* p_ = nullptr;
  size_t pos_ = 0;
  path elem_;
};

// [fs.path.nonmember]
inline void swap(path& lhs, path& rhs) noexcept { lhs.swap(rhs); }
size_t hash_value(const path& p) noexcept;

// [depr.fs.path.factory]: the native encoding is UTF-8 already.
template <class Source>
  requires ycxx::detail::fs_source<Source>
path u8path(const Source& source) {
  static_assert(is_same_v<ycxx::detail::fs_source_char_t<Source>, char> ||
                    is_same_v<ycxx::detail::fs_source_char_t<Source>, char8_t>,
                "[depr.fs.path.factory]/2: the value type of Source must be char or char8_t");
  return path(source);
}
template <class InputIterator>
  requires ycxx::detail::fs_char_iterator<InputIterator>
path u8path(InputIterator first, InputIterator last) {
  using C = remove_cv_t<typename iterator_traits<InputIterator>::value_type>;
  static_assert(is_same_v<C, char> || is_same_v<C, char8_t>,
                "[depr.fs.path.factory]/2: the value type of InputIterator must be char or char8_t");
  return path(first, last);
}

} // namespace std::filesystem

namespace ycxx::detail {
// The shared state of a filesystem_error: copies of an exception share it, so copying never
// allocates.
struct fs_error_data {
  std::filesystem::path p1, p2;
  std::string what;
};
} // namespace ycxx::detail

namespace std::filesystem {

// [fs.class.filesystem.error]. The constructors and the destructor (the key function) are in the
// hosted runtime. what() is "filesystem error: " followed by system_error::what() and the
// non-empty paths in brackets.
class filesystem_error : public system_error {
public:
  filesystem_error(const string& what_arg, error_code ec);
  filesystem_error(const string& what_arg, const path& p1, error_code ec);
  filesystem_error(const string& what_arg, const path& p1, const path& p2, error_code ec);
  filesystem_error(const filesystem_error&) noexcept = default;
  filesystem_error& operator=(const filesystem_error&) noexcept = default;
  ~filesystem_error() override;

  const path& path1() const noexcept { return data_->p1; }
  const path& path2() const noexcept { return data_->p2; }
  const char* what() const noexcept override { return data_->what.c_str(); }

private:
  shared_ptr<const ycxx::detail::fs_error_data> data_;
};

// [fs.enum.file.type]
enum class file_type : signed char {
  none = 0,
  not_found = -1,
  regular = 1,
  directory = 2,
  symlink = 3,
  block = 4,
  character = 5,
  fifo = 6,
  socket = 7,
  unknown = 8,
};

// [fs.enum.copy.opts]
enum class copy_options : unsigned short {
  none = 0,
  skip_existing = 1,
  overwrite_existing = 2,
  update_existing = 4,
  recursive = 8,
  copy_symlinks = 16,
  skip_symlinks = 32,
  directories_only = 64,
  create_symlinks = 128,
  create_hard_links = 256,
};

// [fs.enum.perms]
enum class perms : unsigned {
  none = 0,
  owner_read = 0400,
  owner_write = 0200,
  owner_exec = 0100,
  owner_all = 0700,
  group_read = 040,
  group_write = 020,
  group_exec = 010,
  group_all = 070,
  others_read = 04,
  others_write = 02,
  others_exec = 01,
  others_all = 07,
  all = 0777,
  set_uid = 04000,
  set_gid = 02000,
  sticky_bit = 01000,
  mask = 07777,
  unknown = 0xFFFF,
};

// [fs.enum.perm.opts]
enum class perm_options : unsigned char {
  replace = 1,
  add = 2,
  remove = 4,
  nofollow = 8,
};

// [fs.enum.dir.opts]
enum class directory_options : unsigned char {
  none = 0,
  follow_directory_symlink = 1,
  skip_permission_denied = 2,
};

} // namespace std::filesystem

namespace ycxx::detail {
template <class E>
concept fs_bitmask = std::is_same_v<E, std::filesystem::copy_options> || std::is_same_v<E, std::filesystem::perms> ||
                     std::is_same_v<E, std::filesystem::perm_options> ||
                     std::is_same_v<E, std::filesystem::directory_options>;
} // namespace ycxx::detail

namespace std::filesystem {

// [bitmask.types]: the operators of the four bitmask types.
template <ycxx::detail::fs_bitmask E>
constexpr E operator&(E x, E y) noexcept {
  return static_cast<E>(static_cast<underlying_type_t<E>>(x) & static_cast<underlying_type_t<E>>(y));
}
template <ycxx::detail::fs_bitmask E>
constexpr E operator|(E x, E y) noexcept {
  return static_cast<E>(static_cast<underlying_type_t<E>>(x) | static_cast<underlying_type_t<E>>(y));
}
template <ycxx::detail::fs_bitmask E>
constexpr E operator^(E x, E y) noexcept {
  return static_cast<E>(static_cast<underlying_type_t<E>>(x) ^ static_cast<underlying_type_t<E>>(y));
}
template <ycxx::detail::fs_bitmask E>
constexpr E operator~(E x) noexcept {
  return static_cast<E>(static_cast<underlying_type_t<E>>(~static_cast<underlying_type_t<E>>(x)));
}
template <ycxx::detail::fs_bitmask E>
constexpr E& operator&=(E& x, E y) noexcept {
  return x = x & y;
}
template <ycxx::detail::fs_bitmask E>
constexpr E& operator|=(E& x, E y) noexcept {
  return x = x | y;
}
template <ycxx::detail::fs_bitmask E>
constexpr E& operator^=(E& x, E y) noexcept {
  return x = x ^ y;
}

// [fs.class.file.status]
class file_status {
public:
  file_status() noexcept : file_status(file_type::none) {}
  explicit file_status(file_type ft, perms prms = perms::unknown) noexcept : type_(ft), perms_(prms) {}
  file_status(const file_status&) noexcept = default;
  file_status(file_status&&) noexcept = default;
  ~file_status() = default;
  file_status& operator=(const file_status&) noexcept = default;
  file_status& operator=(file_status&&) noexcept = default;

  void type(file_type ft) noexcept { type_ = ft; }
  void permissions(perms prms) noexcept { perms_ = prms; }
  file_type type() const noexcept { return type_; }
  perms permissions() const noexcept { return perms_; }

  friend bool operator==(const file_status& lhs, const file_status& rhs) noexcept {
    return lhs.type() == rhs.type() && lhs.permissions() == rhs.permissions();
  }

private:
  file_type type_;
  perms perms_;
};

struct space_info {
  uintmax_t capacity;
  uintmax_t free;
  uintmax_t available;
  friend bool operator==(const space_info&, const space_info&) = default;
};

using file_time_type = chrono::time_point<chrono::file_clock>;

} // namespace std::filesystem

namespace ycxx::detail {

[[noreturn, gnu::cold]] inline void fs_raise(const char* what, std::error_code ec) {
  ::ycxx::detail::raise_with(ycxx_error_filesystem_error, what,
                             [&] { return std::filesystem::filesystem_error(what, ec); });
}
[[noreturn, gnu::cold]] inline void fs_raise(const char* what, const std::filesystem::path& p1, std::error_code ec) {
  ::ycxx::detail::raise_with(ycxx_error_filesystem_error, what,
                             [&] { return std::filesystem::filesystem_error(what, p1, ec); });
}
[[noreturn, gnu::cold]] inline void fs_raise(const char* what, const std::filesystem::path& p1,
                                             const std::filesystem::path& p2, std::error_code ec) {
  ::ycxx::detail::raise_with(ycxx_error_filesystem_error, what,
                             [&] { return std::filesystem::filesystem_error(what, p1, p2, ec); });
}
// For the iterator members, which take no path argument ([fs.err.report]/2.1): the exception
// carries no path; the directory being read is named in the message.
[[noreturn, gnu::cold]] inline void fs_raise_in(const char* what, const std::filesystem::path& dir,
                                                std::error_code ec) {
  ::ycxx::detail::raise_with(ycxx_error_filesystem_error, what, [&] {
    std::string msg(what);
    msg += " in \"";
    msg += dir.native();
    msg += '"';
    return std::filesystem::filesystem_error(msg, ec);
  });
}

// The attribute values a directory_entry caches ([fs.class.directory.entry]/2). `level` says
// what is stored: 0 nothing; 1 only the file type from the directory listing (sym_type, and
// type when it is not a symbolic link); 2 the results of lstat (and stat for a symbolic link)
// with the errors they reported, so that the observers return exactly what the operations would.
struct fs_attr_cache {
  unsigned char level = 0;
  std::filesystem::file_type sym_type = std::filesystem::file_type::none;
  std::filesystem::file_type type = std::filesystem::file_type::none;
  std::filesystem::perms sym_perms = std::filesystem::perms::unknown;
  std::filesystem::perms perms = std::filesystem::perms::unknown;
  int sym_err = 0;  // the error of lstat (level 2)
  int stat_err = 0; // the error of stat (level 2)
  std::uintmax_t size = 0;
  std::uintmax_t nlink = 0;
  long long mtime = 0; // nanoseconds since the Unix epoch
};

struct fs_dir_state;
struct fs_rec_state;
struct fs_postfix_entry;

} // namespace ycxx::detail

namespace std::filesystem {

// [fs.op.funcs]: the error_code forms (hosted runtime).
path absolute(const path& p, error_code& ec);
path canonical(const path& p, error_code& ec);
void copy(const path& from, const path& to, copy_options options, error_code& ec);
bool copy_file(const path& from, const path& to, copy_options option, error_code& ec);
void copy_symlink(const path& existing_symlink, const path& new_symlink, error_code& ec) noexcept;
bool create_directories(const path& p, error_code& ec);
bool create_directory(const path& p, error_code& ec) noexcept;
bool create_directory(const path& p, const path& attributes, error_code& ec) noexcept;
void create_directory_symlink(const path& to, const path& new_symlink, error_code& ec) noexcept;
void create_hard_link(const path& to, const path& new_hard_link, error_code& ec) noexcept;
void create_symlink(const path& to, const path& new_symlink, error_code& ec) noexcept;
path current_path(error_code& ec);
void current_path(const path& p, error_code& ec) noexcept;
bool equivalent(const path& p1, const path& p2, error_code& ec) noexcept;
uintmax_t file_size(const path& p, error_code& ec) noexcept;
uintmax_t hard_link_count(const path& p, error_code& ec) noexcept;
bool is_empty(const path& p, error_code& ec);
file_time_type last_write_time(const path& p, error_code& ec) noexcept;
void last_write_time(const path& p, file_time_type new_time, error_code& ec) noexcept;
void permissions(const path& p, perms prms, perm_options opts, error_code& ec);
path read_symlink(const path& p, error_code& ec);
bool remove(const path& p, error_code& ec) noexcept;
uintmax_t remove_all(const path& p, error_code& ec);
void rename(const path& from, const path& to, error_code& ec) noexcept;
void resize_file(const path& p, uintmax_t size, error_code& ec) noexcept;
space_info space(const path& p, error_code& ec) noexcept;
file_status status(const path& p, error_code& ec) noexcept;
file_status symlink_status(const path& p, error_code& ec) noexcept;
path temp_directory_path(error_code& ec);
path weakly_canonical(const path& p, error_code& ec);

// [fs.class.directory.entry]
class directory_entry {
public:
  directory_entry() noexcept = default;
  directory_entry(const directory_entry&) = default;
  directory_entry(directory_entry&&) noexcept = default;
  explicit directory_entry(const filesystem::path& p) : path_(p) { refresh(); }
  directory_entry(const filesystem::path& p, error_code& ec) : path_(p) {
    refresh(ec);
    if (ec && cache_.sym_type != file_type::not_found) // a missing file keeps its path
      path_.clear();
  }
  ~directory_entry() = default;
  directory_entry& operator=(const directory_entry&) = default;
  directory_entry& operator=(directory_entry&&) noexcept = default;

  // [fs.dir.entry.mods]
  void assign(const filesystem::path& p) {
    path_ = p;
    refresh();
  }
  void assign(const filesystem::path& p, error_code& ec) {
    path_ = p;
    refresh(ec);
  }
  void replace_filename(const filesystem::path& p) {
    path_.replace_filename(p);
    refresh();
  }
  void replace_filename(const filesystem::path& p, error_code& ec) {
    path_.replace_filename(p);
    refresh(ec);
  }
  // A file that does not exist is reported in ec, but the throwing form does not throw for it
  // (as status(p) does not: [fs.op.status] Note 1).
  void refresh() {
    error_code ec;
    refresh(ec);
    if (ec && cache_.sym_type != file_type::not_found)
      ycxx::detail::fs_raise("std::filesystem::directory_entry::refresh", path_, ec);
  }
  void refresh(error_code& ec) noexcept;

  // [fs.dir.entry.obs]
  const filesystem::path& path() const noexcept { return path_; }
  operator const filesystem::path&() const noexcept { return path_; }
  bool exists() const { return filesystem_exists(status()); }
  inline bool exists(error_code& ec) const noexcept;
  bool is_block_file() const { return type_or_throw(false) == file_type::block; }
  bool is_block_file(error_code& ec) const noexcept { return type_of(false, ec) == file_type::block; }
  bool is_character_file() const { return type_or_throw(false) == file_type::character; }
  bool is_character_file(error_code& ec) const noexcept { return type_of(false, ec) == file_type::character; }
  bool is_directory() const { return type_or_throw(false) == file_type::directory; }
  bool is_directory(error_code& ec) const noexcept { return type_of(false, ec) == file_type::directory; }
  bool is_fifo() const { return type_or_throw(false) == file_type::fifo; }
  bool is_fifo(error_code& ec) const noexcept { return type_of(false, ec) == file_type::fifo; }
  inline bool is_other() const;
  inline bool is_other(error_code& ec) const noexcept;
  bool is_regular_file() const { return type_or_throw(false) == file_type::regular; }
  bool is_regular_file(error_code& ec) const noexcept { return type_of(false, ec) == file_type::regular; }
  bool is_socket() const { return type_or_throw(false) == file_type::socket; }
  bool is_socket(error_code& ec) const noexcept { return type_of(false, ec) == file_type::socket; }
  bool is_symlink() const { return type_or_throw(true) == file_type::symlink; }
  bool is_symlink(error_code& ec) const noexcept { return type_of(true, ec) == file_type::symlink; }
  uintmax_t file_size() const {
    error_code ec;
    uintmax_t r = file_size(ec);
    if (ec)
      ycxx::detail::fs_raise("std::filesystem::directory_entry::file_size", path_, ec);
    return r;
  }
  uintmax_t file_size(error_code& ec) const noexcept;
  uintmax_t hard_link_count() const {
    error_code ec;
    uintmax_t r = hard_link_count(ec);
    if (ec)
      ycxx::detail::fs_raise("std::filesystem::directory_entry::hard_link_count", path_, ec);
    return r;
  }
  uintmax_t hard_link_count(error_code& ec) const noexcept;
  file_time_type last_write_time() const {
    error_code ec;
    file_time_type r = last_write_time(ec);
    if (ec)
      ycxx::detail::fs_raise("std::filesystem::directory_entry::last_write_time", path_, ec);
    return r;
  }
  file_time_type last_write_time(error_code& ec) const noexcept;
  file_status status() const {
    error_code ec;
    file_status r = status(ec);
    if (r.type() == file_type::none)
      ycxx::detail::fs_raise("std::filesystem::directory_entry::status", path_, ec);
    return r;
  }
  file_status status(error_code& ec) const noexcept;
  file_status symlink_status() const {
    error_code ec;
    file_status r = symlink_status(ec);
    if (r.type() == file_type::none)
      ycxx::detail::fs_raise("std::filesystem::directory_entry::symlink_status", path_, ec);
    return r;
  }
  file_status symlink_status(error_code& ec) const noexcept;

  bool operator==(const directory_entry& rhs) const noexcept { return path_ == rhs.path_; }
  strong_ordering operator<=>(const directory_entry& rhs) const noexcept { return path_ <=> rhs.path_; }

  // [fs.dir.entry.io]
  template <class charT, class traits>
  friend basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& os, const directory_entry& d) {
    return os << d.path();
  }

private:
  friend struct ycxx::detail::fs_dir_state;
  friend struct ycxx::detail::fs_rec_state;

  static bool filesystem_exists(file_status s) noexcept {
    return s.type() != file_type::none && s.type() != file_type::not_found;
  }
  // The type of the file (of the link itself when `link`), from the cache when it holds it.
  file_type type_of(bool link, error_code& ec) const noexcept;
  file_type type_or_throw(bool link) const {
    error_code ec;
    file_type t = type_of(link, ec);
    if (t == file_type::none)
      ycxx::detail::fs_raise("std::filesystem::directory_entry::status", path_, ec);
    return t;
  }

  filesystem::path path_;
  ycxx::detail::fs_attr_cache cache_;
};

// [fs.class.directory.iterator]. Copies share the open directory (an input iterator).
class directory_iterator {
public:
  using iterator_category = input_iterator_tag;
  using value_type = directory_entry;
  using difference_type = ptrdiff_t;
  using pointer = const directory_entry*;
  using reference = const directory_entry&;

  directory_iterator() noexcept = default;
  explicit directory_iterator(const path& p) : directory_iterator(p, directory_options::none) {}
  directory_iterator(const path& p, directory_options options) {
    error_code ec;
    open(p, options, ec);
    if (ec)
      ycxx::detail::fs_raise("std::filesystem::directory_iterator::directory_iterator", p, ec);
  }
  directory_iterator(const path& p, error_code& ec) { open(p, directory_options::none, ec); }
  directory_iterator(const path& p, directory_options options, error_code& ec) { open(p, options, ec); }
  directory_iterator(const directory_iterator& rhs) = default;
  directory_iterator(directory_iterator&& rhs) noexcept = default;
  ~directory_iterator() = default;
  directory_iterator& operator=(const directory_iterator& rhs) = default;
  directory_iterator& operator=(directory_iterator&& rhs) noexcept = default;

  const directory_entry& operator*() const;
  const directory_entry* operator->() const { return __builtin_addressof(**this); }
  directory_iterator& operator++() {
    error_code ec;
    path where;
    advance(ec, &where);
    if (ec)
      ycxx::detail::fs_raise_in("std::filesystem::directory_iterator::operator++", where, ec);
    return *this;
  }
  directory_iterator& increment(error_code& ec) {
    advance(ec, nullptr);
    return *this;
  }
  // [iterator.cpp17.input]: *r++ is the entry before the increment.
  ycxx::detail::fs_postfix_entry operator++(int);

  bool operator==(default_sentinel_t) const noexcept { return state_ == nullptr; }
  friend bool operator==(const directory_iterator& a, const directory_iterator& b) noexcept {
    return a.state_.get() == b.state_.get();
  }

private:
  void open(const path& p, directory_options options, error_code& ec);
  void advance(error_code& ec, path* where);

  shared_ptr<ycxx::detail::fs_dir_state> state_;
};

// [fs.dir.itr.nonmembers]
inline directory_iterator begin(directory_iterator iter) noexcept { return iter; }
inline directory_iterator end(directory_iterator) noexcept { return directory_iterator(); }

// [fs.class.rec.dir.itr]. Subdirectories are opened relative to their parent's descriptor, and
// without following a symbolic link unless follow_directory_symlink is set, so a directory that
// is replaced by a symbolic link while it is being visited is not followed.
class recursive_directory_iterator {
public:
  using iterator_category = input_iterator_tag;
  using value_type = directory_entry;
  using difference_type = ptrdiff_t;
  using pointer = const directory_entry*;
  using reference = const directory_entry&;

  recursive_directory_iterator() noexcept = default;
  explicit recursive_directory_iterator(const path& p) : recursive_directory_iterator(p, directory_options::none) {}
  recursive_directory_iterator(const path& p, directory_options options) {
    error_code ec;
    open(p, options, ec);
    if (ec)
      ycxx::detail::fs_raise("std::filesystem::recursive_directory_iterator::recursive_directory_iterator", p, ec);
  }
  recursive_directory_iterator(const path& p, directory_options options, error_code& ec) { open(p, options, ec); }
  recursive_directory_iterator(const path& p, error_code& ec) { open(p, directory_options::none, ec); }
  recursive_directory_iterator(const recursive_directory_iterator& rhs) = default;
  recursive_directory_iterator(recursive_directory_iterator&& rhs) noexcept = default;
  ~recursive_directory_iterator() = default;

  directory_options options() const;
  int depth() const;
  bool recursion_pending() const;
  const directory_entry& operator*() const;
  const directory_entry* operator->() const { return __builtin_addressof(**this); }

  recursive_directory_iterator& operator=(const recursive_directory_iterator& rhs) = default;
  recursive_directory_iterator& operator=(recursive_directory_iterator&& rhs) noexcept = default;
  recursive_directory_iterator& operator++() {
    error_code ec;
    path where;
    advance(ec, &where);
    if (ec)
      ycxx::detail::fs_raise_in("std::filesystem::recursive_directory_iterator::operator++", where, ec);
    return *this;
  }
  recursive_directory_iterator& increment(error_code& ec) {
    advance(ec, nullptr);
    return *this;
  }
  // [fs.rec.dir.itr.members]/7-8: copies have their own recursion_pending().
  ycxx::detail::fs_postfix_entry operator++(int);
  void pop() {
    error_code ec;
    path where;
    pop(ec, &where);
    if (ec)
      ycxx::detail::fs_raise_in("std::filesystem::recursive_directory_iterator::pop", where, ec);
  }
  void pop(error_code& ec) { pop(ec, nullptr); }
  void disable_recursion_pending();

  bool operator==(default_sentinel_t) const noexcept { return state_ == nullptr; }
  friend bool operator==(const recursive_directory_iterator& a, const recursive_directory_iterator& b) noexcept {
    return a.state_.get() == b.state_.get();
  }

private:
  void open(const path& p, directory_options options, error_code& ec);
  void advance(error_code& ec, path* where);
  void pop(error_code& ec, path* where);

  shared_ptr<ycxx::detail::fs_rec_state> state_;
  bool pending_ = true; // recursion_pending()
};

// [fs.rec.dir.itr.nonmembers]
inline recursive_directory_iterator begin(recursive_directory_iterator iter) noexcept { return iter; }
inline recursive_directory_iterator end(recursive_directory_iterator) noexcept { return recursive_directory_iterator(); }

// ---- [fs.op.funcs]: the throwing forms ----
inline path absolute(const path& p) {
  error_code ec;
  path r = absolute(p, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::absolute", p, ec);
  return r;
}
inline path canonical(const path& p) {
  error_code ec;
  path r = canonical(p, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::canonical", p, ec);
  return r;
}
inline void copy(const path& from, const path& to, copy_options options) {
  error_code ec;
  copy(from, to, options, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::copy", from, to, ec);
}
inline void copy(const path& from, const path& to) { copy(from, to, copy_options::none); }
inline void copy(const path& from, const path& to, error_code& ec) { copy(from, to, copy_options::none, ec); }
inline bool copy_file(const path& from, const path& to, copy_options option) {
  error_code ec;
  bool r = copy_file(from, to, option, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::copy_file", from, to, ec);
  return r;
}
inline bool copy_file(const path& from, const path& to) { return copy_file(from, to, copy_options::none); }
inline bool copy_file(const path& from, const path& to, error_code& ec) {
  return copy_file(from, to, copy_options::none, ec);
}
inline void copy_symlink(const path& existing_symlink, const path& new_symlink) {
  error_code ec;
  copy_symlink(existing_symlink, new_symlink, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::copy_symlink", existing_symlink, new_symlink, ec);
}
inline bool create_directories(const path& p) {
  error_code ec;
  bool r = create_directories(p, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::create_directories", p, ec);
  return r;
}
inline bool create_directory(const path& p) {
  error_code ec;
  bool r = create_directory(p, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::create_directory", p, ec);
  return r;
}
inline bool create_directory(const path& p, const path& attributes) {
  error_code ec;
  bool r = create_directory(p, attributes, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::create_directory", p, attributes, ec);
  return r;
}
inline void create_directory_symlink(const path& to, const path& new_symlink) {
  error_code ec;
  create_directory_symlink(to, new_symlink, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::create_directory_symlink", to, new_symlink, ec);
}
inline void create_hard_link(const path& to, const path& new_hard_link) {
  error_code ec;
  create_hard_link(to, new_hard_link, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::create_hard_link", to, new_hard_link, ec);
}
inline void create_symlink(const path& to, const path& new_symlink) {
  error_code ec;
  create_symlink(to, new_symlink, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::create_symlink", to, new_symlink, ec);
}
inline path current_path() {
  error_code ec;
  path r = current_path(ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::current_path", ec);
  return r;
}
inline void current_path(const path& p) {
  error_code ec;
  current_path(p, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::current_path", p, ec);
}
inline bool equivalent(const path& p1, const path& p2) {
  error_code ec;
  bool r = equivalent(p1, p2, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::equivalent", p1, p2, ec);
  return r;
}
inline uintmax_t file_size(const path& p) {
  error_code ec;
  uintmax_t r = file_size(p, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::file_size", p, ec);
  return r;
}
inline uintmax_t hard_link_count(const path& p) {
  error_code ec;
  uintmax_t r = hard_link_count(p, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::hard_link_count", p, ec);
  return r;
}
inline bool is_empty(const path& p) {
  error_code ec;
  bool r = is_empty(p, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::is_empty", p, ec);
  return r;
}
inline file_time_type last_write_time(const path& p) {
  error_code ec;
  file_time_type r = last_write_time(p, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::last_write_time", p, ec);
  return r;
}
inline void last_write_time(const path& p, file_time_type new_time) {
  error_code ec;
  last_write_time(p, new_time, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::last_write_time", p, ec);
}
inline void permissions(const path& p, perms prms, perm_options opts = perm_options::replace) {
  error_code ec;
  permissions(p, prms, opts, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::permissions", p, ec);
}
inline void permissions(const path& p, perms prms, error_code& ec) noexcept {
  // [fs.op.permissions]/5; the four-argument form allocates nothing that can fail.
  permissions(p, prms, perm_options::replace, ec);
}
inline path read_symlink(const path& p) {
  error_code ec;
  path r = read_symlink(p, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::read_symlink", p, ec);
  return r;
}
inline bool remove(const path& p) {
  error_code ec;
  bool r = remove(p, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::remove", p, ec);
  return r;
}
inline uintmax_t remove_all(const path& p) {
  error_code ec;
  uintmax_t r = remove_all(p, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::remove_all", p, ec);
  return r;
}
inline void rename(const path& from, const path& to) {
  error_code ec;
  rename(from, to, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::rename", from, to, ec);
}
inline void resize_file(const path& p, uintmax_t size) {
  error_code ec;
  resize_file(p, size, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::resize_file", p, ec);
}
inline space_info space(const path& p) {
  error_code ec;
  space_info r = space(p, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::space", p, ec);
  return r;
}
// [fs.op.status]/1: only file_type::none is a failure.
inline file_status status(const path& p) {
  error_code ec;
  file_status r = status(p, ec);
  if (r.type() == file_type::none)
    ycxx::detail::fs_raise("std::filesystem::status", p, ec);
  return r;
}
inline file_status symlink_status(const path& p) {
  error_code ec;
  file_status r = symlink_status(p, ec);
  if (r.type() == file_type::none)
    ycxx::detail::fs_raise("std::filesystem::symlink_status", p, ec);
  return r;
}
inline path temp_directory_path() {
  error_code ec;
  path r = temp_directory_path(ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::temp_directory_path", ec);
  return r;
}
inline path weakly_canonical(const path& p) {
  error_code ec;
  path r = weakly_canonical(p, ec);
  if (ec)
    ycxx::detail::fs_raise("std::filesystem::weakly_canonical", p, ec);
  return r;
}
// [fs.op.proximate], [fs.op.relative]
inline path proximate(const path& p, const path& base, error_code& ec) {
  path a = weakly_canonical(p, ec);
  if (ec)
    return path();
  path b = weakly_canonical(base, ec);
  if (ec)
    return path();
  return a.lexically_proximate(b);
}
inline path proximate(const path& p, error_code& ec) {
  path base = current_path(ec);
  if (ec)
    return path();
  return proximate(p, base, ec);
}
inline path proximate(const path& p, const path& base = current_path()) {
  return weakly_canonical(p).lexically_proximate(weakly_canonical(base));
}
inline path relative(const path& p, const path& base, error_code& ec) {
  path a = weakly_canonical(p, ec);
  if (ec)
    return path();
  path b = weakly_canonical(base, ec);
  if (ec)
    return path();
  return a.lexically_relative(b);
}
inline path relative(const path& p, error_code& ec) {
  path base = current_path(ec);
  if (ec)
    return path();
  return relative(p, base, ec);
}
inline path relative(const path& p, const path& base = current_path()) {
  return weakly_canonical(p).lexically_relative(weakly_canonical(base));
}

// The file-type predicates ([fs.op.exists] - [fs.op.is.symlink]).
inline bool status_known(file_status s) noexcept { return s.type() != file_type::none; }
inline bool exists(file_status s) noexcept { return status_known(s) && s.type() != file_type::not_found; }
inline bool exists(const path& p) { return exists(status(p)); }
inline bool exists(const path& p, error_code& ec) noexcept {
  file_status s = status(p, ec);
  if (status_known(s))
    ec.clear();
  return exists(s);
}
inline bool is_block_file(file_status s) noexcept { return s.type() == file_type::block; }
inline bool is_block_file(const path& p) { return is_block_file(status(p)); }
inline bool is_block_file(const path& p, error_code& ec) noexcept { return is_block_file(status(p, ec)); }
inline bool is_character_file(file_status s) noexcept { return s.type() == file_type::character; }
inline bool is_character_file(const path& p) { return is_character_file(status(p)); }
inline bool is_character_file(const path& p, error_code& ec) noexcept { return is_character_file(status(p, ec)); }
inline bool is_directory(file_status s) noexcept { return s.type() == file_type::directory; }
inline bool is_directory(const path& p) { return is_directory(status(p)); }
inline bool is_directory(const path& p, error_code& ec) noexcept { return is_directory(status(p, ec)); }
inline bool is_fifo(file_status s) noexcept { return s.type() == file_type::fifo; }
inline bool is_fifo(const path& p) { return is_fifo(status(p)); }
inline bool is_fifo(const path& p, error_code& ec) noexcept { return is_fifo(status(p, ec)); }
inline bool is_regular_file(file_status s) noexcept { return s.type() == file_type::regular; }
inline bool is_regular_file(const path& p) { return is_regular_file(status(p)); }
inline bool is_regular_file(const path& p, error_code& ec) noexcept { return is_regular_file(status(p, ec)); }
inline bool is_socket(file_status s) noexcept { return s.type() == file_type::socket; }
inline bool is_socket(const path& p) { return is_socket(status(p)); }
inline bool is_socket(const path& p, error_code& ec) noexcept { return is_socket(status(p, ec)); }
inline bool is_symlink(file_status s) noexcept { return s.type() == file_type::symlink; }
inline bool is_symlink(const path& p) { return is_symlink(symlink_status(p)); }
inline bool is_symlink(const path& p, error_code& ec) noexcept { return is_symlink(symlink_status(p, ec)); }
inline bool is_other(file_status s) noexcept {
  return exists(s) && !is_regular_file(s) && !is_directory(s) && !is_symlink(s);
}
inline bool is_other(const path& p) { return is_other(status(p)); }
inline bool is_other(const path& p, error_code& ec) noexcept { return is_other(status(p, ec)); }

inline bool directory_entry::is_other() const { return filesystem::is_other(status()); }
inline bool directory_entry::is_other(error_code& ec) const noexcept { return filesystem::is_other(status(ec)); }
// [fs.dir.entry.obs]/3: exists(this->status(ec)); unlike the non-member exists(p, ec), the error
// of a file that does not exist is left in ec.
inline bool directory_entry::exists(error_code& ec) const noexcept { return filesystem::exists(status(ec)); }

} // namespace std::filesystem

namespace ycxx::detail {
// What directory_iterator::operator++(int) returns: the entry the iterator designated.
struct fs_postfix_entry {
  std::filesystem::directory_entry entry;
  const std::filesystem::directory_entry& operator*() const noexcept { return entry; }
  const std::filesystem::directory_entry* operator->() const noexcept { return __builtin_addressof(entry); }
};
} // namespace ycxx::detail

namespace std::filesystem {
inline ycxx::detail::fs_postfix_entry directory_iterator::operator++(int) {
  ycxx::detail::fs_postfix_entry r{**this};
  ++*this;
  return r;
}
inline ycxx::detail::fs_postfix_entry recursive_directory_iterator::operator++(int) {
  ycxx::detail::fs_postfix_entry r{**this};
  ++*this;
  return r;
}
} // namespace std::filesystem

namespace std {

// [fs.path.hash]
template <>
struct hash<filesystem::path> {
  size_t operator()(const filesystem::path& p) const noexcept { return filesystem::hash_value(p); }
};

} // namespace std

namespace std::ranges {
template <>
inline constexpr bool enable_borrowed_range<filesystem::directory_iterator> = true;
template <>
inline constexpr bool enable_borrowed_range<filesystem::recursive_directory_iterator> = true;
template <>
inline constexpr bool enable_view<filesystem::directory_iterator> = true;
template <>
inline constexpr bool enable_view<filesystem::recursive_directory_iterator> = true;
} // namespace std::ranges
