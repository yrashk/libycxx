// libycxx hosted: the regular expression constants, regex_error and regex_traits ([re.const],
// [re.badexp], [re.traits]).
//
// The three regex_constants types are unscoped enumerations without enumerators, with the
// bitmask operators as hidden-free functions of the namespace (found by ADL), and the constants
// are inline constexpr variables, as the synopsis declares them.
//
// regex_traits::char_class_type holds the ctype_base::mask bits of the named classes in its low
// 16 bits, plus one bit (word_bit) for the underscore that "w" adds to alnum. The name tables
// (class names, the POSIX collating-symbol names) and regex_error's members are in the hosted
// runtime (src/hosted/regex.cpp).
#pragma once

#include <ycxx/core/basic_string.hpp>
#include <ycxx/core/char_traits.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/hosted/locale_base.hpp>

namespace std::regex_constants {

// [re.synopt]
enum syntax_option_type : unsigned {};
// [re.matchflag]
enum match_flag_type : unsigned {};
// [re.err]
enum error_type : int {};

constexpr syntax_option_type operator&(syntax_option_type a, syntax_option_type b) noexcept {
  return syntax_option_type(unsigned(a) & unsigned(b));
}
constexpr syntax_option_type operator|(syntax_option_type a, syntax_option_type b) noexcept {
  return syntax_option_type(unsigned(a) | unsigned(b));
}
constexpr syntax_option_type operator^(syntax_option_type a, syntax_option_type b) noexcept {
  return syntax_option_type(unsigned(a) ^ unsigned(b));
}
constexpr syntax_option_type operator~(syntax_option_type a) noexcept { return syntax_option_type(~unsigned(a)); }
constexpr syntax_option_type& operator&=(syntax_option_type& a, syntax_option_type b) noexcept { return a = a & b; }
constexpr syntax_option_type& operator|=(syntax_option_type& a, syntax_option_type b) noexcept { return a = a | b; }
constexpr syntax_option_type& operator^=(syntax_option_type& a, syntax_option_type b) noexcept { return a = a ^ b; }

constexpr match_flag_type operator&(match_flag_type a, match_flag_type b) noexcept {
  return match_flag_type(unsigned(a) & unsigned(b));
}
constexpr match_flag_type operator|(match_flag_type a, match_flag_type b) noexcept {
  return match_flag_type(unsigned(a) | unsigned(b));
}
constexpr match_flag_type operator^(match_flag_type a, match_flag_type b) noexcept {
  return match_flag_type(unsigned(a) ^ unsigned(b));
}
constexpr match_flag_type operator~(match_flag_type a) noexcept { return match_flag_type(~unsigned(a)); }
constexpr match_flag_type& operator&=(match_flag_type& a, match_flag_type b) noexcept { return a = a & b; }
constexpr match_flag_type& operator|=(match_flag_type& a, match_flag_type b) noexcept { return a = a | b; }
constexpr match_flag_type& operator^=(match_flag_type& a, match_flag_type b) noexcept { return a = a ^ b; }

inline constexpr syntax_option_type icase = syntax_option_type(1u << 0);
inline constexpr syntax_option_type nosubs = syntax_option_type(1u << 1);
inline constexpr syntax_option_type optimize = syntax_option_type(1u << 2);
inline constexpr syntax_option_type collate = syntax_option_type(1u << 3);
inline constexpr syntax_option_type ECMAScript = syntax_option_type(1u << 4);
inline constexpr syntax_option_type basic = syntax_option_type(1u << 5);
inline constexpr syntax_option_type extended = syntax_option_type(1u << 6);
inline constexpr syntax_option_type awk = syntax_option_type(1u << 7);
inline constexpr syntax_option_type grep = syntax_option_type(1u << 8);
inline constexpr syntax_option_type egrep = syntax_option_type(1u << 9);
inline constexpr syntax_option_type multiline = syntax_option_type(1u << 10);

inline constexpr match_flag_type match_default = {};
inline constexpr match_flag_type match_not_bol = match_flag_type(1u << 0);
inline constexpr match_flag_type match_not_eol = match_flag_type(1u << 1);
inline constexpr match_flag_type match_not_bow = match_flag_type(1u << 2);
inline constexpr match_flag_type match_not_eow = match_flag_type(1u << 3);
inline constexpr match_flag_type match_any = match_flag_type(1u << 4);
inline constexpr match_flag_type match_not_null = match_flag_type(1u << 5);
inline constexpr match_flag_type match_continuous = match_flag_type(1u << 6);
inline constexpr match_flag_type match_prev_avail = match_flag_type(1u << 7);
inline constexpr match_flag_type format_default = {};
inline constexpr match_flag_type format_sed = match_flag_type(1u << 8);
inline constexpr match_flag_type format_no_copy = match_flag_type(1u << 9);
inline constexpr match_flag_type format_first_only = match_flag_type(1u << 10);

inline constexpr error_type error_collate = error_type(1);
inline constexpr error_type error_ctype = error_type(2);
inline constexpr error_type error_escape = error_type(3);
inline constexpr error_type error_backref = error_type(4);
inline constexpr error_type error_brack = error_type(5);
inline constexpr error_type error_paren = error_type(6);
inline constexpr error_type error_brace = error_type(7);
inline constexpr error_type error_badbrace = error_type(8);
inline constexpr error_type error_range = error_type(9);
inline constexpr error_type error_space = error_type(10);
inline constexpr error_type error_badrepeat = error_type(11);
inline constexpr error_type error_complexity = error_type(12);
inline constexpr error_type error_stack = error_type(13);

} // namespace std::regex_constants

namespace std {

// [re.badexp]. The constructor (what() is a fixed message per code) and the destructor (the key
// function) are in the hosted runtime.
class regex_error : public runtime_error {
  regex_constants::error_type code_;

public:
  explicit regex_error(regex_constants::error_type ecode);
  regex_error(const regex_error&) noexcept = default;
  regex_error& operator=(const regex_error&) noexcept = default;
  ~regex_error() override;

  regex_constants::error_type code() const { return code_; }
  const char* what() const noexcept override { return runtime_error::what(); }
};

} // namespace std

namespace ycxx::detail {

// src/hosted/regex.cpp. The names are ASCII, already narrowed; class names lower-cased.
// The class mask of a name of Table 121 (0 if unknown); icase maps lower and upper to alpha.
unsigned regex_class_by_name(const char* name, std::size_t n, bool icase) noexcept;
// The character a single character or a POSIX collating-symbol name ("period", "NUL", ...)
// stands for, or -1.
int regex_collate_by_name(const char* name, std::size_t n) noexcept;
// The fixed message of regex_error(code).
const char* regex_error_message(int code) noexcept;

[[noreturn]] [[gnu::cold]] inline void throw_regex_error(std::regex_constants::error_type e) {
  ::ycxx::detail::raise_with(ycxx_error_regex_error, ::ycxx::detail::regex_error_message(e),
                             [e] { return std::regex_error(e); });
}

// The bit regex_traits adds to alnum for the class "w" (the underscore).
inline constexpr unsigned regex_word_bit = 1u << 16;

} // namespace ycxx::detail

namespace std {

// [re.traits]
template <class charT>
struct regex_traits {
  using char_type = charT;
  using string_type = basic_string<char_type>;
  using locale_type = locale;
  using char_class_type = unsigned;

  regex_traits() { cache(); }

  static size_t length(const char_type* p) { return char_traits<charT>::length(p); }
  charT translate(charT c) const { return c; }
  charT translate_nocase(charT c) const { return ct_->tolower(c); }

  template <class ForwardIterator>
  string_type transform(ForwardIterator first, ForwardIterator last) const {
    string_type s(first, last);
    return col_->transform(s.data(), s.data() + s.size());
  }
  // [re.traits]/7 returns an empty key unless the facet is a collate_byname whose key form is
  // known. The collate facets of every locale libycxx provides (collate and collate_byname alike)
  // compare code points one by one, as the POSIX locale does, so their sort keys have no
  // secondary weights and the whole key is the primary key: every character is its own
  // equivalence class ([[=a=]] matches 'a' only).
  template <class ForwardIterator>
  string_type transform_primary(ForwardIterator first, ForwardIterator last) const {
    return transform(first, last);
  }
  template <class ForwardIterator>
  string_type lookup_collatename(ForwardIterator first, ForwardIterator last) const {
    char buf[32];
    size_t n = 0;
    for (; first != last; ++first) {
      if (n == sizeof buf)
        return string_type();
      const charT c = *first;
      const char nc = ct_->narrow(c, '\0');
      if (nc == '\0' || static_cast<unsigned char>(nc) > 127)
        return string_type();
      buf[n++] = nc;
    }
    if (n == 1)
      return string_type(1, ct_->widen(buf[0]));
    const int code = ::ycxx::detail::regex_collate_by_name(buf, n);
    return code < 0 ? string_type() : string_type(1, ct_->widen(static_cast<char>(code)));
  }
  template <class ForwardIterator>
  char_class_type lookup_classname(ForwardIterator first, ForwardIterator last, bool icase = false) const {
    char buf[16];
    size_t n = 0;
    for (; first != last; ++first) {
      if (n == sizeof buf)
        return 0;
      char nc = ct_->narrow(*first, '\0');
      if (nc >= 'A' && nc <= 'Z')
        nc = static_cast<char>(nc - 'A' + 'a');
      if (nc < 'a' || nc > 'z')
        return 0;
      buf[n++] = nc;
    }
    return ::ycxx::detail::regex_class_by_name(buf, n, icase);
  }
  bool isctype(charT c, char_class_type f) const {
    const auto m = static_cast<ctype_base::mask>(f & 0xFFFFu);
    if (m != 0 && ct_->is(m, c))
      return true;
    return (f & ::ycxx::detail::regex_word_bit) != 0 && c == ct_->widen('_');
  }
  int value(charT ch, int radix) const {
    ::ycxx::detail::precondition(radix == 8 || radix == 10 || radix == 16, "regex_traits::value: radix must be 8, 10 or 16");
    const char c = ct_->narrow(ch, '\0');
    int v = -1;
    if (c >= '0' && c <= '9')
      v = c - '0';
    else if (c >= 'a' && c <= 'f')
      v = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
      v = c - 'A' + 10;
    return v < radix ? v : -1;
  }
  locale_type imbue(locale_type l) {
    locale_type old = loc_;
    loc_ = l;
    cache();
    return old;
  }
  locale_type getloc() const { return loc_; }

private:
  void cache() {
    ct_ = __builtin_addressof(use_facet<ctype<charT>>(loc_));
    col_ = __builtin_addressof(use_facet<collate<charT>>(loc_));
  }

  locale loc_;
  const ctype<charT>* ct_ = nullptr;
  const collate<charT>* col_ = nullptr;
};

} // namespace std
