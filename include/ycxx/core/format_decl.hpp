// libycxx core: what every header declaring a formatter specialization provides without the
// formatting library: the primary template formatter, enable_nonlocking_formatter_optimization,
// the concept formattable, and the specializations of [format.formatter.spec]/2 and /4 (/3 for
// their enable_nonlocking_formatter_optimization values).
//
// "Each header that declares the template formatter provides" those specializations
// ([format.formatter.spec]/2), so <vector>, <stack> and <queue> must make them complete. Nothing
// can call parse or format without the contexts of <format>, so only the class layouts are
// here: the state (fmt_spec) and member functions whose bodies call __ycxx::__detail functions that
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

namespace [[__gnu__::__visibility__("hidden")]] std {
template <class _CharT>
struct char_traits;
template <class _Tp>
class allocator;
template <class __charT, class __traits, class _Allocator>
class basic_string;
template <class __charT, class __traits>
class basic_string_view;

// [format.formatter]: the primary template is disabled ([format.formatter.spec]/5, /7).
template <class _Tp, class __charT = char>
struct formatter {
  formatter() = delete;
  formatter(const formatter&) = delete;
  formatter& operator=(const formatter&) = delete;
};

template <class __charT>
class basic_format_parse_context;
template <class _Out, class __charT>
class basic_format_context;

// [format.formatter.locking]
template <class _Tp>
inline constexpr bool enable_nonlocking_formatter_optimization = false;
} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// format_context::iterator (format_base.hpp).
template <class __charT>
class __fmt_iter;

// A disabled formatter specialization ([format.formatter.spec]/7).
struct __fmt_disabled {
  __fmt_disabled() = delete;
  __fmt_disabled(const __fmt_disabled&) = delete;
  __fmt_disabled& operator=(const __fmt_disabled&) = delete;
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class __charT>
concept __fmt_char = __is_same(__charT, char) || __is_same(__charT, wchar_t);

// [format.formattable]
template <class _Tp, class _Context, class _Formatter = typename _Context::template formatter_type<std::remove_const_t<_Tp>>>
concept __fmt_formattable_with =
    std::semiregular<_Formatter> &&
    requires(_Formatter& __f, const _Formatter& __cf, _Tp&& t, _Context __fc,
             std::basic_format_parse_context<typename _Context::char_type> __pc) {
      { __f.parse(__pc) } -> std::same_as<typename decltype(__pc)::iterator>;
      { __cf.format(t, __fc) } -> std::same_as<typename _Context::iterator>;
    };

// ---- the std-format-spec ([format.string.std]) ------------------------------------------------

enum class __fmt_align : unsigned char { none, left, right, __center };
enum class __fmt_sign : unsigned char { none, plus, minus, space };
enum class __fmt_dyn : unsigned char { none, value, arg };
// The argument categories whose formatters interpret the std-format-spec.
enum class __fmt_cat : unsigned char { __integer, character, __boolean, __floating, string, pointer };

template <class __charT>
struct __fmt_spec {
  __charT fill[4] = {__charT(' ')};
  unsigned char __fill_len = 1;
  __fmt_align align = __fmt_align::none;
  __fmt_sign sign = __fmt_sign::none;
  bool __alt = false;
  bool zero = false;
  bool __localized = false;
  __fmt_dyn __width_kind = __fmt_dyn::none;
  __fmt_dyn __prec_kind = __fmt_dyn::none;
  char type = 0; // presentation type, 0 for none
  std::size_t width = 0;     // value, or argument index
  std::size_t precision = 0; // value, or argument index
};

// The cv-unqualified floating-point types (every one has a <charconv> format).
template <class _Tp>
inline constexpr bool __fmt_is_float = __is_same(_Tp, std::remove_cv_t<_Tp>) && __ycxx::__detail::__is_floating_v<_Tp>;

// Defined in format_base.hpp. parse returns the parse context's iterator, const charT*
// (checked there).
template <class __charT>
constexpr const __charT* __fmt_parse_spec(std::basic_format_parse_context<__charT>& __pc, __fmt_spec<__charT>& s, __fmt_cat cat);
template <class __charT, class _Tp, class _Context>
constexpr typename _Context::iterator __fmt_format_int(_Context& __ctx, _Tp value, const __fmt_spec<__charT>& s);
template <class __charT, class _Context>
constexpr typename _Context::iterator __fmt_write_string(_Context& __ctx, const __charT* p, std::size_t n,
                                                      const __fmt_spec<__charT>& s, bool __is_char = false);
template <class __charT, class _Context>
constexpr typename _Context::iterator __fmt_format_char(_Context& __ctx, __charT c, const __fmt_spec<__charT>& s);
template <class __charT, class _Context>
constexpr typename _Context::iterator __fmt_format_bool(_Context& __ctx, bool b, const __fmt_spec<__charT>& s);
template <class __charT, class _Context>
constexpr typename _Context::iterator __fmt_format_pointer(_Context& __ctx, const void* p, const __fmt_spec<__charT>& s);
template <class __charT, class _Tp, class _Context>
typename _Context::iterator __fmt_format_float(_Context& __ctx, _Tp value, const __fmt_spec<__charT>& s);

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {
// [format.formattable]
template <class _Tp, class __charT>
concept formattable =
    __ycxx::__detail::__fmt_formattable_with<remove_reference_t<_Tp>, basic_format_context<__ycxx::__adl_free::__fmt_iter<__charT>, __charT>>;
} // namespace std

// ---- the formatter specializations of [format.formatter.spec] -----------------------------------

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {

// The formatters interpreting a std-format-spec.
template <class __charT, __ycxx::__detail::__fmt_cat _Cat>
class __fmt_std_formatter {
protected:
  __ycxx::__detail::__fmt_spec<__charT> __spec_;

public:
  constexpr const __charT* parse(std::basic_format_parse_context<__charT>& __pc) {
    return __ycxx::__detail::__fmt_parse_spec(__pc, __spec_, _Cat);
  }
};

template <class __charT>
class __fmt_string_formatter : public __fmt_std_formatter<__charT, __ycxx::__detail::__fmt_cat::string> {
public:
  constexpr void set_debug_format() { this->__spec_.type = '?'; }

protected:
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator __do_format(const __charT* p, std::size_t n, _FormatContext& __ctx) const {
    return __ycxx::__detail::__fmt_write_string(__ctx, p, n, this->__spec_);
  }
};

}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std {

// /2.1: characters.
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<__charT, __charT> : __ycxx::__adl_free::__fmt_std_formatter<__charT, __ycxx::__detail::__fmt_cat::character> {
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(__charT c, _FormatContext& __ctx) const {
    return __ycxx::__detail::__fmt_format_char(__ctx, c, this->__spec_);
  }
  constexpr void set_debug_format() { this->__spec_.type = '?'; }
};
template <>
struct formatter<char, wchar_t> : __ycxx::__adl_free::__fmt_std_formatter<wchar_t, __ycxx::__detail::__fmt_cat::character> {
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(char c, _FormatContext& __ctx) const {
    return __ycxx::__detail::__fmt_format_char(__ctx, static_cast<wchar_t>(static_cast<unsigned char>(c)), this->__spec_);
  }
  constexpr void set_debug_format() { this->__spec_.type = '?'; }
};

// /2.2: strings.
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<__charT*, __charT> : __ycxx::__adl_free::__fmt_string_formatter<__charT> {
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(__charT* s, _FormatContext& __ctx) const {
    return this->__do_format(s, char_traits<__charT>::length(s), __ctx);
  }
};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<const __charT*, __charT> : __ycxx::__adl_free::__fmt_string_formatter<__charT> {
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(const __charT* s, _FormatContext& __ctx) const {
    return this->__do_format(s, char_traits<__charT>::length(s), __ctx);
  }
};
template <__ycxx::__detail::__fmt_char __charT, size_t _Np>
struct formatter<__charT[_Np], __charT> : __ycxx::__adl_free::__fmt_string_formatter<__charT> {
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(const __charT (&s)[_Np], _FormatContext& __ctx) const {
    size_t n = 0;
    while (n != _Np && s[n] != __charT())
      ++n;
    return this->__do_format(s, n, __ctx);
  }
};
template <__ycxx::__detail::__fmt_char __charT, class __traits, class _Allocator>
struct formatter<basic_string<__charT, __traits, _Allocator>, __charT> : __ycxx::__adl_free::__fmt_string_formatter<__charT> {
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(const basic_string<__charT, __traits, _Allocator>& s,
                                                    _FormatContext& __ctx) const {
    return this->__do_format(s.data(), s.size(), __ctx);
  }
};
template <__ycxx::__detail::__fmt_char __charT, class __traits>
struct formatter<basic_string_view<__charT, __traits>, __charT> : __ycxx::__adl_free::__fmt_string_formatter<__charT> {
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(basic_string_view<__charT, __traits> s, _FormatContext& __ctx) const {
    return this->__do_format(s.data(), s.size(), __ctx);
  }
};

// /4: disabled. Only <format> must provide them, but they are explicit and partial
// specializations of the primary template: declared here, a use of one between another header
// and <format> cannot instantiate the primary template first.
template <>
struct formatter<char*, wchar_t> : __ycxx::__adl_free::__fmt_disabled {};
template <>
struct formatter<const char*, wchar_t> : __ycxx::__adl_free::__fmt_disabled {};
template <size_t _Np>
struct formatter<char[_Np], wchar_t> : __ycxx::__adl_free::__fmt_disabled {};
template <class __traits, class _Allocator>
struct formatter<basic_string<char, __traits, _Allocator>, wchar_t> : __ycxx::__adl_free::__fmt_disabled {};
template <class __traits>
struct formatter<basic_string_view<char, __traits>, wchar_t> : __ycxx::__adl_free::__fmt_disabled {};

// /2.3: integers and bool.
template <class _Tp, __ycxx::__detail::__fmt_char __charT>
  requires(__is_same(_Tp, remove_cv_t<_Tp>) && __ycxx::__detail::__is_signed_or_unsigned_integer<_Tp>)
struct formatter<_Tp, __charT> : __ycxx::__adl_free::__fmt_std_formatter<__charT, __ycxx::__detail::__fmt_cat::__integer> {
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(_Tp value, _FormatContext& __ctx) const {
    return __ycxx::__detail::__fmt_format_int(__ctx, value, this->__spec_);
  }
};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<bool, __charT> : __ycxx::__adl_free::__fmt_std_formatter<__charT, __ycxx::__detail::__fmt_cat::__boolean> {
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(bool value, _FormatContext& __ctx) const {
    return __ycxx::__detail::__fmt_format_bool(__ctx, value, this->__spec_);
  }
};

// /2.4: floating-point types.
template <class _Tp, __ycxx::__detail::__fmt_char __charT>
  requires __ycxx::__detail::__fmt_is_float<_Tp>
struct formatter<_Tp, __charT> : __ycxx::__adl_free::__fmt_std_formatter<__charT, __ycxx::__detail::__fmt_cat::__floating> {
  template <class _FormatContext>
  typename _FormatContext::iterator format(_Tp value, _FormatContext& __ctx) const {
    return __ycxx::__detail::__fmt_format_float(__ctx, value, this->__spec_);
  }
};

// /2.5, /2.6: pointers.
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<nullptr_t, __charT> : __ycxx::__adl_free::__fmt_std_formatter<__charT, __ycxx::__detail::__fmt_cat::pointer> {
  template <class _FormatContext>
  constexpr typename _FormatContext::iterator format(nullptr_t, _FormatContext& __ctx) const {
    return __ycxx::__detail::__fmt_format_pointer(__ctx, nullptr, this->__spec_);
  }
};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<void*, __charT> : __ycxx::__adl_free::__fmt_std_formatter<__charT, __ycxx::__detail::__fmt_cat::pointer> {
  template <class _FormatContext>
  typename _FormatContext::iterator format(void* p, _FormatContext& __ctx) const {
    return __ycxx::__detail::__fmt_format_pointer(__ctx, p, this->__spec_);
  }
};
template <__ycxx::__detail::__fmt_char __charT>
struct formatter<const void*, __charT> : __ycxx::__adl_free::__fmt_std_formatter<__charT, __ycxx::__detail::__fmt_cat::pointer> {
  template <class _FormatContext>
  typename _FormatContext::iterator format(const void* p, _FormatContext& __ctx) const {
    return __ycxx::__detail::__fmt_format_pointer(__ctx, p, this->__spec_);
  }
};

// /3.
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<char> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<wchar_t> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<bool> = true;
template <class _Tp>
  requires(__is_same(_Tp, remove_cv_t<_Tp>) &&
           (__ycxx::__detail::__is_signed_or_unsigned_integer<_Tp> || __ycxx::__detail::__fmt_is_float<_Tp>))
inline constexpr bool enable_nonlocking_formatter_optimization<_Tp> = true;
template <__ycxx::__detail::__fmt_char __charT>
inline constexpr bool enable_nonlocking_formatter_optimization<__charT*> = true;
template <__ycxx::__detail::__fmt_char __charT>
inline constexpr bool enable_nonlocking_formatter_optimization<const __charT*> = true;
template <__ycxx::__detail::__fmt_char __charT, size_t _Np>
inline constexpr bool enable_nonlocking_formatter_optimization<__charT[_Np]> = true;
template <__ycxx::__detail::__fmt_char __charT, class __traits, class _Allocator>
inline constexpr bool enable_nonlocking_formatter_optimization<basic_string<__charT, __traits, _Allocator>> = true;
template <__ycxx::__detail::__fmt_char __charT, class __traits>
inline constexpr bool enable_nonlocking_formatter_optimization<basic_string_view<__charT, __traits>> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<nullptr_t> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<void*> = true;
template <>
inline constexpr bool enable_nonlocking_formatter_optimization<const void*> = true;

} // namespace std
