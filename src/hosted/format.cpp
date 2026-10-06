// libycxx hosted runtime: the locale-dependent parts of <format> (defined in
// ycxx/hosted/format_locale.hpp), instantiated for the two context types the library creates, so
// that headers which use only the core of <format> (<ostream>, <thread>, ...) need no <locale>.
#include <format>
#include <locale>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template __fmt_numpunct<char> __fmt_get_numpunct<char, __fmt_context<char>>(__fmt_context<char>&);
template __fmt_numpunct<wchar_t> __fmt_get_numpunct<wchar_t, __fmt_context<wchar_t>>(__fmt_context<wchar_t>&);
template std::string __fmt_get_boolname<char, __fmt_context<char>>(__fmt_context<char>&, bool);
template std::wstring __fmt_get_boolname<wchar_t, __fmt_context<wchar_t>>(__fmt_context<wchar_t>&, bool);

}} // namespace __ycxx::__detail

template std::locale std::format_context::locale();
template std::locale std::wformat_context::locale();
