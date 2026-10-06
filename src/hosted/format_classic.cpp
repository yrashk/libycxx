// libycxx hosted runtime, without a C library (YCXX_PAL=none without the 'clib' layer, DECISIONS
// §18): the values of <format>'s L option ([format.string.std]/17), instantiated for the two
// context types the library creates, as src/hosted/format.cpp does with the C library. Compiled
// freestanding, the templates (ycxx/hosted/format_locale.hpp) give the classic locale's values,
// the only locale there is; basic_format_context::locale() is not instantiated here (it needs the
// locale runtime, which is built with the C library only).
#include <format>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template fmt_numpunct<char> fmt_get_numpunct<char, fmt_context<char>>(fmt_context<char>&);
template fmt_numpunct<wchar_t> fmt_get_numpunct<wchar_t, fmt_context<wchar_t>>(fmt_context<wchar_t>&);
template std::string fmt_get_boolname<char, fmt_context<char>>(fmt_context<char>&, bool);
template std::wstring fmt_get_boolname<wchar_t, fmt_context<wchar_t>>(fmt_context<wchar_t>&, bool);

}} // namespace ycxx::detail
