// libycxx core: what every header declaring a formatter specialization provides without the
// formatting library: the primary template formatter, enable_nonlocking_formatter_optimization,
// the concept formattable, and the specializations of [format.formatter.spec]/2 and /4 (/3 for
// their enable_nonlocking_formatter_optimization values).
//
// "Each header that declares the template formatter provides" those specializations
// ([format.formatter.spec]/2), so <vector>, <stack> and <queue> must make them complete. Nothing
// can call parse or format without the contexts of <format>, so only the class layouts are
// here: the state (fmt_spec) and member functions whose bodies call ycxx::detail functions that
// are only declared here and defined in format_base.hpp. They are found by qualified lookup at
// the definition and instantiated where a formatting function is used, after <format>.
//
// formattable needs the complete basic_format_context, which <format> defines: before it, no
// type is formattable (asking both before and after including <format> is ill-formed, no
// diagnostic required: [temp.constr.atomic]/3; see STATUS.md).
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/cstddef.hpp>
#include <ycxx/core/type_traits.hpp>

namespace [[gnu::visibility("hidden")]] std {
template <class CharT>
struct char_traits;
template <class T>
class allocator;
template <class charT, class traits, class Allocator>
class basic_string;
template <class charT, class traits>
class basic_string_view;

// [format.formatter]: the primary template is disabled ([format.formatter.spec]/5, /7).
template <class T, class charT = char>
struct formatter {
  formatter() = delete;
  formatter(const formatter&) = delete;
  formatter& operator=(const formatter&) = delete;
};

template <class charT>
class basic_format_parse_context;
template <class Out, class charT>
class basic_format_context;

// [format.formatter.locking]
template <class T>
inline constexpr bool enable_nonlocking_formatter_optimization = false;
} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// format_context::iterator (format_base.hpp).
template <class charT>
class fmt_iter;

// A disabled formatter specialization ([format.formatter.spec]/7).
struct fmt_disabled {
  fmt_disabled() = delete;
  fmt_disabled(const fmt_disabled&) = delete;
  fmt_disabled& operator=(const fmt_disabled&) = delete;
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template <class charT>
concept fmt_char = __is_same(charT, char) || __is_same(charT, wchar_t);

// [format.formattable]
template <class T, class Context, class Formatter = typename Context::template formatter_type<std::remove_const_t<T>>>
concept fmt_formattable_with =
    std::semiregular<Formatter> &&
    requires(Formatter& f, const Formatter& cf, T&& t, Context fc,
             std::basic_format_parse_context<typename Context::char_type> pc) {
      { f.parse(pc) } -> std::same_as<typename decltype(pc)::iterator>;
      { cf.format(t, fc) } -> std::same_as<typename Context::iterator>;
    };

// ---- the std-format-spec ([format.string.std]) ------------------------------------------------

enum class fmt_align : unsigned char { none, left, right, center };
enum class fmt_sign : unsigned char { none, plus, minus, space };
enum class fmt_dyn : unsigned char { none, value, arg };
// The argument categories whose formatters interpret the std-format-spec.
enum class fmt_cat : unsigned char { integer, character, boolean, floating, string, pointer };

template <class charT>
struct fmt_spec {
  charT fill[4] = {charT(' ')};
  unsigned char fill_len = 1;
  fmt_align align = fmt_align::none;
  fmt_sign sign = fmt_sign::none;
  bool alt = false;
  bool zero = false;
  bool localized = false;
  fmt_dyn width_kind = fmt_dyn::none;
  fmt_dyn prec_kind = fmt_dyn::none;
  char type = 0; // presentation type, 0 for none
  std::size_t width = 0;     // value, or argument index
  std::size_t precision = 0; // value, or argument index
};

// The cv-unqualified floating-point types (every one has a <charconv> format).
template <class T>
inline constexpr bool fmt_is_float = __is_same(T, std::remove_cv_t<T>) && ycxx::detail::is_floating_v<T>;

// Defined in format_base.hpp. parse returns the parse context's iterator, const charT*
// (checked there).
template <class charT>
constexpr const charT* fmt_parse_spec(std::basic_format_parse_context<charT>& pc, fmt_spec<charT>& s, fmt_cat cat);
template <class charT, class T, class Context>
constexpr typename Context::iterator fmt_format_int(Context& ctx, T value, const fmt_spec<charT>& s);
template <class charT, class Context>
constexpr typename Context::iterator fmt_write_string(Context& ctx, const charT* p, std::size_t n,
                                                      const fmt_spec<charT>& s, bool is_char = false);
template <class charT, class Context>
constexpr typename Context::iterator fmt_format_char(Context& ctx, charT c, const fmt_spec<charT>& s);
template <class charT, class Context>
constexpr typename Context::iterator fmt_format_bool(Context& ctx, bool b, const fmt_spec<charT>& s);
template <class charT, class Context>
constexpr typename Context::iterator fmt_format_pointer(Context& ctx, const void* p, const fmt_spec<charT>& s);
template <class charT, class T, class Context>
typename Context::iterator fmt_format_float(Context& ctx, T value, const fmt_spec<charT>& s);

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {
// [format.formattable]
template <class T, class charT>
concept formattable =
    ycxx::detail::fmt_formattable_with<remove_reference_t<T>, basic_format_context<ycxx::adl_free::fmt_iter<charT>, charT>>;
} // namespace std

// ---- the formatter specializations of [format.formatter.spec] -----------------------------------

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {

// The formatters interpreting a std-format-spec.
template <class charT, ycxx::detail::fmt_cat Cat>
class fmt_std_formatter {
protected:
  ycxx::detail::fmt_spec<charT> spec_;

public:
  constexpr const charT* parse(std::basic_format_parse_context<charT>& pc) {
    return ycxx::detail::fmt_parse_spec(pc, spec_, Cat);
  }
};

template <class charT>
class fmt_string_formatter : public fmt_std_formatter<charT, ycxx::detail::fmt_cat::string> {
public:
  constexpr void set_debug_format() { this->spec_.type = '?'; }

protected:
  template <class FormatContext>
  constexpr typename FormatContext::iterator do_format(const charT* p, std::size_t n, FormatContext& ctx) const {
    return ycxx::detail::fmt_write_string(ctx, p, n, this->spec_);
  }
};

}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std {

// /2.1: characters.
template <ycxx::detail::fmt_char charT>
struct formatter<charT, charT> : ycxx::adl_free::fmt_std_formatter<charT, ycxx::detail::fmt_cat::character> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(charT c, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_char(ctx, c, this->spec_);
  }
  constexpr void set_debug_format() { this->spec_.type = '?'; }
};
template <>
struct formatter<char, wchar_t> : ycxx::adl_free::fmt_std_formatter<wchar_t, ycxx::detail::fmt_cat::character> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(char c, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_char(ctx, static_cast<wchar_t>(static_cast<unsigned char>(c)), this->spec_);
  }
  constexpr void set_debug_format() { this->spec_.type = '?'; }
};

// /2.2: strings.
template <ycxx::detail::fmt_char charT>
struct formatter<charT*, charT> : ycxx::adl_free::fmt_string_formatter<charT> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(charT* s, FormatContext& ctx) const {
    return this->do_format(s, char_traits<charT>::length(s), ctx);
  }
};
template <ycxx::detail::fmt_char charT>
struct formatter<const charT*, charT> : ycxx::adl_free::fmt_string_formatter<charT> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(const charT* s, FormatContext& ctx) const {
    return this->do_format(s, char_traits<charT>::length(s), ctx);
  }
};
template <ycxx::detail::fmt_char charT, size_t N>
struct formatter<charT[N], charT> : ycxx::adl_free::fmt_string_formatter<charT> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(const charT (&s)[N], FormatContext& ctx) const {
    size_t n = 0;
    while (n != N && s[n] != charT())
      ++n;
    return this->do_format(s, n, ctx);
  }
};
template <ycxx::detail::fmt_char charT, class traits, class Allocator>
struct formatter<basic_string<charT, traits, Allocator>, charT> : ycxx::adl_free::fmt_string_formatter<charT> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(const basic_string<charT, traits, Allocator>& s,
                                                    FormatContext& ctx) const {
    return this->do_format(s.data(), s.size(), ctx);
  }
};
template <ycxx::detail::fmt_char charT, class traits>
struct formatter<basic_string_view<charT, traits>, charT> : ycxx::adl_free::fmt_string_formatter<charT> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(basic_string_view<charT, traits> s, FormatContext& ctx) const {
    return this->do_format(s.data(), s.size(), ctx);
  }
};

// /4: disabled. Only <format> must provide them, but they are explicit and partial
// specializations of the primary template: declared here, a use of one between another header
// and <format> cannot instantiate the primary template first.
template <>
struct formatter<char*, wchar_t> : ycxx::adl_free::fmt_disabled {};
template <>
struct formatter<const char*, wchar_t> : ycxx::adl_free::fmt_disabled {};
template <size_t N>
struct formatter<char[N], wchar_t> : ycxx::adl_free::fmt_disabled {};
template <class traits, class Allocator>
struct formatter<basic_string<char, traits, Allocator>, wchar_t> : ycxx::adl_free::fmt_disabled {};
template <class traits>
struct formatter<basic_string_view<char, traits>, wchar_t> : ycxx::adl_free::fmt_disabled {};

// /2.3: integers and bool.
template <class T, ycxx::detail::fmt_char charT>
  requires(__is_same(T, remove_cv_t<T>) && ycxx::detail::is_signed_or_unsigned_integer<T>)
struct formatter<T, charT> : ycxx::adl_free::fmt_std_formatter<charT, ycxx::detail::fmt_cat::integer> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(T value, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_int(ctx, value, this->spec_);
  }
};
template <ycxx::detail::fmt_char charT>
struct formatter<bool, charT> : ycxx::adl_free::fmt_std_formatter<charT, ycxx::detail::fmt_cat::boolean> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(bool value, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_bool(ctx, value, this->spec_);
  }
};

// /2.4: floating-point types.
template <class T, ycxx::detail::fmt_char charT>
  requires ycxx::detail::fmt_is_float<T>
struct formatter<T, charT> : ycxx::adl_free::fmt_std_formatter<charT, ycxx::detail::fmt_cat::floating> {
  template <class FormatContext>
  typename FormatContext::iterator format(T value, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_float(ctx, value, this->spec_);
  }
};

// /2.5, /2.6: pointers.
template <ycxx::detail::fmt_char charT>
struct formatter<nullptr_t, charT> : ycxx::adl_free::fmt_std_formatter<charT, ycxx::detail::fmt_cat::pointer> {
  template <class FormatContext>
  constexpr typename FormatContext::iterator format(nullptr_t, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_pointer(ctx, nullptr, this->spec_);
  }
};
template <ycxx::detail::fmt_char charT>
struct formatter<void*, charT> : ycxx::adl_free::fmt_std_formatter<charT, ycxx::detail::fmt_cat::pointer> {
  template <class FormatContext>
  typename FormatContext::iterator format(void* p, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_pointer(ctx, p, this->spec_);
  }
};
template <ycxx::detail::fmt_char charT>
struct formatter<const void*, charT> : ycxx::adl_free::fmt_std_formatter<charT, ycxx::detail::fmt_cat::pointer> {
  template <class FormatContext>
  typename FormatContext::iterator format(const void* p, FormatContext& ctx) const {
    return ycxx::detail::fmt_format_pointer(ctx, p, this->spec_);
  }
};

// /3.
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<char> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<wchar_t> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<bool> = true;
template <class T>
  requires(__is_same(T, remove_cv_t<T>) &&
           (ycxx::detail::is_signed_or_unsigned_integer<T> || ycxx::detail::fmt_is_float<T>))
inline constexpr bool enable_nonlocking_formatter_optimization<T> = true;
template <ycxx::detail::fmt_char charT>
inline constexpr bool enable_nonlocking_formatter_optimization<charT*> = true;
template <ycxx::detail::fmt_char charT>
inline constexpr bool enable_nonlocking_formatter_optimization<const charT*> = true;
template <ycxx::detail::fmt_char charT, size_t N>
inline constexpr bool enable_nonlocking_formatter_optimization<charT[N]> = true;
template <ycxx::detail::fmt_char charT, class traits, class Allocator>
inline constexpr bool enable_nonlocking_formatter_optimization<basic_string<charT, traits, Allocator>> = true;
template <ycxx::detail::fmt_char charT, class traits>
inline constexpr bool enable_nonlocking_formatter_optimization<basic_string_view<charT, traits>> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<nullptr_t> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<void*> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<const void*> = true;

} // namespace std
