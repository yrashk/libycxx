// libycxx hosted runtime: the locale-dependent parts of <format> (defined in
// ycxx/hosted/format_locale.hpp), instantiated for the two context types the library creates, so
// that headers which use only the core of <format> (<ostream>, <thread>, ...) need no <locale>.
#include <format>
#include <locale>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template fmt_numpunct<char> fmt_get_numpunct<char, fmt_context<char>>(fmt_context<char>&);
template fmt_numpunct<wchar_t> fmt_get_numpunct<wchar_t, fmt_context<wchar_t>>(fmt_context<wchar_t>&);
template std::string fmt_get_boolname<char, fmt_context<char>>(fmt_context<char>&, bool);
template std::wstring fmt_get_boolname<wchar_t, fmt_context<wchar_t>>(fmt_context<wchar_t>&, bool);

}} // namespace ycxx::detail

template std::locale std::format_context::locale();
template std::locale std::wformat_context::locale();
