// [fs.filesystem.syn] declares `template<class charT> struct formatter<filesystem::path, charT>;`
// ([fs.path.fmtr]): with only <filesystem> included it is enabled (semiregular, with
// set_debug_format) for char and wchar_t; [format.formatter.spec]/2 applies (formatter_spec.hpp),
// and /3 makes enable_nonlocking_formatter_optimization<filesystem::path> true (not specified
// otherwise).
#include <filesystem>
#include "formatter_spec.hpp"

static_assert(formatter_spec::debug_enabled<std::formatter<std::filesystem::path, char>>);
static_assert(formatter_spec::debug_enabled<std::formatter<std::filesystem::path, wchar_t>>);
static_assert(std::enable_nonlocking_formatter_optimization<std::filesystem::path>);
static_assert(formatter_spec::check());
