// libycxx hosted: the locale-dependent parts of <format>: basic_format_context::locale(), the
// numpunct values of the L option and the formatting functions taking a locale. The first two
// are also instantiated in the hosted runtime (src/hosted/format.cpp) for format_context and
// wformat_context, for code that has only the core of <format>.
#pragma once

#include <ycxx/core/format_base.hpp>
#include <ycxx/hosted/locale_base.hpp>

template <class Out, class charT>
std::locale std::basic_format_context<Out, charT>::locale() {
  const std::locale* loc = ycxx::detail::fmt_access::locale_ptr(*this);
  return loc != nullptr ? *loc : std::locale();
}

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
template <class charT, class Context>
fmt_numpunct<charT> fmt_get_numpunct(Context& ctx) {
  const std::locale loc = ctx.locale();
  const std::numpunct<charT>& np = std::use_facet<std::numpunct<charT>>(loc);
  return {np.grouping(), np.thousands_sep(), np.decimal_point()};
}

template <class charT, class Context>
std::basic_string<charT> fmt_get_boolname(Context& ctx, bool value) {
  const std::locale loc = ctx.locale();
  const std::numpunct<charT>& np = std::use_facet<std::numpunct<charT>>(loc);
  return value ? np.truename() : np.falsename();
}

// Instantiated in the hosted runtime.
extern template fmt_numpunct<char> fmt_get_numpunct<char, fmt_context<char>>(fmt_context<char>&);
extern template fmt_numpunct<wchar_t> fmt_get_numpunct<wchar_t, fmt_context<wchar_t>>(fmt_context<wchar_t>&);
extern template std::string fmt_get_boolname<char, fmt_context<char>>(fmt_context<char>&, bool);
extern template std::wstring fmt_get_boolname<wchar_t, fmt_context<wchar_t>>(fmt_context<wchar_t>&, bool);
}} // namespace ycxx::detail

extern template std::locale std::format_context::locale();
extern template std::locale std::wformat_context::locale();

namespace [[gnu::visibility("hidden")]] std {

template <class... Args>
string format(const locale& loc, format_string<Args...> fmt, Args&&... args) {
  return ycxx::detail::fmt_vformat_string<char>(fmt.get(), make_format_args(args...), __builtin_addressof(loc));
}
template <class... Args>
wstring format(const locale& loc, wformat_string<Args...> fmt, Args&&... args) {
  return ycxx::detail::fmt_vformat_string<wchar_t>(fmt.get(), make_wformat_args(args...), __builtin_addressof(loc));
}
inline string vformat(const locale& loc, string_view fmt, format_args args) {
  return ycxx::detail::fmt_vformat_string<char>(fmt, args, __builtin_addressof(loc));
}
inline wstring vformat(const locale& loc, wstring_view fmt, wformat_args args) {
  return ycxx::detail::fmt_vformat_string<wchar_t>(fmt, args, __builtin_addressof(loc));
}

template <class Out, class... Args>
  requires output_iterator<Out, const char&>
Out format_to(Out out, const locale& loc, format_string<Args...> fmt, Args&&... args) {
  return ycxx::detail::fmt_vformat_to<char>(static_cast<Out&&>(out), fmt.get(), make_format_args(args...),
                                            __builtin_addressof(loc));
}
template <class Out, class... Args>
  requires output_iterator<Out, const wchar_t&>
Out format_to(Out out, const locale& loc, wformat_string<Args...> fmt, Args&&... args) {
  return ycxx::detail::fmt_vformat_to<wchar_t>(static_cast<Out&&>(out), fmt.get(), make_wformat_args(args...),
                                               __builtin_addressof(loc));
}
template <class Out>
  requires output_iterator<Out, const char&>
Out vformat_to(Out out, const locale& loc, string_view fmt, format_args args) {
  return ycxx::detail::fmt_vformat_to<char>(static_cast<Out&&>(out), fmt, args, __builtin_addressof(loc));
}
template <class Out>
  requires output_iterator<Out, const wchar_t&>
Out vformat_to(Out out, const locale& loc, wstring_view fmt, wformat_args args) {
  return ycxx::detail::fmt_vformat_to<wchar_t>(static_cast<Out&&>(out), fmt, args, __builtin_addressof(loc));
}

template <class Out, class... Args>
  requires output_iterator<Out, const char&>
format_to_n_result<Out> format_to_n(Out out, iter_difference_t<Out> n, const locale& loc, format_string<Args...> fmt,
                                    Args&&... args) {
  return ycxx::detail::fmt_vformat_to_n<char>(static_cast<Out&&>(out), n, fmt.get(), make_format_args(args...),
                                              __builtin_addressof(loc));
}
template <class Out, class... Args>
  requires output_iterator<Out, const wchar_t&>
format_to_n_result<Out> format_to_n(Out out, iter_difference_t<Out> n, const locale& loc, wformat_string<Args...> fmt,
                                    Args&&... args) {
  return ycxx::detail::fmt_vformat_to_n<wchar_t>(static_cast<Out&&>(out), n, fmt.get(), make_wformat_args(args...),
                                                 __builtin_addressof(loc));
}

template <class... Args>
size_t formatted_size(const locale& loc, format_string<Args...> fmt, Args&&... args) {
  return ycxx::detail::fmt_vformatted_size<char>(fmt.get(), make_format_args(args...), __builtin_addressof(loc));
}
template <class... Args>
size_t formatted_size(const locale& loc, wformat_string<Args...> fmt, Args&&... args) {
  return ycxx::detail::fmt_vformatted_size<wchar_t>(fmt.get(), make_wformat_args(args...), __builtin_addressof(loc));
}

} // namespace std
