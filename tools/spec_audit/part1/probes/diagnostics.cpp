// [diagnostics]: <stdexcept> (constexpr constructors and what(), P3068), <cassert>, <cerrno>
// (errno, the E* macros), <system_error> (errc, error_category, error_code, error_condition,
// is_error_code_enum, system_error, hash, formatter), <stacktrace>, <debugging>.
#include <stdexcept>
#include <cassert>
#include <cerrno>
#include <debugging>
#include <format>
#include <stacktrace>
#include <string>
#include <system_error>
#include <type_traits>

// [std.exceptions]
constexpr bool constexpr_stdexcept() {
  std::logic_error l("a");
  std::out_of_range o(std::string("bc"));
  std::runtime_error r = std::overflow_error("d");
  return l.what()[0] == 'a' && o.what()[1] == 'c' && r.what()[0] == 'd';
}
static_assert(constexpr_stdexcept());
static_assert(std::is_base_of_v<std::logic_error, std::invalid_argument> && std::is_base_of_v<std::runtime_error, std::underflow_error>);
static_assert(!std::is_default_constructible_v<std::logic_error> && std::is_nothrow_copy_constructible_v<std::range_error>);
static_assert(!std::is_convertible_v<const char*, std::domain_error>);   // explicit
// [assertions]
constexpr int asserted(int x) { assert(x > 0); return x; }
static_assert(asserted(1) == 1);
// [errno]
static_assert(EDOM != ERANGE && EILSEQ != 0 && ENOTRECOVERABLE != EOWNERDEAD);
static_assert(std::is_same_v<decltype(errno), int&> || std::is_same_v<decltype((errno)), int&>);
// [syserr]
static_assert(std::is_error_condition_enum_v<std::errc> && !std::is_error_code_enum_v<std::errc>);
static_assert(std::is_scoped_enum_v<std::errc> && int(std::errc::invalid_argument) == EINVAL);
static_assert(noexcept(std::error_code()) && noexcept(std::make_error_code(std::errc::io_error)) &&
              noexcept(std::make_error_condition(std::errc::io_error)));
static_assert(noexcept(std::generic_category()) && noexcept(std::system_category()));
static_assert(std::is_nothrow_default_constructible_v<std::error_condition>);
static_assert(std::is_same_v<decltype(std::error_code() <=> std::error_code()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::error_condition() <=> std::error_condition()), std::strong_ordering>);
static_assert(noexcept(std::error_code() == std::error_condition()));
static_assert(std::is_abstract_v<std::error_category> && !std::is_copy_constructible_v<std::error_category>);
static_assert(std::is_base_of_v<std::runtime_error, std::system_error>);
static_assert(std::is_same_v<decltype(std::declval<const std::system_error&>().code()), const std::error_code&>);
static_assert(std::is_default_constructible_v<std::hash<std::error_code>> && std::is_default_constructible_v<std::hash<std::error_condition>>);
static_assert(std::formattable<std::error_code, char> && std::formattable<std::error_code, wchar_t>);   // [system.error.syn]
// [stacktrace]
static_assert(std::is_same_v<std::stacktrace, std::basic_stacktrace<std::allocator<std::stacktrace_entry>>>);
static_assert(std::is_nothrow_default_constructible_v<std::stacktrace_entry> && std::is_nothrow_copy_constructible_v<std::stacktrace_entry>);
static_assert(noexcept(std::stacktrace::current()) && noexcept(std::declval<std::stacktrace&>().size()));
static_assert(std::is_same_v<decltype(std::stacktrace_entry() <=> std::stacktrace_entry()), std::strong_ordering>);
static_assert(std::stacktrace_entry() == std::stacktrace_entry() && !std::stacktrace_entry());   // constexpr
static_assert(std::is_same_v<decltype(std::to_string(std::stacktrace())), std::string>);
static_assert(std::formattable<std::stacktrace_entry, char> && std::formattable<std::stacktrace, char>);
static_assert(std::is_default_constructible_v<std::hash<std::stacktrace_entry>> && std::is_default_constructible_v<std::hash<std::stacktrace>>);
static_assert(std::ranges::random_access_range<std::stacktrace>);
// [debugging]
static_assert(noexcept(std::breakpoint()) && noexcept(std::breakpoint_if_debugging()) && noexcept(std::is_debugger_present()));
static_assert(std::is_same_v<decltype(std::is_debugger_present()), bool>);
