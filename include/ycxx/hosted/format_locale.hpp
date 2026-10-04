// libycxx hosted: the formatting functions taking a locale ([format.functions]). The other
// locale-dependent parts (basic_format_context::locale() and the numpunct values of the L
// option) are defined in the hosted runtime (src/hosted/format.cpp) for the two context types.
#pragma once

#include <ycxx/core/format_base.hpp>
#include <ycxx/hosted/locale_base.hpp>

namespace std {

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
