// [system.error.syn] declares `template<class charT> struct formatter<error_code, charT>;`
// ([syserr.fmt]): with only <system_error> included it is enabled (semiregular) for char and
// wchar_t; [format.formatter.spec]/2 applies (formatter_spec.hpp), and /3 makes
// enable_nonlocking_formatter_optimization<error_code> true (not specified otherwise).
#include <system_error>
#include "formatter_spec.hpp"

static_assert(std::semiregular<std::formatter<std::error_code, char>>);
static_assert(std::semiregular<std::formatter<std::error_code, wchar_t>>);
static_assert(std::enable_nonlocking_formatter_optimization<std::error_code>);
static_assert(formatter_spec::check());
