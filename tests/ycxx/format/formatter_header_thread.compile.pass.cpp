// [thread.syn] declares `template<class charT> struct formatter<thread::id, charT>;`
// ([thread.thread.id]/11-14): with only <thread> included it is an enabled specialization
// (semiregular) for char and wchar_t; [format.formatter.spec]/2 applies (formatter_spec.hpp),
// and /3 makes enable_nonlocking_formatter_optimization<thread::id> true (not specified otherwise).
#include <thread>
#include "formatter_spec.hpp"

static_assert(std::semiregular<std::formatter<std::thread::id, char>>);
static_assert(std::semiregular<std::formatter<std::thread::id, wchar_t>>);
static_assert(std::enable_nonlocking_formatter_optimization<std::thread::id>);
static_assert(formatter_spec::check());
