// libycxx hosted: the locale-dependent parts of <format>: basic_format_context::locale(), the
// numpunct values of the L option and the formatting functions taking a locale. The first two
// are also instantiated in the hosted runtime (src/hosted/format.cpp) for format_context and
// wformat_context, for code that has only the core of <format>.
#pragma once

#include <ycxx/core/format_base.hpp>
#include <ycxx/hosted/locale_base.hpp>

template <class _Out, class __charT>
std::locale std::basic_format_context<_Out, __charT>::locale() {
  const std::locale* __loc = __ycxx::__detail::__fmt_access::__locale_ptr(*this);
  return __loc != nullptr ? *__loc : std::locale();
}

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// Without a C library (DECISIONS §18: YCXX_PAL=none without 'clib') the classic locale is the only
// one, and its numpunct values ([facet.numpunct.virtuals]) are used directly: the locale runtime
// is not built there.
template <class __charT, class _Context>
__fmt_numpunct<__charT> __fmt_get_numpunct(_Context& __ctx) {
  if constexpr (__cfg::__hosted) {
    const std::locale __loc = __ctx.locale();
    const std::numpunct<__charT>& __np = std::use_facet<std::numpunct<__charT>>(__loc);
    return {__np.grouping(), __np.thousands_sep(), __np.decimal_point()};
  } else {
    static_cast<void>(__ctx);
    return {std::string(), static_cast<__charT>(','), static_cast<__charT>('.')};
  }
}

template <class __charT, class _Context>
std::basic_string<__charT> __fmt_get_boolname(_Context& __ctx, bool value) {
  if constexpr (__cfg::__hosted) {
    const std::locale __loc = __ctx.locale();
    const std::numpunct<__charT>& __np = std::use_facet<std::numpunct<__charT>>(__loc);
    return value ? __np.truename() : __np.falsename();
  } else {
    static_cast<void>(__ctx);
    constexpr __charT t[] = {'t', 'r', 'u', 'e'};
    constexpr __charT __f[] = {'f', 'a', 'l', 's', 'e'};
    return value ? std::basic_string<__charT>(t, 4) : std::basic_string<__charT>(__f, 5);
  }
}

// Instantiated in the hosted runtime.
extern template __fmt_numpunct<char> __fmt_get_numpunct<char, __fmt_context<char>>(__fmt_context<char>&);
extern template __fmt_numpunct<wchar_t> __fmt_get_numpunct<wchar_t, __fmt_context<wchar_t>>(__fmt_context<wchar_t>&);
extern template std::string __fmt_get_boolname<char, __fmt_context<char>>(__fmt_context<char>&, bool);
extern template std::wstring __fmt_get_boolname<wchar_t, __fmt_context<wchar_t>>(__fmt_context<wchar_t>&, bool);
}} // namespace __ycxx::__detail

extern template std::locale std::format_context::locale();
extern template std::locale std::wformat_context::locale();

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class... _Args>
string format(const locale& __loc, format_string<_Args...> __fmt, _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformat_string<char>(__fmt.get(), make_format_args(__args...), __builtin_addressof(__loc));
}
template <class... _Args>
wstring format(const locale& __loc, wformat_string<_Args...> __fmt, _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformat_string<wchar_t>(__fmt.get(), make_wformat_args(__args...), __builtin_addressof(__loc));
}
inline string vformat(const locale& __loc, string_view __fmt, format_args __args) {
  return __ycxx::__detail::__fmt_vformat_string<char>(__fmt, __args, __builtin_addressof(__loc));
}
inline wstring vformat(const locale& __loc, wstring_view __fmt, wformat_args __args) {
  return __ycxx::__detail::__fmt_vformat_string<wchar_t>(__fmt, __args, __builtin_addressof(__loc));
}

template <class _Out, class... _Args>
  requires output_iterator<_Out, const char&>
_Out format_to(_Out out, const locale& __loc, format_string<_Args...> __fmt, _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformat_to<char>(static_cast<_Out&&>(out), __fmt.get(), make_format_args(__args...),
                                            __builtin_addressof(__loc));
}
template <class _Out, class... _Args>
  requires output_iterator<_Out, const wchar_t&>
_Out format_to(_Out out, const locale& __loc, wformat_string<_Args...> __fmt, _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformat_to<wchar_t>(static_cast<_Out&&>(out), __fmt.get(), make_wformat_args(__args...),
                                               __builtin_addressof(__loc));
}
template <class _Out>
  requires output_iterator<_Out, const char&>
_Out vformat_to(_Out out, const locale& __loc, string_view __fmt, format_args __args) {
  return __ycxx::__detail::__fmt_vformat_to<char>(static_cast<_Out&&>(out), __fmt, __args, __builtin_addressof(__loc));
}
template <class _Out>
  requires output_iterator<_Out, const wchar_t&>
_Out vformat_to(_Out out, const locale& __loc, wstring_view __fmt, wformat_args __args) {
  return __ycxx::__detail::__fmt_vformat_to<wchar_t>(static_cast<_Out&&>(out), __fmt, __args, __builtin_addressof(__loc));
}

template <class _Out, class... _Args>
  requires output_iterator<_Out, const char&>
format_to_n_result<_Out> format_to_n(_Out out, iter_difference_t<_Out> n, const locale& __loc, format_string<_Args...> __fmt,
                                    _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformat_to_n<char>(static_cast<_Out&&>(out), n, __fmt.get(), make_format_args(__args...),
                                              __builtin_addressof(__loc));
}
template <class _Out, class... _Args>
  requires output_iterator<_Out, const wchar_t&>
format_to_n_result<_Out> format_to_n(_Out out, iter_difference_t<_Out> n, const locale& __loc, wformat_string<_Args...> __fmt,
                                    _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformat_to_n<wchar_t>(static_cast<_Out&&>(out), n, __fmt.get(), make_wformat_args(__args...),
                                                 __builtin_addressof(__loc));
}

template <class... _Args>
size_t formatted_size(const locale& __loc, format_string<_Args...> __fmt, _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformatted_size<char>(__fmt.get(), make_format_args(__args...), __builtin_addressof(__loc));
}
template <class... _Args>
size_t formatted_size(const locale& __loc, wformat_string<_Args...> __fmt, _Args&&... __args) {
  return __ycxx::__detail::__fmt_vformatted_size<wchar_t>(__fmt.get(), make_wformat_args(__args...), __builtin_addressof(__loc));
}

}} // namespace std
