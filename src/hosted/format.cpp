// libycxx hosted runtime: the locale-dependent parts of <format> (ycxx/core/format_base.hpp):
// basic_format_context::locale() ([format.context]/7) and the numpunct values of the L option
// ([format.string.std]/17), for the two context types the library creates.
#include <format>
#include <locale>

template <class Out, class charT>
std::locale std::basic_format_context<Out, charT>::locale() {
  const std::locale* loc = ycxx::detail::fmt_access::locale_ptr(*this);
  return loc != nullptr ? *loc : std::locale();
}

namespace ycxx::detail {

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

template fmt_numpunct<char> fmt_get_numpunct<char, fmt_context<char>>(fmt_context<char>&);
template fmt_numpunct<wchar_t> fmt_get_numpunct<wchar_t, fmt_context<wchar_t>>(fmt_context<wchar_t>&);
template std::string fmt_get_boolname<char, fmt_context<char>>(fmt_context<char>&, bool);
template std::wstring fmt_get_boolname<wchar_t, fmt_context<wchar_t>>(fmt_context<wchar_t>&, bool);

} // namespace ycxx::detail

template std::locale std::format_context::locale();
template std::locale std::wformat_context::locale();
