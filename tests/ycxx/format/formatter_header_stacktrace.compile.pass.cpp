// [stacktrace.syn] declares `template<> struct formatter<stacktrace_entry>;` and `template<class
// Allocator> struct formatter<basic_stacktrace<Allocator>>;` ([stacktrace.format]): with only
// <stacktrace> included both are enabled (semiregular) for char; [format.formatter.spec]/2
// applies (formatter_spec.hpp), and /3 makes enable_nonlocking_formatter_optimization true for both
// (not specified otherwise).
#include <stacktrace>
#include "formatter_spec.hpp"

static_assert(std::semiregular<std::formatter<std::stacktrace_entry>>);
static_assert(std::semiregular<std::formatter<std::stacktrace>>);
static_assert(std::semiregular<std::formatter<std::pmr::stacktrace>>);
static_assert(std::enable_nonlocking_formatter_optimization<std::stacktrace_entry>);
static_assert(std::enable_nonlocking_formatter_optimization<std::stacktrace>);
static_assert(formatter_spec::check());
