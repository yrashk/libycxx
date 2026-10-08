// libycxx hosted runtime, without a C library (YCXX_PAL=none without the 'clib' layer, DECISIONS
// §18): the values of <format>'s L option ([format.string.std]/17), instantiated for the two
// context types the library creates, as src/hosted/format.cpp does with the C library. Compiled
// freestanding, the templates (ycxx/hosted/format_locale.hpp) give the classic locale's values,
// the only locale there is; basic_format_context::locale() is not instantiated here (it needs the
// locale runtime, which is built with the C library only).
#include <format>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template __fmt_numpunct<char> __fmt_get_numpunct<char, __fmt_context<char>>(__fmt_context<char>&);
template __fmt_numpunct<wchar_t> __fmt_get_numpunct<wchar_t, __fmt_context<wchar_t>>(__fmt_context<wchar_t>&);
template std::string __fmt_get_boolname<char, __fmt_context<char>>(__fmt_context<char>&, bool);
template std::wstring __fmt_get_boolname<wchar_t, __fmt_context<wchar_t>>(__fmt_context<wchar_t>&, bool);

}} // namespace __ycxx::__detail
